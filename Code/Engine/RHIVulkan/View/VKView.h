#pragma once
#include <RHIVulkan/RHIVulkanDLL.h>

#include <RHI/View/View.h>
#include <RHIVulkan/GPUDescriptorPool/VKGPUDescriptorPoolRange.h>
#include <RHIVulkan/Resource/VKResource.h>

class plVKDevice;

class plVKView : public plRHIView
{
public:
  plVKView(plVKDevice& device, const plSharedPtr<plVKResource>& resource, const plRHIViewDesc& view_desc);
  plSharedPtr<plRHIResource> GetResource() override;
  plUInt32 GetDescriptorId() const override;
  plUInt32 GetBaseMipLevel() const override;
  plUInt32 GetLevelCount() const override;
  plUInt32 GetBaseArrayLayer() const override;
  plUInt32 GetLayerCount() const override;

  vk::ImageView GetImageView() const;
  vk::WriteDescriptorSet GetDescriptor() const;

private:
  void CreateView();
  void CreateImageView();
  void CreateBufferView();

  plVKDevice& m_device;
  plSharedPtr<plVKResource> m_resource;
  plRHIViewDesc m_view_desc;
  vk::UniqueImageView m_image_view;
  vk::UniqueBufferView m_buffer_view;
  std::shared_ptr<plVKGPUDescriptorPoolRange> m_range;
  vk::DescriptorImageInfo m_descriptor_image = {};
  vk::DescriptorBufferInfo m_descriptor_buffer = {};
  vk::WriteDescriptorSetAccelerationStructureKHR m_descriptor_acceleration_structure = {};
  vk::WriteDescriptorSet m_descriptor = {};
};
