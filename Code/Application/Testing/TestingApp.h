#pragma once

#include <memory>
#include <Foundation/Application/Application.h>
#include <Foundation/Types/UniquePtr.h>

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
  plTestingAppWindow* m_pWindow = nullptr;
};
