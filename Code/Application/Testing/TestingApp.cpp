#include "RHI/Instance/BaseTypes.h"

#include <Core/Input/InputManager.h>
#include <Core/ResourceManager/ResourceManager.h>
#include <Foundation/Configuration/Startup.h>
#include <Foundation/IO/FileSystem/FileSystem.h>
#include <Foundation/Logging/ConsoleWriter.h>
#include <Foundation/Logging/Log.h>
#include <Foundation/Logging/VisualStudioWriter.h>
#include <Foundation/Time/Clock.h>
#include <Testing/TestingApp.h>

#include <Core/System/Window.h>

#include <RHIShaderCompilerHLSL/Compiler.h>
#include <RHIShaderCompilerHLSL/ShaderReflection.h>

static plUInt32 g_uiWindowWidth = 640;
static plUInt32 g_uiWindowHeight = 480;

class plTestingAppWindow : public plWindow
{
public:
    plTestingAppWindow(plTestingApp* pApp)
    : plWindow()
  {
    m_pApp = pApp;
    m_bCloseRequested = false;
  }

  virtual void OnClickClose() override { m_bCloseRequested = true; }

  virtual void OnResize(const plSizeU32& newWindowSize) override
  {
    if (m_pApp)
    {
      m_CreationDescription.m_Resolution = newWindowSize;
      m_pApp->OnResize(m_CreationDescription.m_Resolution.width, m_CreationDescription.m_Resolution.height);
    }
  }

  bool m_bCloseRequested;

private:
  plTestingApp* m_pApp = nullptr;
};

plTestingApp::plTestingApp()
  : plApplication("Render Window")
{
}

