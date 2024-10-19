#include <RHIDX12/BindingSet/DXBindingSet.h>
#include <RHIDX12/CommandList/DXCommandList.h>
#include <RHIDX12/Device/DXDevice.h>
#include <RHIDX12/Framebuffer/DXFramebuffer.h>
#include <RHIDX12/Pipeline/DXComputePipeline.h>
#include <RHIDX12/Pipeline/DXGraphicsPipeline.h>
#include <RHIDX12/Pipeline/DXRayTracingPipeline.h>
#include <RHIDX12/QueryHeap/DXRayTracingQueryHeap.h>
#include <RHIDX12/RenderPass/DXRenderPass.h>
#include <RHIDX12/Resource/DXResource.h>
#include <RHIDX12/Utilities/DXUtility.h>
#include <RHIDX12/View/DXView.h>
#include <directx/d3d12.h>
#include <directx/d3dx12.h>
#include <dxgi1_6.h>
#include <pix.h>

plDXCommandList::plDXCommandList(plDXDevice& device, plRHICommandListType type)
  : m_Device(device)
{
  D3D12_COMMAND_LIST_TYPE dxType{};
  switch (type)
  {
    case plRHICommandListType::kGraphics:
      dxType = D3D12_COMMAND_LIST_TYPE_DIRECT;
      break;
    case plRHICommandListType::kCompute:
      dxType = D3D12_COMMAND_LIST_TYPE_COMPUTE;
      break;
    case plRHICommandListType::kCopy:
      dxType = D3D12_COMMAND_LIST_TYPE_COPY;
      break;
    default:
      assert(false);
      break;
  }
  PL_ASSERT_ALWAYS(device.GetDevice()->CreateCommandAllocator(dxType, IID_PPV_ARGS(&m_CommandAllocator)) == S_OK, "");
  PL_ASSERT_ALWAYS(device.GetDevice()->CreateCommandList(0, dxType, m_CommandAllocator.Get(), nullptr, IID_PPV_ARGS(&m_CommandList)) == S_OK, "");

  m_CommandList.As(&m_CommandList4);
  m_CommandList.As(&m_CommandList5);
  m_CommandList.As(&m_CommandList6);
}

void plDXCommandList::Reset()
{
  Close();
  PL_ASSERT_ALWAYS(m_CommandAllocator->Reset() == S_OK, "");
  PL_ASSERT_ALWAYS(m_CommandList->Reset(m_CommandAllocator.Get(), nullptr) == S_OK, "");
  m_Closed = false;
  m_Heaps.Clear();
  m_State.Clear();
  m_BindingSet.Clear();
  m_LazyVertex.Clear();
  m_ShadingRateImageView.Clear();
}

void plDXCommandList::Close()
{
  if (!m_Closed)
  {
    m_CommandList->Close();
    m_Closed = true;
  }
}

void plDXCommandList::BindPipeline(const plSharedPtr<plRHIPipeline>& state)
{
  if (state == m_State)
    return;
  m_State = state.Downcast<plDXPipeline>();
  m_CommandList->SetComputeRootSignature(m_State->GetRootSignature().Get());
  auto _type = m_State->GetPipelineType();
  if (_type == plRHIPipelineType::kGraphics)
  {
    decltype(auto) dxState = state.Downcast<plDXGraphicsPipeline>();
    m_CommandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
    m_CommandList->SetGraphicsRootSignature(dxState->GetRootSignature().Get());
    m_CommandList->SetPipelineState(dxState->GetPipeline().Get());
    for (const auto& x : dxState->GetStrideMap())
    {
      auto it = m_LazyVertex.Find(x.Key());
      if (it != end(m_LazyVertex))
        IASetVertexBufferImpl(x.Key(), it.Value(), x.Value());
      else
        IASetVertexBufferImpl(x.Key(), {}, 0);
    }
  }
  else if (_type == plRHIPipelineType::kCompute)
  {
    decltype(auto) dxState = state.Downcast<plDXComputePipeline>();
    m_CommandList->SetPipelineState(dxState->GetPipeline().Get());
  }
  else if (_type == plRHIPipelineType::kRayTracing)
  {
    decltype(auto) dxState = state.Downcast<plDXRayTracingPipeline>();
    m_CommandList4->SetPipelineState1(dxState->GetPipeline().Get());
  }
}

void plDXCommandList::BindBindingSet(const plSharedPtr<plRHIBindingSet>& bindingSet)
{
  if (bindingSet == m_BindingSet)
    return;
  decltype(auto) dxBindingSet = bindingSet.Downcast<plDXBindingSet>();
  decltype(auto) newHeaps = dxBindingSet->Apply(m_CommandList);
  m_Heaps.PushBackRange(newHeaps);
  m_BindingSet = bindingSet;
}

