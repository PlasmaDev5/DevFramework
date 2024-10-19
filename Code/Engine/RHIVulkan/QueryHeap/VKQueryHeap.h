#pragma once
#include <RHIVulkan/RHIVulkanDLL.h>

#include <RHI/QueryHeap/QueryHeap.h>

class plVKDevice;

class plVKQueryHeap : public plRHIQueryHeap
{
public:
  plVKQueryHeap(plVKDevice& device, plRHIQueryHeapType type, plUInt32 count);

  plRHIQueryHeapType GetType() const override;

  vk::QueryType GetQueryType() const;
  vk::QueryPool GetQueryPool() const;

private:
  plVKDevice& m_device;
  vk::UniqueQueryPool m_query_pool;
  vk::QueryType m_query_type;
};
