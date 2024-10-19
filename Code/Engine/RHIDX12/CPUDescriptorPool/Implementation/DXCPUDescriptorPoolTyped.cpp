#include <RHIDX12/CPUDescriptorPool/DXCPUDescriptorPoolTyped.h>
#include <RHIDX12/Device/DXDevice.h>
#include <directx/d3dx12.h>

plDXCPUDescriptorPoolTyped::plDXCPUDescriptorPoolTyped(plDXDevice& device, D3D12_DESCRIPTOR_HEAP_TYPE type)
    : m_Device(device)
    , m_Type(type)
    , m_Offset(0)
    , m_Size(0)
{
}

std::shared_ptr<plDXCPUDescriptorHandle> plDXCPUDescriptorPoolTyped::Allocate(size_t count)
{
    if (m_Offset + count > m_Size)
        ResizeHeap(plMath::Max(m_Offset + count, 2 * (m_Size + 1)));
    m_Offset += count;
    return std::make_shared<plDXCPUDescriptorHandle>(m_Device, m_Heap, m_CpuHandle, m_Offset - count, count, m_Device.GetDevice()->GetDescriptorHandleIncrementSize(m_Type), m_Type);
}

void plDXCPUDescriptorPoolTyped::ResizeHeap(size_t reqSize)
{
    if (m_Size >= reqSize)
        return;

    ComPtr<ID3D12DescriptorHeap> heap;
    D3D12_DESCRIPTOR_HEAP_DESC heapDesc = {};
    heapDesc.NumDescriptors = static_cast<plUInt32>(reqSize);
    heapDesc.Type = m_Type;
    PL_ASSERT_ALWAYS(m_Device.GetDevice()->CreateDescriptorHeap(&heapDesc, IID_PPV_ARGS(&heap)) == S_OK, "");
    if (m_Size > 0)
    {
        m_Device.GetDevice()->CopyDescriptorsSimple(
            static_cast<plUInt32>(m_Size),
            heap->GetCPUDescriptorHandleForHeapStart(),
            m_Heap->GetCPUDescriptorHandleForHeapStart(),
            m_Type);
    }

    m_Size = heapDesc.NumDescriptors;
    m_Heap = heap;
    m_CpuHandle = m_Heap->GetCPUDescriptorHandleForHeapStart();
}
