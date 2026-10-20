#include "RenderPass/RenderPassExpandedRootSignature.h"

#include <algorithm>
#include <map>
#include <mutex>
#include <tuple>

#include "HookD3D12/HookD3D12.h"
#include "RenderPass/RenderPassResourceRegistry.h"

namespace RenderPassResourceRegistry
{
	namespace
	{
		std::mutex gExpandedRootMutex;
		std::map<std::string, std::shared_ptr<const ExpandedRootSignature>> gExpandedRoots;

		//pick one visibility when a single stage reads the added SRVs, otherwise expose them to all stages.
		D3D12_SHADER_VISIBILITY ResolveVisibility(uint32_t stages)
		{
			if (!stages || (stages & (stages - 1)))
				return D3D12_SHADER_VISIBILITY_ALL;

			const D3D12_SHADER_VISIBILITY visibilities[] = {
				D3D12_SHADER_VISIBILITY_VERTEX,
				D3D12_SHADER_VISIBILITY_HULL,
				D3D12_SHADER_VISIBILITY_DOMAIN,
				D3D12_SHADER_VISIBILITY_GEOMETRY,
				D3D12_SHADER_VISIBILITY_PIXEL,
				D3D12_SHADER_VISIBILITY_ALL,
				D3D12_SHADER_VISIBILITY_AMPLIFICATION,
				D3D12_SHADER_VISIBILITY_MESH};

			for (UINT stage = 0; stage < ARRAYSIZE(visibilities); ++stage)
				if (stages & (1u << stage))
					return visibilities[stage];

			return D3D12_SHADER_VISIBILITY_ALL;
		}

		D3D12_ROOT_SIGNATURE_FLAGS AllowStages(D3D12_ROOT_SIGNATURE_FLAGS flags, uint32_t stages)
		{
			const D3D12_ROOT_SIGNATURE_FLAGS denies[] = {
				D3D12_ROOT_SIGNATURE_FLAG_DENY_VERTEX_SHADER_ROOT_ACCESS,
				D3D12_ROOT_SIGNATURE_FLAG_DENY_HULL_SHADER_ROOT_ACCESS,
				D3D12_ROOT_SIGNATURE_FLAG_DENY_DOMAIN_SHADER_ROOT_ACCESS,
				D3D12_ROOT_SIGNATURE_FLAG_DENY_GEOMETRY_SHADER_ROOT_ACCESS,
				D3D12_ROOT_SIGNATURE_FLAG_DENY_PIXEL_SHADER_ROOT_ACCESS,
				D3D12_ROOT_SIGNATURE_FLAG_NONE,
				D3D12_ROOT_SIGNATURE_FLAG_DENY_AMPLIFICATION_SHADER_ROOT_ACCESS,
				D3D12_ROOT_SIGNATURE_FLAG_DENY_MESH_SHADER_ROOT_ACCESS};

			for (UINT stage = 0; stage < ARRAYSIZE(denies); ++stage)
				if (stages & (1u << stage))
					flags = static_cast<D3D12_ROOT_SIGNATURE_FLAGS>(flags & ~denies[stage]);

			return flags;
		}

