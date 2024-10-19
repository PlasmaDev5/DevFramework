#pragma once
#include <RHI/BindingSetLayout/BindingSetLayout.h>
#include <RHIDX12/Program/DXProgram.h>
#include <directx/d3d12.h>
#include <wrl.h>
using namespace Microsoft::WRL;

class plDXDevice;

struct plBindingLayout
{
  D3D12_DESCRIPTOR_HEAP_TYPE heapType;
  plUInt32 heapOffset;
};

struct plDescriptorTableDesc
{
  D3D12_DESCRIPTOR_HEAP_TYPE heapType;
  plUInt32 heapOffset;
  bool bindless;
  bool isCompute;
};

class plDXBindingSetLayout
  : public plRHIBindingSetLayout
{
public:
  plDXBindingSetLayout(plDXDevice& device, const std::vector<plRHIBindKey>& descs);

  const plMap<D3D12_DESCRIPTOR_HEAP_TYPE, plUInt32>& GetHeapDescs() const;
  const plMap<plRHIBindKey, plBindingLayout>& GetLayout() const;
  const plMap<plUInt32, plDescriptorTableDesc>& GetDescriptorTables() const;
  const ComPtr<ID3D12RootSignature>& /* DXBindingSetLayout::*/ GetRootSignature() const;

private:
  plDXDevice& m_Device;
  plMap<D3D12_DESCRIPTOR_HEAP_TYPE, plUInt32> m_HeapDescs;
  plMap<plRHIBindKey, plBindingLayout> m_Layout;
  plMap<plUInt32, plDescriptorTableDesc> m_DescriptorTables;
  ComPtr<ID3D12RootSignature> m_RootSignature;
};
