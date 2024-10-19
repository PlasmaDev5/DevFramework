#include <RHIVulkan/RHIVulkanPCH.h>

#include <RHIVulkan/Device/VKDevice.h>
#include <RHIVulkan/QueryHeap/VKQueryHeap.h>

plVKQueryHeap::plVKQueryHeap(plVKDevice& device, plRHIQueryHeapType type, plUInt32 count)
  : m_device(device)
{
  assert(type == plRHIQueryHeapType::kAccelerationStructureCompactedSize);
  m_query_type = vk::QueryType::eAccelerationStructureCompactedSizeKHR;
  vk::QueryPoolCreateInfo desc = {};
  desc.queryCount = count;
  desc.queryType = m_query_type;
  m_query_pool = m_device.GetDevice().createQueryPoolUnique(desc);
}

plRHIQueryHeapType plVKQueryHeap::GetType() const
{
  return plRHIQueryHeapType::kAccelerationStructureCompactedSize;
}

vk::QueryType plVKQueryHeap::GetQueryType() const
{
  return m_query_type;
}

vk::QueryPool plVKQueryHeap::GetQueryPool() const
{
  return m_query_pool.get();
}