		//reject layouts whose existing SRVs already claim an added register, or that leave no room for one more table.
		template <typename Description>
		bool CanAppendTable(const Description& description, const std::vector<ResourceRegister>& bindings, D3D12_SHADER_VISIBILITY visibility)
		{
			if (description.Flags & D3D12_ROOT_SIGNATURE_FLAG_LOCAL_ROOT_SIGNATURE)
				return false;

			UINT cost = 0;

			for (UINT index = 0; index < description.NumParameters; ++index)
			{
				const auto& parameter = description.pParameters[index];

				if (parameter.ParameterType == D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE)
					cost += 1;
				else if (parameter.ParameterType == D3D12_ROOT_PARAMETER_TYPE_32BIT_CONSTANTS)
					cost += parameter.Constants.Num32BitValues;
				else
					cost += 2;

				if (visibility != D3D12_SHADER_VISIBILITY_ALL &&
					parameter.ShaderVisibility != D3D12_SHADER_VISIBILITY_ALL &&
					parameter.ShaderVisibility != visibility)
					continue;

				if (parameter.ParameterType == D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE)
				{
					for (UINT rangeIndex = 0; rangeIndex < parameter.DescriptorTable.NumDescriptorRanges; ++rangeIndex)
					{
						const auto& range = parameter.DescriptorTable.pDescriptorRanges[rangeIndex];

						if (range.RangeType != D3D12_DESCRIPTOR_RANGE_TYPE_SRV)
							continue;

						for (const ResourceRegister& binding : bindings)
						{
							const bool unbounded = range.NumDescriptors == UINT_MAX;
							if (range.RegisterSpace == binding.registerSpace &&
								binding.shaderRegister >= range.BaseShaderRegister &&
								(unbounded || uint64_t(binding.shaderRegister) < uint64_t(range.BaseShaderRegister) + range.NumDescriptors))
								return false;
						}
					}
				}
				else if (parameter.ParameterType == D3D12_ROOT_PARAMETER_TYPE_SRV)
				{
					for (const ResourceRegister& binding : bindings)
						if (parameter.Descriptor.RegisterSpace == binding.registerSpace && parameter.Descriptor.ShaderRegister == binding.shaderRegister)
							return false;
				}
			}

			return cost < 64;
		}

		bool BuildExpandedRoot(
			const std::vector<uint8_t>& originalRoot,
			const std::vector<ResourceRegister>& bindings,
			uint32_t stages,
			std::vector<uint8_t>& outRoot,
			UINT& outTableIndex,
			std::string& outError)
		{
			Microsoft::WRL::ComPtr<ID3D12VersionedRootSignatureDeserializer> deserializer;
			if (originalRoot.empty() ||
				FAILED(D3D12CreateVersionedRootSignatureDeserializer(originalRoot.data(), originalRoot.size(), IID_PPV_ARGS(&deserializer))))
			{
				outError = "Original root signature is unavailable.";
				return false;
			}

			const D3D12_VERSIONED_ROOT_SIGNATURE_DESC* original = deserializer->GetUnconvertedRootSignatureDesc();
			D3D12_VERSIONED_ROOT_SIGNATURE_DESC expanded = *original;
			const D3D12_SHADER_VISIBILITY visibility = ResolveVisibility(stages);
			std::vector<D3D12_ROOT_PARAMETER> parameters0;
			std::vector<D3D12_ROOT_PARAMETER1> parameters1;
			std::vector<D3D12_DESCRIPTOR_RANGE> ranges0;
			std::vector<D3D12_DESCRIPTOR_RANGE1> ranges1;

			if (original->Version == D3D_ROOT_SIGNATURE_VERSION_1_0)
			{
				const D3D12_ROOT_SIGNATURE_DESC& description = original->Desc_1_0;
				if (!CanAppendTable(description, bindings, visibility))
				{
					outError = "Root signature already binds an added SRV register or exceeds its DWORD budget.";
					return false;
				}
				parameters0.assign(description.pParameters, description.pParameters + description.NumParameters);
				for (UINT index = 0; index < bindings.size(); ++index)
					ranges0.push_back({D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, bindings[index].shaderRegister, bindings[index].registerSpace, index});
				D3D12_ROOT_PARAMETER table{};
				table.ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
				table.ShaderVisibility = visibility;
				table.DescriptorTable = {static_cast<UINT>(ranges0.size()), ranges0.data()};
				outTableIndex = description.NumParameters;
				parameters0.push_back(table);
				expanded.Desc_1_0.NumParameters = static_cast<UINT>(parameters0.size());
				expanded.Desc_1_0.pParameters = parameters0.data();
				expanded.Desc_1_0.Flags = AllowStages(description.Flags, stages);
			}
			else if (original->Version == D3D_ROOT_SIGNATURE_VERSION_1_1)
			{
				const D3D12_ROOT_SIGNATURE_DESC1& description = original->Desc_1_1;
				if (!CanAppendTable(description, bindings, visibility))
				{
					outError = "Root signature already binds an added SRV register or exceeds its DWORD budget.";
					return false;
				}
				parameters1.assign(description.pParameters, description.pParameters + description.NumParameters);
				for (UINT index = 0; index < bindings.size(); ++index)
					ranges1.push_back({D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, bindings[index].shaderRegister, bindings[index].registerSpace, D3D12_DESCRIPTOR_RANGE_FLAG_DATA_STATIC, index});
				D3D12_ROOT_PARAMETER1 table{};
				table.ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
				table.ShaderVisibility = visibility;
				table.DescriptorTable = {static_cast<UINT>(ranges1.size()), ranges1.data()};
				outTableIndex = description.NumParameters;
				parameters1.push_back(table);
				expanded.Desc_1_1.NumParameters = static_cast<UINT>(parameters1.size());
				expanded.Desc_1_1.pParameters = parameters1.data();
				expanded.Desc_1_1.Flags = AllowStages(description.Flags, stages);
			}
			else
			{
				outError = "Unsupported root signature version.";
				return false;
			}

			Microsoft::WRL::ComPtr<ID3DBlob> blob;
			Microsoft::WRL::ComPtr<ID3DBlob> diagnostics;
			if (FAILED(D3D12SerializeVersionedRootSignature(&expanded, &blob, &diagnostics)))
			{
				outError = "Expanded root serialization failed.";
				if (diagnostics)
					outError = std::string(static_cast<const char*>(diagnostics->GetBufferPointer()), diagnostics->GetBufferSize());
				return false;
			}

			const auto* bytes = static_cast<const uint8_t*>(blob->GetBufferPointer());
			outRoot.assign(bytes, bytes + blob->GetBufferSize());
			return true;
		}
	} //namespace

