#include "RenderPass/RenderPassExecutor.h"

#include <algorithm>
#include <array>
#include <atomic>
#include <mutex>
#include <unordered_map>

#include "HookD3D12.h"
#include "RenderPass/RenderPassMipChain.h"
#include "RenderPass/RenderTargetState.h"
#include "RenderPass/ExecutorThreadPipelineLookup.h"
#include "RenderPass/ExecutorEventNameLookup.h"
#include "RenderPass/ExecutorGraphicsComputePipeline.h"
#include "RenderPass/RenderPassResourceRegistry.h"
#include "RenderPass/RenderPassReplacement.h"
#include "RenderPass/RenderPassTexturePool.h"
#include "ShaderResource/Runtime/ShaderResourceRuntime.h"
#include "Performance/RenderPassGPUProfiler.h"
#include "Globals.h"
#include "RenderDoc/RenderDocIntegration.h"
#include "IO/ShaderInjectorIO.h"
#include "StringHelper.h"

namespace RenderPassExecutor
{
	std::mutex gPipelineCacheMutex;
	std::unordered_map<std::string, ID3D12PipelineState*> gPipelineCache;
	std::unordered_map<std::string, std::string> gPipelineCreationErrors;
	std::unordered_map<std::string, GraphicsComputePipeline> gGraphicsComputePipelines;
	std::atomic<bool> gLoggedExecutionDuringActiveCapture = false;
	std::atomic<uint64_t> gLoggedCaptureRequestSequence = 0;

	thread_local std::array<ThreadPipelineLookup, 16> gThreadPipelineLookups;
	thread_local size_t gNextThreadPipelineLookup = 0;
	thread_local std::unordered_map<const RenderPass::RenderPassDisk*, ExecutorEventNameLookup> gEventNameLookups;
	thread_local std::vector<D3D12_RESOURCE_BARRIER> gComputeTransitions;
	thread_local std::vector<D3D12_RESOURCE_BARRIER> gComputeCompletionBarriers;

	bool RenderTargetStatesEqual(const RenderTargetState& left, const RenderTargetState& right)
	{
		if (left.count != right.count || left.sampleCount != right.sampleCount ||
			left.sampleQuality != right.sampleQuality)
		{
			return false;
		}

		for (UINT renderTargetIndex = 0; renderTargetIndex < left.count; ++renderTargetIndex)
		{
			if (left.formats[renderTargetIndex] != right.formats[renderTargetIndex])
				return false;
		}
		return true;
	}

	void CacheThreadPipelineLookup(
		const RenderPass::RenderPassDisk& renderPass,
		ID3D12RootSignature* rootSignature,
		const RenderTargetState& renderTargets,
		ID3D12PipelineState* pipelineState)
	{
		for (ThreadPipelineLookup& lookup : gThreadPipelineLookups)
		{
			if (lookup.renderPass == &renderPass && lookup.rootSignature == rootSignature)
			{
				lookup = {&renderPass, rootSignature, renderTargets, pipelineState};
				return;
			}
		}
		gThreadPipelineLookups[gNextThreadPipelineLookup] = {
			&renderPass, rootSignature, renderTargets, pipelineState};
		gNextThreadPipelineLookup = (gNextThreadPipelineLookup + 1) % gThreadPipelineLookups.size();
	}

	const std::wstring& GetRenderPassEventName(const RenderPass::RenderPassDisk& renderPass)
	{
		ExecutorEventNameLookup& lookup = gEventNameLookups[&renderPass];
		if (lookup.eventName.empty() || lookup.renderPassName != renderPass.name)
		{
			lookup.renderPassName = renderPass.name;
			lookup.eventName = StringHelper::Utf8ToWide("Shader Injector Render Pass: " + renderPass.name);
		}
		return lookup.eventName;
	}

	bool BuildRenderTargetState(
		const std::vector<RenderPass::ResourceBindingDiagnostic>& outputBindings,
		RenderTargetState& outState)
	{
		outState = {};
		outState.sampleCount = 1;
		for (const RenderPass::ResourceBindingDiagnostic& binding : outputBindings)
		{
			if (binding.bindingType != "RTV" ||
				binding.descriptorIndex >= D3D12_SIMULTANEOUS_RENDER_TARGET_COUNT ||
				binding.resourceFormat == DXGI_FORMAT_UNKNOWN)
			{
				continue;
			}

			outState.formats[binding.descriptorIndex] = static_cast<DXGI_FORMAT>(binding.resourceFormat);
			outState.count = (std::max)(outState.count, binding.descriptorIndex + 1);
			if (binding.resourceSampleCount)
			{
				outState.sampleCount = binding.resourceSampleCount;
				outState.sampleQuality = binding.resourceSampleQuality;
			}
		}
		return outState.count > 0 && outState.formats[0] != DXGI_FORMAT_UNKNOWN;
	}

