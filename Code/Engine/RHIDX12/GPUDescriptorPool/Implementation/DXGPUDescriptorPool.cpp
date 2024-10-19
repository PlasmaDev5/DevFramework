#include <RHIDX12/GPUDescriptorPool/DXGPUDescriptorPool.h>
#include <RHIDX12/Device/DXDevice.h>
#include <directx/d3dx12.h>
#include <stdexcept>

plDXGPUDescriptorPool::plDXGPUDescriptorPool(plDXDevice& device)
    : m_Device(device)
    , m_ShaderResource(device, D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV)
    , m_ShaderSampler(device, D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER)
{
}

plDXGPUDescriptorPoolRange plDXGPUDescriptorPool::Allocate(D3D12_DESCRIPTOR_HEAP_TYPE descriptorType, plUInt32 count)
{
    switch (descriptorType)
    {
    case D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV:
        return m_ShaderResource.Allocate(count);
    case D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER:
        return m_ShaderSampler.Allocate(count);
    default:
        throw std::runtime_error("wrong descriptor type");
    }
}

ComPtr<ID3D12DescriptorHeap> plDXGPUDescriptorPool::GetHeap(D3D12_DESCRIPTOR_HEAP_TYPE descriptorType)
{
    switch (descriptorType)
    {
    case D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV:
        return m_ShaderResource.GetHeap();
    case D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER:
        return m_ShaderSampler.GetHeap();
    default:
        throw std::runtime_error("wrong descriptor type");
    }
}
