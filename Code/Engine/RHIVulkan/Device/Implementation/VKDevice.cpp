#include <RHIVulkan/RHIVulkanPCH.h>

#include <RHI/Shader/ShaderBase.h>
#include <RHIVulkan/Adapter/VKAdapter.h>
#include <RHIVulkan/BindingSet/VKBindingSet.h>
#include <RHIVulkan/BindingSetLayout/VKBindingSetLayout.h>
#include <RHIVulkan/CommandList/VKCommandList.h>
#include <RHIVulkan/CommandQueue/VKCommandQueue.h>
#include <RHIVulkan/Device/VKDevice.h>
#include <RHIVulkan/Fence/VKTimelineSemaphore.h>
#include <RHIVulkan/Framebuffer/VKFramebuffer.h>
#include <RHIVulkan/Instance/VKInstance.h>
#include <RHIVulkan/Memory/VKMemory.h>
#include <RHIVulkan/Pipeline/VKComputePipeline.h>
#include <RHIVulkan/Pipeline/VKGraphicsPipeline.h>
#include <RHIVulkan/Pipeline/VKRayTracingPipeline.h>
#include <RHIVulkan/Program/VKProgram.h>
#include <RHIVulkan/QueryHeap/VKQueryHeap.h>
#include <RHIVulkan/RenderPass/VKRenderPass.h>
#include <RHIVulkan/Swapchain/VKSwapchain.h>
#include <RHIVulkan/Utilities/VKUtility.h>
#include <RHIVulkan/View/VKView.h>

PL_DEFINE_AS_POD_TYPE(vk::QueueFamilyProperties);
PL_DEFINE_AS_POD_TYPE(vk::ExtensionProperties);
PL_DEFINE_AS_POD_TYPE(vk::PhysicalDeviceFragmentShadingRateKHR);

static vk::IndexType GetVkIndexType(plRHIResourceFormat::Enum format)
{
  vk::Format vk_format = plVKUtils::ToVkFormat(format);
  switch (vk_format)
  {
    case vk::Format::eR16Uint:
      return vk::IndexType::eUint16;
    case vk::Format::eR32Uint:
      return vk::IndexType::eUint32;
    default:
      assert(false);
      return {};
  }
}

vk::ImageLayout ConvertState(plRHIResourceState state)
{
  static std::pair<plRHIResourceState, vk::ImageLayout> mapping[] = {
    {plRHIResourceState::kCommon, vk::ImageLayout::eGeneral},
    {plRHIResourceState::kRenderTarget, vk::ImageLayout::eColorAttachmentOptimal},
    {plRHIResourceState::kUnorderedAccess, vk::ImageLayout::eGeneral},
    {plRHIResourceState::kDepthStencilWrite, vk::ImageLayout::eDepthStencilAttachmentOptimal},
    {plRHIResourceState::kDepthStencilRead, vk::ImageLayout::eDepthStencilReadOnlyOptimal},
    {plRHIResourceState::kNonPixelShaderResource, vk::ImageLayout::eShaderReadOnlyOptimal},
    {plRHIResourceState::kPixelShaderResource, vk::ImageLayout::eShaderReadOnlyOptimal},
    {plRHIResourceState::kCopyDest, vk::ImageLayout::eTransferDstOptimal},
    {plRHIResourceState::kCopySource, vk::ImageLayout::eTransferSrcOptimal},
    {plRHIResourceState::kShadingRateSource, vk::ImageLayout::eFragmentShadingRateAttachmentOptimalKHR},
    {plRHIResourceState::kPresent, vk::ImageLayout::ePresentSrcKHR},
    {plRHIResourceState::kUndefined, vk::ImageLayout::eUndefined},
  };
  for (const auto& m : mapping)
  {
    if (state & m.first)
    {
      assert(state == m.first);
      return m.second;
    }
  }
  assert(false);
  return vk::ImageLayout::eGeneral;
}

