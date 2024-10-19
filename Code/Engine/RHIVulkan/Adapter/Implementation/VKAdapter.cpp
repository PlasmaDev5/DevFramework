#include <RHIVulkan/RHIVulkanPCH.h>

#include <RHIVulkan/Adapter/VKAdapter.h>
#include <RHIVulkan/Device/VKDevice.h>

plVKAdapter::plVKAdapter(plVKInstance& instance, const vk::PhysicalDevice& physical_device)
  : m_instance(instance)
  , m_physical_device(physical_device)
  , m_name(physical_device.getProperties().deviceName.data())
{
}

const plString& plVKAdapter::GetName() const
{
  return m_name;
}

plSharedPtr<plRHIDevice> plVKAdapter::CreateDevice()
{
  return PL_DEFAULT_NEW(plVKDevice, *this);
}

plVKInstance& plVKAdapter::GetInstance()
{
  return m_instance;
}

vk::PhysicalDevice& plVKAdapter::GetPhysicalDevice()
{
  return m_physical_device;
}