	std::shared_ptr<const ExpandedRootSignature> GetOrCreateExpandedRootSignature(
		ID3D12Device* device,
		const std::vector<uint8_t>& originalRoot,
		std::vector<ResourceRegister> bindings,
		uint32_t stages,
		std::string& outError)
	{
		outError.clear();
		if (!device || originalRoot.empty() || bindings.empty())
		{
			outError = "Expanded root requires a device, the original root signature and at least one SRV.";
			return {};
		}

		std::sort(bindings.begin(), bindings.end(), [](const ResourceRegister& left, const ResourceRegister& right)
				  { return std::tie(left.registerSpace, left.shaderRegister) < std::tie(right.registerSpace, right.shaderRegister); });
		bindings.erase(std::unique(bindings.begin(), bindings.end(), [](const ResourceRegister& left, const ResourceRegister& right)
								   { return left.registerSpace == right.registerSpace && left.shaderRegister == right.shaderRegister; }),
					   bindings.end());

		std::string key;
		const auto append = [&](const auto& value)
		{ key.append(reinterpret_cast<const char*>(&value), sizeof(value)); };
		append(device);
		append(stages);
		for (const ResourceRegister& binding : bindings)
		{
			append(binding.registerSpace);
			append(binding.shaderRegister);
		}
		key.append(reinterpret_cast<const char*>(originalRoot.data()), originalRoot.size());

		std::lock_guard<std::mutex> lock(gExpandedRootMutex);
		const auto cached = gExpandedRoots.find(key);
		if (cached != gExpandedRoots.end())
			return cached->second;

		auto layout = std::make_shared<ExpandedRootSignature>();
		std::vector<uint8_t> rootBlob;
		if (!BuildExpandedRoot(originalRoot, bindings, stages, rootBlob, layout->appendedTableIndex, outError))
			return {};

		{
			HookD3D12::ScopedRenderPassInjection injectionScope;
			if (FAILED(device->CreateRootSignature(0, rootBlob.data(), rootBlob.size(), IID_PPV_ARGS(&layout->root))))
			{
				outError = "Expanded root signature creation failed.";
				return {};
			}
		}

		RegisterRootSignature(layout->root.Get(), rootBlob.data(), rootBlob.size());
		layout->bindings = std::move(bindings);
		gExpandedRoots.emplace(std::move(key), layout);
		return layout;
	}

} //namespace RenderPassResourceRegistry
