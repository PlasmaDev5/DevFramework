#include <RHI/Shader/ShaderBase.h>
#include <RHIDX12/Adapter/DXAdapter.h>
#include <RHIDX12/BindingSet/DXBindingSet.h>
#include <RHIDX12/BindingSetLayout/DXBindingSetLayout.h>
#include <RHIDX12/CommandList/DXCommandList.h>
#include <RHIDX12/CommandQueue/DXCommandQueue.h>
#include <RHIDX12/Device/DXDevice.h>
#include <RHIDX12/Fence/DXFence.h>
#include <RHIDX12/Framebuffer/DXFramebuffer.h>
#include <RHIDX12/Memory/DXMemory.h>
#include <RHIDX12/Pipeline/DXComputePipeline.h>
#include <RHIDX12/Pipeline/DXGraphicsPipeline.h>
#include <RHIDX12/Pipeline/DXRayTracingPipeline.h>
#include <RHIDX12/Program/DXProgram.h>
#include <RHIDX12/QueryHeap/DXRayTracingQueryHeap.h>
#include <RHIDX12/RenderPass/DXRenderPass.h>
#include <RHIDX12/Swapchain/DXSwapchain.h>
#include <RHIDX12/Utilities/DXUtility.h>
#include <RHIDX12/View/DXView.h>
#include <directx/d3dx12.h>
#include <dxgi1_6.h>

D3D12_RESOURCE_STATES ConvertState(plRHIResourceState state)
{
  static std::pair<plRHIResourceState, D3D12_RESOURCE_STATES> mapping[] = {
    {plRHIResourceState::kCommon, D3D12_RESOURCE_STATE_COMMON},
    {plRHIResourceState::kVertexAndConstantBuffer, D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER},
    {plRHIResourceState::kIndexBuffer, D3D12_RESOURCE_STATE_INDEX_BUFFER},
    {plRHIResourceState::kRenderTarget, D3D12_RESOURCE_STATE_RENDER_TARGET},
    {plRHIResourceState::kUnorderedAccess, D3D12_RESOURCE_STATE_UNORDERED_ACCESS},
    {plRHIResourceState::kDepthStencilWrite, D3D12_RESOURCE_STATE_DEPTH_WRITE},
    {plRHIResourceState::kDepthStencilRead, D3D12_RESOURCE_STATE_DEPTH_READ},
    {plRHIResourceState::kNonPixelShaderResource, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE},
    {plRHIResourceState::kPixelShaderResource, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE},
    {plRHIResourceState::kIndirectArgument, D3D12_RESOURCE_STATE_INDIRECT_ARGUMENT},
    {plRHIResourceState::kCopyDest, D3D12_RESOURCE_STATE_COPY_DEST},
    {plRHIResourceState::kCopySource, D3D12_RESOURCE_STATE_COPY_SOURCE},
    {plRHIResourceState::kRaytracingAccelerationStructure, D3D12_RESOURCE_STATE_RAYTRACING_ACCELERATION_STRUCTURE},
    {plRHIResourceState::kShadingRateSource, D3D12_RESOURCE_STATE_SHADING_RATE_SOURCE},
    {plRHIResourceState::kPresent, D3D12_RESOURCE_STATE_PRESENT},
    {plRHIResourceState::kGenericRead, D3D12_RESOURCE_STATE_GENERIC_READ},
  };

  D3D12_RESOURCE_STATES res = {};
  for (const auto& m : mapping)
  {
    if (state & m.first)
    {
      res |= m.second;
      state &= ~m.first;
    }
  }
  assert(state == 0);
  return res;
}

D3D12_HEAP_TYPE GetHeapType(plRHIMemoryType memoryType)
{
  switch (memoryType)
  {
    case plRHIMemoryType::kDefault:
      return D3D12_HEAP_TYPE_DEFAULT;
    case plRHIMemoryType::kUpload:
      return D3D12_HEAP_TYPE_UPLOAD;
    case plRHIMemoryType::kReadback:
      return D3D12_HEAP_TYPE_READBACK;
    default:
      assert(false);
      return D3D12_HEAP_TYPE_CUSTOM;
  }
}

