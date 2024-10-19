#pragma once
#include <RHI/View/View.h>
#include <RHIDX12/CPUDescriptorPool/DXCPUDescriptorHandle.h>
#include <RHIDX12/GPUDescriptorPool/DXGPUDescriptorPoolRange.h>
#include <RHIDX12/Resource/DXResource.h>

class plDXDevice;
class DXResource;

class plDXView : public plRHIView
{
public:
  plDXView(plDXDevice& device, const plSharedPtr<plDXResource>& resource, const plRHIViewDesc& viewDesc);
  plSharedPtr<plRHIResource> GetResource() override;
  plUInt32 GetDescriptorId() const override;
  plUInt32 GetBaseMipLevel() const override;
  plUInt32 GetLevelCount() const override;
  plUInt32 GetBaseArrayLayer() const override;
  plUInt32 GetLayerCount() const override;

  D3D12_CPU_DESCRIPTOR_HANDLE GetHandle();

private:
  void CreateView();
  void CreateSRV();
  void CreateRAS();
  void CreateUAV();
  void CreateRTV();
  void CreateDSV();
  void CreateCBV();
  void CreateSampler();

  plDXDevice& m_Device;
  plSharedPtr<plDXResource> m_Resource;
  plRHIViewDesc m_ViewDesc;
  std::shared_ptr<plDXCPUDescriptorHandle> m_Handle;
  std::shared_ptr<plDXGPUDescriptorPoolRange> m_Range;
};