void plTestingApp::AfterCoreSystemsStartup()
{
  plStringBuilder sProjectDir = ">sdk/Data/Applications/Testing";
  plStringBuilder sProjectDirResolved;
  plFileSystem::ResolveSpecialDirectory(sProjectDir, sProjectDirResolved).IgnoreResult();

  plFileSystem::SetSpecialDirectory("project", sProjectDirResolved);

  plFileSystem::AddDataDirectory("", "", ":", plDataDirUsage::AllowWrites).IgnoreResult();
  plFileSystem::AddDataDirectory(">appdir/", "AppBin", "bin", plDataDirUsage::AllowWrites).IgnoreResult();                              // writing to the binary directory
  plFileSystem::AddDataDirectory(">appdir/", "ShaderCache", "shadercache", plDataDirUsage::AllowWrites).IgnoreResult();                 // for shader files
  plFileSystem::AddDataDirectory(">user/ZephyrEngine Project/Testing", "AppData", "appdata", plDataDirUsage::AllowWrites).IgnoreResult(); // app user data

  plFileSystem::AddDataDirectory(">sdk/Data/Base", "Base", "base").IgnoreResult();
  plFileSystem::AddDataDirectory(">project/", "Project", "project", plDataDirUsage::AllowWrites).IgnoreResult();

  plGlobalLog::AddLogWriter(plLogWriter::Console::LogMessageHandler);
  plGlobalLog::AddLogWriter(plLogWriter::VisualStudio::LogMessageHandler);

  // Register Input
  {
    plInputActionConfig cfg;

    cfg = plInputManager::GetInputActionConfig("Main", "CloseApp");
    cfg.m_sInputSlotTrigger[0] = plInputSlot_KeyEscape;
    plInputManager::SetInputActionConfig("Main", "CloseApp", cfg, true);
  }

  plRHIShaderBlobType shaderBlobType;
  plRHIApiType apiType;

#if PL_ENABLED(PL_PLATFORM_WINDOWS)
  shaderBlobType = plRHIShaderBlobType::kDXIL;
  apiType = plRHIApiType::kDX12;
#elif PL_ENABLED(PL_PLATFORM_OSX)
  shaderBlobType = plRHIShaderBlobType::kSPIRV;
  apiType = plRHIApiType::kMetal;
#else
  shaderBlobType = plRHIShaderBlobType::kSPIRV;
  apiType = plRHIApiType::kVulkan;
#endif

  // No need to make metal an option because on OSX we only support metal for rendering.
  plStringView szRendererName = plCommandLineUtils::GetGlobalInstance()->GetStringOption("-rhi", 0, "");
  {
    if (szRendererName.Compare("D3D12") == 0)
    {
      shaderBlobType = plRHIShaderBlobType::kDXIL;
      apiType = plRHIApiType::kDX12;
    }

    if (szRendererName.Compare("Vulkan") == 0)
    {
      shaderBlobType = plRHIShaderBlobType::kSPIRV;
      apiType = plRHIApiType::kVulkan;
    }
  }
  
  // Create a window for rendering
  {
    plWindowCreationDesc WindowCreationDesc;
    WindowCreationDesc.m_Resolution.width = g_uiWindowWidth;
    WindowCreationDesc.m_Resolution.height = g_uiWindowHeight;
    WindowCreationDesc.m_Title = plStringBuilder("Testing", szRendererName);
    WindowCreationDesc.m_bShowMouseCursor = true;
    WindowCreationDesc.m_bClipMouseCursor = false;
    WindowCreationDesc.m_WindowMode = plWindowMode::WindowResizable;
    m_pWindow = PL_DEFAULT_NEW(plTestingAppWindow, this);
    m_pWindow->Initialize(WindowCreationDesc).IgnoreResult();
  }

  renderDeviceDesc.apiType = apiType;
  renderDeviceDesc.frameCount = FRAME_COUNT;
  renderDeviceDesc.roundFps = true;
  renderDeviceDesc.vsync = true;

  m_pInstance = plRHIInstanceFactory::CreateInstance(renderDeviceDesc.apiType);
  m_pAdapter = std::move(m_pInstance->EnumerateAdapters()[renderDeviceDesc.requiredGpuIndex]);
  m_pDevice = m_pAdapter->CreateDevice();

  // now that we have a window and m_pDevice, tell the engine to initialize the rendering infrastructure
  plStartup::StartupHighLevelSystems();

  m_pCommandQueue = m_pDevice->GetCommandQueue(plRHICommandListType::kGraphics);
  m_pSwapchain = m_pDevice->CreateSwapchain(m_pWindow->GetNativeWindowHandle(),
    m_pWindow->GetClientAreaSize().width,
    m_pWindow->GetClientAreaSize().height,
    FRAME_COUNT, renderDeviceDesc.vsync);
  m_pFence = m_pDevice->CreateFence(m_FenceValue);
  
  std::vector<plUInt32> indexData = {0, 1, 2};
  m_pIndexBuffer = m_pDevice->CreateBuffer(plRHIBindFlag::kIndexBuffer | plRHIBindFlag::kCopyDest, (plUInt32)(sizeof(plUInt32) * indexData.size()));
  m_pIndexBuffer->CommitMemory(plRHIMemoryType::kUpload);
  m_pIndexBuffer->UpdateUploadBuffer(0, indexData.data(), sizeof(indexData.front()) * indexData.size());

  std::vector<plVec3> vertexData = {plVec3(-0.5, -0.5, 0.0), plVec3(0.0, 0.5, 0.0), plVec3(0.5, -0.5, 0.0)};
  m_pVertexBuffer = m_pDevice->CreateBuffer(plRHIBindFlag::kVertexBuffer | plRHIBindFlag::kCopyDest, (plUInt32)(sizeof(vertexData.front()) * vertexData.size()));
  m_pVertexBuffer->CommitMemory(plRHIMemoryType::kUpload);
  m_pVertexBuffer->UpdateUploadBuffer(0, vertexData.data(), sizeof(vertexData.front()) * vertexData.size());

  plVec4 constantData = plVec4(1, 0, 0, 1);
  m_pConstantBuffer = m_pDevice->CreateBuffer(plRHIBindFlag::kConstantBuffer | plRHIBindFlag::kCopyDest, sizeof(constantData));
  m_pConstantBuffer->CommitMemory(plRHIMemoryType::kUpload);
  m_pConstantBuffer->UpdateUploadBuffer(0, &constantData, sizeof(constantData));

  plStringBuilder projectDirAbsolutePath;
  if (!plFileSystem::ResolveSpecialDirectory(">project", projectDirAbsolutePath).Succeeded())
  {
    PL_REPORT_FAILURE("Project directory could not be resolved.");
  }

  plStringBuilder vsShaderPath;
  vsShaderPath.Append(projectDirAbsolutePath);
  vsShaderPath.Append("/shaders/Triangle/VertexShader.hlsl");
  vsShaderPath.MakeCleanPath();
  plRHIShaderDesc vsDesc;
  vsDesc.shaderPath = vsShaderPath.GetData();
  vsDesc.entrypoint = "main";
  vsDesc.model = "6_0";
  vsDesc.type = plRHIShaderType::kVertex;
  auto vsBlob = Compile(vsDesc, shaderBlobType);
  auto vsReflection = CreateShaderReflection(shaderBlobType, vsBlob.GetData(), vsBlob.GetCount());
  m_pVertexShader = m_pDevice->CreateShader(vsDesc, vsBlob, vsReflection);


  plStringBuilder psShaderPath;
  psShaderPath.Append(projectDirAbsolutePath);
  psShaderPath.Append("/shaders/Triangle/PixelShader.hlsl");
  psShaderPath.MakeCleanPath();
  plRHIShaderDesc psDesc;
  psDesc.shaderPath = psShaderPath.GetData();
  psDesc.entrypoint = "main";
  psDesc.model = "6_0";
  psDesc.type = plRHIShaderType::kPixel;
  auto psBlob = Compile(psDesc, shaderBlobType);
  auto psReflection = CreateShaderReflection(shaderBlobType, psBlob.GetData(), psBlob.GetCount());
  m_pPixelShader = m_pDevice->CreateShader(psDesc, psBlob, psReflection);

  m_pProgram = m_pDevice->CreateProgram({m_pVertexShader, m_pPixelShader});

  plRHIViewDesc constantBufferViewDesc = {};
  constantBufferViewDesc.viewType = plRHIViewType::kConstantBuffer;
  constantBufferViewDesc.dimension = plRHIViewDimension::kBuffer;
  m_pConstantBufferView = m_pDevice->CreateView(m_pConstantBuffer, constantBufferViewDesc);

  plRHIBindKey settingsKey = {plRHIShaderType::kPixel, plRHIViewType::kConstantBuffer, 0, 0, 1};
  m_pLayout = m_pDevice->CreateBindingSetLayout({settingsKey});
  m_pBindingSet = m_pDevice->CreateBindingSet(m_pLayout);
  m_pBindingSet->WriteBindings({{settingsKey, m_pConstantBufferView}});

  plRHIRenderPassDesc renderPassDesc = {
    {{m_pSwapchain->GetFormat(), plRHIRenderPassLoadOp::kClear, plRHIRenderPassStoreOp::kStore}},
  };
  m_pRenderPass = m_pDevice->CreateRenderPass(renderPassDesc);


  plRHIGraphicsPipelineDesc pipelineDesc = {
    m_pProgram,
    m_pLayout,
    {{0, "POSITION", plRHIResourceFormat::R32G32B32_FLOAT, sizeof(vertexData.front())}},
    m_pRenderPass};
  m_pPipeline = m_pDevice->CreateGraphicsPipeline(pipelineDesc);
}

