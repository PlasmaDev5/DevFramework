#include <RHIVulkan/RHIVulkanPCH.h>

#include <RHIVulkan/View/VKView.h>
#include <RHIVulkan/Device/VKDevice.h>
#include <RHIVulkan/Resource/VKResource.h>
#include <RHIVulkan/BindingSetLayout/VKBindingSetLayout.h>
#include <RHIVulkan/Utilities/VKUtility.h>

plVKView::plVKView(plVKDevice& device, const plSharedPtr<plVKResource>& resource, const plRHIViewDesc& view_desc)
    : m_device(device)
    , m_resource(resource)
    , m_view_desc(view_desc)
{
    if (resource)
    {
        CreateView();
    }

    if (view_desc.bindless)
    {
        vk::DescriptorType type = GetDescriptorType(view_desc.viewType);
        decltype(auto) pool = device.GetGPUBindlessDescriptorPool(type);
        m_range = std::make_shared<plVKGPUDescriptorPoolRange>(pool->Allocate(1));

        m_descriptor.dstSet = m_range->GetDescriptoSet();
        m_descriptor.dstArrayElement = m_range->GetOffset();
        m_descriptor.descriptorType = type;
        m_descriptor.dstBinding = 0;
        m_descriptor.descriptorCount = 1;
        m_device.GetDevice().updateDescriptorSets(1, &m_descriptor, 0, nullptr);
    }
}

vk::ImageViewType GetImageViewType(plRHIViewDimension dimension)
{
    switch (dimension)
    {
        case plRHIViewDimension::kTexture1D:
          return vk::ImageViewType::e1D;
        case plRHIViewDimension::kTexture1DArray:
          return vk::ImageViewType::e1DArray;
        case plRHIViewDimension::kTexture2D:
        case plRHIViewDimension::kTexture2DMS:
          return vk::ImageViewType::e2D;
        case plRHIViewDimension::kTexture2DArray:
        case plRHIViewDimension::kTexture2DMSArray:
          return vk::ImageViewType::e2DArray;
        case plRHIViewDimension::kTexture3D:
          return vk::ImageViewType::e3D;
        case plRHIViewDimension::kTextureCube:
          return vk::ImageViewType::eCube;
        case plRHIViewDimension::kTextureCubeArray:
          return vk::ImageViewType::eCubeArray;
    }
    assert(false);
    return {};
}

void plVKView::CreateView()
{
    switch (m_view_desc.viewType)
    {
        case plRHIViewType::kSampler:
        m_descriptor_image.sampler = m_resource->sampler.res.get();
        m_descriptor.pImageInfo = &m_descriptor_image;
        break;
        case plRHIViewType::kTexture:
    {
        CreateImageView();
        m_descriptor_image.imageLayout = vk::ImageLayout::eShaderReadOnlyOptimal;
        m_descriptor_image.imageView = m_image_view.get();
        m_descriptor.pImageInfo = &m_descriptor_image;
        break;
    }
    case plRHIViewType::kRWTexture:
    {
        CreateImageView();
        m_descriptor_image.imageLayout = vk::ImageLayout::eGeneral;
        m_descriptor_image.imageView = m_image_view.get();
        m_descriptor.pImageInfo = &m_descriptor_image;
        break;
    }
    case plRHIViewType::kAccelerationStructure:
    {
        m_descriptor_acceleration_structure.accelerationStructureCount = 1;
        m_descriptor_acceleration_structure.pAccelerationStructures = &m_resource->acceleration_structure_handle.get();
        m_descriptor.pNext = &m_descriptor_acceleration_structure;
        break;
    }
    case plRHIViewType::kShadingRateSource:
    case plRHIViewType::kRenderTarget:
    case plRHIViewType::kDepthStencil:
    {
        CreateImageView();
        break;
    }
    case plRHIViewType::kConstantBuffer:
    case plRHIViewType::kStructuredBuffer:
    case plRHIViewType::kRWStructuredBuffer:
        m_descriptor_buffer.buffer = m_resource->buffer.res.get();
        m_descriptor_buffer.offset = m_view_desc.offset;
        m_descriptor_buffer.range = m_view_desc.bufferSize;
        m_descriptor.pBufferInfo = &m_descriptor_buffer;
        break;
    case plRHIViewType::kBuffer:
    case plRHIViewType::kRWBuffer:
        CreateBufferView();
        m_descriptor.pTexelBufferView = &m_buffer_view.get();
        break;
    default:
        assert(false);
        break;
    }
}

void plVKView::CreateImageView()
{
    vk::ImageViewCreateInfo image_view_desc = {};
    image_view_desc.image = m_resource->image.res;
    image_view_desc.format = m_resource->image.format;
    image_view_desc.viewType = GetImageViewType(m_view_desc.dimension);
    image_view_desc.subresourceRange.baseMipLevel = GetBaseMipLevel();
    image_view_desc.subresourceRange.levelCount = GetLevelCount();
    image_view_desc.subresourceRange.baseArrayLayer = GetBaseArrayLayer();
    image_view_desc.subresourceRange.layerCount = GetLayerCount();
    image_view_desc.subresourceRange.aspectMask = m_device.GetAspectFlags(image_view_desc.format);

    if (image_view_desc.subresourceRange.aspectMask & (vk::ImageAspectFlagBits::eDepth | vk::ImageAspectFlagBits::eStencil))
    {
        if (m_view_desc.planeSlice == 0)
        {
            image_view_desc.subresourceRange.aspectMask = vk::ImageAspectFlagBits::eDepth;
        }
        else
        {
            image_view_desc.subresourceRange.aspectMask = vk::ImageAspectFlagBits::eStencil;
            image_view_desc.components.g = vk::ComponentSwizzle::eR;
        }
    }

    m_image_view = m_device.GetDevice().createImageViewUnique(image_view_desc);
}

void plVKView::CreateBufferView()
{
    vk::BufferViewCreateInfo buffer_view_desc = {};
    buffer_view_desc.buffer = m_resource->buffer.res.get();
    buffer_view_desc.format = plVKUtils::ToVkFormat(m_view_desc.bufferFormat);
    buffer_view_desc.offset = m_view_desc.offset;
    buffer_view_desc.range = m_view_desc.bufferSize;
    m_buffer_view = m_device.GetDevice().createBufferViewUnique(buffer_view_desc);
}

plSharedPtr<plRHIResource> plVKView::GetResource()
{
    return m_resource;
}

plUInt32 plVKView::GetDescriptorId() const
{
    if (m_range)
        return m_range->GetOffset();
    return -1;
}

plUInt32 plVKView::GetBaseMipLevel() const
{
    return m_view_desc.baseMipLevel;
}

plUInt32 plVKView::GetLevelCount() const
{
    return std::min<plUInt32>(m_view_desc.levelCount, m_resource->GetLevelCount() - m_view_desc.baseMipLevel);
}

plUInt32 plVKView::GetBaseArrayLayer() const
{
    return m_view_desc.baseArrayLayer;
}

plUInt32 plVKView::GetLayerCount() const
{
    return std::min<plUInt32>(m_view_desc.layerCount, m_resource->GetLayerCount() - m_view_desc.baseArrayLayer);
}

vk::ImageView plVKView::GetImageView() const
{
    return m_image_view.get();
}

vk::WriteDescriptorSet plVKView::GetDescriptor() const
{
    return m_descriptor;
}
