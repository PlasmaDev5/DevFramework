#pragma once
#include <RHI/CommandList/CommandList.h>
#include <directx/d3d12.h>
#include <dxgi.h>
#include <wrl.h>
using namespace Microsoft::WRL;

class plDXDevice;
class plDXResource;
class plDXPipeline;

class plDXCommandList : public plRHICommandList
{
public:
  plDXCommandList(plDXDevice& device, plRHICommandListType type);
  void Reset() override;
  void Close() override;
  void BindPipeline(const plSharedPtr<plRHIPipeline>& state) override;
  void BindBindingSet(const plSharedPtr<plRHIBindingSet>& bindingSet) override;
  void BeginRenderPass(const plSharedPtr<plRHIRenderPass>& render_Pss, const plSharedPtr<plRHIFramebuffer>& framebuffer, const plRHIClearDesc& clearDesc) override;
  void EndRenderPass() override;
  void BeginEvent(const plString& name) override;
  void EndEvent() override;
  void Draw(plUInt32 vertexCount, plUInt32 instanceCount, plUInt32 firstVertex, plUInt32 firstInstance) override;
  void DrawIndexed(plUInt32 indexCount, plUInt32 instanceCount, plUInt32 firstIndex, plInt32 vertexOffset, plUInt32 firstInstance) override;
  void DrawIndirect(const plSharedPtr<plRHIResource>& pArgumentBuffer, plUInt64 argumentBufferOffset) override;
  void DrawIndexedIndirect(const plSharedPtr<plRHIResource>& pArgumentBuffer, plUInt64 argumentBufferOffset) override;
  void DrawIndirectCount(
    const plSharedPtr<plRHIResource>& pArgumentBuffer,
    plUInt64 argumentBufferOffset,
    const plSharedPtr<plRHIResource>& countBuffer,
    plUInt64 countBufferOffset,
    plUInt32 maxDrawCount,
    plUInt32 stride) override;
  void DrawIndexedIndirectCount(
    const plSharedPtr<plRHIResource>& pArgumentBuffer,
    plUInt64 argumentBufferOffset,
    const plSharedPtr<plRHIResource>& countBuffer,
    plUInt64 countBufferOffset,
    plUInt32 maxDrawCount,
    plUInt32 stride) override;
  void Dispatch(plUInt32 threadGroupCountX, plUInt32 threadGroupCountY, plUInt32 threadGroupCountZ) override;
  void DispatchIndirect(const plSharedPtr<plRHIResource>& pArgumentBuffer, plUInt64 argumentBufferOffset) override;
  void DispatchMesh(plUInt32 threadGroupCountX) override;
  void DispatchRays(const plRHIRayTracingShaderTables& shaderTables, plUInt32 width, plUInt32 height, plUInt32 depth) override;
  void ResourceBarrier(const std::vector<plRHIResourceBarrierDesc>& barriers) override;
  void UAVResourceBarrier(const plSharedPtr<plRHIResource>& resource) override;
  void SetViewport(float x, float y, float width, float height) override;
  void SetScissorRect(plInt32 left, plInt32 top, plUInt32 right, plUInt32 bottom) override;
  void IASetIndexBuffer(const plSharedPtr<plRHIResource>& resource, plRHIResourceFormat::Enum format) override;
  void IASetVertexBuffer(plUInt32 slot, const plSharedPtr<plRHIResource>& resource) override;
  void RSSetShadingRate(plRHIShadingRate shadingRate, const std::array<plRHIShadingRateCombiner, 2>& combiners) override;
  void BuildBottomLevelAS(
    const plSharedPtr<plRHIResource>& src,
    const plSharedPtr<plRHIResource>& dst,
    const plSharedPtr<plRHIResource>& scratch,
    plUInt64 scratchOffset,
    const std::vector<plRHIRaytracingGeometryDesc>& descs,
    plRHIBuildAccelerationStructureFlags flags) override;
  void BuildTopLevelAS(
    const plSharedPtr<plRHIResource>& src,
    const plSharedPtr<plRHIResource>& dst,
    const plSharedPtr<plRHIResource>& scratch,
    plUInt64 scratchOffset,
    const plSharedPtr<plRHIResource>& instanceData,
    plUInt64 instanceOffset,
    plUInt32 instanceCount,
    plRHIBuildAccelerationStructureFlags flags) override;
  void CopyAccelerationStructure(const plSharedPtr<plRHIResource>& src, const plSharedPtr<plRHIResource>& dst, plRHICopyAccelerationStructureMode mode) override;
  void CopyBuffer(const plSharedPtr<plRHIResource>& srcBuffer, const plSharedPtr<plRHIResource>& dstBuffer,
    const std::vector<plRHIBufferCopyRegion>& regions) override;
  void CopyBufferToTexture(const plSharedPtr<plRHIResource>& srcBuffer, const plSharedPtr<plRHIResource>& dstTexture,
    const std::vector<plRHIBufferToTextureCopyRegion>& regions) override;
  void CopyTexture(const plSharedPtr<plRHIResource>& srcTexture, const plSharedPtr<plRHIResource>& dstTexture,
    const std::vector<plRHITextureCopyRegion>& regions) override;
  void WriteAccelerationStructuresProperties(
    const std::vector<plSharedPtr<plRHIResource>>& accelerationStructures,
    const plSharedPtr<plRHIQueryHeap>& queryHeap,
    plUInt32 firstQuery) override;
  void ResolveQueryData(
    const plSharedPtr<plRHIQueryHeap>& queryHeap,
    plUInt32 firstQuery,
    plUInt32 queryCount,
    const plSharedPtr<plRHIResource>& dstBuffer,
    plUInt64 dstOffset) override;

