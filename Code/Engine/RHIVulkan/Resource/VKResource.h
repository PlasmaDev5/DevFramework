#pragma once
#include <RHIVulkan/RHIVulkanDLL.h>

#include <RHI/Resource/ResourceBase.h>

static bool operator<(const VkImageSubresourceRange& lhs, const VkImageSubresourceRange& rhs)
{
    return std::tie(lhs.aspectMask, lhs.baseArrayLayer, lhs.baseMipLevel, lhs.layerCount, lhs.levelCount) <
        std::tie(rhs.aspectMask, rhs.baseArrayLayer, rhs.baseMipLevel, rhs.layerCount, rhs.levelCount);
};

class plVKDevice;

class plVKResource : public plRHIResourceBase
{
public:
    plVKResource(plVKDevice& device);

    void CommitMemory(plRHIMemoryType memoryType) override;
    void BindMemory(const plSharedPtr<plRHIMemory>& memory, plUInt64 offset) override;
    plUInt64 GetWidth() const override;
    plUInt32 GetHeight() const override;
    plUInt16 GetLayerCount() const override;
    plUInt16 GetLevelCount() const override;
    plUInt32 GetSampleCount() const override;
    plUInt64 GetAccelerationStructureHandle() const override;
    void SetName(const plString& name) override;
    plUInt8* Map() override;
    void Unmap() override;
    bool AllowCommonStatePromotion(plRHIResourceState state_after) override;
    plRHIMemoryRequirements GetMemoryRequirements() const override;

    struct Image
    {
        vk::Image res;
        vk::UniqueImage res_owner;
        vk::Format format = vk::Format::eUndefined;
        vk::Extent2D size = {};
        plUInt32 level_count = 1;
        plUInt32 sample_count = 1;
        plUInt32 array_layers = 1;
    } image;

    struct Buffer
    {
        vk::UniqueBuffer res;
        plUInt32 size = 0;
    } buffer;

    struct Sampler
    {
        vk::UniqueSampler res;
    } sampler;

    vk::UniqueAccelerationStructureKHR acceleration_structure_handle = {};

private:
    plVKDevice& m_device;
    vk::DeviceMemory m_vk_memory;
};