	bool BuildRenderTargetState(
		const RenderPassTexturePool::TextureView& runtimeOutput,
		RenderTargetState& outState)
	{
		outState = {};
		if (!runtimeOutput.resource || !runtimeOutput.renderTargetView.ptr ||
			runtimeOutput.description.format == DXGI_FORMAT_UNKNOWN)
		{
			return false;
		}
		outState.count = 1;
		outState.formats[0] = runtimeOutput.description.format;
		outState.sampleCount = runtimeOutput.description.sampleCount;
		return true;
	}

	void RestoreGameOutputState(
		ID3D12GraphicsCommandList* commandList,
		const RenderPassMipChain::GraphicsStateSnapshot& gameState)
	{
		const D3D12_CPU_DESCRIPTOR_HANDLE* depthStencil = nullptr;
		if (gameState.depthStencil.ptr)
			depthStencil = &gameState.depthStencil;
		const D3D12_CPU_DESCRIPTOR_HANDLE* renderTargets = nullptr;
		if (!gameState.renderTargets.empty())
			renderTargets = gameState.renderTargets.data();
		commandList->OMSetRenderTargets(
			static_cast<UINT>(gameState.renderTargets.size()),
			renderTargets,
			FALSE,
			depthStencil);
		if (!gameState.viewports.empty())
			commandList->RSSetViewports(static_cast<UINT>(gameState.viewports.size()), gameState.viewports.data());
		if (!gameState.scissorRectangles.empty())
			commandList->RSSetScissorRects(
				static_cast<UINT>(gameState.scissorRectangles.size()),
				gameState.scissorRectangles.data());
	}

	std::string BuildPipelineCacheKey(
		const RenderPass::RenderPassDisk& renderPass,
		ID3D12RootSignature* rootSignature,
		const RenderTargetState& renderTargets)
	{
		std::string key = renderPass.id + ':' + StringHelper::PointerToString(rootSignature) + ':' +
						  std::to_string(renderPass.vertexShaderBlobHash) + ':' +
						  std::to_string(renderPass.fragmentShaderBlobHash) + ':' +
						  std::to_string(renderTargets.sampleCount) + ':' +
						  std::to_string(renderTargets.sampleQuality);
		for (UINT renderTargetIndex = 0; renderTargetIndex < renderTargets.count; ++renderTargetIndex)
			key += ':' + std::to_string(static_cast<UINT>(renderTargets.formats[renderTargetIndex]));
		return key;
	}

	D3D12_BLEND_DESC BuildBlendState(UINT renderTargetCount)
	{
		D3D12_BLEND_DESC blendState{};
		for (UINT renderTargetIndex = 0;
			 renderTargetIndex < (std::min)(renderTargetCount, static_cast<UINT>(D3D12_SIMULTANEOUS_RENDER_TARGET_COUNT));
			 ++renderTargetIndex)
		{
			D3D12_RENDER_TARGET_BLEND_DESC& target = blendState.RenderTarget[renderTargetIndex];
			target.BlendEnable = FALSE;
			target.LogicOpEnable = FALSE;
			target.SrcBlend = D3D12_BLEND_ONE;
			target.DestBlend = D3D12_BLEND_ZERO;
			target.BlendOp = D3D12_BLEND_OP_ADD;
			target.SrcBlendAlpha = D3D12_BLEND_ONE;
			target.DestBlendAlpha = D3D12_BLEND_ZERO;
			target.BlendOpAlpha = D3D12_BLEND_OP_ADD;
			target.LogicOp = D3D12_LOGIC_OP_NOOP;
			target.RenderTargetWriteMask = D3D12_COLOR_WRITE_ENABLE_ALL;
		}
		return blendState;
	}

	D3D12_RASTERIZER_DESC BuildRasterizerState(UINT sampleCount)
	{
		D3D12_RASTERIZER_DESC rasterizer{};
		rasterizer.FillMode = D3D12_FILL_MODE_SOLID;
		rasterizer.CullMode = D3D12_CULL_MODE_NONE;
		rasterizer.FrontCounterClockwise = FALSE;
		rasterizer.DepthBias = D3D12_DEFAULT_DEPTH_BIAS;
		rasterizer.DepthBiasClamp = D3D12_DEFAULT_DEPTH_BIAS_CLAMP;
		rasterizer.SlopeScaledDepthBias = D3D12_DEFAULT_SLOPE_SCALED_DEPTH_BIAS;
		rasterizer.DepthClipEnable = TRUE;
		rasterizer.MultisampleEnable = sampleCount > 1;
		rasterizer.AntialiasedLineEnable = FALSE;
		rasterizer.ForcedSampleCount = 0;
		rasterizer.ConservativeRaster = D3D12_CONSERVATIVE_RASTERIZATION_MODE_OFF;
		return rasterizer;
	}