  ComPtr<ID3D12GraphicsCommandList> GetCommandList();

private:
  void ExecuteIndirect(
    D3D12_INDIRECT_ARGUMENT_TYPE type,
    const plSharedPtr<plRHIResource>& pArgumentBuffer,
    plUInt64 argumentBufferOffset,
    const plSharedPtr<plRHIResource>& countBuffer,
    plUInt64 countBufferOffset,
    plUInt32 maxDrawCount,
    plUInt32 stride);

  void BeginRenderPassImpl(const plSharedPtr<plRHIRenderPass>& renderPass, const plSharedPtr<plRHIFramebuffer>& framebuffer, const plRHIClearDesc& clearDesc);
  void OMSetFramebuffer(const plSharedPtr<plRHIRenderPass>& renderPass, const plSharedPtr<plRHIFramebuffer>& framebuffer, const plRHIClearDesc& clearDesc);
  void IASetVertexBufferImpl(plUInt32 slot, const plSharedPtr<plRHIResource>& resource, plUInt32 stride);
  void BuildAccelerationStructure(D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_INPUTS& inputs, const plSharedPtr<plRHIResource>& src, const plSharedPtr<plRHIResource>& dst, const plSharedPtr<plRHIResource>& scratch, plUInt64 scratchOffset);

  plDXDevice& m_Device;
  ComPtr<ID3D12CommandAllocator> m_CommandAllocator;
  ComPtr<ID3D12GraphicsCommandList> m_CommandList;
  ComPtr<ID3D12GraphicsCommandList4> m_CommandList4;
  ComPtr<ID3D12GraphicsCommandList5> m_CommandList5;
  ComPtr<ID3D12GraphicsCommandList6> m_CommandList6;
  bool m_Closed = false;
  plDynamicArray<ComPtr<ID3D12DescriptorHeap>> m_Heaps;
  plSharedPtr<plDXPipeline> m_State;
  plSharedPtr<plRHIBindingSet> m_BindingSet;
  plMap<plUInt32, plSharedPtr<plRHIResource>> m_LazyVertex;
  plSharedPtr<plRHIView> m_ShadingRateImageView;
};