plVKDevice::plVKDevice(plVKAdapter& adapter)
  : m_Adapter(adapter)
  , m_PhysicalDevice(adapter.GetPhysicalDevice())
  , m_GPUDescriptorPool(*this)
{
  plUInt32 queueFamiliesCount = 0;
  m_PhysicalDevice.getQueueFamilyProperties(&queueFamiliesCount, nullptr);

  plDynamicArray<vk::QueueFamilyProperties> queueFamilies;
  queueFamilies.SetCountUninitialized(queueFamiliesCount);
  m_PhysicalDevice.getQueueFamilyProperties(&queueFamiliesCount, queueFamilies.GetData());

  auto hasAllBits = [](auto flags, auto bits) {
    return (flags & bits) == bits;
  };
  auto hasAnyBits = [](auto flags, auto bits) {
    return flags & bits;
  };
  for (plUInt32 i = 0; i < queueFamilies.GetCount(); ++i)
  {
    const auto& queue = queueFamilies[i];
    if (queue.queueCount > 0 && hasAllBits(queue.queueFlags, vk::QueueFlagBits::eGraphics | vk::QueueFlagBits::eCompute | vk::QueueFlagBits::eTransfer))
    {
      m_QueuesInfo[plRHICommandListType::kGraphics].QueueFamilyIndex = i;
      m_QueuesInfo[plRHICommandListType::kGraphics].QueueCount = queue.queueCount;
    }
    else if (queue.queueCount > 0 && hasAllBits(queue.queueFlags, vk::QueueFlagBits::eCompute | vk::QueueFlagBits::eTransfer) && !hasAnyBits(queue.queueFlags, vk::QueueFlagBits::eGraphics))
    {
      m_QueuesInfo[plRHICommandListType::kCompute].QueueFamilyIndex = i;
      m_QueuesInfo[plRHICommandListType::kCompute].QueueCount = queue.queueCount;
    }
    else if (queue.queueCount > 0 && hasAllBits(queue.queueFlags, vk::QueueFlagBits::eTransfer) && !hasAnyBits(queue.queueFlags, vk::QueueFlagBits::eGraphics | vk::QueueFlagBits::eCompute))
    {
      m_QueuesInfo[plRHICommandListType::kCopy].QueueFamilyIndex = i;
      m_QueuesInfo[plRHICommandListType::kCopy].QueueCount = queue.queueCount;
    }
  }

  plUInt32 extensionsCount = 0;
  vk::Result result = m_PhysicalDevice.enumerateDeviceExtensionProperties(nullptr, &extensionsCount, nullptr);

  plDynamicArray<vk::ExtensionProperties> extensions;
  extensions.SetCountUninitialized(extensionsCount);
  result = m_PhysicalDevice.enumerateDeviceExtensionProperties(nullptr, &extensionsCount, extensions.GetData());

  plSet<plString> requiredExtensions;

  requiredExtensions.Insert(VK_KHR_SWAPCHAIN_EXTENSION_NAME);
  requiredExtensions.Insert(VK_KHR_RAY_TRACING_PIPELINE_EXTENSION_NAME);
  requiredExtensions.Insert(VK_KHR_RAY_QUERY_EXTENSION_NAME);
  requiredExtensions.Insert(VK_KHR_ACCELERATION_STRUCTURE_EXTENSION_NAME);
  requiredExtensions.Insert(VK_KHR_BUFFER_DEVICE_ADDRESS_EXTENSION_NAME);
  requiredExtensions.Insert(VK_KHR_DEFERRED_HOST_OPERATIONS_EXTENSION_NAME);
  requiredExtensions.Insert(VK_KHR_MAINTENANCE3_EXTENSION_NAME);
  requiredExtensions.Insert(VK_KHR_GET_PHYSICAL_DEVICE_PROPERTIES_2_EXTENSION_NAME);
  requiredExtensions.Insert(VK_KHR_GET_MEMORY_REQUIREMENTS_2_EXTENSION_NAME);
  requiredExtensions.Insert(VK_KHR_FRAGMENT_SHADING_RATE_EXTENSION_NAME);
  requiredExtensions.Insert(VK_KHR_TIMELINE_SEMAPHORE_EXTENSION_NAME);
  requiredExtensions.Insert(VK_KHR_MAINTENANCE1_EXTENSION_NAME);
  requiredExtensions.Insert(VK_KHR_DEDICATED_ALLOCATION_EXTENSION_NAME);
  requiredExtensions.Insert(VK_EXT_MEMORY_BUDGET_EXTENSION_NAME);
  requiredExtensions.Insert(VK_NV_MESH_SHADER_EXTENSION_NAME);
  requiredExtensions.Insert(VK_KHR_CREATE_RENDERPASS_2_EXTENSION_NAME);

  plDynamicArray<const char*> foundExtensions;
  for (const auto& extension : extensions)
  {
    if (requiredExtensions.Contains(extension.extensionName.data()))
      foundExtensions.PushBack(extension.extensionName);

    if (plStringUtils::IsEqual(extension.extensionName.data(), VK_KHR_FRAGMENT_SHADING_RATE_EXTENSION_NAME))
      m_is_variable_rate_shading_supported = true;
    if (plStringUtils::IsEqual(extension.extensionName.data(), VK_KHR_RAY_TRACING_PIPELINE_EXTENSION_NAME))
      m_is_dxr_supported = true;
    if (plStringUtils::IsEqual(extension.extensionName.data(), VK_NV_MESH_SHADER_EXTENSION_NAME))
      m_is_mesh_shading_supported = true;
    if (plStringUtils::IsEqual(extension.extensionName.data(), VK_KHR_RAY_QUERY_EXTENSION_NAME))
      m_is_ray_query_supported = true;
  }

  void* deviceCreateInfoNext = nullptr;
  auto addExtension = [&](auto& extension) {
    extension.pNext = deviceCreateInfoNext;
    deviceCreateInfoNext = &extension;
  };

  if (m_is_variable_rate_shading_supported)
  {
    plMap<plRHIShadingRate, vk::Extent2D> shadingRatePalette;
    shadingRatePalette.Insert(plRHIShadingRate::k1x1, vk::Extent2D{1, 1});
    shadingRatePalette.Insert(plRHIShadingRate::k1x2, vk::Extent2D{1, 2});
    shadingRatePalette.Insert(plRHIShadingRate::k2x1, vk::Extent2D{2, 1});
    shadingRatePalette.Insert(plRHIShadingRate::k2x2, vk::Extent2D{2, 2});
    shadingRatePalette.Insert(plRHIShadingRate::k2x4, vk::Extent2D{2, 4});
    shadingRatePalette.Insert(plRHIShadingRate::k4x2, vk::Extent2D{4, 2});
    shadingRatePalette.Insert(plRHIShadingRate::k4x4, vk::Extent2D{4, 4});

    plUInt32 fragmentShadingRatesCount = 0;
    result = m_Adapter.GetPhysicalDevice().getFragmentShadingRatesKHR(&fragmentShadingRatesCount, nullptr);

    plDynamicArray<vk::PhysicalDeviceFragmentShadingRateKHR> fragmentShadingRates;
    fragmentShadingRates.SetCountUninitialized(fragmentShadingRatesCount);
    result = m_Adapter.GetPhysicalDevice().getFragmentShadingRatesKHR(&fragmentShadingRatesCount, fragmentShadingRates.GetData());

    for (const auto& fragmentShadingRate : fragmentShadingRates)
    {
      vk::Extent2D size = fragmentShadingRate.fragmentSize;
      plUInt8 shadingRate = ((size.width >> 1) << 2) | (size.height >> 1);
      PL_ASSERT_ALWAYS((1 << ((shadingRate >> 2) & 3)) == size.width, "");
      PL_ASSERT_ALWAYS((1 << (shadingRate & 3)) == size.height, "");
      PL_ASSERT_ALWAYS(shadingRatePalette[(plRHIShadingRate)shadingRate] == size, "");
      shadingRatePalette.Remove((plRHIShadingRate)shadingRate);
    }
    PL_ASSERT_ALWAYS(shadingRatePalette.IsEmpty(), "");

    vk::PhysicalDeviceFragmentShadingRatePropertiesKHR shadingRateImageProperties = {};
    vk::PhysicalDeviceProperties2 deviceProps2 = {};
    deviceProps2.pNext = &shadingRateImageProperties;
    m_Adapter.GetPhysicalDevice().getProperties2(&deviceProps2);
    PL_ASSERT_ALWAYS(shadingRateImageProperties.minFragmentShadingRateAttachmentTexelSize == shadingRateImageProperties.maxFragmentShadingRateAttachmentTexelSize, "");
    PL_ASSERT_ALWAYS(shadingRateImageProperties.minFragmentShadingRateAttachmentTexelSize.width == shadingRateImageProperties.minFragmentShadingRateAttachmentTexelSize.height, "");
    PL_ASSERT_ALWAYS(shadingRateImageProperties.maxFragmentShadingRateAttachmentTexelSize.width == shadingRateImageProperties.maxFragmentShadingRateAttachmentTexelSize.height, "");
    m_shading_rate_image_tile_size = shadingRateImageProperties.maxFragmentShadingRateAttachmentTexelSize.width;

    vk::PhysicalDeviceFragmentShadingRateFeaturesKHR fragmentShadingRateFeatures = {};
    fragmentShadingRateFeatures.attachmentFragmentShadingRate = true;
    addExtension(fragmentShadingRateFeatures);
  }

  if (m_is_dxr_supported)
  {
    vk::PhysicalDeviceRayTracingPipelinePropertiesKHR rayTracingProperties = {};
    vk::PhysicalDeviceProperties2 deviceProps2 = {};
    deviceProps2.pNext = &rayTracingProperties;
    m_PhysicalDevice.getProperties2(&deviceProps2);
    m_shader_group_handle_size = rayTracingProperties.shaderGroupHandleSize;
    m_shader_record_alignment = rayTracingProperties.shaderGroupHandleSize;
    m_shader_table_alignment = rayTracingProperties.shaderGroupBaseAlignment;
  }

  const float queuePriority = 1.0f;
  plDynamicArray<vk::DeviceQueueCreateInfo> queuesCreateInfo;
  for (const auto& queueInfo : m_QueuesInfo)
  {
    vk::DeviceQueueCreateInfo& queueCreateInfo = queuesCreateInfo.ExpandAndGetRef();
    queueCreateInfo.queueFamilyIndex = queueInfo.Value().QueueFamilyIndex;
    queueCreateInfo.queueCount = 1;
    queueCreateInfo.pQueuePriorities = &queuePriority;
  }

  vk::PhysicalDeviceFeatures deviceFeatures = {};
  deviceFeatures.textureCompressionBC = true;
  deviceFeatures.vertexPipelineStoresAndAtomics = true;
  deviceFeatures.samplerAnisotropy = true;
  deviceFeatures.fragmentStoresAndAtomics = true;
  deviceFeatures.sampleRateShading = true;
  deviceFeatures.geometryShader = true;
  deviceFeatures.imageCubeArray = true;
  deviceFeatures.shaderImageGatherExtended = true;

  vk::PhysicalDeviceVulkan12Features deviceVulkan12Features = {};
  deviceVulkan12Features.drawIndirectCount = true;
  deviceVulkan12Features.bufferDeviceAddress = true;
  deviceVulkan12Features.timelineSemaphore = true;
  deviceVulkan12Features.runtimeDescriptorArray = true;
  deviceVulkan12Features.descriptorBindingVariableDescriptorCount = true;
  addExtension(deviceVulkan12Features);

  vk::PhysicalDeviceMeshShaderFeaturesNV meshShaderFeature = {};
  meshShaderFeature.taskShader = true;
  meshShaderFeature.meshShader = true;
  if (m_is_mesh_shading_supported)
  {
    addExtension(meshShaderFeature);
  }

  vk::PhysicalDeviceRayTracingPipelineFeaturesKHR rayTracingPipelineFeature = {};
  rayTracingPipelineFeature.rayTracingPipeline = true;

  vk::PhysicalDeviceAccelerationStructureFeaturesKHR accelerationStructureFeature = {};
  accelerationStructureFeature.accelerationStructure = true;

  vk::PhysicalDeviceRayQueryFeaturesKHR rayQueryPipelineFeature = {};
  rayQueryPipelineFeature.rayQuery = true;

  if (m_is_dxr_supported)
  {
    addExtension(rayTracingPipelineFeature);
    addExtension(accelerationStructureFeature);

    if (m_is_ray_query_supported)
    {
      rayTracingPipelineFeature.rayTraversalPrimitiveCulling = true;
      addExtension(rayQueryPipelineFeature);
    }
  }

  vk::DeviceCreateInfo deviceCreateInfo = {};
  deviceCreateInfo.pNext = deviceCreateInfoNext;
  deviceCreateInfo.queueCreateInfoCount = queuesCreateInfo.GetCount();
  deviceCreateInfo.pQueueCreateInfos = queuesCreateInfo.GetData();
  deviceCreateInfo.pEnabledFeatures = &deviceFeatures;
  deviceCreateInfo.enabledExtensionCount = foundExtensions.GetCount();
  deviceCreateInfo.ppEnabledExtensionNames = foundExtensions.GetData();

  m_Device = m_PhysicalDevice.createDeviceUnique(deviceCreateInfo);
  VULKAN_HPP_DEFAULT_DISPATCHER.init(m_Device.get());

  for (auto& queueInfo : m_QueuesInfo)
  {
    vk::CommandPoolCreateInfo cmdPoolCreateInfo = {};
    cmdPoolCreateInfo.flags = vk::CommandPoolCreateFlagBits::eResetCommandBuffer;
    cmdPoolCreateInfo.queueFamilyIndex = queueInfo.Value().QueueFamilyIndex;
    m_CmdPools.Insert(queueInfo.Key(), m_Device->createCommandPoolUnique(cmdPoolCreateInfo));
    m_CommandQueues[queueInfo.Key()] = PL_DEFAULT_NEW(plVKCommandQueue, *this, queueInfo.Key(), queueInfo.Value().QueueFamilyIndex);
  }
}

