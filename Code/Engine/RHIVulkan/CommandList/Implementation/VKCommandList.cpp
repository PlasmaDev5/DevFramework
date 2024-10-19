#include <RHIVulkan/RHIVulkanPCH.h>

#include <RHIVulkan/Adapter/VKAdapter.h>
#include <RHIVulkan/BindingSet/VKBindingSet.h>
#include <RHIVulkan/CommandList/VKCommandList.h>
#include <RHIVulkan/Device/VKDevice.h>
#include <RHIVulkan/Framebuffer/VKFramebuffer.h>
#include <RHIVulkan/Pipeline/VKComputePipeline.h>
#include <RHIVulkan/Pipeline/VKGraphicsPipeline.h>
#include <RHIVulkan/Pipeline/VKRayTracingPipeline.h>
#include <RHIVulkan/QueryHeap/VKQueryHeap.h>
#include <RHIVulkan/Resource/VKResource.h>
#include <RHIVulkan/Utilities/VKUtility.h>
#include <RHIVulkan/View/VKView.h>

PL_DEFINE_AS_POD_TYPE(vk::ClearValue);

plVKCommandList::plVKCommandList(plVKDevice& device, plRHICommandListType type)
  : m_device(device)
{
  vk::CommandBufferAllocateInfo cmd_buf_alloc_info = {};
  cmd_buf_alloc_info.commandPool = device.GetCmdPool(type);
  cmd_buf_alloc_info.commandBufferCount = 1;
  cmd_buf_alloc_info.level = vk::CommandBufferLevel::ePrimary;
  std::vector<vk::UniqueCommandBuffer> cmd_bufs = device.GetDevice().allocateCommandBuffersUnique(cmd_buf_alloc_info);
  m_command_list = std::move(cmd_bufs.front());
  vk::CommandBufferBeginInfo begin_info = {};
  m_command_list->begin(begin_info);
}

void plVKCommandList::Reset()
{
  Close();
  vk::CommandBufferBeginInfo begin_info = {};
  m_command_list->begin(begin_info);
  m_closed = false;
  m_state.Clear();
  m_binding_set.Clear();
}

void plVKCommandList::Close()
{
  if (!m_closed)
  {
    m_command_list->end();
    m_closed = true;
  }
}

vk::PipelineBindPoint GetPipelineBindPoint(plRHIPipelineType type)
{
  switch (type)
  {
    case plRHIPipelineType::kGraphics:
      return vk::PipelineBindPoint::eGraphics;
    case plRHIPipelineType::kCompute:
      return vk::PipelineBindPoint::eCompute;
    case plRHIPipelineType::kRayTracing:
      return vk::PipelineBindPoint::eRayTracingKHR;
  }
  assert(false);
  return {};
}

void plVKCommandList::BindPipeline(const plSharedPtr<plRHIPipeline>& state)
{
  if (state == m_state)
    return;
  m_state = state.Downcast<plVKPipeline>();
  m_command_list->bindPipeline(GetPipelineBindPoint(m_state->GetPipelineType()), m_state->GetPipeline());
}

void plVKCommandList::BindBindingSet(const plSharedPtr<plRHIBindingSet>& binding_set)
{
  if (binding_set == m_binding_set)
    return;
  m_binding_set = binding_set;
  decltype(auto) vk_binding_set = binding_set.Downcast<plVKBindingSet>();
  decltype(auto) descriptor_sets = vk_binding_set->GetDescriptorSets();
  if (descriptor_sets.IsEmpty())
    return;
  m_command_list->bindDescriptorSets(GetPipelineBindPoint(m_state->GetPipelineType()), m_state->GetPipelineLayout(), 0, descriptor_sets.GetCount(), descriptor_sets.GetData(), 0, nullptr);
}