	ID3D12PipelineState* GetOrCreatePipelineState(
		const RenderPass::RenderPassDisk& renderPass,
		ID3D12GraphicsCommandList* commandList,
		ID3D12RootSignature* rootSignature,
		const RenderTargetState& renderTargets,
		std::string& outError)
	{
		for (const ThreadPipelineLookup& lookup : gThreadPipelineLookups)
		{
			if (lookup.renderPass == &renderPass &&
				lookup.rootSignature == rootSignature &&
				lookup.pipelineState &&
				RenderTargetStatesEqual(lookup.renderTargets, renderTargets))
			{
				return lookup.pipelineState;
			}
		}

		const std::string cacheKey = BuildPipelineCacheKey(renderPass, rootSignature, renderTargets);
		std::lock_guard<std::mutex> cacheLock(gPipelineCacheMutex);
		const auto cachedPipelineIt = gPipelineCache.find(cacheKey);
		if (cachedPipelineIt != gPipelineCache.end())
		{
			CacheThreadPipelineLookup(renderPass, rootSignature, renderTargets, cachedPipelineIt->second);
			return cachedPipelineIt->second;
		}
		const auto cachedErrorIt = gPipelineCreationErrors.find(cacheKey);
		if (cachedErrorIt != gPipelineCreationErrors.end())
		{
			outError = cachedErrorIt->second;
			return nullptr;
		}

		ID3D12Device* device = nullptr;
		if (FAILED(commandList->GetDevice(IID_PPV_ARGS(&device))) || !device)
		{
			outError = "Could not query the D3D12 device from the command list.";
			return nullptr;
		}

		D3D12_GRAPHICS_PIPELINE_STATE_DESC description{};
		description.pRootSignature = rootSignature;
		description.VS = {renderPass.vertexShaderBlob.data(), renderPass.vertexShaderBlob.size()};
		description.PS = {renderPass.fragmentShaderBlob.data(), renderPass.fragmentShaderBlob.size()};
		description.BlendState = BuildBlendState(renderTargets.count);
		description.SampleMask = UINT_MAX;
		description.RasterizerState = BuildRasterizerState(renderTargets.sampleCount);
		description.DepthStencilState.DepthEnable = FALSE;
		description.DepthStencilState.DepthWriteMask = D3D12_DEPTH_WRITE_MASK_ZERO;
		description.DepthStencilState.DepthFunc = D3D12_COMPARISON_FUNC_ALWAYS;
		description.DepthStencilState.StencilEnable = FALSE;
		description.InputLayout = {nullptr, 0};
		description.IBStripCutValue = D3D12_INDEX_BUFFER_STRIP_CUT_VALUE_DISABLED;
		description.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
		description.NumRenderTargets = renderTargets.count;
		for (UINT renderTargetIndex = 0; renderTargetIndex < renderTargets.count; ++renderTargetIndex)
			description.RTVFormats[renderTargetIndex] = renderTargets.formats[renderTargetIndex];
		description.DSVFormat = DXGI_FORMAT_UNKNOWN;
		description.SampleDesc = {renderTargets.sampleCount, renderTargets.sampleQuality};

		ID3D12PipelineState* pipelineState = nullptr;
		HRESULT result = E_FAIL;
		{
			HookD3D12::ScopedRenderPassInjection injectionScope;
			result = device->CreateGraphicsPipelineState(
				&description,
				IID_PPV_ARGS(&pipelineState));
		}
		device->Release();
		if (FAILED(result) || !pipelineState)
		{
			outError = "Fullscreen pipeline creation failed with " + StringHelper::FormatHRESULT(result);
			gPipelineCreationErrors[cacheKey] = outError;
			ShaderInjectorIO::WriteToLogFileError(
				"RenderPassExecutor->GetOrCreatePipelineState: " + renderPass.name + ": " + outError);
			return nullptr;
		}

		const std::wstring pipelineName = StringHelper::Utf8ToWide(
			"Shader Injector Render Pass: " + renderPass.name);
		pipelineState->SetName(pipelineName.c_str());
		gPipelineCache.emplace(cacheKey, pipelineState);
		CacheThreadPipelineLookup(renderPass, rootSignature, renderTargets, pipelineState);
		ShaderInjectorIO::WriteToLogFile(
			"RenderPassExecutor->GetOrCreatePipelineState: created fullscreen pipeline for " + renderPass.name);
		return pipelineState;
	}

