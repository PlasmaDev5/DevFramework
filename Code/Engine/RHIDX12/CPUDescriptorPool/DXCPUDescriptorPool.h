#pragma once
#include <RHIDX12/CPUDescriptorPool/DXCPUDescriptorHandle.h>
#include <RHIDX12/CPUDescriptorPool/DXCPUDescriptorPoolTyped.h>
#include <RHI/Instance/BaseTypes.h>
#include <RHIDX12/Utilities/DXUtility.h>
#include <algorithm>
#include <memory>
#include <wrl.h>
#include <directx/d3d12.h>
using namespace Microsoft::WRL;

class plDXDevice;

class plDXCPUDescriptorPool
{
public:
    plDXCPUDescriptorPool(plDXDevice& device);
    std::shared_ptr<plDXCPUDescriptorHandle> AllocateDescriptor(plRHIViewType viewType);

private:
    plDXCPUDescriptorPoolTyped& SelectHeap(plRHIViewType viewType);

    plDXDevice& m_Device;
    plDXCPUDescriptorPoolTyped m_Resource;
    plDXCPUDescriptorPoolTyped m_Sampler;
    plDXCPUDescriptorPoolTyped m_Rtv;
    plDXCPUDescriptorPoolTyped m_Dsv;
};