void plVKCommandList::BeginRenderPass(const plSharedPtr<plRHIRenderPass>& renderPass, const plSharedPtr<plRHIFramebuffer>& framebuffer, const plRHIClearDesc& clearDesc)
{
  decltype(auto) vkFramebuffer = framebuffer.Downcast<plVKFramebuffer>();
  decltype(auto) vkRenderPass = renderPass.Downcast<plVKRenderPass>();
  vk::RenderPassBeginInfo renderPassInfo = {};
  renderPassInfo.renderPass = vkRenderPass->GetRenderPass();
  renderPassInfo.framebuffer = vkFramebuffer->GetFramebuffer();
  renderPassInfo.renderArea.extent = vkFramebuffer->GetExtent();
  plDynamicArray<vk::ClearValue> clearValues;
  for (size_t i = 0; i < clearDesc.colors.size(); ++i)
  {
    auto& clear_value = clearValues.ExpandAndGetRef();
    clear_value.color.float32[0] = clearDesc.colors[i].r;
    clear_value.color.float32[1] = clearDesc.colors[i].g;
    clear_value.color.float32[2] = clearDesc.colors[i].b;
    clear_value.color.float32[3] = clearDesc.colors[i].a;
  }
  clearValues.SetCountUninitialized((plUInt32)vkRenderPass->GetDesc().colors.size());
  if (vkRenderPass->GetDesc().depthStencil.format != plRHIResourceFormat::UNKNOWN)
  {
    vk::ClearValue clear_value = {};
    clear_value.depthStencil.depth = clearDesc.depth;
    clear_value.depthStencil.stencil = clearDesc.stencil;
    clearValues.PushBack(clear_value);
  }
  renderPassInfo.clearValueCount = clearValues.GetCount();
  renderPassInfo.pClearValues = clearValues.GetData();
  m_command_list->beginRenderPass(renderPassInfo, vk::SubpassContents::eInline);
}

void plVKCommandList::EndRenderPass()
{
  m_command_list->endRenderPass();
}

void plVKCommandList::BeginEvent(const plString& name)
{
  vk::DebugUtilsLabelEXT label = {};
  label.pLabelName = name.GetData();
  m_command_list->beginDebugUtilsLabelEXT(&label);
}

void plVKCommandList::EndEvent()
{
  m_command_list->endDebugUtilsLabelEXT();
}

void plVKCommandList::Draw(plUInt32 vertex_count, plUInt32 instance_count, plUInt32 first_vertex, plUInt32 first_instance)
{
  m_command_list->draw(vertex_count, instance_count, first_vertex, first_instance);
}

void plVKCommandList::DrawIndexed(plUInt32 index_count, plUInt32 instance_count, plUInt32 first_index, plInt32 vertex_offset, plUInt32 first_instance)
{
  m_command_list->drawIndexed(index_count, instance_count, first_index, vertex_offset, first_instance);
}

void plVKCommandList::DrawIndirect(const plSharedPtr<plRHIResource>& pArgumentBuffer, plUInt64 argument_buffer_offset)
{
  DrawIndirectCount(pArgumentBuffer, argument_buffer_offset, {}, 0, 1, sizeof(plRHIDrawIndirectCommand));
}

void plVKCommandList::DrawIndexedIndirect(const plSharedPtr<plRHIResource>& pArgumentBuffer, plUInt64 argument_buffer_offset)
{
  DrawIndexedIndirectCount(pArgumentBuffer, argument_buffer_offset, {}, 0, 1, sizeof(plRHIDrawIndexedIndirectCommand));
}

void plVKCommandList::DrawIndirectCount(
  const plSharedPtr<plRHIResource>& pArgumentBuffer,
  plUInt64 argument_buffer_offset,
  const plSharedPtr<plRHIResource>& count_buffer,
  plUInt64 count_buffer_offset,
  plUInt32 max_draw_count,
  plUInt32 stride)
{
  decltype(auto) vk_argument_buffer = pArgumentBuffer.Downcast<plVKResource>();
  if (count_buffer)
  {
    decltype(auto) vk_count_buffer = count_buffer.Downcast<plVKResource>();
    m_command_list->drawIndirectCount(
      vk_argument_buffer->buffer.res.get(),
      argument_buffer_offset,
      vk_count_buffer->buffer.res.get(),
      count_buffer_offset,
      max_draw_count,
      stride);
  }
  else
  {
    assert(count_buffer_offset == 0);
    m_command_list->drawIndirect(
      vk_argument_buffer->buffer.res.get(),
      argument_buffer_offset,
      max_draw_count,
      stride);
  }
}

void plVKCommandList::DrawIndexedIndirectCount(
  const plSharedPtr<plRHIResource>& pArgumentBuffer,
  plUInt64 argument_buffer_offset,
  const plSharedPtr<plRHIResource>& count_buffer,
  plUInt64 count_buffer_offset,
  plUInt32 max_draw_count,
  plUInt32 stride)
{
  decltype(auto) vk_argument_buffer = pArgumentBuffer.Downcast<plVKResource>();
  if (count_buffer)
  {
    decltype(auto) vk_count_buffer = count_buffer.Downcast<plVKResource>();
    m_command_list->drawIndexedIndirectCount(
      vk_argument_buffer->buffer.res.get(),
      argument_buffer_offset,
      vk_count_buffer->buffer.res.get(),
      count_buffer_offset,
      max_draw_count,
      stride);
  }
  else
  {
    assert(count_buffer_offset == 0);
    m_command_list->drawIndexedIndirect(
      vk_argument_buffer->buffer.res.get(),
      argument_buffer_offset,
      max_draw_count,
      stride);
  }
}