	bool ExecuteFullscreenTriangle(
		const RenderPass::RenderPassDisk& renderPass,
		ID3D12GraphicsCommandList* commandList,
		ID3D12RootSignature* graphicsRootSignature,
		ID3D12PipelineState* pipelineStateToRestore,
		D3D12_PRIMITIVE_TOPOLOGY primitiveTopologyToRestore,
		const std::vector<RenderPass::ResourceBindingDiagnostic>& outputBindings,
		const RenderPassTexturePool::TextureView* runtimeOutput,
		const RenderPassMipChain::GraphicsStateSnapshot* gameStateToRestore,
		std::string& outError)
	{
		outError.clear();

		if (!commandList || !graphicsRootSignature || !pipelineStateToRestore)
		{
			outError = "The target draw does not have complete graphics state.";
			return false;
		}

		if (primitiveTopologyToRestore == D3D_PRIMITIVE_TOPOLOGY_UNDEFINED)
		{
			outError = "The target draw's primitive topology has not been observed yet.";
			return false;
		}

		if (!RenderPass::HasCompiledShaders(renderPass))
		{
			outError = "Render Pass shaders have not been compiled and loaded.";
			return false;
		}

		if (!HookD3D12::Original_SetPipelineState ||
			!HookD3D12::Original_IASetPrimitiveTopology ||
			!HookD3D12::Original_DrawInstanced)
		{
			outError = "One or more original D3D12 draw functions are unavailable.";
			return false;
		}

		if (runtimeOutput && (!gameStateToRestore ||
							  gameStateToRestore->renderTargets.empty() ||
							  gameStateToRestore->viewports.empty() ||
							  gameStateToRestore->scissorRectangles.empty()))
		{
			outError = "Runtime output requires captured render-target, viewport, and scissor state.";
			return false;
		}

		RenderTargetState renderTargets{};
		bool renderTargetStateAvailable = false;
		if (runtimeOutput)
			renderTargetStateAvailable = BuildRenderTargetState(*runtimeOutput, renderTargets);
		else
			renderTargetStateAvailable = BuildRenderTargetState(outputBindings, renderTargets);

		if (!renderTargetStateAvailable)
		{
			outError = "The target draw's render-target formats are not available yet.";
			return false;
		}

		ID3D12PipelineState* fullscreenPipelineState = GetOrCreatePipelineState(
			renderPass,
			commandList,
			graphicsRootSignature,
			renderTargets,
			outError);

		if (!fullscreenPipelineState)
			return false;

		const std::wstring& eventName = GetRenderPassEventName(renderPass);
		const bool renderDocCaptureActive = Globals::gRenderDocIntegrationEnabled && RenderDocIntegration::IsFrameCapturing();
		const uint64_t captureRequestSequence = RenderDocIntegration::GetCaptureRequestSequence();
		uint64_t loggedCaptureRequestSequence = gLoggedCaptureRequestSequence.load(std::memory_order_relaxed);

		if (captureRequestSequence > loggedCaptureRequestSequence &&
			gLoggedCaptureRequestSequence.compare_exchange_strong(
				loggedCaptureRequestSequence,
				captureRequestSequence,
				std::memory_order_relaxed))
		{
			unsigned int captureActive = 0;
			if (renderDocCaptureActive)
				captureActive = 1;
			ShaderInjectorIO::WriteToLogFile(StringHelper::Format(
				"RenderPassExecutor->ExecuteFullscreenTriangle: first pass execution after capture request sequence=%llu captureActive=%u pass=%s commandList=%p",
				static_cast<unsigned long long>(captureRequestSequence),
				captureActive,
				renderPass.name.c_str(),
				commandList));
		}
		if (renderDocCaptureActive)
		{
			if (!gLoggedExecutionDuringActiveCapture.exchange(true, std::memory_order_relaxed))
			{
				ShaderInjectorIO::WriteToLogFileSuccess("RenderPassExecutor->ExecuteFullscreenTriangle: pass executed during active RenderDoc capture: " + renderPass.name);
			}
		}
		else
		{
			gLoggedExecutionDuringActiveCapture.store(false, std::memory_order_relaxed);
		}

		{
			HookD3D12::ScopedRenderPassInjection injectionScope;
			commandList->BeginEvent(
				0,
				eventName.c_str(),
				static_cast<UINT>((eventName.size() + 1) * sizeof(wchar_t)));
			commandList->SetPipelineState(fullscreenPipelineState);
			commandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

			if (runtimeOutput)
			{
				D3D12_RESOURCE_BARRIER barrier{};
				barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
				barrier.Transition.pResource = runtimeOutput->resource.Get();
				barrier.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
				barrier.Transition.StateBefore = runtimeOutput->initialState;
				barrier.Transition.StateAfter = D3D12_RESOURCE_STATE_RENDER_TARGET;
				commandList->ResourceBarrier(1, &barrier);
				commandList->OMSetRenderTargets(1, &runtimeOutput->renderTargetView, FALSE, nullptr);
				const D3D12_VIEWPORT viewport{
					0.0f,
					0.0f,
					static_cast<float>(runtimeOutput->description.width),
					static_cast<float>(runtimeOutput->description.height),
					0.0f,
					1.0f};
				const D3D12_RECT scissor{
					0,
					0,
					static_cast<LONG>(runtimeOutput->description.width),
					static_cast<LONG>(runtimeOutput->description.height)};
				commandList->RSSetViewports(1, &viewport);
				commandList->RSSetScissorRects(1, &scissor);
			}

			commandList->DrawInstanced(3, 1, 0, 0);

			if (runtimeOutput)
			{
				D3D12_RESOURCE_BARRIER barrier{};
				barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
				barrier.Transition.pResource = runtimeOutput->resource.Get();
				barrier.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
				barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_RENDER_TARGET;
				barrier.Transition.StateAfter = runtimeOutput->initialState;
				commandList->ResourceBarrier(1, &barrier);
				if (gameStateToRestore)
					RestoreGameOutputState(commandList, *gameStateToRestore);
			}

			commandList->IASetPrimitiveTopology(primitiveTopologyToRestore);
			commandList->SetPipelineState(pipelineStateToRestore);
			commandList->EndEvent();
		}

		return true;
	}

