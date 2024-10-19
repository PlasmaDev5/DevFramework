#include <Core/CorePCH.h>

#include <Core/ActorSystem/ActorManager.h>
#include <Core/GameApplication/GameApplicationBase.h>
#include <Core/Input/InputManager.h>
#include <Core/ResourceManager/ResourceManager.h>
#include <Core/System/Window.h>
#include <Foundation/Communication/GlobalEvent.h>
#include <Foundation/Communication/Telemetry.h>
#include <Foundation/Configuration/Singleton.h>
#include <Foundation/Configuration/Startup.h>
#include <Foundation/IO/FileSystem/FileWriter.h>
#include <Foundation/Memory/FrameAllocator.h>
#include <Foundation/Profiling/Profiling.h>
#include <Foundation/Threading/TaskSystem.h>
#include <Foundation/Time/Clock.h>
#include <Foundation/Time/Stopwatch.h>
#include <Foundation/Time/Timestamp.h>
#include <Texture/Image/Image.h>

plGameApplicationBase* plGameApplicationBase::s_pGameApplicationBaseInstance = nullptr;

plGameApplicationBase::plGameApplicationBase(plStringView sAppName)
  : plApplication(sAppName)
{
  s_pGameApplicationBaseInstance = this;
}

plGameApplicationBase::~plGameApplicationBase()
{
  s_pGameApplicationBaseInstance = nullptr;
}

void AppendCurrentTimestamp(plStringBuilder& out_sString)
{
  const plDateTime dt = plDateTime::MakeFromTimestamp(plTimestamp::CurrentTimestamp());

  out_sString.AppendFormat("_{0}-{1}-{2}_{3}-{4}-{5}-{6}", dt.GetYear(), plArgU(dt.GetMonth(), 2, true), plArgU(dt.GetDay(), 2, true), plArgU(dt.GetHour(), 2, true), plArgU(dt.GetMinute(), 2, true), plArgU(dt.GetSecond(), 2, true), plArgU(dt.GetMicroseconds() / 1000, 3, true));
}

//////////////////////////////////////////////////////////////////////////

plResult plGameApplicationBase::ActivateGameState(plWorld* pWorld /*= nullptr*/, const plTransform* pStartPosition /*= nullptr*/)
{
  PL_ASSERT_DEBUG(m_pGameState == nullptr, "ActivateGameState cannot be called when another GameState is already active");

  m_pGameState = CreateGameState(pWorld);

  if (m_pGameState == nullptr)
    return PL_FAILURE;

  m_pWorldLinkedWithGameState = pWorld;
  m_pGameState->OnActivation(pWorld, pStartPosition);

  plGameApplicationStaticEvent e;
  e.m_Type = plGameApplicationStaticEvent::Type::AfterGameStateActivated;
  m_StaticEvents.Broadcast(e);

  PL_BROADCAST_EVENT(AfterGameStateActivation, m_pGameState.Borrow());

  return PL_SUCCESS;
}

void plGameApplicationBase::DeactivateGameState()
{
  if (m_pGameState == nullptr)
    return;

  PL_BROADCAST_EVENT(BeforeGameStateDeactivation, m_pGameState.Borrow());

  plGameApplicationStaticEvent e;
  e.m_Type = plGameApplicationStaticEvent::Type::BeforeGameStateDeactivated;
  m_StaticEvents.Broadcast(e);

  m_pGameState->OnDeactivation();

  plActorManager::GetSingleton()->DestroyAllActors(m_pGameState.Borrow());

  m_pGameState = nullptr;
}

plGameStateBase* plGameApplicationBase::GetActiveGameStateLinkedToWorld(const plWorld* pWorld) const
{
  if (m_pWorldLinkedWithGameState == pWorld)
    return m_pGameState.Borrow();

  return nullptr;
}