void plVKCommandList::Dispatch(plUInt32 thread_group_count_x, plUInt32 thread_group_count_y, plUInt32 thread_group_count_z)
{
  m_command_list->dispatch(thread_group_count_x, thread_group_count_y, thread_group_count_z);
}

void plVKCommandList::DispatchIndirect(const plSharedPtr<plRHIResource>& pArgumentBuffer, plUInt64 argument_buffer_offset)
{
  decltype(auto) vk_argument_buffer = pArgumentBuffer.Downcast<plVKResource>();
  m_command_list->dispatchIndirect(
    vk_argument_buffer->buffer.res.get(),
    argument_buffer_offset);
}

void plVKCommandList::DispatchMesh(plUInt32 thread_group_count_x)
{
  m_command_list->drawMeshTasksNV(thread_group_count_x, 0);
}

static vk::StridedDeviceAddressRegionKHR GetStridedDeviceAddressRegion(plVKDevice& device, const plRHIRayTracingShaderTable& table)
{
  if (!table.resource)
  {
    return {};
  }
  decltype(auto) vk_resource = table.resource.Downcast<plVKResource>();
  vk::StridedDeviceAddressRegionKHR vk_table = {};
  vk_table.deviceAddress = device.GetDevice().getBufferAddress({vk_resource->buffer.res.get()}) + table.offset;
  vk_table.size = table.size;
  vk_table.stride = table.stride;
  return vk_table;
}

void plVKCommandList::DispatchRays(const plRHIRayTracingShaderTables& shader_tables, plUInt32 width, plUInt32 height, plUInt32 depth)
{
  m_command_list->traceRaysKHR(
    GetStridedDeviceAddressRegion(m_device, shader_tables.raygen),
    GetStridedDeviceAddressRegion(m_device, shader_tables.miss),
    GetStridedDeviceAddressRegion(m_device, shader_tables.hit),
    GetStridedDeviceAddressRegion(m_device, shader_tables.callable),
    width,
    height,
    depth);
}