	bool PrepareGraphicsComputeState(
		const RenderPass::RenderPassDisk& renderPass,
		ID3D12GraphicsCommandList* commandList,
		const RenderPassMipChain::GraphicsStateSnapshot& graphicsState,
		RenderPassMipChain::GraphicsStateSnapshot& outComputeState,
		ID3D12PipelineState*& outPipelineState,
		std::string& outError)
	{
		outPipelineState = nullptr;
		const std::string cacheKey = renderPass.id + ":compute:" + StringHelper::PointerToString(graphicsState.rootSignature);
		std::lock_guard<std::mutex> cacheLock(gPipelineCacheMutex);
		auto pipelineIterator = gGraphicsComputePipelines.find(cacheKey);

		if (pipelineIterator == gGraphicsComputePipelines.end())
		{
			std::vector<uint8_t> rootBytes;
			uint64_t rootHash = 0;

			if (!HookD3D12::GetRootSignatureBlob(graphicsState.rootSignature, rootBytes, rootHash))
			{
				outError = "Graphics-anchored compute requires a captured root signature.";
				return false;
			}

			Microsoft::WRL::ComPtr<ID3D12VersionedRootSignatureDeserializer> deserializer;
			const D3D12_VERSIONED_ROOT_SIGNATURE_DESC* sourceDescription = nullptr;

			if (FAILED(D3D12CreateVersionedRootSignatureDeserializer(rootBytes.data(), rootBytes.size(), IID_PPV_ARGS(&deserializer))) || FAILED(deserializer->GetRootSignatureDescAtVersion(D3D_ROOT_SIGNATURE_VERSION_1_0, &sourceDescription)))
			{
				outError = "Could not inspect the graphics root for a custom compute pass.";
				return false;
			}

			GraphicsComputePipeline computePipeline;
			const auto& sourceRoot = sourceDescription->Desc_1_0;
			computePipeline.rootParameterMap.resize(sourceRoot.NumParameters, UINT_MAX);
			std::vector<D3D12_ROOT_PARAMETER> parameters;
			std::vector<D3D12_STATIC_SAMPLER_DESC> staticSamplers;
			std::vector<D3D12_DESCRIPTOR_RANGE> outputRanges;

			//pixel and vertex stages can reuse a register, so discard vertex-only arguments before promoting visibility.
			for (UINT parameterIndex = 0; parameterIndex < sourceRoot.NumParameters; ++parameterIndex)
			{
				auto parameter = sourceRoot.pParameters[parameterIndex];

				if (parameter.ShaderVisibility != D3D12_SHADER_VISIBILITY_PIXEL && parameter.ShaderVisibility != D3D12_SHADER_VISIBILITY_ALL)
					continue;

				parameter.ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;
				computePipeline.rootParameterMap[parameterIndex] = static_cast<UINT>(parameters.size());
				parameters.push_back(parameter);
			}

			for (UINT samplerIndex = 0; samplerIndex < sourceRoot.NumStaticSamplers; ++samplerIndex)
			{
				auto sampler = sourceRoot.pStaticSamplers[samplerIndex];

				if (sampler.ShaderVisibility != D3D12_SHADER_VISIBILITY_PIXEL && sampler.ShaderVisibility != D3D12_SHADER_VISIBILITY_ALL)
					continue;

				sampler.ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;
				staticSamplers.push_back(sampler);
			}

			//a compact appended table contains only declared outputs; every descriptor is supplied by the pass.
			for (const auto& output : renderPass.outputs)
			{
				if (output.origin == ShaderResource::ResourceOrigin::Runtime && output.access == RenderPass::ResourceAccess::UnorderedAccess)
					outputRanges.push_back({D3D12_DESCRIPTOR_RANGE_TYPE_UAV, 1, output.shaderRegister, output.registerSpace, D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND});
			}

			if (!outputRanges.empty())
			{
				D3D12_ROOT_PARAMETER outputParameter{};
				outputParameter.ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
				outputParameter.ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;
				outputParameter.DescriptorTable = {static_cast<UINT>(outputRanges.size()), outputRanges.data()};
				parameters.push_back(outputParameter);
			}

			D3D12_ROOT_SIGNATURE_DESC rootDescription{};
			rootDescription.NumParameters = static_cast<UINT>(parameters.size());
			rootDescription.pParameters = parameters.data();
			rootDescription.NumStaticSamplers = static_cast<UINT>(staticSamplers.size());
			rootDescription.pStaticSamplers = staticSamplers.data();
			Microsoft::WRL::ComPtr<ID3DBlob> serializedRoot;
			Microsoft::WRL::ComPtr<ID3DBlob> serializationErrors;
			HRESULT result = D3D12SerializeRootSignature(&rootDescription, D3D_ROOT_SIGNATURE_VERSION_1_0, &serializedRoot, &serializationErrors);

			if (FAILED(result))
			{
				outError = "Custom compute root serialization failed with " + StringHelper::FormatHRESULT(result);
				return false;
			}

			Microsoft::WRL::ComPtr<ID3D12Device> device;
			commandList->GetDevice(IID_PPV_ARGS(&device));
			HookD3D12::ScopedRenderPassInjection injectionScope;
			result = device->CreateRootSignature(0, serializedRoot->GetBufferPointer(), serializedRoot->GetBufferSize(), IID_PPV_ARGS(&computePipeline.rootSignature));

			if (FAILED(result))
			{
				outError = "Custom compute root creation failed with " + StringHelper::FormatHRESULT(result);
				return false;
			}

			D3D12_COMPUTE_PIPELINE_STATE_DESC pipelineDescription{};
			pipelineDescription.pRootSignature = computePipeline.rootSignature.Get();
			pipelineDescription.CS = {renderPass.fragmentShaderBlob.data(), renderPass.fragmentShaderBlob.size()};
			result = device->CreateComputePipelineState(&pipelineDescription, IID_PPV_ARGS(&computePipeline.pipelineState));

			if (FAILED(result))
			{
				outError = "Graphics-anchored compute pipeline creation failed with " + StringHelper::FormatHRESULT(result);
				return false;
			}

			RenderPassResourceRegistry::RegisterRootSignature(computePipeline.rootSignature.Get(), serializedRoot->GetBufferPointer(), serializedRoot->GetBufferSize());
			pipelineIterator = gGraphicsComputePipelines.emplace(cacheKey, std::move(computePipeline)).first;
		}

		const auto& computePipeline = pipelineIterator->second;
		outComputeState = graphicsState;
		outComputeState.rootSignature = computePipeline.rootSignature.Get();
		outComputeState.rootBindings.clear();

		for (auto binding : graphicsState.rootBindings)
		{
			if (binding.rootParameterIndex >= computePipeline.rootParameterMap.size() || computePipeline.rootParameterMap[binding.rootParameterIndex] == UINT_MAX)
				continue;

			binding.rootParameterIndex = computePipeline.rootParameterMap[binding.rootParameterIndex];
			outComputeState.rootBindings.push_back(std::move(binding));
		}

		outPipelineState = computePipeline.pipelineState.Get();
		return true;
	}

