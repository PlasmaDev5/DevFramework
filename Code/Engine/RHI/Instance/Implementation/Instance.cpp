#include <RHI/RHIPCH.h>

#include <RHI/Instance/Instance.h>

struct CreatorFuncInfo
{
  plRHIInstanceFactory::CreatorFunc m_Func;
};

static plMap<plRHIApiType, CreatorFuncInfo> s_CreatorFuncs;

CreatorFuncInfo* GetCreatorFuncInfo(plRHIApiType type)
{
  auto pFuncInfo = s_CreatorFuncs.GetValue(type);
  if (pFuncInfo == nullptr)
  {
    plStringBuilder sPluginName = "PlasmaRHI";
    if (type == plRHIApiType::kDX12)
      sPluginName.Append("DX12");
    else if (type == plRHIApiType::kMetal)
      sPluginName.Append("Metal");
    else
      sPluginName.Append("Vulkan");

    PL_VERIFY(plPlugin::LoadPlugin(sPluginName).Succeeded(), "Renderer plugin '{}' not found", sPluginName);

    pFuncInfo = s_CreatorFuncs.GetValue(type);
    PL_ASSERT_DEV(pFuncInfo != nullptr, "Renderer '{}' is not registered", sPluginName.GetData());
  }

  return pFuncInfo;
}

plSharedPtr<plRHIInstance> plRHIInstanceFactory::CreateInstance(plRHIApiType type)
{
  if (auto pFuncInfo = GetCreatorFuncInfo(type))
  {
    return pFuncInfo->m_Func();
  }

  return nullptr;
}

void plRHIInstanceFactory::RegisterCreatorFunc(plRHIApiType type, const CreatorFunc& func)
{
  CreatorFuncInfo funcInfo;
  funcInfo.m_Func = func;

  if (s_CreatorFuncs.Contains(type))
  {
    PL_REPORT_FAILURE("Creator func already registered");
    return;
  }

  s_CreatorFuncs.Insert(type, funcInfo);
}

void plRHIInstanceFactory::UnregisterCreatorFunc(plRHIApiType type)
{
  PL_VERIFY(s_CreatorFuncs.Remove(type), "Creator func not registered");
}