void plDXCommandList::BeginRenderPass(const plSharedPtr<plRHIRenderPass>& renderPass, const plSharedPtr<plRHIFramebuffer>& framebuffer, const plRHIClearDesc& clearDesc)
{
  if (m_Device.IsRenderPassesSupported())
  {
    BeginRenderPassImpl(renderPass, framebuffer, clearDesc);
  }
  else
  {
    OMSetFramebuffer(renderPass, framebuffer, clearDesc);
  }

  decltype(auto) shadingRateImageView = framebuffer.Downcast<plRHIFramebufferBase>()->GetDesc().shadingRateImage;
  if (shadingRateImageView == m_ShadingRateImageView)
  {
    return;
  }

  if (shadingRateImageView)
  {
    decltype(auto) dxShadingRateImage = shadingRateImageView->GetResource().Downcast<plDXResource>();
    m_CommandList5->RSSetShadingRateImage(dxShadingRateImage->resource.Get());
  }
  else
  {
    m_CommandList5->RSSetShadingRateImage(nullptr);
  }
  m_ShadingRateImageView = shadingRateImageView;
}

D3D12_RENDER_PASS_BEGINNING_ACCESS_TYPE Convert(plRHIRenderPassLoadOp op)
{
  switch (op)
  {
    case plRHIRenderPassLoadOp::kLoad:
      return D3D12_RENDER_PASS_BEGINNING_ACCESS_TYPE_PRESERVE;
    case plRHIRenderPassLoadOp::kClear:
      return D3D12_RENDER_PASS_BEGINNING_ACCESS_TYPE_CLEAR;
    case plRHIRenderPassLoadOp::kDontCare:
      return D3D12_RENDER_PASS_BEGINNING_ACCESS_TYPE_DISCARD;
  }
  return D3D12_RENDER_PASS_BEGINNING_ACCESS_TYPE_DISCARD;
}

D3D12_RENDER_PASS_ENDING_ACCESS_TYPE Convert(plRHIRenderPassStoreOp op)
{
  switch (op)
  {
    case plRHIRenderPassStoreOp::kStore:
      return D3D12_RENDER_PASS_ENDING_ACCESS_TYPE_PRESERVE;
    case plRHIRenderPassStoreOp::kDontCare:
      return D3D12_RENDER_PASS_ENDING_ACCESS_TYPE_DISCARD;
  }

  return D3D12_RENDER_PASS_ENDING_ACCESS_TYPE_DISCARD;
}

void plDXCommandList::BeginRenderPassImpl(const plSharedPtr<plRHIRenderPass>& renderPass, const plSharedPtr<plRHIFramebuffer>& framebuffer, const plRHIClearDesc& clearDesc)
{
  decltype(auto) dxRenderPass = renderPass.Downcast<plDXRenderPass>();
  decltype(auto) dxFramebuffer = framebuffer.Downcast<plDXFramebuffer>();
  auto& rtvs = dxFramebuffer->GetDesc().colors;
  auto& dsv = dxFramebuffer->GetDesc().depthStencil;

  auto getHandle = [](const plSharedPtr<plRHIView>& view)
  {
    if (!view)
      return D3D12_CPU_DESCRIPTOR_HANDLE{};
    decltype(auto) dxView = view.Downcast<plDXView>();
    return dxView->GetHandle();
  };

  std::vector<D3D12_RENDER_PASS_RENDER_TARGET_DESC> omRtv;
  for (plUInt32 slot = 0; slot < rtvs.size(); ++slot)
  {
    auto handle = getHandle(rtvs[slot]);
    if (!handle.ptr)
      continue;
    D3D12_RENDER_PASS_BEGINNING_ACCESS begin = {Convert(dxRenderPass->GetDesc().colors[slot].loadOp), {}};
    if (slot < clearDesc.colors.size())
    {
      begin.Clear.ClearValue.Color[0] = clearDesc.colors[slot].r;
      begin.Clear.ClearValue.Color[1] = clearDesc.colors[slot].g;
      begin.Clear.ClearValue.Color[2] = clearDesc.colors[slot].b;
      begin.Clear.ClearValue.Color[3] = clearDesc.colors[slot].a;
    }
    D3D12_RENDER_PASS_ENDING_ACCESS end = {Convert(dxRenderPass->GetDesc().colors[slot].storeOp), {}};
    omRtv.push_back({handle, begin, end});
  }

  D3D12_RENDER_PASS_DEPTH_STENCIL_DESC omDsv = {};
  D3D12_RENDER_PASS_DEPTH_STENCIL_DESC* omDsvPtr = nullptr;
  D3D12_CPU_DESCRIPTOR_HANDLE omDsvHandle = getHandle(dsv);
  if (omDsvHandle.ptr)
  {
    D3D12_RENDER_PASS_BEGINNING_ACCESS depthBegin = {Convert(dxRenderPass->GetDesc().depthStencil.depthLoadOp), {}};
    D3D12_RENDER_PASS_ENDING_ACCESS depthEnd = {Convert(dxRenderPass->GetDesc().depthStencil.depthStoreOp), {}};
    D3D12_RENDER_PASS_BEGINNING_ACCESS stencilBegin = {Convert(dxRenderPass->GetDesc().depthStencil.stencilLoadOp), {}};
    D3D12_RENDER_PASS_ENDING_ACCESS stencilEnd = {Convert(dxRenderPass->GetDesc().depthStencil.stencilStoreOp), {}};
    depthBegin.Clear.ClearValue.DepthStencil.Depth = clearDesc.depth;
    stencilBegin.Clear.ClearValue.DepthStencil.Stencil = clearDesc.stencil;
    omDsv = {omDsvHandle, depthBegin, stencilBegin, depthEnd, stencilEnd};
    omDsvPtr = &omDsv;
  }

  m_CommandList4->BeginRenderPass(static_cast<plUInt32>(omRtv.size()), omRtv.data(), omDsvPtr, D3D12_RENDER_PASS_FLAG_NONE);
}