plUniquePtr<plGameStateBase> plGameApplicationBase::CreateGameState(plWorld* pWorld)
{
  PL_LOG_BLOCK("Create Game State");

  plUniquePtr<plGameStateBase> pCurState;

  {
    plInt32 iBestPriority = -1;

    plRTTI::ForEachDerivedType<plGameStateBase>(
      [&](const plRTTI* pRtti) {
        plUniquePtr<plGameStateBase> pState = pRtti->GetAllocator()->Allocate<plGameStateBase>();

        const plInt32 iPriority = (plInt32)pState->DeterminePriority(pWorld);
        if (iPriority > iBestPriority)
        {
          iBestPriority = iPriority;

          pCurState = std::move(pState);
        }
      },
      plRTTI::ForEachOptions::ExcludeNonAllocatable);
  }

  return pCurState;
}

void plGameApplicationBase::ActivateGameStateAtStartup()
{
  ActivateGameState().IgnoreResult();
}

plResult plGameApplicationBase::BeforeCoreSystemsStartup()
{
  plStartup::AddApplicationTag("runtime");

  ExecuteBaseInitFunctions();

  return SUPER::BeforeCoreSystemsStartup();
}

void plGameApplicationBase::AfterCoreSystemsStartup()
{
  SUPER::AfterCoreSystemsStartup();

  ExecuteInitFunctions();

  // If one of the init functions already requested the application to quit,
  // something must have gone wrong. Don't continue initialization and let the
  // application exit.
  if (WasQuitRequested())
  {
    return;
  }

  plStartup::StartupHighLevelSystems();

  ActivateGameStateAtStartup();
}

void plGameApplicationBase::ExecuteBaseInitFunctions()
{
  BaseInit_ConfigureLogging();
}

void plGameApplicationBase::BeforeHighLevelSystemsShutdown()
{
  DeactivateGameState();

  {
    // make sure that no resources continue to be streamed in, while the engine shuts down
    plResourceManager::EngineAboutToShutdown();
    plResourceManager::ExecuteAllResourceCleanupCallbacks();
    plResourceManager::FreeAllUnusedResources();
  }
}

void plGameApplicationBase::BeforeCoreSystemsShutdown()
{
  // shut down all actors and APIs that may have been in use
  if (plActorManager::GetSingleton() != nullptr)
  {
    plActorManager::GetSingleton()->Shutdown();
  }

  {
    plFrameAllocator::Reset();
    plResourceManager::FreeAllUnusedResources();
  }

  {
    Deinit_ShutdownGraphicsDevice();
    plResourceManager::FreeAllUnusedResources();
  }

  Deinit_UnloadPlugins();

  // shut down telemetry if it was set up
  {
    plTelemetry::CloseConnection();
  }

  Deinit_ShutdownLogging();

  SUPER::BeforeCoreSystemsShutdown();
}

static bool s_bUpdatePluginsExecuted = false;

PL_ON_GLOBAL_EVENT(GameApp_UpdatePlugins)
{
  PL_IGNORE_UNUSED(param0);
  PL_IGNORE_UNUSED(param1);
  PL_IGNORE_UNUSED(param2);
  PL_IGNORE_UNUSED(param3);

  s_bUpdatePluginsExecuted = true;
}

plApplication::Execution plGameApplicationBase::Run()
{
  if (m_bWasQuitRequested)
    return plApplication::Execution::Quit;

  RunOneFrame();
  return plApplication::Execution::Continue;
}

