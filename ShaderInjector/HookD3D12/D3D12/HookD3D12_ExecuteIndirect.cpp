#include "HookD3D12RuntimeState.h"
#include "../HookD3D12.h"

#include <atomic>

#include "HookD3D12ExecutionHookHelpers.h"
#include "Globals.h"
#include "Performance/PerformanceMetrics.h"
#include "RenderPass/RenderPassRuntime.h"

namespace HookD3D12
{
	std::atomic<bool> gLoggedExecuteIndirectHook = false;
	static std::atomic<bool> loggedHiddenMeshIndirect = false;

	void STDMETHODCALLTYPE Hook_ExecuteIndirect(ID3D12GraphicsCommandList* commandList, ID3D12CommandSignature* commandSignature, UINT maximumCommandCount, ID3D12Resource* argumentBuffer, UINT64 argumentBufferOffset, ID3D12Resource* countBuffer, UINT64 countBufferOffset)
	{
		Handle_ExecuteIndirect(commandList, commandSignature, maximumCommandCount, argumentBuffer, argumentBufferOffset, countBuffer, countBufferOffset);
	}

	void STDMETHODCALLTYPE Handle_ExecuteIndirect(ID3D12GraphicsCommandList* commandList, ID3D12CommandSignature* commandSignature, UINT maximumCommandCount, ID3D12Resource* argumentBuffer, UINT64 argumentBufferOffset, ID3D12Resource* countBuffer, UINT64 countBufferOffset)
	{
		thread_local bool commandHookLogChecked = false;
		if (!commandHookLogChecked)
		{
			LogFirstCommandHookHit(gLoggedExecuteIndirectHook, "Hook_ExecuteIndirect", commandList);
			commandHookLogChecked = true;
		}

		ID3D12PipelineState* hiddenMeshPipeline = gHiddenMeshPipelineState.load(std::memory_order_acquire);
		if (hiddenMeshPipeline && IsInjectorEnabledForCommandList(commandList) && !IsInsideRenderPassInjection() &&
			GetCommandListPipelineState(commandList).pipelineState.load(std::memory_order_acquire) == hiddenMeshPipeline)
		{
			LogFirstCommandHookHit(loggedHiddenMeshIndirect, "HiddenMeshExecuteIndirect", commandList);
			return;
		}

		//dispatch signatures use the compute boundary; unknown signatures keep the graphics boundary.
		bool computeDispatch = false;
		bool writesRootArguments = true;
		RenderPassRuntime::FindCommandSignature(commandSignature, computeDispatch, writesRootArguments);

		if (!IsInjectorEnabledForCommandList(commandList) || !RenderPassRuntime::IsPipelineExecutionTrackingRequired(computeDispatch) || IsInsideRenderPassInjection())
		{
			Original_ExecuteIndirect(commandList, commandSignature, maximumCommandCount, argumentBuffer, argumentBufferOffset, countBuffer, countBufferOffset);
			return;
		}

		PerformanceMetrics::Increment(PerformanceMetrics::Counter::ExecuteIndirect);

		uint32_t boundaryMask = 0;
		{
			PerformanceMetrics::ScopedTimer lookupTimer(PerformanceMetrics::Timing::ExecutionHookLookup, 512);
			boundaryMask = RenderPassRuntime::GetExecutionBoundaryMask(commandList, computeDispatch);
		}

		if (boundaryMask)
			PerformanceMetrics::Increment(PerformanceMetrics::Counter::ExecutionBoundaryCandidate);

		if ((boundaryMask & 1u) != 0)
			RenderPassRuntime::RecordExecutionBoundary(commandList, computeDispatch, RenderPassRuntime::ExecutionBoundary::Before, "ExecuteIndirect", 0, 0, 0, true, writesRootArguments);

		Original_ExecuteIndirect(commandList, commandSignature, maximumCommandCount, argumentBuffer, argumentBufferOffset, countBuffer, countBufferOffset);
		RenderPassRuntime::TrackExecuteIndirect(commandList, commandSignature, computeDispatch);

		if ((boundaryMask & 1u) != 0 && computeDispatch)
			RenderPassRuntime::CompleteComputeExecutionBoundary(commandList);
		else if ((boundaryMask & 1u) != 0)
			RenderPassRuntime::CompleteGraphicsExecutionBoundary(commandList);

		if ((boundaryMask & 2u) != 0)
			RenderPassRuntime::RecordExecutionBoundary(commandList, computeDispatch, RenderPassRuntime::ExecutionBoundary::After, "ExecuteIndirect", 0, 0, 0, true, writesRootArguments);
	}
} //namespace HookD3D12