	void RestoreComputeState(ID3D12GraphicsCommandList* commandList, const RenderPassMipChain::GraphicsStateSnapshot& computeState)
	{
		commandList->SetComputeRootSignature(computeState.rootSignature);

		for (const auto& binding : computeState.rootBindings)
		{
			switch (binding.type)
			{
			case RenderPassMipChain::RootArgumentType::DescriptorTable:
				commandList->SetComputeRootDescriptorTable(binding.rootParameterIndex, {binding.value});
				break;
			case RenderPassMipChain::RootArgumentType::ConstantBufferView:
				commandList->SetComputeRootConstantBufferView(binding.rootParameterIndex, binding.value);
				break;
			case RenderPassMipChain::RootArgumentType::ShaderResourceView:
				commandList->SetComputeRootShaderResourceView(binding.rootParameterIndex, binding.value);
				break;
			case RenderPassMipChain::RootArgumentType::UnorderedAccessView:
				commandList->SetComputeRootUnorderedAccessView(binding.rootParameterIndex, binding.value);
				break;
			case RenderPassMipChain::RootArgumentType::Constants:
				commandList->SetComputeRoot32BitConstants(binding.rootParameterIndex, static_cast<UINT>(binding.constants.size()), binding.constants.data(), 0);
				break;
			}
		}
	}