void plDXCommandList::OMSetFramebuffer(const plSharedPtr<plRHIRenderPass>& renderPass, const plSharedPtr<plRHIFramebuffer>& framebuffer, const plRHIClearDesc& clearDesc)
{
  decltype(auto) dxRenderPass = renderPass.Downcast<plDXRenderPass>();
  decltype(auto) dxFramebuffer = framebuffer.Downcast<plDXFramebuffer>();
  auto& rtvs = dxFramebuffer->GetDesc().colors;
  auto& dsv = dxFramebuffer->GetDesc().depthStencil;

  std::vector<D3D12_CPU_DESCRIPTOR_HANDLE> omRtv(rtvs.size());
  auto getHandle = [](const plSharedPtr<plRHIView>& view)
  {
    if (!view)
      return D3D12_CPU_DESCRIPTOR_HANDLE{};
    decltype(auto) dxView = view.Downcast<plDXView>();
    return dxView->GetHandle();
  };
  for (plUInt32 slot = 0; slot < rtvs.size(); ++slot)
  {
    omRtv[slot] = getHandle(rtvs[slot]);
    if (dxRenderPass->GetDesc().colors[slot].loadOp == plRHIRenderPassLoadOp::kClear)
      m_CommandList->ClearRenderTargetView(omRtv[slot], &clearDesc.colors[slot].a, 0, nullptr);
  }
  while (!omRtv.empty() && omRtv.back().ptr == 0)
  {
    omRtv.pop_back();
  }
  D3D12_CPU_DESCRIPTOR_HANDLE omDsv = getHandle(dsv);
  D3D12_CLEAR_FLAGS clearFlags = {};
  if (dxRenderPass->GetDesc().depthStencil.depthLoadOp == plRHIRenderPassLoadOp::kClear)
    clearFlags |= D3D12_CLEAR_FLAG_DEPTH;
  if (dxRenderPass->GetDesc().depthStencil.stencilLoadOp == plRHIRenderPassLoadOp::kClear)
    clearFlags |= D3D12_CLEAR_FLAG_STENCIL;
  if (omDsv.ptr && clearFlags)
    m_CommandList->ClearDepthStencilView(omDsv, clearFlags, clearDesc.depth, clearDesc.stencil, 0, nullptr);
  m_CommandList->OMSetRenderTargets(static_cast<plUInt32>(omRtv.size()), omRtv.data(), FALSE, omDsv.ptr ? &omDsv : nullptr);
}

void plDXCommandList::EndRenderPass()
{
  if (!m_Device.IsRenderPassesSupported())
  {
    return;
  }
  m_CommandList4->EndRenderPass();
}

void plDXCommandList::BeginEvent(const plString& name)
{
  if (!m_Device.IsUnderGraphicsDebugger())
    return;
  PIXBeginEvent(m_CommandList.Get(), 0, plStringWChar(name).GetData());
}

void plDXCommandList::EndEvent()
{
  if (!m_Device.IsUnderGraphicsDebugger())
    return;
  PIXEndEvent(m_CommandList.Get());
}

void plDXCommandList::Draw(plUInt32 vertexCount, plUInt32 instanceCount, plUInt32 firstVertex, plUInt32 firstInstance)
{
  m_CommandList->DrawInstanced(vertexCount, instanceCount, firstVertex, firstInstance);
}

void plDXCommandList::DrawIndexed(plUInt32 indexCount, plUInt32 instanceCount, plUInt32 firstIndex, plInt32 vertexOffset, plUInt32 firstInstance)
{
  m_CommandList->DrawIndexedInstanced(indexCount, instanceCount, firstIndex, vertexOffset, firstInstance);
}

