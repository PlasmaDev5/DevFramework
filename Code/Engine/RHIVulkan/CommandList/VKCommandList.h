#pragma once
#include <RHIVulkan/RHIVulkanDLL.h>

#include <RHI/CommandList/CommandList.h>

class plVKDevice;
class plVKPipeline;

class plVKCommandList : public plRHICommandList
{
public:
    plVKCommandList(plVKDevice& device, plRHICommandListType type);
    void Reset() override;
    void Close() override;
    void BindPipeline(const plSharedPtr<plRHIPipeline>& state) override;
    void BindBindingSet(const plSharedPtr<plRHIBindingSet>& binding_set) override;
    void BeginRenderPass(const plSharedPtr<plRHIRenderPass>& render_pass, const plSharedPtr<plRHIFramebuffer>& framebuffer, const plRHIClearDesc& clear_desc) override;
    void EndRenderPass() override;
    void BeginEvent(const plString& name) override;
    void EndEvent() override;
    void Draw(plUInt32 vertex_count, plUInt32 instance_count, plUInt32 first_vertex, plUInt32 first_instance) override;
    void DrawIndexed(plUInt32 index_count, plUInt32 instance_count, plUInt32 first_index, plInt32 vertex_offset, plUInt32 first_instance) override;
    void DrawIndirect(const plSharedPtr<plRHIResource>& pArgumentBuffer, plUInt64 argument_buffer_offset) override;
    void DrawIndexedIndirect(const plSharedPtr<plRHIResource>& pArgumentBuffer, plUInt64 argument_buffer_offset) override;
    void DrawIndirectCount(
        const plSharedPtr<plRHIResource>& pArgumentBuffer,
        plUInt64 argument_buffer_offset,
        const plSharedPtr<plRHIResource>& count_buffer,
        plUInt64 count_buffer_offset,
        plUInt32 max_draw_count,
        plUInt32 stride) override;
    void DrawIndexedIndirectCount(
        const plSharedPtr<plRHIResource>& pArgumentBuffer,
        plUInt64 argument_buffer_offset,
        const plSharedPtr<plRHIResource>& count_buffer,
        plUInt64 count_buffer_offset,
        plUInt32 max_draw_count,
        plUInt32 stride) override;
    void Dispatch(plUInt32 thread_group_count_x, plUInt32 thread_group_count_y, plUInt32 thread_group_count_z) override;
    void DispatchIndirect(const plSharedPtr<plRHIResource>& pArgumentBuffer, plUInt64 argument_buffer_offset) override;
    void DispatchMesh(plUInt32 thread_group_count_x) override;
    void DispatchRays(const plRHIRayTracingShaderTables& shader_tables, plUInt32 width, plUInt32 height, plUInt32 depth) override;
    void ResourceBarrier(const std::vector<plRHIResourceBarrierDesc>& barriers) override;
    void UAVResourceBarrier(const plSharedPtr<plRHIResource>& resource) override;
    void SetViewport(float x, float y, float width, float height) override;
    void SetScissorRect(plInt32 left, plInt32 top, plUInt32 right, plUInt32 bottom) override;
    void IASetIndexBuffer(const plSharedPtr<plRHIResource>& resource, plRHIResourceFormat::Enum format) override;
    void IASetVertexBuffer(plUInt32 slot, const plSharedPtr<plRHIResource>& resource) override;
    void RSSetShadingRate(plRHIShadingRate shading_rate, const std::array<plRHIShadingRateCombiner, 2>& combiners) override;
    void BuildBottomLevelAS(
        const plSharedPtr<plRHIResource>& src,
        const plSharedPtr<plRHIResource>& dst,
        const plSharedPtr<plRHIResource>& scratch,
        plUInt64 scratch_offset,
        const std::vector<plRHIRaytracingGeometryDesc>& descs,
        plRHIBuildAccelerationStructureFlags flags) override;
    void BuildTopLevelAS(
        const plSharedPtr<plRHIResource>& src,
        const plSharedPtr<plRHIResource>& dst,
        const plSharedPtr<plRHIResource>& scratch,
        plUInt64 scratch_offset,
        const plSharedPtr<plRHIResource>& instance_data,
        plUInt64 instance_offset,
        plUInt32 instance_count,
        plRHIBuildAccelerationStructureFlags flags) override;
    void CopyAccelerationStructure(const plSharedPtr<plRHIResource>& src, const plSharedPtr<plRHIResource>& dst, plRHICopyAccelerationStructureMode mode) override;
    void CopyBuffer(const plSharedPtr<plRHIResource>& src_buffer, const plSharedPtr<plRHIResource>& dst_buffer,
                    const std::vector<plRHIBufferCopyRegion>& regions) override;
    void CopyBufferToTexture(const plSharedPtr<plRHIResource>& src_buffer, const plSharedPtr<plRHIResource>& dst_texture,
                             const std::vector<plRHIBufferToTextureCopyRegion>& regions) override;
    void CopyTexture(const plSharedPtr<plRHIResource>& src_texture, const plSharedPtr<plRHIResource>& dst_texture,
                     const std::vector<plRHITextureCopyRegion>& regions) override;
    void WriteAccelerationStructuresProperties(
        const std::vector<plSharedPtr<plRHIResource>>& acceleration_structures,
        const plSharedPtr<plRHIQueryHeap>& query_heap,
        plUInt32 first_query) override;
    void ResolveQueryData(
        const plSharedPtr<plRHIQueryHeap>& query_heap,
        plUInt32 first_query,
        plUInt32 query_count,
        const plSharedPtr<plRHIResource>& dst_buffer,
        plUInt64 dst_offset) override;

    vk::CommandBuffer GetCommandList();

private:
    void BuildAccelerationStructure(vk::AccelerationStructureCreateInfoKHR& build_info, const vk::Buffer& instance_data, plUInt64 instance_offset, const plSharedPtr<plRHIResource>& src, const plSharedPtr<plRHIResource>& dst, const plSharedPtr<plRHIResource>& scratch, plUInt64 scratch_offset);

    plVKDevice& m_device;
    vk::UniqueCommandBuffer m_command_list;
    bool m_closed = false;
    plSharedPtr<plVKPipeline> m_state;
    plSharedPtr<plRHIBindingSet> m_binding_set;
};