	bool ExecuteCompute(
		const RenderPass::RenderPassDisk& renderPass,
		ID3D12GraphicsCommandList* commandList,
		ID3D12PipelineState* computePipelineState,
		ID3D12PipelineState* pipelineStateToRestore,
		UINT threadGroupCountX,
		UINT threadGroupCountY,
		UINT threadGroupCountZ,
		const std::vector<RenderPassTexturePool::TextureView>& unorderedAccessOutputs,
		std::string& outError)
	{
		outError.clear();
		gComputeTransitions.clear();
		gComputeCompletionBarriers.clear();

		if (!commandList || !computePipelineState || !pipelineStateToRestore)
		{
			outError = "The target dispatch does not have complete compute state.";
			return false;
		}

		if (!threadGroupCountX || !threadGroupCountY || !threadGroupCountZ)
		{
			outError = "The injected compute dispatch resolved to zero thread groups.";
			return false;
		}

		if (!HookD3D12::Original_SetPipelineState || !HookD3D12::Original_Dispatch)
		{
			outError = "One or more original D3D12 compute functions are unavailable.";
			return false;
		}

		gComputeTransitions.reserve(unorderedAccessOutputs.size());

		for (const RenderPassTexturePool::TextureView& output : unorderedAccessOutputs)
		{
			if (!output.resource || !output.unorderedAccessView.ptr)
			{
				gComputeTransitions.clear();
				outError = "A compute output is missing its texture or UAV descriptor.";
				return false;
			}
			//same-state UAV outputs have no transition; their ordering stays after the dispatch.
			if (output.initialState == D3D12_RESOURCE_STATE_UNORDERED_ACCESS)
				continue;

			D3D12_RESOURCE_BARRIER barrier{};
			barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
			barrier.Transition.pResource = output.resource.Get();
			barrier.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
			barrier.Transition.StateBefore = output.initialState;
			barrier.Transition.StateAfter = D3D12_RESOURCE_STATE_UNORDERED_ACCESS;
			gComputeTransitions.push_back(barrier);
		}

		const std::wstring& eventName = GetRenderPassEventName(renderPass);
		{
			HookD3D12::ScopedRenderPassInjection injectionScope;
			commandList->BeginEvent(
				0,
				eventName.c_str(),
				static_cast<UINT>((eventName.size() + 1) * sizeof(wchar_t)));

			if (!gComputeTransitions.empty())
				commandList->ResourceBarrier(static_cast<UINT>(gComputeTransitions.size()), gComputeTransitions.data());

			commandList->SetPipelineState(computePipelineState);
			HookD3D12::Original_Dispatch(
				commandList,
				threadGroupCountX,
				threadGroupCountY,
				threadGroupCountZ);

			if (!unorderedAccessOutputs.empty())
			{
				gComputeCompletionBarriers.reserve(unorderedAccessOutputs.size());

				for (const RenderPassTexturePool::TextureView& output : unorderedAccessOutputs)
				{
					if (output.initialState == D3D12_RESOURCE_STATE_UNORDERED_ACCESS)
					{
						//this output stays in UAV state, so retain ordering for its next same-state access.
						D3D12_RESOURCE_BARRIER unorderedAccessBarrier{};
						unorderedAccessBarrier.Type = D3D12_RESOURCE_BARRIER_TYPE_UAV;
						unorderedAccessBarrier.UAV.pResource = output.resource.Get();
						gComputeCompletionBarriers.push_back(unorderedAccessBarrier);
						continue;
					}

					//the UAV-to-output-state transition orders prior writes, so a separate UAV barrier is redundant.
					D3D12_RESOURCE_BARRIER transition{};
					transition.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
					transition.Transition.pResource = output.resource.Get();
					transition.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
					transition.Transition.StateBefore = D3D12_RESOURCE_STATE_UNORDERED_ACCESS;
					transition.Transition.StateAfter = output.initialState;
					gComputeCompletionBarriers.push_back(transition);
				}

				commandList->ResourceBarrier(static_cast<UINT>(gComputeCompletionBarriers.size()), gComputeCompletionBarriers.data());
			}

			commandList->SetPipelineState(pipelineStateToRestore);
			commandList->EndEvent();
		}
		return true;
	}