void plDXCommandList::ExecuteIndirect(
  D3D12_INDIRECT_ARGUMENT_TYPE type,
  const plSharedPtr<plRHIResource>& pArgumentBuffer,
  plUInt64 argumentBufferOffset,
  const plSharedPtr<plRHIResource>& countBuffer,
  plUInt64 countBufferOffset,
  plUInt32 maxDrawCount,
  plUInt32 stride)
{
  decltype(auto) dxArgumentBuffer = pArgumentBuffer.Downcast<plDXResource>();
  ID3D12Resource* dxCountBuffer = nullptr;
  if (countBuffer)
  {
    dxCountBuffer = countBuffer.Downcast<plDXResource>()->resource.Get();
  }
  else
  {
    assert(countBufferOffset == 0);
  }
  m_CommandList->ExecuteIndirect(
    m_Device.GetCommandSignature(type, stride),
    maxDrawCount,
    dxArgumentBuffer->resource.Get(),
    argumentBufferOffset,
    dxCountBuffer,
    countBufferOffset);
}

void plDXCommandList::DrawIndirect(const plSharedPtr<plRHIResource>& pArgumentBuffer, plUInt64 argumentBufferOffset)
{
  DrawIndirectCount(pArgumentBuffer, argumentBufferOffset, {}, 0, 1, sizeof(plRHIDrawIndirectCommand));
}

void plDXCommandList::DrawIndexedIndirect(const plSharedPtr<plRHIResource>& pArgumentBuffer, plUInt64 argumentBufferOffset)
{
  DrawIndexedIndirectCount(pArgumentBuffer, argumentBufferOffset, {}, 0, 1, sizeof(plRHIDrawIndexedIndirectCommand));
}

void plDXCommandList::DrawIndirectCount(
  const plSharedPtr<plRHIResource>& pArgumentBuffer,
  plUInt64 argumentBufferOffset,
  const plSharedPtr<plRHIResource>& countBuffer,
  plUInt64 countBufferOffset,
  plUInt32 maxDrawCount,
  plUInt32 stride)
{
  ExecuteIndirect(
    D3D12_INDIRECT_ARGUMENT_TYPE_DRAW,
    pArgumentBuffer,
    argumentBufferOffset,
    countBuffer,
    countBufferOffset,
    maxDrawCount,
    stride);
}

void plDXCommandList::DrawIndexedIndirectCount(
  const plSharedPtr<plRHIResource>& pArgumentBuffer,
  plUInt64 argumentBufferOffset,
  const plSharedPtr<plRHIResource>& countBuffer,
  plUInt64 countBufferOffset,
  plUInt32 maxDrawCount,
  plUInt32 stride)
{
  ExecuteIndirect(
    D3D12_INDIRECT_ARGUMENT_TYPE_DRAW_INDEXED,
    pArgumentBuffer,
    argumentBufferOffset,
    countBuffer,
    countBufferOffset,
    maxDrawCount,
    stride);
}

void plDXCommandList::Dispatch(plUInt32 threadGroupCountX, plUInt32 threadGroupCountY, plUInt32 threadGroupCountZ)
{
  m_CommandList->Dispatch(threadGroupCountX, threadGroupCountY, threadGroupCountZ);
}

void plDXCommandList::DispatchIndirect(const plSharedPtr<plRHIResource>& pArgumentBuffer, plUInt64 argumentBufferOffset)
{
  ExecuteIndirect(
    D3D12_INDIRECT_ARGUMENT_TYPE_DISPATCH,
    pArgumentBuffer,
    argumentBufferOffset,
    {},
    0,
    1,
    sizeof(plRHIDispatchIndirectCommand));
}

void plDXCommandList::DispatchMesh(plUInt32 threadGroupCountX)
{
  m_CommandList6->DispatchMesh(threadGroupCountX, 1, 1);
}

static D3D12_GPU_VIRTUAL_ADDRESS GetVirtualAddress(const plRHIRayTracingShaderTable& table)
{
  if (!table.resource)
  {
    return 0;
  }
  decltype(auto) dxResource = table.resource.Downcast<plDXResource>();
  return dxResource->resource->GetGPUVirtualAddress() + table.offset;
}

void plDXCommandList::DispatchRays(const plRHIRayTracingShaderTables& shaderTables, plUInt32 width, plUInt32 height, plUInt32 depth)
{
  D3D12_DISPATCH_RAYS_DESC dispatchRaysDesc = {};

  dispatchRaysDesc.RayGenerationShaderRecord.StartAddress = GetVirtualAddress(shaderTables.raygen);
  dispatchRaysDesc.RayGenerationShaderRecord.SizeInBytes = shaderTables.raygen.size;

  dispatchRaysDesc.MissShaderTable.StartAddress = GetVirtualAddress(shaderTables.miss);
  dispatchRaysDesc.MissShaderTable.SizeInBytes = shaderTables.miss.size;
  dispatchRaysDesc.MissShaderTable.StrideInBytes = shaderTables.miss.stride;

  dispatchRaysDesc.HitGroupTable.StartAddress = GetVirtualAddress(shaderTables.hit);
  dispatchRaysDesc.HitGroupTable.SizeInBytes = shaderTables.hit.size;
  dispatchRaysDesc.HitGroupTable.StrideInBytes = shaderTables.hit.stride;

  dispatchRaysDesc.CallableShaderTable.StartAddress = GetVirtualAddress(shaderTables.callable);
  dispatchRaysDesc.CallableShaderTable.SizeInBytes = shaderTables.callable.size;
  dispatchRaysDesc.CallableShaderTable.StrideInBytes = shaderTables.callable.stride;

  dispatchRaysDesc.Width = width;
  dispatchRaysDesc.Height = height;
  dispatchRaysDesc.Depth = depth;

  m_CommandList4->DispatchRays(&dispatchRaysDesc);
}

