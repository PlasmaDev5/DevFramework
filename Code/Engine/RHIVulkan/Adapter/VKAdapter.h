#pragma once
#include <RHIVulkan/RHIVulkanDLL.h>

#include <RHI/Adapter/Adapter.h>

class plVKInstance;

class plVKAdapter : public plRHIAdapter
{
public:
    plVKAdapter(plVKInstance& instance, const vk::PhysicalDevice& physical_device);
    const plString& GetName() const override;
    plSharedPtr<plRHIDevice> CreateDevice() override;
    plVKInstance& GetInstance();
    vk::PhysicalDevice& GetPhysicalDevice();

private:
    plVKInstance& m_instance;
    vk::PhysicalDevice m_physical_device;
    plString m_name;
};