void plGameApplicationBase::RunOneFrame()
{
  PL_PROFILE_SCOPE("Run");
  s_bUpdatePluginsExecuted = false;

  plActorManager::GetSingleton()->Update();

  if (!IsGameUpdateEnabled())
    return;

  {
    // for plugins that need to hook into this without a link dependency on this lib
    PL_PROFILE_SCOPE("GameApp_BeginAppTick");
    PL_BROADCAST_EVENT(GameApp_BeginAppTick);
    plGameApplicationExecutionEvent e;
    e.m_Type = plGameApplicationExecutionEvent::Type::BeginAppTick;
    m_ExecutionEvents.Broadcast(e);
  }

  Run_InputUpdate();

  Run_AcquireImage();

  Run_WorldUpdateAndRender();

  if (!s_bUpdatePluginsExecuted)
  {
    Run_UpdatePlugins();

    PL_ASSERT_DEV(s_bUpdatePluginsExecuted, "plGameApplicationBase::Run_UpdatePlugins has been overridden, but it does not broadcast the "
                                            "global event 'GameApp_UpdatePlugins' anymore.");
  }

  {
    // for plugins that need to hook into this without a link dependency on this lib
    PL_PROFILE_SCOPE("GameApp_EndAppTick");
    PL_BROADCAST_EVENT(GameApp_EndAppTick);

    plGameApplicationExecutionEvent e;
    e.m_Type = plGameApplicationExecutionEvent::Type::EndAppTick;
    m_ExecutionEvents.Broadcast(e);
  }

  {
    PL_PROFILE_SCOPE("BeforePresent");
    plGameApplicationExecutionEvent e;
    e.m_Type = plGameApplicationExecutionEvent::Type::BeforePresent;
    m_ExecutionEvents.Broadcast(e);
  }

  {
    PL_PROFILE_SCOPE("Run_PresentImage");
    Run_PresentImage();
  }
  plClock::GetGlobalClock()->Update();
  UpdateFrameTime();

  {
    PL_PROFILE_SCOPE("AfterPresent");
    plGameApplicationExecutionEvent e;
    e.m_Type = plGameApplicationExecutionEvent::Type::AfterPresent;
    m_ExecutionEvents.Broadcast(e);
  }

  {
    PL_PROFILE_SCOPE("Run_FinishFrame");
    Run_FinishFrame();
  }
}

void plGameApplicationBase::Run_InputUpdate()
{
  PL_PROFILE_SCOPE("Run_InputUpdate");
  plInputManager::Update(plClock::GetGlobalClock()->GetTimeDiff());

  if (!Run_ProcessApplicationInput())
    return;

  if (m_pGameState)
  {
    m_pGameState->ProcessInput();
  }
}

bool plGameApplicationBase::Run_ProcessApplicationInput()
{
  return true;
}

void plGameApplicationBase::Run_AcquireImage()
{
}

void plGameApplicationBase::Run_BeforeWorldUpdate()
{
  PL_PROFILE_SCOPE("GameApplication.BeforeWorldUpdate");

  if (m_pGameState)
  {
    m_pGameState->BeforeWorldUpdate();
  }

  {
    plGameApplicationExecutionEvent e;
    e.m_Type = plGameApplicationExecutionEvent::Type::BeforeWorldUpdates;
    m_ExecutionEvents.Broadcast(e);
  }
}

void plGameApplicationBase::Run_AfterWorldUpdate()
{
  PL_PROFILE_SCOPE("GameApplication.AfterWorldUpdate");

  if (m_pGameState)
  {
    m_pGameState->AfterWorldUpdate();
  }

  {
    plGameApplicationExecutionEvent e;
    e.m_Type = plGameApplicationExecutionEvent::Type::AfterWorldUpdates;
    m_ExecutionEvents.Broadcast(e);
  }
}

void plGameApplicationBase::Run_UpdatePlugins()
{
  PL_PROFILE_SCOPE("Run_UpdatePlugins");
  {
    plGameApplicationExecutionEvent e;
    e.m_Type = plGameApplicationExecutionEvent::Type::BeforeUpdatePlugins;
    m_ExecutionEvents.Broadcast(e);
  }

  // for plugins that need to hook into this without a link dependency on this lib
  PL_BROADCAST_EVENT(GameApp_UpdatePlugins);

  {
    plGameApplicationExecutionEvent e;
    e.m_Type = plGameApplicationExecutionEvent::Type::AfterUpdatePlugins;
    m_ExecutionEvents.Broadcast(e);
  }
}

void plGameApplicationBase::Run_PresentImage() {}

void plGameApplicationBase::Run_FinishFrame()
{
  plTelemetry::PerFrameUpdate();
  plResourceManager::PerFrameUpdate();
  plTaskSystem::FinishFrameTasks();
  plFrameAllocator::Swap();
  plProfilingSystem::StartNewFrame();

  // if many messages have been logged, make sure they get written to disk
  plLog::Flush(100, plTime::MakeFromSeconds(10));
}

void plGameApplicationBase::UpdateFrameTime()
{
  // Do not use plClock for this, it smooths and clamps the timestep
  const plTime tNow = plTime::Now();

  static plTime tLast = tNow;
  m_FrameTime = tNow - tLast;
  tLast = tNow;
}




PL_STATICLINK_FILE(Core, Core_GameApplication_Implementation_GameApplicationBase);

