#pragma once
#include <RHI/RHIDLL.h>

#include <RHI/BindingSet/BindingSet.h>
#include <RHI/Framebuffer/Framebuffer.h>
#include <RHI/Instance/BaseTypes.h>
#include <RHI/Instance/QueryInterface.h>
#include <RHI/Pipeline/Pipeline.h>
#include <RHI/QueryHeap/QueryHeap.h>
#include <RHI/RHIDLL.h>
#include <RHI/Resource/Resource.h>
#include <RHI/View/View.h>
#include <array>
#include <memory>

class PL_RHI_DLL plRHICommandList : public plRefCounted
{
public:
  virtual ~plRHICommandList() = default;
  virtual void Reset() = 0;
  virtual void Close() = 0;
  virtual void BindPipeline(const plSharedPtr<plRHIPipeline>& state) = 0;
  virtual void BindBindingSet(const plSharedPtr<plRHIBindingSet>& bindingSet) = 0;
  virtual void BeginRenderPass(const plSharedPtr<plRHIRenderPass>& renderPass, const plSharedPtr<plRHIFramebuffer>& framebuffer, const plRHIClearDesc& clearDesc) = 0;
  virtual void EndRenderPass() = 0;
  virtual void BeginEvent(const plString& name) = 0;
  virtual void EndEvent() = 0;
  virtual void Draw(plUInt32 vertexCount, plUInt32 instanceCount, plUInt32 firstVertex, plUInt32 firstInstance) = 0;
  virtual void DrawIndexed(plUInt32 indexCount, plUInt32 instanceCount, plUInt32 firstIndex, plInt32 vertexOffset, plUInt32 firstInstance) = 0;
  virtual void DrawIndirect(const plSharedPtr<plRHIResource>& pArgumentBuffer, plUInt64 argumentBufferOffset) = 0;
  virtual void DrawIndexedIndirect(const plSharedPtr<plRHIResource>& pArgumentBuffer, plUInt64 argumentBufferOffset) = 0;
  virtual void DrawIndirectCount(
    const plSharedPtr<plRHIResource>& pArgumentBuffer,
    plUInt64 argumentBufferOffset,
    const plSharedPtr<plRHIResource>& countBuffer,
    plUInt64 countBufferOffset,
    plUInt32 maxDrawCount,
    plUInt32 stride) = 0;
  virtual void DrawIndexedIndirectCount(
    const plSharedPtr<plRHIResource>& pArgumentBuffer,
    plUInt64 argumentBufferOffset,
    const plSharedPtr<plRHIResource>& countBuffer,
    plUInt64 countBufferOffset,
    plUInt32 maxDrawCount,
    plUInt32 stride) = 0;
  virtual void Dispatch(plUInt32 threadGroupCountX, plUInt32 threadGroupCountY, plUInt32 threadGroupCountZ) = 0;
  virtual void DispatchIndirect(const plSharedPtr<plRHIResource>& pArgumentBuffer, plUInt64 argumentBufferOffset) = 0;
  virtual void DispatchMesh(plUInt32 threadGroupCountX) = 0;
  virtual void DispatchRays(const plRHIRayTracingShaderTables& shaderTables, plUInt32 width, plUInt32 height, plUInt32 depth) = 0;
  virtual void ResourceBarrier(const std::vector<plRHIResourceBarrierDesc>& barriers) = 0;
  virtual void UAVResourceBarrier(const plSharedPtr<plRHIResource>& resource) = 0;
  virtual void SetViewport(float x, float y, float width, float height) = 0;
  virtual void SetScissorRect(plInt32 left, plInt32 top, plUInt32 right, plUInt32 bottom) = 0;
  virtual void IASetIndexBuffer(const plSharedPtr<plRHIResource>& resource, plRHIResourceFormat::Enum format) = 0;
  virtual void IASetVertexBuffer(plUInt32 slot, const plSharedPtr<plRHIResource>& resource) = 0;
  virtual void RSSetShadingRate(plRHIShadingRate shading_rate, const std::array<plRHIShadingRateCombiner, 2>& combiners) = 0;
  virtual void BuildBottomLevelAS(
    const plSharedPtr<plRHIResource>& src,
    const plSharedPtr<plRHIResource>& dst,
    const plSharedPtr<plRHIResource>& scratch,
    plUInt64 scratchOffset,
    const std::vector<plRHIRaytracingGeometryDesc>& descs,
    plRHIBuildAccelerationStructureFlags flags) = 0;
  virtual void BuildTopLevelAS(
    const plSharedPtr<plRHIResource>& src,
    const plSharedPtr<plRHIResource>& dst,
    const plSharedPtr<plRHIResource>& scratch,
    plUInt64 scratchOffset,
    const plSharedPtr<plRHIResource>& instanceData,
    plUInt64 instanceOffset,
    plUInt32 instanceCount,
    plRHIBuildAccelerationStructureFlags flags) = 0;
  virtual void CopyAccelerationStructure(const plSharedPtr<plRHIResource>& src, const plSharedPtr<plRHIResource>& dst, plRHICopyAccelerationStructureMode mode) = 0;
  virtual void CopyBuffer(const plSharedPtr<plRHIResource>& srcBuffer, const plSharedPtr<plRHIResource>& dstBuffer,
    const std::vector<plRHIBufferCopyRegion>& regions) = 0;
  virtual void CopyBufferToTexture(const plSharedPtr<plRHIResource>& srcBuffer, const plSharedPtr<plRHIResource>& dstTexture,
    const std::vector<plRHIBufferToTextureCopyRegion>& regions) = 0;
  virtual void CopyTexture(const plSharedPtr<plRHIResource>& srcTexture, const plSharedPtr<plRHIResource>& dstTexture,
    const std::vector<plRHITextureCopyRegion>& regions) = 0;
  virtual void WriteAccelerationStructuresProperties(
    const std::vector<plSharedPtr<plRHIResource>>& accelerationStructures,
    const plSharedPtr<plRHIQueryHeap>& queryHeap,
    plUInt32 firstQuery) = 0;
  virtual void ResolveQueryData(
    const plSharedPtr<plRHIQueryHeap>& queryHeap,
    plUInt32 firstQuery,
    plUInt32 queryCount,
    const plSharedPtr<plRHIResource>& dstBuffer,
    plUInt64 dstOffset) = 0;
};