static const GUID renderdocUuid = {0xa7aa6116, 0x9c8d, 0x4bba, {0x90, 0x83, 0xb4, 0xd8, 0x16, 0xb7, 0x1b, 0x78}};
static const GUID pixUuid = {0x9f251514, 0x9d4d, 0x4902, {0x9d, 0x60, 0x18, 0x98, 0x8a, 0xb7, 0xd4, 0xb5}};
static const GUID gpaUuuid = {0xccffef16, 0x7b69, 0x468f, {0xbc, 0xe3, 0xcd, 0x95, 0x33, 0x69, 0xa3, 0x9a}};

plDXDevice::plDXDevice(plDXAdapter& adapter)
  : m_Adapter(adapter)
  , m_CpuDescriptorPool(*this)
  , m_GpuDescriptorPool(*this)
{
  PL_ASSERT_ALWAYS(D3D12CreateDevice(m_Adapter.GetAdapter().Get(), D3D_FEATURE_LEVEL_11_1, IID_PPV_ARGS(&m_Device)) == S_OK, "");
  m_Device.As(&m_Device5);

  ComPtr<IUnknown> renderdoc;
  if (SUCCEEDED(m_Device->QueryInterface(renderdocUuid, &renderdoc)))
    m_IsUnderGraphicsDebugger |= !!renderdoc;

  ComPtr<IUnknown> pix;
  if (SUCCEEDED(DXGIGetDebugInterface1(0, pixUuid, &pix)))
    m_IsUnderGraphicsDebugger |= !!pix;

  ComPtr<IUnknown> gpa;
  if (SUCCEEDED(m_Device->QueryInterface(gpaUuuid, &gpa)))
    m_IsUnderGraphicsDebugger |= !!gpa;

  D3D12_FEATURE_DATA_D3D12_OPTIONS5 featureSupport5 = {};
  if (SUCCEEDED(m_Device->CheckFeatureSupport(D3D12_FEATURE_D3D12_OPTIONS5, &featureSupport5, sizeof(featureSupport5))))
  {
    m_IsDxrSupported = featureSupport5.RaytracingTier >= D3D12_RAYTRACING_TIER_1_0;
    m_IsRenderPassesSupported = featureSupport5.RenderPassesTier >= D3D12_RENDER_PASS_TIER_0;
    m_IsRayQuerySupported = featureSupport5.RaytracingTier >= D3D12_RAYTRACING_TIER_1_1;
  }

  D3D12_FEATURE_DATA_D3D12_OPTIONS6 featureSupport6 = {};
  if (SUCCEEDED(m_Device->CheckFeatureSupport(D3D12_FEATURE_D3D12_OPTIONS6, &featureSupport6, sizeof(featureSupport6))))
  {
    m_IsVariableRateShadingSupported = featureSupport6.VariableShadingRateTier >= D3D12_VARIABLE_SHADING_RATE_TIER_2;
    m_ShadingRateImageTileSize = featureSupport6.ShadingRateImageTileSize;
  }

  D3D12_FEATURE_DATA_D3D12_OPTIONS7 featureSupport7 = {};
  if (SUCCEEDED(m_Device->CheckFeatureSupport(D3D12_FEATURE_D3D12_OPTIONS7, &featureSupport7, sizeof(featureSupport7))))
  {
    m_IsCreateNotZeroedAvailable = true;
    m_IsMeshShadingSupported = featureSupport7.MeshShaderTier >= D3D12_MESH_SHADER_TIER_1;
  }

  m_CommandQueues[plRHICommandListType::kGraphics] = PL_DEFAULT_NEW(plDXCommandQueue, *this, plRHICommandListType::kGraphics);
  m_CommandQueues[plRHICommandListType::kCompute] = PL_DEFAULT_NEW(plDXCommandQueue, *this, plRHICommandListType::kCompute);
  m_CommandQueues[plRHICommandListType::kCopy] = PL_DEFAULT_NEW(plDXCommandQueue, *this, plRHICommandListType::kCopy);

  static const bool debugEnabled = IsDebuggerPresent();
  if (debugEnabled)
  {
    ComPtr<ID3D12InfoQueue> infoQueue;
    if (SUCCEEDED(m_Device.As(&infoQueue)))
    {
      infoQueue->SetBreakOnSeverity(D3D12_MESSAGE_SEVERITY_CORRUPTION, true);
      infoQueue->SetBreakOnSeverity(D3D12_MESSAGE_SEVERITY_ERROR, true);

      D3D12_MESSAGE_SEVERITY severities[] =
        {
          D3D12_MESSAGE_SEVERITY_INFO,
        };

      D3D12_MESSAGE_ID denyIds[] =
        {
          D3D12_MESSAGE_ID_COPY_DESCRIPTORS_INVALID_RANGES,
        };

      D3D12_INFO_QUEUE_FILTER filter = {};
      filter.DenyList.NumSeverities = PL_ARRAY_SIZE(severities);
      filter.DenyList.pSeverityList = severities;
      filter.DenyList.NumIDs = PL_ARRAY_SIZE(denyIds);
      filter.DenyList.pIDList = denyIds;
      infoQueue->PushStorageFilter(&filter);
    }

    /*ComPtr<ID3D12DebugDevice2> debugDevice;
        m_Device.As(&debugDevice);
        D3D12_DEBUG_FEATURE debugFeature = D3D12_DEBUG_FEATURE_CONSERVATIVE_RESOURCE_STATE_TRACKING;
        debugDevice->SetDebugParameter(D3D12_DEBUG_DEVICE_PARAMETER_FEATURE_FLAGS, &debugFeature, sizeof(debugFeature));*/
  }
}