void plDXCommandList::ResourceBarrier(const std::vector<plRHIResourceBarrierDesc>& barriers)
{
  std::vector<D3D12_RESOURCE_BARRIER> dxBarriers;
  for (const auto& barrier : barriers)
  {
    if (!barrier.resource)
    {
      assert(false);
      continue;
    }
    if (barrier.stateBefore == plRHIResourceState::kRaytracingAccelerationStructure)
      continue;

    decltype(auto) dxResource = barrier.resource.Downcast<plDXResource>();
    D3D12_RESOURCE_STATES dxStateBefore = ConvertState(barrier.stateBefore);
    D3D12_RESOURCE_STATES dxStateAfter = ConvertState(barrier.stateAfter);
    if (dxStateBefore == dxStateAfter)
      continue;

    assert(barrier.baseMipLevel + barrier.levelCount <= dxResource->desc.MipLevels);
    assert(barrier.baseArrayLayer + barrier.layerCount <= dxResource->desc.DepthOrArraySize);

    if (barrier.baseMipLevel == 0 && barrier.levelCount == dxResource->desc.MipLevels &&
        barrier.baseArrayLayer == 0 && barrier.layerCount == dxResource->desc.DepthOrArraySize)
    {
      dxBarriers.emplace_back(CD3DX12_RESOURCE_BARRIER::Transition(dxResource->resource.Get(), dxStateBefore, dxStateAfter));
    }
    else
    {
      for (plUInt32 i = barrier.baseMipLevel; i < barrier.baseMipLevel + barrier.levelCount; ++i)
      {
        for (plUInt32 j = barrier.baseArrayLayer; j < barrier.baseArrayLayer + barrier.layerCount; ++j)
        {
          plUInt32 subresource = i + j * dxResource->desc.MipLevels;
          dxBarriers.emplace_back(CD3DX12_RESOURCE_BARRIER::Transition(dxResource->resource.Get(), dxStateBefore, dxStateAfter, subresource));
        }
      }
    }
  }
  if (!dxBarriers.empty())
    m_CommandList->ResourceBarrier((plUInt32)dxBarriers.size(), dxBarriers.data());
}

void plDXCommandList::UAVResourceBarrier(const plSharedPtr<plRHIResource>& resource)
{
  D3D12_RESOURCE_BARRIER uavBarrier = {};
  uavBarrier.Type = D3D12_RESOURCE_BARRIER_TYPE_UAV;
  if (resource)
  {
    decltype(auto) dxResource = resource.Downcast<plDXResource>();
    uavBarrier.UAV.pResource = dxResource->resource.Get();
  }
  m_CommandList4->ResourceBarrier(1, &uavBarrier);
}

void plDXCommandList::SetViewport(float x, float y, float width, float height)
{
  D3D12_VIEWPORT viewport = {};
  viewport.TopLeftX = x;
  viewport.TopLeftY = y;
  viewport.Width = width;
  viewport.Height = height;
  viewport.MinDepth = 0.0f;
  viewport.MaxDepth = 1.0f;
  m_CommandList->RSSetViewports(1, &viewport);
}

void plDXCommandList::SetScissorRect(plInt32 left, plInt32 top, plUInt32 right, plUInt32 bottom)
{
  D3D12_RECT rect = {(LONG)left, (LONG)top, (LONG)right, (LONG)bottom};
  m_CommandList->RSSetScissorRects(1, &rect);
}

void plDXCommandList::IASetIndexBuffer(const plSharedPtr<plRHIResource>& resource, plRHIResourceFormat::Enum format)
{
  DXGI_FORMAT dxFormat = plDXUtils::ToDXGIFormat(format); // static_cast<DXGI_FORMAT>(gli::dx().translate(format).DXGIFormat.DDS);
  decltype(auto) dxResource = resource.Downcast<plDXResource>();
  D3D12_INDEX_BUFFER_VIEW indexBufferView = {};
  indexBufferView.Format = dxFormat;
  indexBufferView.SizeInBytes = (plUInt32)dxResource->desc.Width;
  indexBufferView.BufferLocation = dxResource->resource->GetGPUVirtualAddress();
  m_CommandList->IASetIndexBuffer(&indexBufferView);
}

