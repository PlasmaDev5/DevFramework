#pragma once
#include <RHIVulkan/RHIVulkanDLL.h>

#include <RHI/Instance/Instance.h>

class plVKInstance : public plRHIInstance
{
public:
    plVKInstance();
    plDynamicArray<plSharedPtr<plRHIAdapter>> EnumerateAdapters() override;
    vk::Instance& GetInstance();

private:
    vk::DynamicLoader m_DynamicLoader;
    vk::UniqueInstance m_Instance;
    vk::UniqueDebugReportCallbackEXT m_Callback;
};