plSharedPtr<plRHIMemory> plDXDevice::AllocateMemory(plUInt64 size, plRHIMemoryType memoryType, plUInt32 memoryTypeBits)
{
  return PL_DEFAULT_NEW(plDXMemory, *this, size, memoryType, memoryTypeBits);
}

plSharedPtr<plRHICommandQueue> plDXDevice::GetCommandQueue(plRHICommandListType type)
{
  return m_CommandQueues[type];
}

plSharedPtr<plRHISwapchain> plDXDevice::CreateSwapchain(plRHIWindow window, plUInt32 width, plUInt32 height, plUInt32 frameCount, bool vsync)
{
  return PL_DEFAULT_NEW(plDXSwapchain, *m_CommandQueues[plRHICommandListType::kGraphics], window, width, height, frameCount, vsync);
}

plSharedPtr<plRHICommandList> plDXDevice::CreateCommandList(plRHICommandListType type)
{
  return PL_DEFAULT_NEW(plDXCommandList, *this, type);
}

plSharedPtr<plRHIFence> plDXDevice::CreateFence(plUInt64 initialValue)
{
  return PL_DEFAULT_NEW(plDXFence, *this, initialValue);
}

plSharedPtr<plRHIResource> plDXDevice::CreateTexture(plRHITextureType type, plUInt32 bindFlags, plRHIResourceFormat::Enum format, plUInt32 sampleCount, int width, int height, int depth, int mipLevels)
{
  DXGI_FORMAT dxFormat = plDXUtils::ToDXGIFormat(format); //static_cast<DXGI_FORMAT>(gli::dx().translate(format).DXGIFormat.DDS);
  if (bindFlags & plRHIBindFlag::kShaderResource)
  {
    dxFormat = plDXUtils::MakeTypelessDepthStencil(dxFormat);
  }

  plSharedPtr<plDXResource> res = PL_DEFAULT_NEW(plDXResource, *this);
  res->ResourceType = plRHIResourceType::kTexture;
  res->Format = format;

  D3D12_RESOURCE_DESC desc = {};
  switch (type)
  {
    case plRHITextureType::k1D:
      desc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE1D;
      break;
    case plRHITextureType::k2D:
      desc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
      break;
    case plRHITextureType::k3D:
      desc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE3D;
      break;
  }
  desc.Width = width;
  desc.Height = height;
  desc.DepthOrArraySize = (plUInt16)depth;
  desc.MipLevels = (plUInt16)mipLevels;
  desc.Format = dxFormat;

  D3D12_FEATURE_DATA_MULTISAMPLE_QUALITY_LEVELS msCheckDesc = {};
  msCheckDesc.Format = desc.Format;
  msCheckDesc.SampleCount = sampleCount;
  m_Device->CheckFeatureSupport(D3D12_FEATURE_MULTISAMPLE_QUALITY_LEVELS, &msCheckDesc, sizeof(msCheckDesc));
  desc.SampleDesc.Count = sampleCount;
  desc.SampleDesc.Quality = msCheckDesc.NumQualityLevels - 1;

  if (bindFlags & plRHIBindFlag::kRenderTarget)
    desc.Flags |= D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET;
  if (bindFlags & plRHIBindFlag::kDepthStencil)
    desc.Flags |= D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL;
  if (bindFlags & plRHIBindFlag::kUnorderedAccess)
    desc.Flags |= D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS;

  res->desc = desc;
  res->SetInitialState(plRHIResourceState::kCommon);
  return res;
}

