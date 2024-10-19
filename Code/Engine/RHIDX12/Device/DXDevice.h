#pragma once
#include <RHI/Device/Device.h>
#include <RHIDX12/CPUDescriptorPool/DXCPUDescriptorPool.h>
#include <RHIDX12/GPUDescriptorPool/DXGPUDescriptorPool.h>
#include <directx/d3d12.h>
#include <dxgi.h>
#include <wrl.h>
using namespace Microsoft::WRL;

class plDXAdapter;
class plDXCommandQueue;

class plDXDevice : public plRHIDevice
{
public:
  plDXDevice(plDXAdapter& adapter);
  plSharedPtr<plRHIMemory> AllocateMemory(plUInt64 size, plRHIMemoryType memoryType, plUInt32 memoryTypeBits) override;
  plSharedPtr<plRHICommandQueue> GetCommandQueue(plRHICommandListType type) override;

  plSharedPtr<plRHISwapchain> CreateSwapchain(plRHIWindow window, plUInt32 width, plUInt32 height, plUInt32 frameCount, bool vsync) override;
  void DestroySwapchain(plRHISwapchain* pSwapChain) override {}

  plSharedPtr<plRHICommandList> CreateCommandList(plRHICommandListType type) override;
  void DestroyCommandList(plRHICommandList* pCommandList) override {}

  plSharedPtr<plRHIFence> CreateFence(plUInt64 initialValue) override;
  void DestroyFence(plRHIFence* pFence) override {}

  plSharedPtr<plRHIResource> CreateTexture(plRHITextureType type, plUInt32 bindFlag, plRHIResourceFormat::Enum format, plUInt32 sampleCount, int width, int height, int depth, int mipLevels) override;
  void DestroyTexture(plRHIResource* pTexture) override {}

  plSharedPtr<plRHIResource> CreateBuffer(plUInt32 bindFlag, plUInt32 bufferSize) override;
  void DestroyBuffer(plRHIResource* pBuffer) override {}

  plSharedPtr<plRHIResource> CreateSampler(const plRHISamplerDesc& desc) override;
  void DestroySampler(plRHIResource* pSampler) override {}

  plSharedPtr<plRHIView> CreateView(const plSharedPtr<plRHIResource>& resource, const plRHIViewDesc& viewDesc) override;
  void DestroyView(plRHIView* pView) override {}

  plSharedPtr<plRHIBindingSetLayout> CreateBindingSetLayout(const std::vector<plRHIBindKey>& descs) override;
  void DestroyBindingSetLayout(plRHIBindingSetLayout* pBindingSetLayout) override {}

  plSharedPtr<plRHIBindingSet> CreateBindingSet(const plSharedPtr<plRHIBindingSetLayout>& layout) override;
  void DestroyBindingSet(plRHIBindingSet* pBindingSet) override {}

  plSharedPtr<plRHIRenderPass> CreateRenderPass(const plRHIRenderPassDesc& desc) override;
  void DestroyRenderPass(plRHIRenderPass* pRenderPass) override {}

  plSharedPtr<plRHIFramebuffer> CreateFramebuffer(const plRHIFramebufferDesc& desc) override;
  void DestroyFramebuffer(plRHIFramebuffer* pFramebuffer) override {}

  plSharedPtr<plRHIShader> CreateShader(const plRHIShaderDesc& desc, plDynamicArray<plUInt8> byteCode, plSharedPtr<plRHIShaderReflection> reflection) override;
  void DestroyShader(plRHIShader* pShader) override {}

  plSharedPtr<plRHIProgram> CreateProgram(const std::vector<plSharedPtr<plRHIShader>>& shaders) override;
  void DestroyProgram(plRHIProgram* pProgram) override {}

  plSharedPtr<plRHIPipeline> CreateGraphicsPipeline(const plRHIGraphicsPipelineDesc& desc) override;
  void DestroyGraphicsPipeline(plRHIPipeline* pPipeline) override {}

