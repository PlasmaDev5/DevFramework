#pragma once
#include <RHI/BindingSet/BindingSet.h>
#include <RHIDX12/Program/DXProgram.h>
#include <directx/d3d12.h>
#include <wrl.h>
using namespace Microsoft::WRL;

class plDXDevice;
class plDXBindingSetLayout;
class plDXGPUDescriptorPoolRange;

class plDXBindingSet
  : public plRHIBindingSet
{
public:
  plDXBindingSet(plDXDevice& device, const plSharedPtr<plDXBindingSetLayout>& layout);

  void WriteBindings(const std::vector<plRHIBindingDesc>& bindings) override;

  plDynamicArray<ComPtr<ID3D12DescriptorHeap>> Apply(const ComPtr<ID3D12GraphicsCommandList>& commandList);

private:
  plDXDevice& m_Device;
  plSharedPtr<plDXBindingSetLayout> m_Layout;
  plMap<D3D12_DESCRIPTOR_HEAP_TYPE, plSharedPtr<plDXGPUDescriptorPoolRange>> m_DescriptorRanges;
};
