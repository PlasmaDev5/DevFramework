#include <RHIVulkan/RHIVulkanPCH.h>

#include <RHIVulkan/Device/VKDevice.h>
#include <RHIVulkan/Memory/VKMemory.h>
#include <RHIVulkan/Resource/VKResource.h>
#include <RHIVulkan/Utilities/VKUtility.h>
#include <RHIVulkan/View/VKView.h>

plVKResource::plVKResource(plVKDevice& device)
  : m_device(device)
{
}

void plVKResource::CommitMemory(plRHIMemoryType memoryType)
{
  plRHIMemoryRequirements mem_requirements = GetMemoryRequirements();
  vk::MemoryDedicatedAllocateInfoKHR dedicated_allocate_info = {};
  vk::MemoryDedicatedAllocateInfoKHR* p_dedicated_allocate_info = nullptr;
  if (ResourceType == plRHIResourceType::kBuffer)
  {
    dedicated_allocate_info.buffer = buffer.res.get();
    p_dedicated_allocate_info = &dedicated_allocate_info;
  }
  else if (ResourceType == plRHIResourceType::kTexture)
  {
    dedicated_allocate_info.image = image.res;
    p_dedicated_allocate_info = &dedicated_allocate_info;
  }
  auto memory = PL_DEFAULT_NEW(plVKMemory, m_device, mem_requirements.size, memoryType, mem_requirements.memoryTypeBits, p_dedicated_allocate_info);
  BindMemory(memory, 0);
}

void plVKResource::BindMemory(const plSharedPtr<plRHIMemory>& memory, plUInt64 offset)
{
  m_Memory = memory;
  m_MemoryType = m_Memory->GetMemoryType();
  m_vk_memory = m_Memory.Downcast<plVKMemory>()->GetMemory();

  if (ResourceType == plRHIResourceType::kBuffer)
  {
    m_device.GetDevice().bindBufferMemory(buffer.res.get(), m_vk_memory, offset);
  }
  else if (ResourceType == plRHIResourceType::kTexture)
  {
    m_device.GetDevice().bindImageMemory(image.res, m_vk_memory, offset);
  }
}

plUInt64 plVKResource::GetWidth() const
{
  if (ResourceType == plRHIResourceType::kTexture)
    return image.size.width;
  return buffer.size;
}

plUInt32 plVKResource::GetHeight() const
{
  return image.size.height;
}

plUInt16 plVKResource::GetLayerCount() const
{
  return image.array_layers;
}

plUInt16 plVKResource::GetLevelCount() const
{
  return image.level_count;
}

plUInt32 plVKResource::GetSampleCount() const
{
  return image.sample_count;
}

plUInt64 plVKResource::GetAccelerationStructureHandle() const
{
  return m_device.GetDevice().getAccelerationStructureAddressKHR({acceleration_structure_handle.get()});
}

void plVKResource::SetName(const plString& name)
{
  vk::DebugUtilsObjectNameInfoEXT info = {};
  info.pObjectName = name.GetData();
  if (ResourceType == plRHIResourceType::kBuffer)
  {
    info.objectType = buffer.res.get().objectType;
    info.objectHandle = reinterpret_cast<plUInt64>(static_cast<VkBuffer>(buffer.res.get()));
  }
  else if (ResourceType == plRHIResourceType::kTexture)
  {
    info.objectType = image.res.objectType;
    info.objectHandle = reinterpret_cast<plUInt64>(static_cast<VkImage>(image.res));
  }
  m_device.GetDevice().setDebugUtilsObjectNameEXT(info);
}

plUInt8* plVKResource::Map()
{
  plUInt8* dst_data = nullptr;
  vk::Result res = m_device.GetDevice().mapMemory(m_vk_memory, 0, VK_WHOLE_SIZE, {}, reinterpret_cast<void**>(&dst_data));
  return dst_data;
}

void plVKResource::Unmap()
{
  m_device.GetDevice().unmapMemory(m_vk_memory);
}

bool plVKResource::AllowCommonStatePromotion(plRHIResourceState state_after)
{
  return false;
}

plRHIMemoryRequirements plVKResource::GetMemoryRequirements() const
{
  vk::MemoryRequirements2 mem_requirements = {};
  if (ResourceType == plRHIResourceType::kBuffer)
  {
    vk::BufferMemoryRequirementsInfo2KHR buffer_mem_req = {};
    buffer_mem_req.buffer = buffer.res.get();
    m_device.GetDevice().getBufferMemoryRequirements2(&buffer_mem_req, &mem_requirements);
  }
  else if (ResourceType == plRHIResourceType::kTexture)
  {
    vk::ImageMemoryRequirementsInfo2KHR image_mem_req = {};
    image_mem_req.image = image.res;
    m_device.GetDevice().getImageMemoryRequirements2(&image_mem_req, &mem_requirements);
  }
  return {mem_requirements.memoryRequirements.size, mem_requirements.memoryRequirements.alignment, mem_requirements.memoryRequirements.memoryTypeBits};
}