  plSharedPtr<plRHIPipeline> CreateComputePipeline(const plRHIComputePipelineDesc& desc) override;
  void DestroyComputePipeline(plRHIPipeline* pPipeline) override {}

  plSharedPtr<plRHIPipeline> CreateRayTracingPipeline(const plRHIRayTracingPipelineDesc& desc) override;
  void DestroyRayTracingPipeline(plRHIPipeline* pPipeline) override {}

  plSharedPtr<plRHIResource> CreateAccelerationStructure(plRHIAccelerationStructureType type, const plSharedPtr<plRHIResource>& resource, plUInt64 offset) override;
  void DestroyAccelerationStructure(plRHIResource* pAccelerationStructure) override {}

  plSharedPtr<plRHIQueryHeap> CreateQueryHeap(plRHIQueryHeapType type, plUInt32 count) override;
  void DestroyQueryHeap(plRHIQueryHeap* pQueryHeap) override {}

  plUInt32 GetTextureDataPitchAlignment() const override;
  bool IsDxrSupported() const override;
  bool IsRayQuerySupported() const override;
  bool IsVariableRateShadingSupported() const override;
  bool IsMeshShadingSupported() const override;
  plUInt32 GetShadingRateImageTileSize() const override;
  plRHIMemoryBudget GetMemoryBudget() const override;
  plUInt32 GetShaderGroupHandleSize() const override;
  plUInt32 GetShaderRecordAlignment() const override;
  plUInt32 GetShaderTableAlignment() const override;
  plRHIRaytracingASPrebuildInfo GetBLASPrebuildInfo(const std::vector<plRHIRaytracingGeometryDesc>& descs, plRHIBuildAccelerationStructureFlags flags) const override;
  plRHIRaytracingASPrebuildInfo GetTLASPrebuildInfo(plUInt32 instanceCount, plRHIBuildAccelerationStructureFlags flags) const override;

  plDXAdapter& GetAdapter();
  ComPtr<ID3D12Device> GetDevice();
  plDXCPUDescriptorPool& GetCPUDescriptorPool();
  plDXGPUDescriptorPool& GetGPUDescriptorPool();
  bool IsRenderPassesSupported() const;
  bool IsUnderGraphicsDebugger() const;
  bool IsCreateNotZeroedAvailable() const;
  ID3D12CommandSignature* GetCommandSignature(D3D12_INDIRECT_ARGUMENT_TYPE type, plUInt32 stride);

private:
  plRHIRaytracingASPrebuildInfo GetAccelerationStructurePrebuildInfo(const D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_INPUTS& inputs) const;

  plDXAdapter& m_Adapter;
  ComPtr<ID3D12Device> m_Device;
  ComPtr<ID3D12Device5> m_Device5;
  plMap<plRHICommandListType, plSharedPtr<plDXCommandQueue>> m_CommandQueues;
  plDXCPUDescriptorPool m_CpuDescriptorPool;
  plDXGPUDescriptorPool m_GpuDescriptorPool;
  bool m_IsDxrSupported = false;
  bool m_IsRayQuerySupported = false;
  bool m_IsRenderPassesSupported = false;
  bool m_IsVariableRateShadingSupported = false;
  bool m_IsMeshShadingSupported = false;
  plUInt32 m_ShadingRateImageTileSize = 0;
  bool m_IsUnderGraphicsDebugger = false;
  bool m_IsCreateNotZeroedAvailable = false;
  plMap<std::pair<D3D12_INDIRECT_ARGUMENT_TYPE, plUInt32>, ComPtr<ID3D12CommandSignature>> m_CommandSignatureCache;
};

D3D12_RESOURCE_STATES ConvertState(plRHIResourceState state);
D3D12_HEAP_TYPE GetHeapType(plRHIMemoryType memoryType);
D3D12_RAYTRACING_GEOMETRY_DESC FillRaytracingGeometryDesc(const plRHIBufferDesc& vertex, const plRHIBufferDesc& index, plRHIRaytracingGeometryFlags flags);
D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BUILD_FLAGS Convert(plRHIBuildAccelerationStructureFlags flags);