void plDXCommandList::IASetVertexBuffer(plUInt32 slot, const plSharedPtr<plRHIResource>& resource)
{
  if (m_State && m_State->GetPipelineType() == plRHIPipelineType::kGraphics)
  {
    decltype(auto) dxState = m_State.Downcast<plDXGraphicsPipeline>();
    auto& strides = dxState->GetStrideMap();
    auto it = strides.Find(slot);
    if (it != end(strides))
      IASetVertexBufferImpl(slot, resource, it.Value());
    else
      IASetVertexBufferImpl(slot, {}, 0);
  }
  m_LazyVertex[slot] = resource;
}

void plDXCommandList::IASetVertexBufferImpl(plUInt32 slot, const plSharedPtr<plRHIResource>& resource, plUInt32 stride)
{
  D3D12_VERTEX_BUFFER_VIEW vertexBufferView = {};
  if (resource)
  {
    decltype(auto) dxResource = resource.Downcast<plDXResource>();
    vertexBufferView.BufferLocation = dxResource->resource->GetGPUVirtualAddress();
    vertexBufferView.SizeInBytes = (plUInt32)dxResource->desc.Width;
    vertexBufferView.StrideInBytes = stride;
  }
  m_CommandList->IASetVertexBuffers(slot, 1, &vertexBufferView);
}

void plDXCommandList::RSSetShadingRate(plRHIShadingRate shadingRate, const std::array<plRHIShadingRateCombiner, 2>& combiners)
{
  m_CommandList5->RSSetShadingRate(static_cast<D3D12_SHADING_RATE>(shadingRate), reinterpret_cast<const D3D12_SHADING_RATE_COMBINER*>(combiners.data()));
}

void plDXCommandList::BuildAccelerationStructure(D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_INPUTS& inputs, const plSharedPtr<plRHIResource>& src, const plSharedPtr<plRHIResource>& dst, const plSharedPtr<plRHIResource>& scratch, plUInt64 scratchOffset)
{
  decltype(auto) dxDst = dst.Downcast<plDXResource>();
  decltype(auto) dxScratch = scratch.Downcast<plDXResource>();

  D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_DESC accelerationStructureDesc = {};
  accelerationStructureDesc.Inputs = inputs;
  if (src)
  {
    decltype(auto) dxSrc = src.Downcast<plDXResource>();
    accelerationStructureDesc.SourceAccelerationStructureData = dxSrc->accelerationStructureHandle;
    accelerationStructureDesc.Inputs.Flags |= D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BUILD_FLAG_PERFORM_UPDATE;
  }
  accelerationStructureDesc.DestAccelerationStructureData = dxDst->accelerationStructureHandle;
  accelerationStructureDesc.ScratchAccelerationStructureData = dxScratch->resource->GetGPUVirtualAddress() + scratchOffset;
  m_CommandList4->BuildRaytracingAccelerationStructure(&accelerationStructureDesc, 0, nullptr);
}

void plDXCommandList::BuildBottomLevelAS(
  const plSharedPtr<plRHIResource>& src,
  const plSharedPtr<plRHIResource>& dst,
  const plSharedPtr<plRHIResource>& scratch,
  plUInt64 scratchOffset,
  const std::vector<plRHIRaytracingGeometryDesc>& descs,
  plRHIBuildAccelerationStructureFlags flags)
{
  std::vector<D3D12_RAYTRACING_GEOMETRY_DESC> geometryDescs;
  for (const auto& desc : descs)
  {
    geometryDescs.emplace_back(FillRaytracingGeometryDesc(desc.vertex, desc.index, desc.flags));
  }
  D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_INPUTS inputs = {};
  inputs.Type = D3D12_RAYTRACING_ACCELERATION_STRUCTURE_TYPE_BOTTOM_LEVEL;
  inputs.Flags = Convert(flags);
  inputs.DescsLayout = D3D12_ELEMENTS_LAYOUT_ARRAY;
  inputs.NumDescs = (plUInt32)geometryDescs.size();
  inputs.pGeometryDescs = geometryDescs.data();
  BuildAccelerationStructure(inputs, src, dst, scratch, scratchOffset);
}

void plDXCommandList::BuildTopLevelAS(
  const plSharedPtr<plRHIResource>& src,
  const plSharedPtr<plRHIResource>& dst,
  const plSharedPtr<plRHIResource>& scratch,
  plUInt64 scratchOffset,
  const plSharedPtr<plRHIResource>& instanceData,
  plUInt64 instanceOffset,
  plUInt32 instanceCount,
  plRHIBuildAccelerationStructureFlags flags)
{
  decltype(auto) dxInstanceData = instanceData.Downcast<plDXResource>();
  D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_INPUTS inputs = {};
  inputs.Type = D3D12_RAYTRACING_ACCELERATION_STRUCTURE_TYPE_TOP_LEVEL;
  inputs.Flags = Convert(flags);
  inputs.DescsLayout = D3D12_ELEMENTS_LAYOUT_ARRAY;
  inputs.NumDescs = instanceCount;
  inputs.InstanceDescs = dxInstanceData->resource->GetGPUVirtualAddress() + instanceOffset;
  BuildAccelerationStructure(inputs, src, dst, scratch, scratchOffset);
}

