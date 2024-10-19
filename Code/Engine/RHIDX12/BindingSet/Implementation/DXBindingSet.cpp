#include <RHIDX12/BindingSet/DXBindingSet.h>
#include <RHIDX12/BindingSetLayout/DXBindingSetLayout.h>
#include <RHIDX12/Device/DXDevice.h>
#include <RHIDX12/GPUDescriptorPool/DXGPUDescriptorPoolRange.h>
#include <RHIDX12/Program/DXProgram.h>
#include <RHIDX12/View/DXView.h>

plDXBindingSet::plDXBindingSet(plDXDevice& device, const plSharedPtr<plDXBindingSetLayout>& layout)
  : m_Device(device)
  , m_Layout(layout)
{
  for (const auto& desc : m_Layout->GetHeapDescs())
  {
    plSharedPtr<plDXGPUDescriptorPoolRange> heapRange = PL_DEFAULT_NEW(plDXGPUDescriptorPoolRange, m_Device.GetGPUDescriptorPool().Allocate(desc.Key(), desc.Value()));
    m_DescriptorRanges.Insert(desc.Key(), heapRange);
  }
}

void plDXBindingSet::WriteBindings(const std::vector<plRHIBindingDesc>& bindings)
{
  for (const auto& binding : bindings)
  {
    if (!binding.view)
    {
      continue;
    }
    const plBindingLayout& bindingLayout = *m_Layout->GetLayout().GetValue(binding.bindKey);
    plSharedPtr<plDXGPUDescriptorPoolRange> heapRange = m_DescriptorRanges[bindingLayout.heapType];
    decltype(auto) srcCPUHandle = binding.view.Downcast<plDXView>()->GetHandle();
    heapRange->CopyCpuHandle(bindingLayout.heapOffset, srcCPUHandle);
  }
}

void SetRootDescriptorTable(const ComPtr<ID3D12GraphicsCommandList>& commandList, plUInt32 rootParameter, bool isCompute, D3D12_GPU_DESCRIPTOR_HANDLE baseDescriptor)
{
  if (isCompute)
  {
    commandList->SetComputeRootDescriptorTable(rootParameter, baseDescriptor);
  }
  else
  {
    commandList->SetGraphicsRootDescriptorTable(rootParameter, baseDescriptor);
  }
}

plDynamicArray<ComPtr<ID3D12DescriptorHeap>> plDXBindingSet::Apply(const ComPtr<ID3D12GraphicsCommandList>& commandList)
{
  plMap<D3D12_DESCRIPTOR_HEAP_TYPE, ComPtr<ID3D12DescriptorHeap>> heapMap;
  for (const auto& table : m_Layout->GetDescriptorTables())
  {
    D3D12_DESCRIPTOR_HEAP_TYPE heapType = table.Value().heapType;
    ComPtr<ID3D12DescriptorHeap> heap;
    if (table.Value().bindless)
    {
      heap = m_Device.GetGPUDescriptorPool().GetHeap(heapType);
    }
    else
    {
      heap = m_DescriptorRanges[heapType]->GetHeap();
    }

    auto it = heapMap.Find(heapType);
    if (it == end(heapMap))
    {
      heapMap.Insert(heapType, m_Device.GetGPUDescriptorPool().GetHeap(heapType));
    }
    else
    {
      PL_ASSERT_ALWAYS(it.Value() == heap, "");
    }
  }

  plDynamicArray<ComPtr<ID3D12DescriptorHeap>> descriptorHeaps;
  plDynamicArray<ID3D12DescriptorHeap*> descriptorHeapsPtr;
  for (const auto& heap : heapMap)
  {
    descriptorHeaps.PushBack(heap.Value());
    descriptorHeapsPtr.PushBack(heap.Value().Get());
  }
  if (descriptorHeapsPtr.GetCount())
  {
    commandList->SetDescriptorHeaps(descriptorHeapsPtr.GetCount(), descriptorHeapsPtr.GetData());
  }

  for (const auto& table : m_Layout->GetDescriptorTables())
  {
    D3D12_DESCRIPTOR_HEAP_TYPE heapType = table.Value().heapType;
    D3D12_GPU_DESCRIPTOR_HANDLE baseDescriptor = {};
    if (table.Value().bindless)
    {
      baseDescriptor = m_Device.GetGPUDescriptorPool().GetHeap(heapType)->GetGPUDescriptorHandleForHeapStart();
    }
    else
    {
      decltype(auto) heapRange = m_DescriptorRanges[heapType];
      baseDescriptor = heapRange->GetGpuHandle(table.Value().heapOffset);
    }
    SetRootDescriptorTable(commandList, table.Key(), table.Value().isCompute, baseDescriptor);
  }
  return descriptorHeaps;
}