void plVKCommandList::ResourceBarrier(const std::vector<plRHIResourceBarrierDesc>& barriers)
{
  std::vector<vk::ImageMemoryBarrier> image_memory_barriers;
  for (const auto& barrier : barriers)
  {
    if (!barrier.resource)
    {
      assert(false);
      continue;
    }

    decltype(auto) vk_resource = barrier.resource.Downcast<plVKResource>();
    plVKResource::Image& image = vk_resource->image;
    if (!image.res)
      continue;

    vk::ImageLayout vk_state_before = ConvertState(barrier.stateBefore);
    vk::ImageLayout vk_state_after = ConvertState(barrier.stateAfter);
    if (vk_state_before == vk_state_after)
      continue;

    vk::ImageMemoryBarrier& image_memory_barrier = image_memory_barriers.emplace_back();
    image_memory_barrier.oldLayout = vk_state_before;
    image_memory_barrier.newLayout = vk_state_after;
    image_memory_barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    image_memory_barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    image_memory_barrier.image = image.res;

    vk::ImageSubresourceRange& range = image_memory_barrier.subresourceRange;
    range.aspectMask = m_device.GetAspectFlags(image.format);
    range.baseMipLevel = barrier.baseMipLevel;
    range.levelCount = barrier.levelCount;
    range.baseArrayLayer = barrier.baseArrayLayer;
    range.layerCount = barrier.layerCount;

    // Source layouts (old)
    // Source access mask controls actions that have to be finished on the old layout
    // before it will be transitioned to the new layout
    switch (image_memory_barrier.oldLayout)
    {
      case vk::ImageLayout::eUndefined:
        // Image layout is undefined (or does not matter)
        // Only valid as initial layout
        // No flags required, listed only for completeness
        image_memory_barrier.srcAccessMask = {};
        break;
      case vk::ImageLayout::ePreinitialized:
        // Image is preinitialized
        // Only valid as initial layout for linear images, preserves memory contents
        // Make sure host writes have been finished
        image_memory_barrier.srcAccessMask = vk::AccessFlagBits::eHostWrite;
        break;
      case vk::ImageLayout::eColorAttachmentOptimal:
        // Image is a color attachment
        // Make sure any writes to the color buffer have been finished
        image_memory_barrier.srcAccessMask = vk::AccessFlagBits::eColorAttachmentWrite;
        break;
      case vk::ImageLayout::eDepthAttachmentOptimal:
        // Image is a depth/stencil attachment
        // Make sure any writes to the depth/stencil buffer have been finished
        image_memory_barrier.srcAccessMask = vk::AccessFlagBits::eDepthStencilAttachmentWrite;
        break;
      case vk::ImageLayout::eTransferSrcOptimal:
        // Image is a transfer source
        // Make sure any reads from the image have been finished
        image_memory_barrier.srcAccessMask = vk::AccessFlagBits::eTransferRead;
        break;
      case vk::ImageLayout::eTransferDstOptimal:
        // Image is a transfer destination
        // Make sure any writes to the image have been finished
        image_memory_barrier.srcAccessMask = vk::AccessFlagBits::eTransferWrite;
        break;

      case vk::ImageLayout::eShaderReadOnlyOptimal:
        // Image is read by a shader
        // Make sure any shader reads from the image have been finished
        image_memory_barrier.srcAccessMask = vk::AccessFlagBits::eShaderRead;
        break;
      case vk::ImageLayout::eFragmentShadingRateAttachmentOptimalKHR:
        image_memory_barrier.srcAccessMask = vk::AccessFlagBits::eFragmentShadingRateAttachmentReadKHR;
      default:
        // Other source layouts aren't handled (yet)
        break;
    }

    // Target layouts (new)
    // Destination access mask controls the dependency for the new image layout
    switch (image_memory_barrier.newLayout)
    {
      case vk::ImageLayout::eTransferDstOptimal:
        // Image will be used as a transfer destination
        // Make sure any writes to the image have been finished
        image_memory_barrier.dstAccessMask = vk::AccessFlagBits::eTransferWrite;
        break;

      case vk::ImageLayout::eTransferSrcOptimal:
        // Image will be used as a transfer source
        // Make sure any reads from the image have been finished
        image_memory_barrier.dstAccessMask = vk::AccessFlagBits::eTransferRead;
        break;

      case vk::ImageLayout::eColorAttachmentOptimal:
        // Image will be used as a color attachment
        // Make sure any writes to the color buffer have been finished
        image_memory_barrier.dstAccessMask = vk::AccessFlagBits::eColorAttachmentWrite;
        break;

      case vk::ImageLayout::eDepthAttachmentOptimal:
        // Image layout will be used as a depth/stencil attachment
        // Make sure any writes to depth/stencil buffer have been finished
        image_memory_barrier.dstAccessMask = image_memory_barrier.dstAccessMask | vk::AccessFlagBits::eDepthStencilAttachmentWrite;
        break;

      case vk::ImageLayout::eShaderReadOnlyOptimal:
        // Image will be read in a shader (sampler, input attachment)
        // Make sure any writes to the image have been finished
        if (!image_memory_barrier.srcAccessMask)
        {
          image_memory_barrier.srcAccessMask = vk::AccessFlagBits::eHostWrite | vk::AccessFlagBits::eTransferWrite;
        }
        image_memory_barrier.dstAccessMask = vk::AccessFlagBits::eShaderRead;
        break;
      case vk::ImageLayout::eFragmentShadingRateAttachmentOptimalKHR:
        image_memory_barrier.dstAccessMask = vk::AccessFlagBits::eFragmentShadingRateAttachmentReadKHR;
        break;
      default:
        // Other source layouts aren't handled (yet)
        break;
    }
  }

  if (!image_memory_barriers.empty())
  {
    m_command_list->pipelineBarrier(
      vk::PipelineStageFlagBits::eAllCommands, vk::PipelineStageFlagBits::eAllCommands,
      vk::DependencyFlagBits::eByRegion,
      0, nullptr,
      0, nullptr,
      (plUInt32)image_memory_barriers.size(), image_memory_barriers.data());
  }
}

