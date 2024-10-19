#include <RHIDX12/CPUDescriptorPool/DXCPUDescriptorPool.h>
#include <RHIDX12/Device/DXDevice.h>
#include <directx/d3dx12.h>

plDXCPUDescriptorPool::plDXCPUDescriptorPool(plDXDevice& device)
    : m_Device(device)
    , m_Resource(m_Device, D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV)
    , m_Sampler(m_Device, D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER)
    , m_Rtv(m_Device, D3D12_DESCRIPTOR_HEAP_TYPE_RTV)
    , m_Dsv(m_Device, D3D12_DESCRIPTOR_HEAP_TYPE_DSV)
{
}

std::shared_ptr<plDXCPUDescriptorHandle> plDXCPUDescriptorPool::AllocateDescriptor(plRHIViewType viewType)
{
    plDXCPUDescriptorPoolTyped& pool = SelectHeap(viewType);
    return pool.Allocate(1);
}

plDXCPUDescriptorPoolTyped& plDXCPUDescriptorPool::SelectHeap(plRHIViewType viewType)
{
    switch (viewType)
    {
      case plRHIViewType::kAccelerationStructure:
      case plRHIViewType::kConstantBuffer:
      case plRHIViewType::kTexture:
      case plRHIViewType::kRWTexture:
      case plRHIViewType::kBuffer:
      case plRHIViewType::kRWBuffer:
      case plRHIViewType::kStructuredBuffer:
      case plRHIViewType::kRWStructuredBuffer:
        return m_Resource;
      case plRHIViewType::kSampler:
        return m_Sampler;
      case plRHIViewType::kRenderTarget:
        return m_Rtv;
      case plRHIViewType::kDepthStencil:
        return m_Dsv;
    default:
        throw "fatal failure";
    }
}
