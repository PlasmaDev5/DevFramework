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
  plStringBuilder sProjectDir = ">sdk/Data/Samples/RHISample";
  plStringBuilder sProjectDirResolved;
  plFileSystem::ResolveSpecialDirectory(sProjectDir, sProjectDirResolved).IgnoreResult();

  plFileSystem::SetSpecialDirectory("project", sProjectDirResolved);

  plFileSystem::AddDataDirectory("", "", ":", plDataDirUsage::AllowWrites).IgnoreResult();
  plFileSystem::AddDataDirectory(">appdir/", "AppBin", "bin", plDataDirUsage::AllowWrites).IgnoreResult();                              // writing to the binary directory
  plFileSystem::AddDataDirectory(">appdir/", "ShaderCache", "shadercache", plDataDirUsage::AllowWrites).IgnoreResult();                 // for shader files
  plFileSystem::AddDataDirectory(">user/plEngine Project/RHISample", "AppData", "appdata", plDataDirUsage::AllowWrites).IgnoreResult(); // app user data

  plFileSystem::AddDataDirectory(">sdk/Data/Base", "Base", "base").IgnoreResult();
  plFileSystem::AddDataDirectory(">project/", "Project", "project", plDataDirUsage::AllowWrites).IgnoreResult();

  plGlobalLog::AddLogWriter(plLogWriter::Console::LogMessageHandler);
  plGlobalLog::AddLogWriter(plLogWriter::VisualStudio::LogMessageHandler);

  plPlugin::LoadPlugin("plasmaInspectorPlugin").IgnoreResult();

  // Register Input
  {
    plInputActionConfig cfg;

    cfg = plInputManager::GetInputActionConfig("Main", "CloseApp");
    cfg.m_sInputSlotTrigger[0] = plInputSlot_KeyEscape;
    plInputManager::SetInputActionConfig("Main", "CloseApp", cfg, true);
  }


  // Create a window for rendering
  {
    plWindowCreationDesc WindowCreationDesc;
    WindowCreationDesc.m_Resolution.width = g_uiWindowWidth;
    WindowCreationDesc.m_Resolution.height = g_uiWindowHeight;
    WindowCreationDesc.m_Title = "Testing";
    WindowCreationDesc.m_bShowMouseCursor = true;
    WindowCreationDesc.m_bClipMouseCursor = false;
    WindowCreationDesc.m_WindowMode = plWindowMode::WindowResizable;
    m_pWindow = PL_DEFAULT_NEW(plTestingAppWindow, this);
    m_pWindow->Initialize(WindowCreationDesc).IgnoreResult();
  }

  

  // now that we have a window and m_pDevice, tell the engine to initialize the rendering infrastructure
  plStartup::StartupHighLevelSystems();
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


  // needs to be called once per frame
  plResourceManager::PerFrameUpdate();

  // tell the task system to finish its work for this frame
  // this has to be done at the very end, so that the task system will only use up the time that is left in this frame for
  // uploading GPU data etc.
  plTaskSystem::FinishFrameTasks();

  return plApplication::Execution::Continue;
}

PL_CONSOLEAPP_ENTRY_POINT(plTestingApp);