plSharedPtr<plRHIMemory> plVKDevice::AllocateMemory(plUInt64 size, plRHIMemoryType memoryType, plUInt32 memoryTypeBits)
{
  return PL_DEFAULT_NEW(plVKMemory, *this, size, memoryType, memoryTypeBits, nullptr);
}

plSharedPtr<plRHICommandQueue> plVKDevice::GetCommandQueue(plRHICommandListType type)
{
  return m_CommandQueues[GetAvailableCommandListType(type)];
}

plSharedPtr<plRHISwapchain> plVKDevice::CreateSwapchain(plRHIWindow window, plUInt32 width, plUInt32 height, plUInt32 frameCount, bool vsync)
{
  return PL_DEFAULT_NEW(plVKSwapchain, *m_CommandQueues[plRHICommandListType::kGraphics], window, width, height, frameCount, vsync);
}

plSharedPtr<plRHICommandList> plVKDevice::CreateCommandList(plRHICommandListType type)
{
  return PL_DEFAULT_NEW(plVKCommandList, *this, type);
}

plSharedPtr<plRHIFence> plVKDevice::CreateFence(plUInt64 initialValue)
{
  return PL_DEFAULT_NEW(plVKTimelineSemaphore, *this, initialValue);
}

plSharedPtr<plRHIResource> plVKDevice::CreateTexture(plRHITextureType type, plUInt32 bindFlags, plRHIResourceFormat::Enum format, plUInt32 sample_count, int width, int height, int depth, int mipLevels)
{
  plSharedPtr<plVKResource> res = PL_DEFAULT_NEW(plVKResource, *this);
  res->Format = format;
  res->ResourceType = plRHIResourceType::kTexture;
  res->image.size.height = height;
  res->image.size.width = width;
  res->image.format = plVKUtils::ToVkFormat(format);
  res->image.level_count = mipLevels;
  res->image.sample_count = sample_count;
  res->image.array_layers = depth;

  vk::ImageUsageFlags usage = {};
  if (bindFlags & plRHIBindFlag::kDepthStencil)
    usage |= vk::ImageUsageFlagBits::eDepthStencilAttachment | vk::ImageUsageFlagBits::eTransferDst;
  if (bindFlags & plRHIBindFlag::kShaderResource)
    usage |= vk::ImageUsageFlagBits::eSampled;
  if (bindFlags & plRHIBindFlag::kRenderTarget)
    usage |= vk::ImageUsageFlagBits::eColorAttachment | vk::ImageUsageFlagBits::eTransferDst;
  if (bindFlags & plRHIBindFlag::kUnorderedAccess)
    usage |= vk::ImageUsageFlagBits::eStorage;
  if (bindFlags & plRHIBindFlag::kCopyDest)
    usage |= vk::ImageUsageFlagBits::eTransferDst;
  if (bindFlags & plRHIBindFlag::kCopySource)
    usage |= vk::ImageUsageFlagBits::eTransferSrc;
  if (bindFlags & plRHIBindFlag::kShadingRateSource)
    usage |= vk::ImageUsageFlagBits::eFragmentShadingRateAttachmentKHR;

  vk::ImageCreateInfo image_info = {};
  switch (type)
  {
    case plRHITextureType::k1D:
      image_info.imageType = vk::ImageType::e1D;
      break;
    case plRHITextureType::k2D:
      image_info.imageType = vk::ImageType::e2D;
      break;
    case plRHITextureType::k3D:
      image_info.imageType = vk::ImageType::e3D;
      break;
  }
  image_info.extent.width = width;
  image_info.extent.height = height;
  if (type == plRHITextureType::k3D)
    image_info.extent.depth = depth;
  else
    image_info.extent.depth = 1;
  image_info.mipLevels = mipLevels;
  if (type == plRHITextureType::k3D)
    image_info.arrayLayers = 1;
  else
    image_info.arrayLayers = depth;
  image_info.format = res->image.format;
  image_info.tiling = vk::ImageTiling::eOptimal;
  image_info.initialLayout = vk::ImageLayout::eUndefined;
  image_info.usage = usage;
  image_info.samples = static_cast<vk::SampleCountFlagBits>(sample_count);
  image_info.sharingMode = vk::SharingMode::eExclusive;

  if (image_info.arrayLayers % 6 == 0)
    image_info.flags = vk::ImageCreateFlagBits::eCubeCompatible;

  res->image.res_owner = m_Device->createImageUnique(image_info);
  res->image.res = res->image.res_owner.get();

  res->SetInitialState(plRHIResourceState::kUndefined);

  return res;
}