void plDXCommandList::CopyAccelerationStructure(const plSharedPtr<plRHIResource>& src, const plSharedPtr<plRHIResource>& dst, plRHICopyAccelerationStructureMode mode)
{
  decltype(auto) dxSrc = src.Downcast<plDXResource>();
  decltype(auto) dxDst = dst.Downcast<plDXResource>();
  D3D12_RAYTRACING_ACCELERATION_STRUCTURE_COPY_MODE dxMode = {};
  switch (mode)
  {
    case plRHICopyAccelerationStructureMode::kClone:
      dxMode = D3D12_RAYTRACING_ACCELERATION_STRUCTURE_COPY_MODE_CLONE;
      break;
    case plRHICopyAccelerationStructureMode::kCompact:
      dxMode = D3D12_RAYTRACING_ACCELERATION_STRUCTURE_COPY_MODE_COMPACT;
      break;
    default:
      assert(false);
  }
  m_CommandList4->CopyRaytracingAccelerationStructure(
    dxDst->accelerationStructureHandle,
    dxSrc->accelerationStructureHandle,
    dxMode);
}

void plDXCommandList::CopyBuffer(const plSharedPtr<plRHIResource>& srcBuffer, const plSharedPtr<plRHIResource>& dstBuffer,
  const std::vector<plRHIBufferCopyRegion>& regions)
{
  decltype(auto) dxSrcBuffer = srcBuffer.Downcast<plDXResource>();
  decltype(auto) dxDstBuffer = dstBuffer.Downcast<plDXResource>();
  for (const auto& region : regions)
  {
    m_CommandList->CopyBufferRegion(dxDstBuffer->resource.Get(), region.dstOffset, dxSrcBuffer->resource.Get(), region.srcOffset, region.numBytes);
  }
}

void plDXCommandList::CopyBufferToTexture(const plSharedPtr<plRHIResource>& srcBuffer, const plSharedPtr<plRHIResource>& dstTexture,
  const std::vector<plRHIBufferToTextureCopyRegion>& regions)
{
  decltype(auto) dxSrcBuffer = srcBuffer.Downcast<plDXResource>();
  decltype(auto) dxDstTexture = dstTexture.Downcast<plDXResource>();
  auto format = dstTexture->GetFormat();
  DXGI_FORMAT dxFormat = plDXUtils::ToDXGIFormat(format); // static_cast<DXGI_FORMAT>(gli::dx().translate(format).DXGIFormat.DDS);
  for (const auto& region : regions)
  {
    D3D12_TEXTURE_COPY_LOCATION dst = {};
    dst.pResource = dxDstTexture->resource.Get();
    dst.Type = D3D12_TEXTURE_COPY_TYPE_SUBRESOURCE_INDEX;
    dst.SubresourceIndex = region.textureArrayLayer * dxDstTexture->GetLevelCount() + region.textureMipLevel;

    D3D12_TEXTURE_COPY_LOCATION src = {};
    src.pResource = dxSrcBuffer->resource.Get();
    src.Type = D3D12_TEXTURE_COPY_TYPE_PLACED_FOOTPRINT;
    src.PlacedFootprint.Offset = region.bufferOffset;
    src.PlacedFootprint.Footprint.Width = region.textureExtent.width;
    src.PlacedFootprint.Footprint.Height = region.textureExtent.height;
    src.PlacedFootprint.Footprint.Depth = region.textureExtent.depth;
    if (plRHIResourceFormat::IsFormatBlockCompressed(format))
    {
      auto extent = plRHIResourceFormat::GetBlockExtent(format);
      src.PlacedFootprint.Footprint.Width = std::max<plUInt32>(extent.x, src.PlacedFootprint.Footprint.Width);
      src.PlacedFootprint.Footprint.Height = std::max<plUInt32>(extent.y, src.PlacedFootprint.Footprint.Height);
      src.PlacedFootprint.Footprint.Depth = std::max<plUInt32>(extent.z, src.PlacedFootprint.Footprint.Depth);
    }
    src.PlacedFootprint.Footprint.RowPitch = region.bufferRowPitch;
    src.PlacedFootprint.Footprint.Format = dxFormat;

    m_CommandList->CopyTextureRegion(&dst, region.textureOffset.x, region.textureOffset.y, region.textureOffset.z, &src, nullptr);
  }
}