plSharedPtr<plRHIResource> plDXDevice::CreateBuffer(plUInt32 bindFlag, plUInt32 bufferSize)
{
  if (bufferSize == 0)
    return {};

  plSharedPtr<plDXResource> res = PL_DEFAULT_NEW(plDXResource, *this);

  if (bindFlag & plRHIBindFlag::kConstantBuffer)
    bufferSize = (bufferSize + 255) & ~255;

  auto desc = CD3DX12_RESOURCE_DESC::Buffer(bufferSize);

  res->ResourceType = plRHIResourceType::kBuffer;

  plRHIResourceState state = plRHIResourceState::kCommon;
  if (bindFlag & plRHIBindFlag::kRenderTarget)
  {
    desc.Flags = D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET;
  }
  if (bindFlag & plRHIBindFlag::kDepthStencil)
  {
    desc.Flags = D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL;
  }
  if (bindFlag & plRHIBindFlag::kUnorderedAccess)
  {
    desc.Flags = D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS;
  }
  if (bindFlag & plRHIBindFlag::kAccelerationStructure)
  {
    state = plRHIResourceState::kRaytracingAccelerationStructure;
    desc.Flags = D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS;
  }

  res->desc = desc;
  res->SetInitialState(state);
  return res;
}

plSharedPtr<plRHIResource> plDXDevice::CreateSampler(const plRHISamplerDesc& desc)
{
  plSharedPtr<plDXResource> res = PL_DEFAULT_NEW(plDXResource, *this);
  D3D12_SAMPLER_DESC& samplerDesc = res->samplerDesc;

  switch (desc.filter)
  {
    case plRHISamplerFilter::kAnisotropic:
      samplerDesc.Filter = D3D12_FILTER_ANISOTROPIC;
      break;
    case plRHISamplerFilter::kMinMagMipLinear:
      samplerDesc.Filter = D3D12_FILTER_MIN_MAG_MIP_LINEAR;
      break;
    case plRHISamplerFilter::kComparisonMinMagMipLinear:
      samplerDesc.Filter = D3D12_FILTER_COMPARISON_MIN_MAG_MIP_LINEAR;
      break;
  }

  switch (desc.mode)
  {
    case plRHISamplerTextureAddressMode::kWrap:
      samplerDesc.AddressU = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
      samplerDesc.AddressV = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
      samplerDesc.AddressW = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
      break;
    case plRHISamplerTextureAddressMode::kClamp:
      samplerDesc.AddressU = D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
      samplerDesc.AddressV = D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
      samplerDesc.AddressW = D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
      break;
  }

  switch (desc.func)
  {
    case plRHISamplerComparisonFunc::kNever:
      samplerDesc.ComparisonFunc = D3D12_COMPARISON_FUNC_NEVER;
      break;
    case plRHISamplerComparisonFunc::kAlways:
      samplerDesc.ComparisonFunc = D3D12_COMPARISON_FUNC_ALWAYS;
      break;
    case plRHISamplerComparisonFunc::kLess:
      samplerDesc.ComparisonFunc = D3D12_COMPARISON_FUNC_LESS;
      break;
  }

  samplerDesc.MinLOD = 0;
  samplerDesc.MaxLOD = plMath::MaxValue<float>();
  samplerDesc.MaxAnisotropy = 1;

  return res;
}

plSharedPtr<plRHIView> plDXDevice::CreateView(const plSharedPtr<plRHIResource>& resource, const plRHIViewDesc& viewDesc)
{
  return PL_DEFAULT_NEW(plDXView, *this, resource.Downcast<plDXResource>(), viewDesc);
}

plSharedPtr<plRHIBindingSetLayout> plDXDevice::CreateBindingSetLayout(const std::vector<plRHIBindKey>& descs)
{
  return PL_DEFAULT_NEW(plDXBindingSetLayout, *this, descs);
}