plSharedPtr<plRHIResource> plVKDevice::CreateBuffer(plUInt32 bind_flag, plUInt32 buffer_size)
{
  if (buffer_size == 0)
    return {};

  plSharedPtr<plVKResource> res = PL_DEFAULT_NEW(plVKResource, *this);
  res->ResourceType = plRHIResourceType::kBuffer;
  res->buffer.size = buffer_size;

  vk::BufferCreateInfo buffer_info = {};
  buffer_info.size = buffer_size;
  buffer_info.usage = vk::BufferUsageFlagBits::eShaderDeviceAddress;

  if (bind_flag & plRHIBindFlag::kVertexBuffer)
    buffer_info.usage |= vk::BufferUsageFlagBits::eVertexBuffer;
  if (bind_flag & plRHIBindFlag::kIndexBuffer)
    buffer_info.usage |= vk::BufferUsageFlagBits::eIndexBuffer;
  if (bind_flag & plRHIBindFlag::kConstantBuffer)
    buffer_info.usage |= vk::BufferUsageFlagBits::eUniformBuffer;
  if (bind_flag & plRHIBindFlag::kUnorderedAccess)
  {
    buffer_info.usage |= vk::BufferUsageFlagBits::eStorageBuffer;
    buffer_info.usage |= vk::BufferUsageFlagBits::eStorageTexelBuffer;
  }
  if (bind_flag & plRHIBindFlag::kShaderResource)
  {
    buffer_info.usage |= vk::BufferUsageFlagBits::eStorageBuffer;
    buffer_info.usage |= vk::BufferUsageFlagBits::eUniformTexelBuffer;
  }
  if (bind_flag & plRHIBindFlag::kAccelerationStructure)
    buffer_info.usage |= vk::BufferUsageFlagBits::eAccelerationStructureStorageKHR;
  if (bind_flag & plRHIBindFlag::kCopySource)
    buffer_info.usage |= vk::BufferUsageFlagBits::eTransferSrc;
  if (bind_flag & plRHIBindFlag::kCopyDest)
    buffer_info.usage |= vk::BufferUsageFlagBits::eTransferDst;
  if (bind_flag & plRHIBindFlag::kShaderTable)
    buffer_info.usage |= vk::BufferUsageFlagBits::eShaderBindingTableKHR;
  if (bind_flag & plRHIBindFlag::kIndirectBuffer)
    buffer_info.usage |= vk::BufferUsageFlagBits::eIndirectBuffer;

  res->buffer.res = m_Device->createBufferUnique(buffer_info);
  res->SetInitialState(plRHIResourceState::kCommon);

  return res;
}

