#pragma once
#include <RHIVulkan/RHIVulkanDLL.h>

#include <RHI/Device/Device.h>
#include <RHIVulkan/GPUDescriptorPool/VKGPUBindlessDescriptorPoolTyped.h>
#include <RHIVulkan/GPUDescriptorPool/VKGPUDescriptorPool.h>

class plVKAdapter;
class plVKCommandQueue;

class plVKDevice : public plRHIDevice
{
public:
  plVKDevice(plVKAdapter& adapter);
  plSharedPtr<plRHIMemory> AllocateMemory(plUInt64 size, plRHIMemoryType memoryType, plUInt32 memoryTypeBits) override;
  plSharedPtr<plRHICommandQueue> GetCommandQueue(plRHICommandListType type) override;

  plSharedPtr<plRHISwapchain> CreateSwapchain(plRHIWindow window, plUInt32 width, plUInt32 height, plUInt32 frameCount, bool vsync) override;
  void DestroySwapchain(plRHISwapchain* pSwapChain) override {}

  plSharedPtr<plRHICommandList> CreateCommandList(plRHICommandListType type) override;
  void DestroyCommandList(plRHICommandList* pCommandList) override {}

  plSharedPtr<plRHIFence> CreateFence(plUInt64 initial_value) override;
  void DestroyFence(plRHIFence* pFence) override {}

  plSharedPtr<plRHIResource> CreateTexture(plRHITextureType type, plUInt32 bindFlags, plRHIResourceFormat::Enum format, plUInt32 sample_count, int width, int height, int depth, int mipLevels) override;
  void DestroyTexture(plRHIResource* pTexture) override {}

  plSharedPtr<plRHIResource> CreateBuffer(plUInt32 bind_flag, plUInt32 buffer_size) override;
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
  plRHIRaytracingASPrebuildInfo GetTLASPrebuildInfo(plUInt32 instance_count, plRHIBuildAccelerationStructureFlags flags) const override;

  plVKAdapter& GetAdapter();
  vk::Device GetDevice();
  plRHICommandListType GetAvailableCommandListType(plRHICommandListType type);
  vk::CommandPool GetCmdPool(plRHICommandListType type);
  vk::ImageAspectFlags GetAspectFlags(vk::Format format) const;
  plUniquePtr<plVKGPUBindlessDescriptorPoolTyped>& GetGPUBindlessDescriptorPool(vk::DescriptorType type);
  plVKGPUDescriptorPool& GetGPUDescriptorPool();
  plUInt32 FindMemoryType(plUInt32 type_filter, vk::MemoryPropertyFlags properties);
  vk::AccelerationStructureGeometryKHR FillRaytracingGeometryTriangles(const plRHIBufferDesc& vertex, const plRHIBufferDesc& index, plRHIRaytracingGeometryFlags flags) const;

private:
  plRHIRaytracingASPrebuildInfo GetAccelerationStructurePrebuildInfo(const vk::AccelerationStructureBuildGeometryInfoKHR& acceleration_structure_info, const std::vector<plUInt32>& max_primitive_counts) const;

  plVKAdapter& m_Adapter;
  const vk::PhysicalDevice& m_PhysicalDevice;
  vk::UniqueDevice m_Device;
  struct QueueInfo
  {
    plUInt32 QueueFamilyIndex;
    plUInt32 QueueCount;
  };
  plMap<plRHICommandListType, QueueInfo> m_QueuesInfo;
  plMap<plRHICommandListType, vk::UniqueCommandPool> m_CmdPools;
  plMap<plRHICommandListType, plSharedPtr<plVKCommandQueue>> m_CommandQueues;
  plMap<vk::DescriptorType, plUniquePtr<plVKGPUBindlessDescriptorPoolTyped>> m_GPUBindlessDescriptorPool;
  plVKGPUDescriptorPool m_GPUDescriptorPool;
  bool m_is_variable_rate_shading_supported = false;
  plUInt32 m_shading_rate_image_tile_size = 0;
  bool m_is_dxr_supported = false;
  bool m_is_ray_query_supported = false;
  bool m_is_mesh_shading_supported = false;
  plUInt32 m_shader_group_handle_size = 0;
  plUInt32 m_shader_record_alignment = 0;
  plUInt32 m_shader_table_alignment = 0;
};

vk::ImageLayout ConvertState(plRHIResourceState state);
vk::BuildAccelerationStructureFlagsKHR Convert(plRHIBuildAccelerationStructureFlags flags);