void plVKCommandList::UAVResourceBarrier(const plSharedPtr<plRHIResource>& /*resource*/)
{
  vk::MemoryBarrier memory_barrier = {};
  memory_barrier.srcAccessMask = vk::AccessFlagBits::eAccelerationStructureWriteKHR | vk::AccessFlagBits::eAccelerationStructureReadKHR | vk::AccessFlagBits::eShaderWrite | vk::AccessFlagBits::eShaderRead;
  memory_barrier.dstAccessMask = memory_barrier.srcAccessMask;
  m_command_list->pipelineBarrier(vk::PipelineStageFlagBits::eAllCommands, vk::PipelineStageFlagBits::eAllCommands, vk::DependencyFlagBits::eByRegion, 1, &memory_barrier, 0, 0, 0, 0);
}

void plVKCommandList::SetViewport(float x, float y, float width, float height)
{
  vk::Viewport viewport = {};
  viewport.x = 0;
  viewport.y = height - y;
  viewport.width = width;
  viewport.height = -height;
  viewport.minDepth = 0;
  viewport.maxDepth = 1.0;
  m_command_list->setViewport(0, 1, &viewport);
}

void plVKCommandList::SetScissorRect(plInt32 left, plInt32 top, plUInt32 right, plUInt32 bottom)
{
  vk::Rect2D rect = {};
  rect.offset.x = left;
  rect.offset.y = top;
  rect.extent.width = right;
  rect.extent.height = bottom;
  m_command_list->setScissor(0, 1, &rect);
}

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

void plVKCommandList::IASetIndexBuffer(const plSharedPtr<plRHIResource>& resource, plRHIResourceFormat::Enum format)
{
  decltype(auto) vk_resource = resource.Downcast<plVKResource>();
  vk::IndexType index_type = GetVkIndexType(format);
  m_command_list->bindIndexBuffer(vk_resource->buffer.res.get(), 0, index_type);
}

void plVKCommandList::IASetVertexBuffer(plUInt32 slot, const plSharedPtr<plRHIResource>& resource)
{
  decltype(auto) vk_resource = resource.Downcast<plVKResource>();
  vk::Buffer vertex_buffers[] = {vk_resource->buffer.res.get()};
  vk::DeviceSize offsets[] = {0};
  m_command_list->bindVertexBuffers(slot, 1, vertex_buffers, offsets);
}

void plVKCommandList::RSSetShadingRate(plRHIShadingRate shading_rate, const std::array<plRHIShadingRateCombiner, 2>& combiners)
{
  vk::Extent2D fragment_size = {1, 1};
  switch (shading_rate)
  {
    case plRHIShadingRate::k1x1:
      fragment_size.width = 1;
      fragment_size.height = 1;
      break;
    case plRHIShadingRate::k1x2:
      fragment_size.width = 1;
      fragment_size.height = 2;
      break;
    case plRHIShadingRate::k2x1:
      fragment_size.width = 2;
      fragment_size.height = 1;
      break;
    case plRHIShadingRate::k2x2:
      fragment_size.width = 2;
      fragment_size.height = 2;
      break;
    case plRHIShadingRate::k2x4:
      fragment_size.width = 2;
      fragment_size.height = 4;
      break;
    case plRHIShadingRate::k4x2:
      fragment_size.width = 4;
      fragment_size.height = 2;
      break;
    case plRHIShadingRate::k4x4:
      fragment_size.width = 4;
      fragment_size.height = 4;
      break;
    default:
      assert(false);
      break;
  }

  std::array<vk::FragmentShadingRateCombinerOpKHR, 2> vk_combiners;
  for (size_t i = 0; i < vk_combiners.size(); ++i)
  {
    switch (combiners[i])
    {
      case plRHIShadingRateCombiner::kPassthrough:
        vk_combiners[i] = vk::FragmentShadingRateCombinerOpKHR::eKeep;
        break;
      case plRHIShadingRateCombiner::kOverride:
        vk_combiners[i] = vk::FragmentShadingRateCombinerOpKHR::eReplace;
        break;
      case plRHIShadingRateCombiner::kMin:
        vk_combiners[i] = vk::FragmentShadingRateCombinerOpKHR::eMin;
        break;
      case plRHIShadingRateCombiner::kMax:
        vk_combiners[i] = vk::FragmentShadingRateCombinerOpKHR::eMax;
        break;
      case plRHIShadingRateCombiner::kSum:
        vk_combiners[i] = vk::FragmentShadingRateCombinerOpKHR::eMul;
        break;
      default:
        assert(false);
        break;
    }
  }

  m_command_list->setFragmentShadingRateKHR(&fragment_size, vk_combiners.data());
}