plSharedPtr<plRHIBindingSet> plDXDevice::CreateBindingSet(const plSharedPtr<plRHIBindingSetLayout>& layout)
{
  return PL_DEFAULT_NEW(plDXBindingSet, *this, layout.Downcast<plDXBindingSetLayout>());
}

plSharedPtr<plRHIRenderPass> plDXDevice::CreateRenderPass(const plRHIRenderPassDesc& desc)
{
  return PL_DEFAULT_NEW(plDXRenderPass, *this, desc);
}

plSharedPtr<plRHIFramebuffer> plDXDevice::CreateFramebuffer(const plRHIFramebufferDesc& desc)
{
  return PL_DEFAULT_NEW(plDXFramebuffer, desc);
}

plSharedPtr<plRHIShader> plDXDevice::CreateShader(const plRHIShaderDesc& desc, plDynamicArray<plUInt8> byteCode, plSharedPtr<plRHIShaderReflection> reflection)
{
  return PL_DEFAULT_NEW(plRHIShaderBase, desc, byteCode, reflection, plRHIShaderBlobType::kDXIL);
}

plSharedPtr<plRHIProgram> plDXDevice::CreateProgram(const std::vector<plSharedPtr<plRHIShader>>& shaders)
{
  return PL_DEFAULT_NEW(plDXProgram, *this, shaders);
}

plSharedPtr<plRHIPipeline> plDXDevice::CreateGraphicsPipeline(const plRHIGraphicsPipelineDesc& desc)
{
  return PL_DEFAULT_NEW(plDXGraphicsPipeline, *this, desc);
}

plSharedPtr<plRHIPipeline> plDXDevice::CreateComputePipeline(const plRHIComputePipelineDesc& desc)
{
  return PL_DEFAULT_NEW(plDXComputePipeline, *this, desc);
}

plSharedPtr<plRHIPipeline> plDXDevice::CreateRayTracingPipeline(const plRHIRayTracingPipelineDesc& desc)
{
  return PL_DEFAULT_NEW(plDXRayTracingPipeline, *this, desc);
}

plSharedPtr<plRHIResource> plDXDevice::CreateAccelerationStructure(plRHIAccelerationStructureType type, const plSharedPtr<plRHIResource>& resource, plUInt64 offset)
{
  plSharedPtr<plDXResource> res = PL_DEFAULT_NEW(plDXResource, *this);
  res->ResourceType = plRHIResourceType::kAccelerationStructure;
  res->accelerationStructuresMemory = resource;
  res->accelerationStructureHandle = resource.Downcast<plDXResource>()->resource->GetGPUVirtualAddress() + offset;
  return res;
}

plSharedPtr<plRHIQueryHeap> plDXDevice::CreateQueryHeap(plRHIQueryHeapType type, plUInt32 count)
{
  if (type == plRHIQueryHeapType::kAccelerationStructureCompactedSize)
  {
    return PL_DEFAULT_NEW(plDXRayTracingQueryHeap, *this, type, count);
  }
  return {};
}

D3D12_RAYTRACING_GEOMETRY_DESC FillRaytracingGeometryDesc(const plRHIBufferDesc& vertex, const plRHIBufferDesc& index, plRHIRaytracingGeometryFlags flags)
{
  D3D12_RAYTRACING_GEOMETRY_DESC geometryDesc = {};

  auto vertexRes = vertex.res.Downcast<plDXResource>();
  auto indexRes = index.res.Downcast<plDXResource>();

  geometryDesc.Type = D3D12_RAYTRACING_GEOMETRY_TYPE_TRIANGLES;
  switch (flags)
  {
    case plRHIRaytracingGeometryFlags::kOpaque:
      geometryDesc.Flags = D3D12_RAYTRACING_GEOMETRY_FLAG_OPAQUE;
      break;
    case plRHIRaytracingGeometryFlags::kNoDuplicateAnyHitInvocation:
      geometryDesc.Flags = D3D12_RAYTRACING_GEOMETRY_FLAG_NO_DUPLICATE_ANYHIT_INVOCATION;
      break;
  }

  auto vertexStride = plRHIResourceFormat::GetFormatStride(vertex.format); //gli::detail::bits_per_pixel(vertex.format) / 8;
  geometryDesc.Triangles.VertexBuffer.StartAddress = vertexRes->resource->GetGPUVirtualAddress() + vertex.offset * vertexStride;
  geometryDesc.Triangles.VertexBuffer.StrideInBytes = vertexStride;
  geometryDesc.Triangles.VertexFormat = plDXUtils::ToDXGIFormat(vertex.format); //static_cast<DXGI_FORMAT>(gli::dx().translate(vertex.format).DXGIFormat.DDS);
  geometryDesc.Triangles.VertexCount = vertex.count;
  if (indexRes)
  {
    auto indexStride = plRHIResourceFormat::GetFormatStride(index.format); //gli::detail::bits_per_pixel(index.format) / 8;
    geometryDesc.Triangles.IndexBuffer = indexRes->resource->GetGPUVirtualAddress() + index.offset * indexStride;
    geometryDesc.Triangles.IndexFormat = plDXUtils::ToDXGIFormat(index.format); //static_cast<DXGI_FORMAT>(gli::dx().translate(index.format).DXGIFormat.DDS);
    geometryDesc.Triangles.IndexCount = index.count;
  }

  return geometryDesc;
}

