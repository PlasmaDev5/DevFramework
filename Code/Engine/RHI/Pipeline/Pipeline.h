#pragma once
#include <RHI/RHIDLL.h>

#include <RHI/Instance/BaseTypes.h>
#include <RHI/Instance/QueryInterface.h>
#include <memory>

class PL_RHI_DLL plRHIPipeline : public plRefCounted
{
public:
  virtual ~plRHIPipeline() = default;
  virtual plRHIPipelineType GetPipelineType() const = 0;
  virtual plDynamicArray<plUInt8> GetRayTracingShaderGroupHandles(plUInt32 firstGroup, plUInt32 groupCount) const = 0;
};