plSharedPtr<plRHIResource> plVKDevice::CreateSampler(const plRHISamplerDesc& desc)
{
  plSharedPtr<plVKResource> res = PL_DEFAULT_NEW(plVKResource, *this);

  vk::SamplerCreateInfo samplerInfo = {};
  samplerInfo.magFilter = vk::Filter::eLinear;
  samplerInfo.minFilter = vk::Filter::eLinear;
  samplerInfo.addressModeU = vk::SamplerAddressMode::eRepeat;
  samplerInfo.addressModeV = vk::SamplerAddressMode::eRepeat;
  samplerInfo.addressModeW = vk::SamplerAddressMode::eRepeat;
  samplerInfo.anisotropyEnable = true;
  samplerInfo.maxAnisotropy = 16;
  samplerInfo.borderColor = vk::BorderColor::eIntOpaqueBlack;
  samplerInfo.unnormalizedCoordinates = VK_FALSE;
  samplerInfo.compareEnable = VK_FALSE;
  samplerInfo.compareOp = vk::CompareOp::eAlways;
  samplerInfo.mipmapMode = vk::SamplerMipmapMode::eLinear;
  samplerInfo.mipLodBias = 0.0f;
  samplerInfo.minLod = 0.0f;
  samplerInfo.maxLod = plMath::MaxValue<float>();

  switch (desc.mode)
  {
    case plRHISamplerTextureAddressMode::kWrap:
      samplerInfo.addressModeU = vk::SamplerAddressMode::eRepeat;
      samplerInfo.addressModeV = vk::SamplerAddressMode::eRepeat;
      samplerInfo.addressModeW = vk::SamplerAddressMode::eRepeat;
      break;
    case plRHISamplerTextureAddressMode::kClamp:
      samplerInfo.addressModeU = vk::SamplerAddressMode::eClampToEdge;
      samplerInfo.addressModeV = vk::SamplerAddressMode::eClampToEdge;
      samplerInfo.addressModeW = vk::SamplerAddressMode::eClampToEdge;
      break;
  }

  switch (desc.func)
  {
    case plRHISamplerComparisonFunc::kNever:
      samplerInfo.compareOp = vk::CompareOp::eNever;
      break;
    case plRHISamplerComparisonFunc::kAlways:
      samplerInfo.compareEnable = true;
      samplerInfo.compareOp = vk::CompareOp::eAlways;
      break;
    case plRHISamplerComparisonFunc::kLess:
      samplerInfo.compareEnable = true;
      samplerInfo.compareOp = vk::CompareOp::eLess;
      break;
  }

  res->sampler.res = m_Device->createSamplerUnique(samplerInfo);

  res->ResourceType = plRHIResourceType::kSampler;
  return res;
}