void plDXCommandList::CopyTexture(const plSharedPtr<plRHIResource>& srcTexture, const plSharedPtr<plRHIResource>& dstTexture,
  const std::vector<plRHITextureCopyRegion>& regions)
{
  decltype(auto) dxSrcTexture = srcTexture.Downcast<plDXResource>();
  decltype(auto) dxDstTexture = dstTexture.Downcast<plDXResource>();
  for (const auto& region : regions)
  {
    D3D12_TEXTURE_COPY_LOCATION dst = {};
    dst.pResource = dxDstTexture->resource.Get();
    dst.Type = D3D12_TEXTURE_COPY_TYPE_SUBRESOURCE_INDEX;
    dst.SubresourceIndex = region.dstArrayLayer * dxDstTexture->GetLevelCount() + region.dstMipLevel;

    D3D12_TEXTURE_COPY_LOCATION src = {};
    src.pResource = dxSrcTexture->resource.Get();
    src.Type = D3D12_TEXTURE_COPY_TYPE_SUBRESOURCE_INDEX;
    src.SubresourceIndex = region.srcArrayLayer * dxSrcTexture->GetLevelCount() + region.srcMipLevel;

    D3D12_BOX srcBox = {};
    srcBox.left = region.srcOffset.x;
    srcBox.top = region.srcOffset.y;
    srcBox.front = region.srcOffset.z;
    srcBox.right = region.srcOffset.x + region.extent.width;
    srcBox.bottom = region.srcOffset.y + region.extent.height;
    srcBox.back = region.srcOffset.z + region.extent.depth;

    m_CommandList->CopyTextureRegion(&dst, region.dstOffset.x, region.dstOffset.y, region.dstOffset.z, &src, &srcBox);
  }
}

void plDXCommandList::WriteAccelerationStructuresProperties(
  const std::vector<plSharedPtr<plRHIResource>>& accelerationStructures,
  const plSharedPtr<plRHIQueryHeap>& queryHeap,
  plUInt32 firstQuery)
{
  if (queryHeap->GetType() != plRHIQueryHeapType::kAccelerationStructureCompactedSize)
  {
    assert(false);
    return;
  }
  decltype(auto) dxQueryHeap = queryHeap.Downcast<plDXRayTracingQueryHeap>();
  D3D12_RAYTRACING_ACCELERATION_STRUCTURE_POSTBUILD_INFO_DESC desc = {};
  desc.InfoType = D3D12_RAYTRACING_ACCELERATION_STRUCTURE_POSTBUILD_INFO_COMPACTED_SIZE;
  desc.DestBuffer = dxQueryHeap->GetResource()->GetGPUVirtualAddress() + firstQuery * sizeof(plUInt64);
  std::vector<D3D12_GPU_VIRTUAL_ADDRESS> dxAccelerationStructures;
  dxAccelerationStructures.reserve(accelerationStructures.size());
  for (const auto& accelerationStructure : accelerationStructures)
  {
    dxAccelerationStructures.emplace_back(accelerationStructure.Downcast<plDXResource>()->accelerationStructureHandle);
  }
  m_CommandList4->EmitRaytracingAccelerationStructurePostbuildInfo(&desc, (plUInt32)dxAccelerationStructures.size(), dxAccelerationStructures.data());
}

void plDXCommandList::ResolveQueryData(
  const plSharedPtr<plRHIQueryHeap>& queryHeap,
  plUInt32 firstQuery,
  plUInt32 queryCount,
  const plSharedPtr<plRHIResource>& dstBuffer,
  plUInt64 dstOffset)
{
  if (queryHeap->GetType() != plRHIQueryHeapType::kAccelerationStructureCompactedSize)
  {
    assert(false);
    return;
  }

  decltype(auto) dxQueryHeap = queryHeap.Downcast<plDXRayTracingQueryHeap>();
  decltype(auto) dxDstBuffer = dstBuffer.Downcast<plDXResource>();

  auto barrier1 = CD3DX12_RESOURCE_BARRIER::Transition(dxQueryHeap->GetResource().Get(), D3D12_RESOURCE_STATE_UNORDERED_ACCESS, D3D12_RESOURCE_STATE_COPY_SOURCE, 0);
  m_CommandList->ResourceBarrier(1, &barrier1);
  m_CommandList->CopyBufferRegion(
    dxDstBuffer->resource.Get(),
    dstOffset,
    dxQueryHeap->GetResource().Get(),
    firstQuery * sizeof(plUInt64),
    queryCount * sizeof(plUInt64));

  auto barrier2 = CD3DX12_RESOURCE_BARRIER::Transition(dxQueryHeap->GetResource().Get(), D3D12_RESOURCE_STATE_COPY_SOURCE, D3D12_RESOURCE_STATE_UNORDERED_ACCESS, 0);
  m_CommandList->ResourceBarrier(1, &barrier2);
}

ComPtr<ID3D12GraphicsCommandList> plDXCommandList::GetCommandList()
{
  return m_CommandList;
}