	bool ExecuteTextureCopy(
		ID3D12GraphicsCommandList* commandList,
		const RenderPassTexturePool::TextureView& source,
		const RenderPassTexturePool::TextureView& destination,
		std::string& outError)
	{
		outError.clear();

		if (!commandList || !source.resource || !destination.resource)
		{
			outError = "Copy pass requires valid source and destination textures.";
			return false;
		}

		if (source.resource.Get() == destination.resource.Get())
		{
			outError = "Copy pass source and destination refer to the same texture.";
			return false;
		}

		const auto& left = source.description;
		const auto& right = destination.description;

		if (left.dimension != right.dimension || left.format != right.format ||
			left.width != right.width || left.height != right.height ||
			left.depth != right.depth || left.arraySize != right.arraySize ||
			left.mipLevels != right.mipLevels || left.sampleCount != right.sampleCount)
		{
			outError = "Copy pass textures must have matching dimensions, format, mip count, and sample count.";
			return false;
		}

		std::array<D3D12_RESOURCE_BARRIER, 2> barriers{};
		UINT barrierCount = 0;
		if (source.initialState != D3D12_RESOURCE_STATE_COPY_SOURCE)
		{
			D3D12_RESOURCE_BARRIER& sourceBarrier = barriers[barrierCount++];
			sourceBarrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
			sourceBarrier.Transition.pResource = source.resource.Get();
			sourceBarrier.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
			sourceBarrier.Transition.StateBefore = source.initialState;
			sourceBarrier.Transition.StateAfter = D3D12_RESOURCE_STATE_COPY_SOURCE;
		}
		if (destination.initialState != D3D12_RESOURCE_STATE_COPY_DEST)
		{
			D3D12_RESOURCE_BARRIER& destinationBarrier = barriers[barrierCount++];
			destinationBarrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
			destinationBarrier.Transition.pResource = destination.resource.Get();
			destinationBarrier.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
			destinationBarrier.Transition.StateBefore = destination.initialState;
			destinationBarrier.Transition.StateAfter = D3D12_RESOURCE_STATE_COPY_DEST;
		}
		{
			HookD3D12::ScopedRenderPassInjection injectionScope;

			//the transition to COPY_SOURCE orders earlier UAV writes before the copy.
			if (barrierCount)
				commandList->ResourceBarrier(barrierCount, barriers.data());
			commandList->CopyResource(destination.resource.Get(), source.resource.Get());
			for (UINT barrierIndex = 0; barrierIndex < barrierCount; ++barrierIndex)
				std::swap(barriers[barrierIndex].Transition.StateBefore, barriers[barrierIndex].Transition.StateAfter);
			if (barrierCount)
				commandList->ResourceBarrier(barrierCount, barriers.data());
		}

		return true;
	}

	void ReleaseResources()
	{
		RenderPassMipChain::ReleaseResources();
		RenderPassReplacement::ReleaseResources();
		RenderPassTexturePool::ReleaseResources();
		ShaderResourceRuntime::ReleaseResources();
		RenderPassGPUProfiler::ReleaseResources();
		std::lock_guard<std::mutex> cacheLock(gPipelineCacheMutex);

		for (auto& cachedPipeline : gPipelineCache)
		{
			if (cachedPipeline.second)
				cachedPipeline.second->Release();
		}

		gPipelineCache.clear();
		gGraphicsComputePipelines.clear();
		gPipelineCreationErrors.clear();
		gThreadPipelineLookups = {};
		gNextThreadPipelineLookup = 0;
		gEventNameLookups.clear();
	}
} //namespace RenderPassExecutor