plSharedPtr<plRHIView> plVKDevice::CreateView(const plSharedPtr<plRHIResource>& resource, const plRHIViewDesc& view_desc)
{
  return PL_DEFAULT_NEW(plVKView, *this, resource.Downcast<plVKResource>(), view_desc);
}

plSharedPtr<plRHIBindingSetLayout> plVKDevice::CreateBindingSetLayout(const std::vector<plRHIBindKey>& descs)
{
  return PL_DEFAULT_NEW(plVKBindingSetLayout, *this, descs);
}

plSharedPtr<plRHIBindingSet> plVKDevice::CreateBindingSet(const plSharedPtr<plRHIBindingSetLayout>& layout)
{
  return PL_DEFAULT_NEW(plVKBindingSet, *this, layout.Downcast<plVKBindingSetLayout>());
}

plSharedPtr<plRHIRenderPass> plVKDevice::CreateRenderPass(const plRHIRenderPassDesc& desc)
{
  return PL_DEFAULT_NEW(plVKRenderPass, *this, desc);
}

plSharedPtr<plRHIFramebuffer> plVKDevice::CreateFramebuffer(const plRHIFramebufferDesc& desc)
{
  return PL_DEFAULT_NEW(plVKFramebuffer, *this, desc);
}

plSharedPtr<plRHIShader> plVKDevice::CreateShader(const plRHIShaderDesc& desc, plDynamicArray<plUInt8> byteCode, plSharedPtr<plRHIShaderReflection> reflection)
{
  return PL_DEFAULT_NEW(plRHIShaderBase, desc, byteCode, reflection, plRHIShaderBlobType::kSPIRV);
}

plSharedPtr<plRHIProgram> plVKDevice::CreateProgram(const std::vector<plSharedPtr<plRHIShader>>& shaders)
{
  return PL_DEFAULT_NEW(plVKProgram, *this, shaders);
}

plSharedPtr<plRHIPipeline> plVKDevice::CreateGraphicsPipeline(const plRHIGraphicsPipelineDesc& desc)
{
  return PL_DEFAULT_NEW(plVKGraphicsPipeline, *this, desc);
}

plSharedPtr<plRHIPipeline> plVKDevice::CreateComputePipeline(const plRHIComputePipelineDesc& desc)
{
  return PL_DEFAULT_NEW(plVKComputePipeline, *this, desc);
}

plSharedPtr<plRHIPipeline> plVKDevice::CreateRayTracingPipeline(const plRHIRayTracingPipelineDesc& desc)
{
  return PL_DEFAULT_NEW(plVKRayTracingPipeline, *this, desc);
}

vk::AccelerationStructureGeometryKHR plVKDevice::FillRaytracingGeometryTriangles(const plRHIBufferDesc& vertex, const plRHIBufferDesc& index, plRHIRaytracingGeometryFlags flags) const
{
  vk::AccelerationStructureGeometryKHR geometry_desc = {};
  geometry_desc.geometryType = vk::GeometryTypeNV::eTriangles;
  switch (flags)
  {
    case plRHIRaytracingGeometryFlags::kOpaque:
      geometry_desc.flags = vk::GeometryFlagBitsKHR::eOpaque;
      break;
    case plRHIRaytracingGeometryFlags::kNoDuplicateAnyHitInvocation:
      geometry_desc.flags = vk::GeometryFlagBitsKHR::eNoDuplicateAnyHitInvocation;
      break;
  }

  auto vk_vertex_res = vertex.res.Downcast<plVKResource>();
  auto vk_index_res = index.res.Downcast<plVKResource>();

  auto vertex_stride = plRHIResourceFormat::GetFormatStride(vertex.format);
  geometry_desc.geometry.triangles.vertexData = m_Device->getBufferAddress({vk_vertex_res->buffer.res.get()}) + vertex.offset * vertex_stride;
  geometry_desc.geometry.triangles.vertexStride = vertex_stride;
  geometry_desc.geometry.triangles.vertexFormat = plVKUtils::ToVkFormat(vertex.format);
  geometry_desc.geometry.triangles.maxVertex = vertex.count;
  if (vk_index_res)
  {
    auto index_stride = plRHIResourceFormat::GetFormatStride(index.format);
    geometry_desc.geometry.triangles.indexData = m_Device->getBufferAddress({vk_index_res->buffer.res.get()}) + index.offset * index_stride;
    geometry_desc.geometry.triangles.indexType = GetVkIndexType(index.format);
  }
  else
  {
    geometry_desc.geometry.triangles.indexType = vk::IndexType::eNoneNV;
  }

  return geometry_desc;
}

