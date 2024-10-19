#pragma once

#include <memory>
#include <Foundation/Application/Application.h>
#include <Foundation/Types/UniquePtr.h>

#include <RHI/Device/Device.h>
#include <RHI/Swapchain/Swapchain.h>
#include <RHI/Fence/Fence.h>
#include <RHI/Instance/Instance.h>

class plTestingAppWindow;

constexpr plUInt32 FRAME_COUNT = 3;

class plTestingApp : public plApplication
{
public:
  typedef plApplication SUPER;

  plTestingApp();

  virtual Execution Run() override;

  virtual void AfterCoreSystemsStartup() override;

  virtual void BeforeHighLevelSystemsShutdown() override;

  void OnResize(plUInt32 width, plUInt32 height);

private:
  plSharedPtr<plRHIInstance> m_pInstance;
  plSharedPtr<plRHIAdapter> m_pAdapter;
  plSharedPtr<plRHIDevice> m_pDevice;
  plTestingAppWindow* m_pWindow = nullptr;
  plSharedPtr<plRHISwapchain> m_pSwapchain = nullptr;
  plSharedPtr<plRHIFence> m_pFence = nullptr;
  plUInt64 m_FenceValue = 0;
  plSharedPtr<plRHICommandQueue> m_pCommandQueue = nullptr;
  std::array<plUInt64, FRAME_COUNT> m_FenceValues = {};
  //std::vector<std::shared_ptr<plRHICommandList>> command_lists;
  //std::vector<plSharedPtr<plRHIFramebuffer>> framebuffers;
  plSharedPtr<plRHIPipeline> m_pPipeline;
  plSharedPtr<plRHIRenderPass> m_pRenderPass;
  plSharedPtr<plRHIProgram> m_pProgram;
  plSharedPtr<plRHIView> m_pConstantBufferView;
  plSharedPtr<plRHIResource> m_pConstantBuffer;
  plSharedPtr<plRHIResource> m_pIndexBuffer;
  plSharedPtr<plRHIResource> m_pVertexBuffer;
  plSharedPtr<plRHIShader> m_pVertexShader;
  plSharedPtr<plRHIShader> m_pPixelShader;
  plSharedPtr<plRHIBindingSetLayout> m_pLayout;
  plSharedPtr<plRHIBindingSet> m_pBindingSet;
  plUInt32 m_FrameIndex = 0;
  plRHIRenderDeviceDesc renderDeviceDesc;
};
