#pragma once

#include <Core/GameApplication/WindowOutputTargetBase.h>
#include <Core/GameState/GameStateBase.h>
#include <Core/System/Window.h>
#include <Foundation/Application/Application.h>
#include <Foundation/Types/UniquePtr.h>

class plWindowBase;
struct plWindowCreationDesc;
class plWorld;

/// Allows custom code to inject logic at specific points during
/// initialization or during shutdown. The events are listed in
/// the order in which they typically happen.
struct plGameApplicationStaticEvent
{
  enum class Type
  {
    AfterGameStateActivated,
    BeforeGameStateDeactivated
  };

  Type m_Type;
};

/// Allows custom code to inject logic at specific update points.
/// The events are listed in the order in which they typically happen.
struct plGameApplicationExecutionEvent
{
  enum class Type
  {
    BeginAppTick,
    BeforeWorldUpdates,
    AfterWorldUpdates,
    BeforeUpdatePlugins,
    AfterUpdatePlugins,
    BeforePresent,
    AfterPresent,
    EndAppTick,
  };

  Type m_Type;
};

// TODO: document this and update plGameApplication comments

class PL_CORE_DLL plGameApplicationBase : public plApplication
{
public:
  using SUPER = plApplication;

  plGameApplicationBase(plStringView sAppName);
  ~plGameApplicationBase();

  /// \name Basics
  ///@{

public:
  /// \brief Returns the plGameApplicationBase singleton
  static plGameApplicationBase* GetGameApplicationBaseInstance() { return s_pGameApplicationBaseInstance; }

protected:
  static plGameApplicationBase* s_pGameApplicationBaseInstance;

  ///@}
  /// \name GameState
  ///@{
public:
  /// \brief Creates and activates the game state for this application.
  ///
  /// If the application already has a world (such as the editor), it can pass this to the newly created game state.
  /// Otherwise the game state should create its own world.
  ///
  /// In the editor case, there are cases where a 'player start position' is specified, which can be used
  /// by the game state to place the player.
  ///
  /// Broadcasts local event: plGameApplicationStaticEvent::AfterGameStateActivated
  /// Broadcasts global event: AfterGameStateActivation(plGameStateBase*)
  plResult ActivateGameState(plWorld* pWorld = nullptr, const plTransform* pStartPosition = nullptr);

  /// \brief Deactivates and destroys the active game state.
  ///
  /// Broadcasts local event: plGameApplicationStaticEvent::BeforeGameStateDeactivated
  /// Broadcasts global event: BeforeGameStateDeactivation(plGameStateBase*)
  void DeactivateGameState();

  /// \brief Returns the currently active game state. Could be nullptr.
  plGameStateBase* GetActiveGameState() const { return m_pGameState.Borrow(); }

  /// \brief Returns the currently active game state IF it was created for the given world.
  ///
  /// This is mostly for editor use cases, where some documents want to handle the game state, but only
  /// it it was set up for a particular document.
  plGameStateBase* GetActiveGameStateLinkedToWorld(const plWorld* pWorld) const;

protected:
  /// \brief Creates a game state for the application to use.
  ///
  /// \a pWorld is typically nullptr in a stand-alone app, but may be existing already when called from the editor.
  ///
  /// The default implementation will query all available game states for the best match.
  /// By overriding this, one can also just create a specific game state directly.
  virtual plUniquePtr<plGameStateBase> CreateGameState(plWorld* pWorld);

  /// \brief Allows to override whether a game state is created and activated at application startup.
  ///
  /// The default implementation just calls ActivateGameState(), but applications that run inside the editor override this to do nothing,
  /// as they only want the game state to become active during simulation, not during editing.
  virtual void ActivateGameStateAtStartup();

  plUniquePtr<plGameStateBase> m_pGameState;
  plWorld* m_pWorldLinkedWithGameState = nullptr;

  ///@}
  /// \name Application Startup
  ///@{
protected:
  virtual plResult BeforeCoreSystemsStartup() override;
  virtual void AfterCoreSystemsStartup() override;

  /// \brief Returns the target of the 'project' special data directory.
  ///
  /// The return value of this function will be passed into plFileSystem::SetSpecialDirectory.
  /// Afterwards, any path starting with the special directory marker (">project/") will point
  /// into this directory.
  virtual plString FindProjectDirectory() const = 0;

  /// \brief Returns the target of the 'base' data directory.
  ///
  /// Path needs to start with a special directory marker (">marker/").
  /// This is passed into the target of the 'base' data directory. Target defaults to ">sdk/Data/Base".
  virtual plString GetBaseDataDirectoryPath() const;

  /// \brief Returns the target of the 'project' data directory.
  ///
  /// Path needs to start with a special directory marker (">marker/").
  /// This is passed into the target of the 'project' data directory. Target defaults to ">project/".
  virtual plString GetProjectDataDirectoryPath() const;

  /// \brief Executes all 'BaseInit_' functions. Typically done very early, before core system startup
  virtual void ExecuteBaseInitFunctions();
  virtual void BaseInit_ConfigureLogging();

  plEventSubscriptionID m_LogToConsoleID = 0;
  plEventSubscriptionID m_LogToVsID = 0;

  /// \brief Executes all 'Init_' functions. Typically done after core system startup
  virtual void ExecuteInitFunctions();
  virtual void Init_ConfigureTelemetry();
  virtual void Init_FileSystem_SetSpecialDirs();
  virtual void Init_LoadRequiredPlugins();
  virtual void Init_ConfigureAssetManagement();
  virtual void Init_FileSystem_ConfigureDataDirs();
  virtual void Init_LoadProjectPlugins();
  virtual void Init_ConfigureInput();
  virtual void Init_ConfigureTags();
  virtual void Init_ConfigureCVars();
  virtual void Init_SetupGraphicsDevice() = 0;
  virtual void Init_SetupDefaultResources();

  plEvent<const plGameApplicationStaticEvent&> m_StaticEvents;

  ///@}
  /// \name Application Shutdown
  ///@{
protected:
  virtual void BeforeHighLevelSystemsShutdown() override;
  virtual void BeforeCoreSystemsShutdown() override;

  virtual void Deinit_ShutdownGraphicsDevice() = 0;
  virtual void Deinit_UnloadPlugins();
  virtual void Deinit_ShutdownLogging();

  ///@}
  /// \name Application Execution
  ///@{

public:
  virtual plApplication::Execution Run() override;

  void RunOneFrame();

  plCopyOnBroadcastEvent<const plGameApplicationExecutionEvent&> m_ExecutionEvents;

  plTime GetFrameTime() const { return m_FrameTime; }

protected:
  virtual bool IsGameUpdateEnabled() const { return true; }

  virtual void Run_InputUpdate();
  virtual bool Run_ProcessApplicationInput();
  /// \brief This function can be used to acquire a new window from a swap-chain or do any other update operations on windows before the multi-threaded rendering and update phase starts.
  virtual void Run_AcquireImage();
  virtual void Run_WorldUpdateAndRender() = 0;
  virtual void Run_BeforeWorldUpdate();
  virtual void Run_AfterWorldUpdate();
  virtual void Run_UpdatePlugins();
  /// \brief This function can be used to present the final image to a window. It is run at the end of the rendering phase. It can also be used to inspect the swap-chain e.g. for screenshot purposes before presenting.
  virtual void Run_PresentImage();
  virtual void Run_FinishFrame();

  void UpdateFrameTime();

  plTime m_FrameTime;
  ///@}
};