plRHIRaytracingASPrebuildInfo plVKDevice::GetAccelerationStructurePrebuildInfo(const vk::AccelerationStructureBuildGeometryInfoKHR& acceleration_structure_info, const std::vector<plUInt32>& max_primitive_counts) const
{
  vk::AccelerationStructureBuildSizesInfoKHR size_info = {};
  m_Device->getAccelerationStructureBuildSizesKHR(vk::AccelerationStructureBuildTypeKHR::eDevice, &acceleration_structure_info, max_primitive_counts.data(), &size_info);
  plRHIRaytracingASPrebuildInfo prebuild_info = {};
  prebuild_info.accelerationStructureSize = size_info.accelerationStructureSize;
  prebuild_info.buildScratchDataSize = size_info.buildScratchSize;
  prebuild_info.updateScratchDataSize = size_info.updateScratchSize;
  return prebuild_info;
}

vk::BuildAccelerationStructureFlagsKHR Convert(plRHIBuildAccelerationStructureFlags flags)
{
  vk::BuildAccelerationStructureFlagsKHR vk_flags = {};
  if (flags & plRHIBuildAccelerationStructureFlags::kAllowUpdate)
    vk_flags |= vk::BuildAccelerationStructureFlagBitsKHR::eAllowUpdate;
  if (flags & plRHIBuildAccelerationStructureFlags::kAllowCompaction)
    vk_flags |= vk::BuildAccelerationStructureFlagBitsKHR::eAllowCompaction;
  if (flags & plRHIBuildAccelerationStructureFlags::kPreferFastTrace)
    vk_flags |= vk::BuildAccelerationStructureFlagBitsKHR::ePreferFastTrace;
  if (flags & plRHIBuildAccelerationStructureFlags::kPreferFastBuild)
    vk_flags |= vk::BuildAccelerationStructureFlagBitsKHR::ePreferFastBuild;
  if (flags & plRHIBuildAccelerationStructureFlags::kMinimizeMemory)
    vk_flags |= vk::BuildAccelerationStructureFlagBitsKHR::eLowMemory;
  return vk_flags;
}

vk::AccelerationStructureTypeKHR Convert(plRHIAccelerationStructureType type)
{
  switch (type)
  {
    case plRHIAccelerationStructureType::kTopLevel:
      return vk::AccelerationStructureTypeKHR::eTopLevel;
    case plRHIAccelerationStructureType::kBottomLevel:
      return vk::AccelerationStructureTypeKHR::eBottomLevel;
  }
  assert(false);
  return {};
}

plSharedPtr<plRHIResource> plVKDevice::CreateAccelerationStructure(plRHIAccelerationStructureType type, const plSharedPtr<plRHIResource>& resource, plUInt64 offset)
{
  plSharedPtr<plVKResource> res = PL_DEFAULT_NEW(plVKResource, *this);
  res->ResourceType = plRHIResourceType::kAccelerationStructure;
  res->accelerationStructuresMemory = resource;

  vk::AccelerationStructureCreateInfoKHR accelerationStructureCreateInfo = {};
  accelerationStructureCreateInfo.buffer = resource.Downcast<plVKResource>()->buffer.res.get();
  accelerationStructureCreateInfo.offset = offset;
  accelerationStructureCreateInfo.size = 0;
  accelerationStructureCreateInfo.type = Convert(type);
  res->acceleration_structure_handle = m_Device->createAccelerationStructureKHRUnique(accelerationStructureCreateInfo);

  return res;
}

plSharedPtr<plRHIQueryHeap> plVKDevice::CreateQueryHeap(plRHIQueryHeapType type, plUInt32 count)
{
  return PL_DEFAULT_NEW(plVKQueryHeap, *this, type, count);
}

plUInt32 plVKDevice::GetTextureDataPitchAlignment() const
{
  return 1;
}

bool plVKDevice::IsDxrSupported() const
{
  return m_is_dxr_supported;
}

bool plVKDevice::IsRayQuerySupported() const
{
  return m_is_ray_query_supported;
}

bool plVKDevice::IsVariableRateShadingSupported() const
{
  return m_is_variable_rate_shading_supported;
}

bool plVKDevice::IsMeshShadingSupported() const
{
  return m_is_mesh_shading_supported;
}

plUInt32 plVKDevice::GetShadingRateImageTileSize() const
{
  return m_shading_rate_image_tile_size;
}

plRHIMemoryBudget plVKDevice::GetMemoryBudget() const
{
  vk::PhysicalDeviceMemoryBudgetPropertiesEXT memory_budget = {};
  vk::PhysicalDeviceMemoryProperties2 mem_properties = {};
  mem_properties.pNext = &memory_budget;
  m_Adapter.GetPhysicalDevice().getMemoryProperties2(&mem_properties);
  plRHIMemoryBudget res = {};
  for (size_t i = 0; i < VK_MAX_MEMORY_HEAPS; ++i)
  {
    res.budget += memory_budget.heapBudget[i];
    res.usage += memory_budget.heapUsage[i];
  }
  return res;
}

plUInt32 plVKDevice::GetShaderGroupHandleSize() const
{
  return m_shader_group_handle_size;
}