void plTestingApp::BeforeHighLevelSystemsShutdown()
{
  // tell the engine that we are about to destroy window and graphics m_pDevice,
  // and that it therefore needs to cleanup anything that depends on that
  plStartup::ShutdownHighLevelSystems();

  // destroy m_pDevice

  // finally destroy the window
  m_pWindow->Destroy().IgnoreResult();
  PL_DEFAULT_DELETE(m_pWindow);
}

void plTestingApp::OnResize(plUInt32 width, plUInt32 height)
{
  m_pSwapchain.Clear();
  m_pSwapchain = m_pDevice->CreateSwapchain(m_pWindow->GetNativeWindowHandle(), width, height, FRAME_COUNT, renderDeviceDesc.vsync);
  m_FrameIndex = 0;
}

plApplication::Execution plTestingApp::Run()
{
  m_pWindow->ProcessWindowMessages();

  if (m_pWindow->m_bCloseRequested || plInputManager::GetInputActionState("Main", "CloseApp") == plKeyState::Pressed)
    return Execution::Quit;

  // make sure time goes on
  plClock::GetGlobalClock()->Update();

  // update all input state
  plInputManager::Update(plClock::GetGlobalClock()->GetTimeDiff());

  // do the rendering
  {
    m_FrameIndex = m_pSwapchain->NextImage(m_pFence, ++m_FenceValue);

    plRHIClearDesc clear_desc = {{{0.0f, 0.2f, 0.4f, 1.0f}}};

    plRHIViewDesc backBufferViewDesc = {};
    backBufferViewDesc.viewType = plRHIViewType::kRenderTarget;
    backBufferViewDesc.dimension = plRHIViewDimension::kTexture2D;
    plSharedPtr<plRHIResource> backBuffer = m_pSwapchain->GetBackBuffer(m_FrameIndex);
    plSharedPtr<plRHIView> backBufferView = m_pDevice->CreateView(backBuffer, backBufferViewDesc);
    plRHIFramebufferDesc framebufferDesc = {};
    framebufferDesc.renderPass = m_pRenderPass;
    framebufferDesc.width = m_pWindow->GetClientAreaSize().width;
    framebufferDesc.height = m_pWindow->GetClientAreaSize().height;
    framebufferDesc.colors = {backBufferView};
    plSharedPtr<plRHIFramebuffer> framebuffer = m_pDevice->CreateFramebuffer(framebufferDesc);
    plSharedPtr<plRHICommandList> commandList = m_pDevice->CreateCommandList(plRHICommandListType::kGraphics);
    commandList->BindPipeline(m_pPipeline);
    commandList->BindBindingSet(m_pBindingSet);
    commandList->SetViewport(0, 0, (float)m_pWindow->GetClientAreaSize().width, (float)m_pWindow->GetClientAreaSize().height);
    commandList->SetScissorRect(0, 0, m_pWindow->GetClientAreaSize().width, m_pWindow->GetClientAreaSize().height);
    commandList->IASetIndexBuffer(m_pIndexBuffer, plRHIResourceFormat::R32_UINT);
    commandList->IASetVertexBuffer(0, m_pVertexBuffer);
    commandList->ResourceBarrier({{backBuffer, plRHIResourceState::kPresent, plRHIResourceState::kRenderTarget}});
    commandList->BeginRenderPass(m_pRenderPass, framebuffer, clear_desc);
    commandList->DrawIndexed(3, 1, 0, 0, 0);
    commandList->EndRenderPass();
    commandList->ResourceBarrier({{backBuffer, plRHIResourceState::kRenderTarget, plRHIResourceState::kPresent}});
    commandList->Close();

    m_pCommandQueue->Wait(m_pFence, m_FenceValue);
    m_pFence->Wait(m_FenceValues[m_FrameIndex]);
    m_pCommandQueue->ExecuteCommandLists({commandList});
    m_pCommandQueue->Signal(m_pFence, m_FenceValues[m_FrameIndex] = ++m_FenceValue);
    m_pSwapchain->Present(m_pFence, m_FenceValues[m_FrameIndex]);


    m_pCommandQueue->Signal(m_pFence, ++m_FenceValue);
    m_pFence->Wait(m_FenceValue);
  }

  // needs to be called once per frame
  plResourceManager::PerFrameUpdate();

  // tell the task system to finish its work for this frame
  // this has to be done at the very end, so that the task system will only use up the time that is left in this frame for
  // uploading GPU data etc.
  plTaskSystem::FinishFrameTasks();

  return plApplication::Execution::Continue;
}

PL_CONSOLEAPP_ENTRY_POINT(plTestingApp);