plRHIRaytracingASPrebuildInfo plDXDevice::GetAccelerationStructurePrebuildInfo(const D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_INPUTS& inputs) const
{
  D3D12_RAYTRACING_ACCELERATION_STRUCTURE_PREBUILD_INFO info = {};
  m_Device5->GetRaytracingAccelerationStructurePrebuildInfo(&inputs, &info);
  plRHIRaytracingASPrebuildInfo prebuildInfo = {};
  prebuildInfo.accelerationStructureSize = info.ResultDataMaxSizeInBytes;
  prebuildInfo.buildScratchDataSize = info.ScratchDataSizeInBytes;
  prebuildInfo.updateScratchDataSize = info.UpdateScratchDataSizeInBytes;
  return prebuildInfo;
}

D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BUILD_FLAGS Convert(plRHIBuildAccelerationStructureFlags flags)
{
  D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BUILD_FLAGS dxFlags = {};
  if (flags & plRHIBuildAccelerationStructureFlags::kAllowUpdate)
    dxFlags |= D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BUILD_FLAG_ALLOW_UPDATE;
  if (flags & plRHIBuildAccelerationStructureFlags::kAllowCompaction)
    dxFlags |= D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BUILD_FLAG_ALLOW_COMPACTION;
  if (flags & plRHIBuildAccelerationStructureFlags::kPreferFastTrace)
    dxFlags |= D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BUILD_FLAG_PREFER_FAST_TRACE;
  if (flags & plRHIBuildAccelerationStructureFlags::kPreferFastBuild)
    dxFlags |= D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BUILD_FLAG_PREFER_FAST_BUILD;
  if (flags & plRHIBuildAccelerationStructureFlags::kMinimizeMemory)
    dxFlags |= D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BUILD_FLAG_MINIMIZE_MEMORY;
  return dxFlags;
}

plUInt32 plDXDevice::GetTextureDataPitchAlignment() const
{
  return D3D12_TEXTURE_DATA_PITCH_ALIGNMENT;
}

bool plDXDevice::IsDxrSupported() const
{
  return m_IsDxrSupported;
}

bool plDXDevice::IsRayQuerySupported() const
{
  return m_IsRayQuerySupported;
}

bool plDXDevice::IsVariableRateShadingSupported() const
{
  return m_IsVariableRateShadingSupported;
}

bool plDXDevice::IsMeshShadingSupported() const
{
  return m_IsMeshShadingSupported;
}

plUInt32 plDXDevice::GetShadingRateImageTileSize() const
{
  return m_ShadingRateImageTileSize;
}

plRHIMemoryBudget plDXDevice::GetMemoryBudget() const
{
  ComPtr<IDXGIAdapter3> adapter3;
  m_Adapter.GetAdapter().As(&adapter3);
  DXGI_QUERY_VIDEO_MEMORY_INFO localMemoryInfo = {};
  adapter3->QueryVideoMemoryInfo(0, DXGI_MEMORY_SEGMENT_GROUP_LOCAL, &localMemoryInfo);
  DXGI_QUERY_VIDEO_MEMORY_INFO nonLocalMemoryInfo = {};
  adapter3->QueryVideoMemoryInfo(0, DXGI_MEMORY_SEGMENT_GROUP_NON_LOCAL, &nonLocalMemoryInfo);
  return {localMemoryInfo.Budget + nonLocalMemoryInfo.Budget, localMemoryInfo.CurrentUsage + nonLocalMemoryInfo.CurrentUsage};
}

