#include "ShaderPatchRuntime.h"

#include <mutex>
#include <vector>

#include "Hash/Hash.h"
#include "HookD3D12/HookD3D12.h"
#include "IO/ShaderInjectorIO.h"
#include "ShaderPatch/ShaderPatchCache.h"
#include "StringHelper.h"

namespace ShaderPatchRuntime
{
	namespace
	{
		//the original stage bytecode of any captured pipeline that uses this shader.
		bool FindCapturedShader(uint64_t hash, ShaderTarget::ShaderType stage, std::vector<uint8_t>& outBytecode)
		{
			const auto take = [&](uint64_t capturedHash, const std::vector<uint8_t>& bytecode)
			{
				if (capturedHash != hash || bytecode.empty())
					return false;
				outBytecode = bytecode;
				return true;
			};

			std::lock_guard<std::mutex> pipelineLock(HookD3D12::gPipelineMutex);

			for (const HookD3D12::GraphicsPipelineInfo& pipeline : HookD3D12::gGraphicsPipelines)
			{
				if ((stage == ShaderTarget::VertexShader && take(pipeline.vertexShaderHash, pipeline.vertexShaderBytecode)) ||
					(stage == ShaderTarget::PixelShader && take(pipeline.pixelShaderHash, pipeline.pixelShaderBytecode)))
					return true;
			}

			for (const HookD3D12::ComputePipelineInfo& pipeline : HookD3D12::gComputePipelines)
			{
				if (stage == ShaderTarget::ComputeShader && take(pipeline.computeShaderHash, pipeline.computeShaderBytecode))
					return true;
			}

			for (const HookD3D12::PipelineStateInfo& pipeline : HookD3D12::gPipelineStates)
			{
				if ((stage == ShaderTarget::VertexShader && take(pipeline.vertexShaderHash, pipeline.vertexShaderBytecode)) ||
					(stage == ShaderTarget::PixelShader && take(pipeline.pixelShaderHash, pipeline.pixelShaderBytecode)) ||
					(stage == ShaderTarget::ComputeShader && take(pipeline.computeShaderHash, pipeline.computeShaderBytecode)) ||
					(stage == ShaderTarget::MeshShader && take(pipeline.meshShaderHash, pipeline.meshShaderBytecode)))
					return true;
			}

			return false;
		}
	} //namespace

	DumpResult DumpShader(const ShaderPatchRuntime::PatchedShaderInfo& shader, bool patched)
	{
		DumpResult result;
		std::vector<uint8_t> bytecode;
		if (patched)
		{
			ShaderPatch::PatchResult cached;
			if (shader.cacheKey.empty() || !ShaderPatchCache::Find(shader.cacheKey, cached))
				return result;
			bytecode = std::move(cached.bytecode);
		}
		else if (!FindCapturedShader(shader.originalHash, shader.stage, bytecode))
			return result;
		if (bytecode.empty())
			return result;

		//both files are named by the original shader's hash, so an original and its patch sit side by side.
		const std::string directory = ShaderInjectorIO::GetDumpsDirectory();
		ShaderInjectorIO::DirectoryCreate(directory);
		const std::string name = StringHelper::ShaderTypeToString(shader.stage) + "_" + Hash::FormatHash(shader.originalHash) + (patched ? ".patched.bin" : ".bin");
		result.bytecodePath = ShaderInjectorIO::JoinPath(directory, name);
		result.bytecodeWritten = ShaderInjectorIO::WriteBinaryFile(result.bytecodePath, bytecode.data(), bytecode.size());
		if (result.bytecodeWritten && shader.shaderModel >= Globals::ShaderModel::ShaderModel6_0)
		{
			result.disassemblyAttempted = true;
			result.disassemblyWritten = ShaderInjectorIO::GenerateShaderTextDXIL(result.bytecodePath);
		}
		return result;
	}
} //namespace ShaderPatchRuntime
