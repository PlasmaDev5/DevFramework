#pragma once
#include <RHI/RHIDLL.h>

#include <RHI/Instance/BaseTypes.h>
#include <RHI/Instance/QueryInterface.h>
#include <memory>
#include <string>
#include <vector>

enum class plRHIShaderKind
{
  kUnknown = 0,
  kPixel,
  kVertex,
  kGeometry,
  kCompute,
  kLibrary,
  kRayGeneration,
  kIntersection,
  kAnyHit,
  kClosestHit,
  kMiss,
  kCallable,
  kMesh,
  kAmplification,
};

struct PL_RHI_DLL plRHIEntryPoint
{
  plString name;
  plRHIShaderKind kind;
  plUInt32 payloadSize;
  plUInt32 attributeSize;
};

inline bool operator==(const plRHIEntryPoint& lhs, const plRHIEntryPoint& rhs)
{
  return std::tie(lhs.name, lhs.kind) == std::tie(rhs.name, rhs.kind);
}

inline bool operator<(const plRHIEntryPoint& lhs, const plRHIEntryPoint& rhs)
{
  return std::tie(lhs.name, lhs.kind) < std::tie(rhs.name, rhs.kind);
}

inline auto MakeTie(const plRHIResourceBindingDesc& desc)
{
  return std::tie(desc.name, desc.type, desc.slot, desc.space, desc.dimension);
};

inline bool operator==(const plRHIResourceBindingDesc& lhs, const plRHIResourceBindingDesc& rhs)
{
  return MakeTie(lhs) == MakeTie(rhs);
}

inline bool operator<(const plRHIResourceBindingDesc& lhs, const plRHIResourceBindingDesc& rhs)
{
  return MakeTie(lhs) < MakeTie(rhs);
}

struct PL_RHI_DLL plRHIInputParameterDesc
{
  plUInt32 Location;
  plString semanticName;
  plRHIResourceFormat::Enum Format;
};

struct PL_RHI_DLL plRHIOutputParameterDesc
{
  plUInt32 slot;
};

enum class plRHIVariableType
{
  kStruct,
  kFloat,
  kInt,
  kUint,
  kBool,
};

struct PL_RHI_DLL plRHIVariableLayout
{
  plString name;
  plRHIVariableType type;
  plUInt32 offset;
  plUInt32 size;
  plUInt32 rows;
  plUInt32 columns;
  plUInt32 elements;
  std::vector<plRHIVariableLayout> members;
};

inline auto MakeTie(const plRHIVariableLayout& desc)
{
  return std::tie(desc.name, desc.type, desc.offset, desc.size, desc.rows, desc.columns, desc.elements, desc.members);
};

inline bool operator==(const plRHIVariableLayout& lhs, const plRHIVariableLayout& rhs)
{
  return MakeTie(lhs) == MakeTie(rhs);
}

inline bool operator<(const plRHIVariableLayout& lhs, const plRHIVariableLayout& rhs)
{
  return MakeTie(lhs) < MakeTie(rhs);
}

struct PL_RHI_DLL plRHIShaderFeatureInfo
{
  bool ResourceDescriptorHeapIndexing = false;
  bool SamplerDescriptorHeapIndexing = false;
};

class PL_RHI_DLL plRHIShaderReflection : public plRefCounted
{
public:
  virtual ~plRHIShaderReflection() = default;
  virtual const std::vector<plRHIEntryPoint>& GetEntryPoints() const = 0;
  virtual const std::vector<plRHIResourceBindingDesc>& GetBindings() const = 0;
  virtual const std::vector<plRHIVariableLayout>& GetVariableLayouts() const = 0;
  virtual const std::vector<plRHIInputParameterDesc>& GetInputParameters() const = 0;
  virtual const std::vector<plRHIOutputParameterDesc>& GetOutputParameters() const = 0;
  virtual const plRHIShaderFeatureInfo& GetShaderFeatureInfo() const = 0;
};
