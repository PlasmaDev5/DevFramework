#pragma once
#include <RHI/RHIDLL.h>

#include <RHI/BindingSetLayout/BindingSetLayout.h>
#include <RHI/CommandList/CommandList.h>
#include <RHI/CommandQueue/CommandQueue.h>
#include <RHI/Fence/Fence.h>
#include <RHI/Framebuffer/Framebuffer.h>
#include <RHI/Instance/BaseTypes.h>
#include <RHI/Instance/QueryInterface.h>
#include <RHI/Memory/Memory.h>
#include <RHI/Pipeline/Pipeline.h>
#include <RHI/Program/Program.h>
#include <RHI/QueryHeap/QueryHeap.h>
#include <RHI/RenderPass/RenderPass.h>
#include <RHI/Shader/Shader.h>
#include <RHI/Swapchain/Swapchain.h>
#include <memory>
#include <vector>

class plRHIDevice;

static plSharedPtr<plRHIDevice> g_GraphicsDevice = nullptr;

struct PL_RHI_DLL plRHIMemoryBudget
{
  plUInt64 budget;
  plUInt64 usage;
};

class PL_RHI_DLL plRHIDevice : public plRefCounted
{
public:
  virtual ~plRHIDevice() = default;
  virtual plSharedPtr<plRHIMemory> AllocateMemory(plUInt64 size, plRHIMemoryType memoryType, plUInt32 memoryTypeBits) = 0;
  virtual plSharedPtr<plRHICommandQueue> GetCommandQueue(plRHICommandListType type) = 0;

  virtual plSharedPtr<plRHISwapchain> CreateSwapchain(plRHIWindow window, plUInt32 width, plUInt32 height, plUInt32 frameCount, bool vsync) = 0;
  virtual void DestroySwapchain(plRHISwapchain* pSwapChain) = 0;

  virtual plSharedPtr<plRHICommandList> CreateCommandList(plRHICommandListType type) = 0;
  virtual void DestroyCommandList(plRHICommandList* pCommandList) = 0;

  virtual plSharedPtr<plRHIFence> CreateFence(plUInt64 initialValue) = 0;
  virtual void DestroyFence(plRHIFence* pFence) = 0;

  virtual plSharedPtr<plRHIResource> CreateTexture(plRHITextureType type, plUInt32 bindFlags, plRHIResourceFormat::Enum format, plUInt32 sampleCount, int width, int height, int depth, int mipLevels) = 0;
  virtual void DestroyTexture(plRHIResource* pTexture) = 0;

  virtual plSharedPtr<plRHIResource> CreateBuffer(plUInt32 bindFlag, plUInt32 bufferSize) = 0;
  virtual void DestroyBuffer(plRHIResource* pBuffer) = 0;

  virtual plSharedPtr<plRHIResource> CreateSampler(const plRHISamplerDesc& desc) = 0;
  virtual void DestroySampler(plRHIResource* pSampler) = 0;

  virtual plSharedPtr<plRHIView> CreateView(const plSharedPtr<plRHIResource>& resource, const plRHIViewDesc& viewDesc) = 0;
  virtual void DestroyView(plRHIView* pView) = 0;

  virtual plSharedPtr<plRHIBindingSetLayout> CreateBindingSetLayout(const std::vector<plRHIBindKey>& descs) = 0;
  virtual void DestroyBindingSetLayout(plRHIBindingSetLayout* pBindingSetLayout) = 0;

  virtual plSharedPtr<plRHIBindingSet> CreateBindingSet(const plSharedPtr<plRHIBindingSetLayout>& layout) = 0;
  virtual void DestroyBindingSet(plRHIBindingSet* pBindingSet) = 0;

  virtual plSharedPtr<plRHIRenderPass> CreateRenderPass(const plRHIRenderPassDesc& desc) = 0;
  virtual void DestroyRenderPass(plRHIRenderPass* pRenderPass) = 0;

  virtual plSharedPtr<plRHIFramebuffer> CreateFramebuffer(const plRHIFramebufferDesc& desc) = 0;
  virtual void DestroyFramebuffer(plRHIFramebuffer* pFramebuffer) = 0;

  virtual plSharedPtr<plRHIShader> CreateShader(const plRHIShaderDesc& desc, plDynamicArray<plUInt8> byteCode, plSharedPtr<plRHIShaderReflection> reflection) = 0;
  virtual void DestroyShader(plRHIShader* pShader) = 0;

  virtual plSharedPtr<plRHIProgram> CreateProgram(const std::vector<plSharedPtr<plRHIShader>>& shaders) = 0;
  virtual void DestroyProgram(plRHIProgram* pProgram) = 0;

  virtual plSharedPtr<plRHIPipeline> CreateGraphicsPipeline(const plRHIGraphicsPipelineDesc& desc) = 0;
  virtual void DestroyGraphicsPipeline(plRHIPipeline* pPipeline) = 0;

  virtual plSharedPtr<plRHIPipeline> CreateComputePipeline(const plRHIComputePipelineDesc& desc) = 0;
  virtual void DestroyComputePipeline(plRHIPipeline* pPipeline) = 0;

  virtual plSharedPtr<plRHIPipeline> CreateRayTracingPipeline(const plRHIRayTracingPipelineDesc& desc) = 0;
  virtual void DestroyRayTracingPipeline(plRHIPipeline* pPipeline) = 0;

  virtual plSharedPtr<plRHIResource> CreateAccelerationStructure(plRHIAccelerationStructureType type, const plSharedPtr<plRHIResource>& resource, plUInt64 offset) = 0;
  virtual void DestroyAccelerationStructure(plRHIResource* pAccelerationStructure) = 0;

  virtual plSharedPtr<plRHIQueryHeap> CreateQueryHeap(plRHIQueryHeapType type, plUInt32 count) = 0;
  virtual void DestroyQueryHeap(plRHIQueryHeap* pQueryHeap) = 0;

  virtual plUInt32 GetTextureDataPitchAlignment() const = 0;
  virtual bool IsDxrSupported() const = 0;
  virtual bool IsRayQuerySupported() const = 0;
  virtual bool IsVariableRateShadingSupported() const = 0;
  virtual bool IsMeshShadingSupported() const = 0;
  virtual plUInt32 GetShadingRateImageTileSize() const = 0;
  virtual plRHIMemoryBudget GetMemoryBudget() const = 0;
  virtual plUInt32 GetShaderGroupHandleSize() const = 0;
  virtual plUInt32 GetShaderRecordAlignment() const = 0;
  virtual plUInt32 GetShaderTableAlignment() const = 0;
  virtual plRHIRaytracingASPrebuildInfo GetBLASPrebuildInfo(const std::vector<plRHIRaytracingGeometryDesc>& descs, plRHIBuildAccelerationStructureFlags flags) const = 0;
  virtual plRHIRaytracingASPrebuildInfo GetTLASPrebuildInfo(plUInt32 instanceCount, plRHIBuildAccelerationStructureFlags flags) const = 0;
};