void plVKCommandList::BuildBottomLevelAS(
  const plSharedPtr<plRHIResource>& src,
  const plSharedPtr<plRHIResource>& dst,
  const plSharedPtr<plRHIResource>& scratch,
  plUInt64 scratch_offset,
  const std::vector<plRHIRaytracingGeometryDesc>& descs,
  plRHIBuildAccelerationStructureFlags flags)
{
  std::vector<vk::AccelerationStructureGeometryKHR> geometry_descs;
  for (const auto& desc : descs)
  {
    geometry_descs.emplace_back(m_device.FillRaytracingGeometryTriangles(desc.vertex, desc.index, desc.flags));
  }

  decltype(auto) vk_dst = dst.Downcast<plVKResource>();
  decltype(auto) vk_scratch = scratch.Downcast<plVKResource>();

  vk::AccelerationStructureKHR vk_src_as = {};
  if (src)
  {
    decltype(auto) vk_src = src.Downcast<plVKResource>();
    vk_src_as = vk_src->acceleration_structure_handle.get();
  }

  std::vector<vk::AccelerationStructureBuildRangeInfoKHR> ranges;
  for (const auto& desc : descs)
  {
    vk::AccelerationStructureBuildRangeInfoKHR& offset = ranges.emplace_back();
    if (desc.index.res)
      offset.primitiveCount = desc.index.count / 3;
    else
      offset.primitiveCount = desc.vertex.count / 3;
  }
  std::vector<const vk::AccelerationStructureBuildRangeInfoKHR*> range_infos(ranges.size());
  for (size_t i = 0; i < ranges.size(); ++i)
    range_infos[i] = &ranges[i];

  vk::AccelerationStructureBuildGeometryInfoKHR infos = {};
  infos.type = vk::AccelerationStructureTypeKHR::eBottomLevel;
  infos.flags = Convert(flags);
  infos.dstAccelerationStructure = vk_dst->acceleration_structure_handle.get();
  infos.srcAccelerationStructure = vk_src_as;
  if (vk_src_as)
    infos.mode = vk::BuildAccelerationStructureModeKHR::eUpdate;
  else
    infos.mode = vk::BuildAccelerationStructureModeKHR::eBuild;
  infos.scratchData = m_device.GetDevice().getBufferAddress({vk_scratch->buffer.res.get()}) + scratch_offset;
  infos.pGeometries = geometry_descs.data();
  infos.geometryCount = (plUInt32)geometry_descs.size();

  m_command_list->buildAccelerationStructuresKHR(1, &infos, range_infos.data());
}

void plVKCommandList::BuildTopLevelAS(
  const plSharedPtr<plRHIResource>& src,
  const plSharedPtr<plRHIResource>& dst,
  const plSharedPtr<plRHIResource>& scratch,
  plUInt64 scratch_offset,
  const plSharedPtr<plRHIResource>& instance_data,
  plUInt64 instance_offset,
  plUInt32 instance_count,
  plRHIBuildAccelerationStructureFlags flags)
{
  decltype(auto) vk_instance_data = instance_data.Downcast<plVKResource>();
  vk::DeviceAddress instance_address = m_device.GetDevice().getBufferAddress(vk_instance_data->buffer.res.get()) + instance_offset;

  vk::AccelerationStructureGeometryKHR top_as_geometry = {};
  top_as_geometry.geometryType = vk::GeometryTypeKHR::eInstances;
  top_as_geometry.geometry.setInstances({});
  top_as_geometry.geometry.instances.arrayOfPointers = VK_FALSE;
  top_as_geometry.geometry.instances.data = instance_address;

  decltype(auto) vk_dst = dst.Downcast<plVKResource>();
  decltype(auto) vk_scratch = scratch.Downcast<plVKResource>();

  vk::AccelerationStructureKHR vk_src_as = {};
  if (src)
  {
    decltype(auto) vk_src = src.Downcast<plVKResource>();
    vk_src_as = vk_src->acceleration_structure_handle.get();
  }

  vk::AccelerationStructureBuildRangeInfoKHR acceleration_structure_build_range_info = {};
  acceleration_structure_build_range_info.primitiveCount = instance_count;
  std::vector<vk::AccelerationStructureBuildRangeInfoKHR*> offset_infos = {&acceleration_structure_build_range_info};

  vk::AccelerationStructureBuildGeometryInfoKHR infos = {};
  infos.type = vk::AccelerationStructureTypeKHR::eTopLevel;
  infos.flags = Convert(flags);
  infos.dstAccelerationStructure = vk_dst->acceleration_structure_handle.get();
  infos.srcAccelerationStructure = vk_src_as;
  if (vk_src_as)
    infos.mode = vk::BuildAccelerationStructureModeKHR::eUpdate;
  else
    infos.mode = vk::BuildAccelerationStructureModeKHR::eBuild;
  infos.scratchData = m_device.GetDevice().getBufferAddress({vk_scratch->buffer.res.get()}) + scratch_offset;
  infos.pGeometries = &top_as_geometry;
  infos.geometryCount = 1;

  m_command_list->buildAccelerationStructuresKHR(1, &infos, offset_infos.data());
}

