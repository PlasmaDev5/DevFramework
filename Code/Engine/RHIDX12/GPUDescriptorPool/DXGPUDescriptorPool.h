#pragma once
#include <RHI/Instance/BaseTypes.h>
#include <RHIDX12/GPUDescriptorPool/DXGPUDescriptorPoolRange.h>
#include <RHIDX12/GPUDescriptorPool/DXGPUDescriptorPoolTyped.h>
#include <RHIDX12/Utilities/DXUtility.h>
#include <algorithm>
#include <memory>
#include <wrl.h>
#include <directx/d3d12.h>
using namespace Microsoft::WRL;

class plDXDevice;

class plDXGPUDescriptorPool
{
public:
  plDXGPUDescriptorPool(plDXDevice& device);
  plDXGPUDescriptorPoolRange Allocate(D3D12_DESCRIPTOR_HEAP_TYPE descriptorType, plUInt32 count);
  ComPtr<ID3D12DescriptorHeap> GetHeap(D3D12_DESCRIPTOR_HEAP_TYPE descriptorType);

private:
  plDXDevice& m_Device;
  plDXGPUDescriptorPoolTyped m_ShaderResource;
  plDXGPUDescriptorPoolTyped m_ShaderSampler;
};