plUInt32 plVKDevice::GetShaderRecordAlignment() const
{
  return m_shader_record_alignment;
}

plUInt32 plVKDevice::GetShaderTableAlignment() const
{
  return m_shader_table_alignment;
}

plRHIRaytracingASPrebuildInfo plVKDevice::GetBLASPrebuildInfo(const std::vector<plRHIRaytracingGeometryDesc>& descs, plRHIBuildAccelerationStructureFlags flags) const
{
  std::vector<vk::AccelerationStructureGeometryKHR> geometry_descs;
  std::vector<plUInt32> max_primitive_counts;
  for (const auto& desc : descs)
  {
    geometry_descs.emplace_back(FillRaytracingGeometryTriangles(desc.vertex, desc.index, desc.flags));
    if (desc.index.res)
      max_primitive_counts.emplace_back(desc.index.count / 3);
    else
      max_primitive_counts.emplace_back(desc.vertex.count / 3);
  }
  vk::AccelerationStructureBuildGeometryInfoKHR acceleration_structure_info = {};
  acceleration_structure_info.type = vk::AccelerationStructureTypeKHR::eBottomLevel;
  acceleration_structure_info.geometryCount = (plUInt32)geometry_descs.size();
  acceleration_structure_info.pGeometries = geometry_descs.data();
  acceleration_structure_info.flags = Convert(flags);
  return GetAccelerationStructurePrebuildInfo(acceleration_structure_info, max_primitive_counts);
}

plRHIRaytracingASPrebuildInfo plVKDevice::GetTLASPrebuildInfo(plUInt32 instance_count, plRHIBuildAccelerationStructureFlags flags) const
{
  vk::AccelerationStructureGeometryKHR geometry_info{};
  geometry_info.geometryType = vk::GeometryTypeKHR::eInstances;
  geometry_info.geometry.setInstances({});

  vk::AccelerationStructureBuildGeometryInfoKHR acceleration_structure_info = {};
  acceleration_structure_info.type = vk::AccelerationStructureTypeKHR::eTopLevel;
  acceleration_structure_info.pGeometries = &geometry_info;
  acceleration_structure_info.geometryCount = 1;
  acceleration_structure_info.flags = Convert(flags);
  return GetAccelerationStructurePrebuildInfo(acceleration_structure_info, {instance_count});
}

plVKAdapter& plVKDevice::GetAdapter()
{
  return m_Adapter;
}

vk::Device plVKDevice::GetDevice()
{
  return m_Device.get();
}

plRHICommandListType plVKDevice::GetAvailableCommandListType(plRHICommandListType type)
{
  if (m_QueuesInfo.Contains(type))
  {
    return type;
  }
  return plRHICommandListType::kGraphics;
}

vk::CommandPool plVKDevice::GetCmdPool(plRHICommandListType type)
{
  return m_CmdPools[GetAvailableCommandListType(type)].get();
}

vk::ImageAspectFlags plVKDevice::GetAspectFlags(vk::Format format) const
{
  switch (format)
  {
    case vk::Format::eD32SfloatS8Uint:
    case vk::Format::eD24UnormS8Uint:
    case vk::Format::eD16UnormS8Uint:
      return vk::ImageAspectFlagBits::eDepth | vk::ImageAspectFlagBits::eStencil;
    case vk::Format::eD16Unorm:
    case vk::Format::eD32Sfloat:
    case vk::Format::eX8D24UnormPack32:
      return vk::ImageAspectFlagBits::eDepth;
    case vk::Format::eS8Uint:
      return vk::ImageAspectFlagBits::eStencil;
    default:
      return vk::ImageAspectFlagBits::eColor;
  }
}

plUInt32 plVKDevice::FindMemoryType(plUInt32 type_filter, vk::MemoryPropertyFlags properties)
{
  vk::PhysicalDeviceMemoryProperties memProperties;
  m_PhysicalDevice.getMemoryProperties(&memProperties);

  for (plUInt32 i = 0; i < memProperties.memoryTypeCount; ++i)
  {
    if ((type_filter & (1 << i)) && (memProperties.memoryTypes[i].propertyFlags & properties) == properties)
      return i;
  }
  throw std::runtime_error("failed to find suitable memory type!");
}

plUniquePtr<plVKGPUBindlessDescriptorPoolTyped>& plVKDevice::GetGPUBindlessDescriptorPool(vk::DescriptorType type)
{
  //auto it = m_GPUBindlessDescriptorPool.find(type);
  //if (it == m_GPUBindlessDescriptorPool.end())
  //  it = m_GPUBindlessDescriptorPool.emplace(std::piecewise_construct, std::forward_as_tuple(type), std::forward_as_tuple(*this, type)).first;
  //return it->second;

  auto it = m_GPUBindlessDescriptorPool.Find(type);
  if (it == end(m_GPUBindlessDescriptorPool))
  {
    it = m_GPUBindlessDescriptorPool.Insert(type, PL_DEFAULT_NEW(plVKGPUBindlessDescriptorPoolTyped, *this, type));
    //it = m_GPUBindlessDescriptorPool.emplace(std::piecewise_construct, std::forward_as_tuple(type), std::forward_as_tuple(*this, type)).first;
  }
  return it.Value();
}

plVKGPUDescriptorPool& plVKDevice::GetGPUDescriptorPool()
{
  return m_GPUDescriptorPool;
}