void plVKCommandList::CopyAccelerationStructure(const plSharedPtr<plRHIResource>& src, const plSharedPtr<plRHIResource>& dst, plRHICopyAccelerationStructureMode mode)
{
  decltype(auto) vk_src = src.Downcast<plVKResource>();
  decltype(auto) vk_dst = dst.Downcast<plVKResource>();
  vk::CopyAccelerationStructureInfoKHR info = {};
  switch (mode)
  {
    case plRHICopyAccelerationStructureMode::kClone:
      info.mode = vk::CopyAccelerationStructureModeKHR::eClone;
      break;
    case plRHICopyAccelerationStructureMode::kCompact:
      info.mode = vk::CopyAccelerationStructureModeKHR::eCompact;
      break;
    default:
      assert(false);
      info.mode = vk::CopyAccelerationStructureModeKHR::eClone;
      break;
  }
  info.dst = vk_dst->acceleration_structure_handle.get();
  info.src = vk_src->acceleration_structure_handle.get();
  m_command_list->copyAccelerationStructureKHR(info);
}

void plVKCommandList::CopyBuffer(const plSharedPtr<plRHIResource>& src_buffer, const plSharedPtr<plRHIResource>& dst_buffer,
  const std::vector<plRHIBufferCopyRegion>& regions)
{
  decltype(auto) vk_src_buffer = src_buffer.Downcast<plVKResource>();
  decltype(auto) vk_dst_buffer = dst_buffer.Downcast<plVKResource>();
  std::vector<vk::BufferCopy> vk_regions;
  for (const auto& region : regions)
  {
    vk_regions.emplace_back(region.srcOffset, region.dstOffset, region.numBytes);
  }
  m_command_list->copyBuffer(vk_src_buffer->buffer.res.get(), vk_dst_buffer->buffer.res.get(), vk_regions);
}

void plVKCommandList::CopyBufferToTexture(const plSharedPtr<plRHIResource>& src_buffer, const plSharedPtr<plRHIResource>& dst_texture,
  const std::vector<plRHIBufferToTextureCopyRegion>& regions)
{
  decltype(auto) vk_src_buffer = src_buffer.Downcast<plVKResource>();
  decltype(auto) vk_dst_texture = dst_texture.Downcast<plVKResource>();
  std::vector<vk::BufferImageCopy> vk_regions;
  auto format = dst_texture->GetFormat();
  for (const auto& region : regions)
  {
    auto& vk_region = vk_regions.emplace_back();
    vk_region.bufferOffset = region.bufferOffset;
    if (plRHIResourceFormat::IsFormatBlockCompressed(format))
    {
      plVec3U32 extent = plRHIResourceFormat::GetBlockExtent(format);
      vk_region.bufferRowLength = region.bufferRowPitch / plRHIResourceFormat::GetBlockSize(format) * extent.x;
      vk_region.bufferImageHeight = ((region.textureExtent.height + extent.y - 1) / extent.y) * extent.x;
    }
    else
    {
      vk_region.bufferRowLength = region.bufferRowPitch / plRHIResourceFormat::GetFormatStride(format);
      vk_region.bufferImageHeight = region.textureExtent.height;
    }
    vk_region.imageSubresource.aspectMask = vk::ImageAspectFlagBits::eColor;
    vk_region.imageSubresource.mipLevel = region.textureMipLevel;
    vk_region.imageSubresource.baseArrayLayer = region.textureArrayLayer;
    vk_region.imageSubresource.layerCount = 1;
    vk_region.imageOffset.x = region.textureOffset.x;
    vk_region.imageOffset.y = region.textureOffset.y;
    vk_region.imageOffset.z = region.textureOffset.z;
    vk_region.imageExtent.width = region.textureExtent.width;
    vk_region.imageExtent.height = region.textureExtent.height;
    vk_region.imageExtent.depth = region.textureExtent.depth;
  }
  m_command_list->copyBufferToImage(
    vk_src_buffer->buffer.res.get(),
    vk_dst_texture->image.res,
    vk::ImageLayout::eTransferDstOptimal,
    vk_regions);
}