plUInt32 plDXDevice::GetShaderGroupHandleSize() const
{
  return D3D12_SHADER_IDENTIFIER_SIZE_IN_BYTES;
}

plUInt32 plDXDevice::GetShaderRecordAlignment() const
{
  return D3D12_RAYTRACING_SHADER_RECORD_BYTE_ALIGNMENT;
}

plUInt32 plDXDevice::GetShaderTableAlignment() const
{
  return D3D12_RAYTRACING_SHADER_TABLE_BYTE_ALIGNMENT;
}

plRHIRaytracingASPrebuildInfo plDXDevice::GetBLASPrebuildInfo(const std::vector<plRHIRaytracingGeometryDesc>& descs, plRHIBuildAccelerationStructureFlags flags) const
{
  std::vector<D3D12_RAYTRACING_GEOMETRY_DESC> geometryDescs;
  for (const auto& desc : descs)
  {
    geometryDescs.emplace_back(FillRaytracingGeometryDesc(desc.vertex, desc.index, desc.flags));
  }
  D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_INPUTS inputs = {};
  inputs.Type = D3D12_RAYTRACING_ACCELERATION_STRUCTURE_TYPE_BOTTOM_LEVEL;
  inputs.DescsLayout = D3D12_ELEMENTS_LAYOUT_ARRAY;
  inputs.NumDescs = (plUInt32)geometryDescs.size();
  inputs.pGeometryDescs = geometryDescs.data();
  inputs.Flags = Convert(flags);
  return GetAccelerationStructurePrebuildInfo(inputs);
}

plRHIRaytracingASPrebuildInfo plDXDevice::GetTLASPrebuildInfo(plUInt32 instanceCount, plRHIBuildAccelerationStructureFlags flags) const
{
  D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_INPUTS inputs = {};
  inputs.Type = D3D12_RAYTRACING_ACCELERATION_STRUCTURE_TYPE_TOP_LEVEL;
  inputs.DescsLayout = D3D12_ELEMENTS_LAYOUT_ARRAY;
  inputs.NumDescs = instanceCount;
  inputs.Flags = Convert(flags);
  return GetAccelerationStructurePrebuildInfo(inputs);
}

plDXAdapter& plDXDevice::GetAdapter()
{
  return m_Adapter;
}

ComPtr<ID3D12Device> plDXDevice::GetDevice()
{
  return m_Device;
}

plDXCPUDescriptorPool& plDXDevice::GetCPUDescriptorPool()
{
  return m_CpuDescriptorPool;
}

plDXGPUDescriptorPool& plDXDevice::GetGPUDescriptorPool()
{
  return m_GpuDescriptorPool;
}

bool plDXDevice::IsRenderPassesSupported() const
{
  return m_IsRenderPassesSupported;
}

bool plDXDevice::IsUnderGraphicsDebugger() const
{
  return m_IsUnderGraphicsDebugger;
}

bool plDXDevice::IsCreateNotZeroedAvailable() const
{
  return m_IsCreateNotZeroedAvailable;
}

ID3D12CommandSignature* plDXDevice::GetCommandSignature(D3D12_INDIRECT_ARGUMENT_TYPE type, plUInt32 stride)
{
  auto it = m_CommandSignatureCache.Find(std::pair{type, stride});
  if (it != end(m_CommandSignatureCache))
  {
    return it.Value().Get();
  }

  D3D12_INDIRECT_ARGUMENT_DESC arg = {};
  arg.Type = type;
  D3D12_COMMAND_SIGNATURE_DESC desc = {};
  desc.NumArgumentDescs = 1;
  desc.pArgumentDescs = &arg;
  desc.ByteStride = stride;
  ComPtr<ID3D12CommandSignature> commandSignature;
  PL_ASSERT_ALWAYS(m_Device->CreateCommandSignature(
                     &desc,
                     nullptr,
                     IID_PPV_ARGS(&commandSignature)) == S_OK,
    "");

  m_CommandSignatureCache.Insert(std::pair(type, stride), ComPtr<ID3D12CommandSignature>(commandSignature));

  return commandSignature.Get();
}