void plVKCommandList::CopyTexture(const plSharedPtr<plRHIResource>& src_texture, const plSharedPtr<plRHIResource>& dst_texture,
  const std::vector<plRHITextureCopyRegion>& regions)
{
  decltype(auto) vk_src_texture = src_texture.Downcast<plVKResource>();
  decltype(auto) vk_dst_texture = dst_texture.Downcast<plVKResource>();
  std::vector<vk::ImageCopy> vk_regions;
  for (const auto& region : regions)
  {
    auto& vk_region = vk_regions.emplace_back();
    vk_region.srcSubresource.aspectMask = vk::ImageAspectFlagBits::eColor;
    vk_region.srcSubresource.mipLevel = region.srcMipLevel;
    vk_region.srcSubresource.baseArrayLayer = region.srcArrayLayer;
    vk_region.srcSubresource.layerCount = 1;
    vk_region.srcOffset.x = region.srcOffset.x;
    vk_region.srcOffset.y = region.srcOffset.y;
    vk_region.srcOffset.z = region.srcOffset.z;

    vk_region.dstSubresource.aspectMask = vk::ImageAspectFlagBits::eColor;
    vk_region.dstSubresource.mipLevel = region.dstMipLevel;
    vk_region.dstSubresource.baseArrayLayer = region.dstArrayLayer;
    vk_region.dstSubresource.layerCount = 1;
    vk_region.dstOffset.x = region.dstOffset.x;
    vk_region.dstOffset.y = region.dstOffset.y;
    vk_region.dstOffset.z = region.dstOffset.z;

    vk_region.extent.width = region.extent.width;
    vk_region.extent.height = region.extent.height;
    vk_region.extent.depth = region.extent.depth;
  }
  m_command_list->copyImage(vk_src_texture->image.res, vk::ImageLayout::eTransferSrcOptimal, vk_dst_texture->image.res, vk::ImageLayout::eTransferDstOptimal, vk_regions);
}

void plVKCommandList::WriteAccelerationStructuresProperties(
  const std::vector<plSharedPtr<plRHIResource>>& acceleration_structures,
  const plSharedPtr<plRHIQueryHeap>& query_heap,
  plUInt32 first_query)
{
  std::vector<vk::AccelerationStructureKHR> vk_acceleration_structures;
  vk_acceleration_structures.reserve(acceleration_structures.size());
  for (const auto& acceleration_structure : acceleration_structures)
  {
    vk_acceleration_structures.emplace_back(acceleration_structure.Downcast<plVKResource>()->acceleration_structure_handle.get());
  }
  decltype(auto) vk_query_heap = query_heap.Downcast<plVKQueryHeap>();
  auto query_type = vk_query_heap->GetQueryType();
  assert(query_type == vk::QueryType::eAccelerationStructureCompactedSizeKHR);
  m_command_list->resetQueryPool(vk_query_heap->GetQueryPool(), first_query, (plUInt32)acceleration_structures.size());
  m_command_list->writeAccelerationStructuresPropertiesKHR(
    (plUInt32)vk_acceleration_structures.size(),
    vk_acceleration_structures.data(),
    query_type,
    vk_query_heap->GetQueryPool(),
    first_query);
}

void plVKCommandList::ResolveQueryData(
  const plSharedPtr<plRHIQueryHeap>& query_heap,
  plUInt32 first_query,
  plUInt32 query_count,
  const plSharedPtr<plRHIResource>& dst_buffer,
  plUInt64 dst_offset)
{
  decltype(auto) vk_query_heap = query_heap.Downcast<plVKQueryHeap>();
  auto query_type = vk_query_heap->GetQueryType();
  assert(query_type == vk::QueryType::eAccelerationStructureCompactedSizeKHR);
  m_command_list->copyQueryPoolResults(
    vk_query_heap->GetQueryPool(),
    first_query,
    query_count,
    dst_buffer.Downcast<plVKResource>()->buffer.res.get(),
    dst_offset,
    sizeof(plUInt64),
    vk::QueryResultFlagBits::eWait);
}

vk::CommandBuffer plVKCommandList::GetCommandList()
{
  return m_command_list.get();
}
