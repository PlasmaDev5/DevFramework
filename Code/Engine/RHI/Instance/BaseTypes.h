#pragma once
#include <RHI/RHIDLL.h>

#include <RHI/Instance/EnumUtils.h>
#include <array>
#include <cassert>
#include <cstdint>
#include <map>
#include <memory>
#include <string>
#include <tuple>
#include <vector>

namespace enum_class
{
  enum plRHIResourceState : plUInt32
  {
    kUnknown = 0,
    kCommon = 1 << 0,
    kVertexAndConstantBuffer = 1 << 1,
    kIndexBuffer = 1 << 2,
    kRenderTarget = 1 << 3,
    kUnorderedAccess = 1 << 4,
    kDepthStencilWrite = 1 << 5,
    kDepthStencilRead = 1 << 6,
    kNonPixelShaderResource = 1 << 7,
    kPixelShaderResource = 1 << 8,
    kIndirectArgument = 1 << 9,
    kCopyDest = 1 << 10,
    kCopySource = 1 << 11,
    kRaytracingAccelerationStructure = 1 << 12,
    kShadingRateSource = 1 << 13,
    kPresent = 1 << 14,
    kGenericRead =
      plRHIResourceState::kVertexAndConstantBuffer |
      plRHIResourceState::kIndexBuffer |
      plRHIResourceState::kCopySource |
      plRHIResourceState::kNonPixelShaderResource |
      plRHIResourceState::kPixelShaderResource |
      plRHIResourceState::kIndirectArgument,
    kUndefined = 1 << 15,
  };
}

using plRHIResourceState = enum_class::plRHIResourceState;
ENABLE_BITMASK_OPERATORS(plRHIResourceState);

enum class plRHIViewDimension
{
  kUnknown,
  kBuffer,
  kTexture1D,
  kTexture1DArray,
  kTexture2D,
  kTexture2DArray,
  kTexture2DMS,
  kTexture2DMSArray,
  kTexture3D,
  kTextureCube,
  kTextureCubeArray,
};

enum class plRHISamplerFilter
{
  kAnisotropic,
  kMinMagMipLinear,
  kComparisonMinMagMipLinear,
};

enum class plRHISamplerTextureAddressMode
{
  kWrap,
  kClamp
};

enum class plRHISamplerComparisonFunc
{
  kNever,
  kAlways,
  kLess
};

struct plRHISamplerDesc
{
  plRHISamplerFilter filter;
  plRHISamplerTextureAddressMode mode;
  plRHISamplerComparisonFunc func;
};

enum class plRHIViewType
{
  kUnknown,
  kConstantBuffer,
  kSampler,
  kTexture,
  kRWTexture,
  kBuffer,
  kRWBuffer,
  kStructuredBuffer,
  kRWStructuredBuffer,
  kAccelerationStructure,
  kShadingRateSource,
  kRenderTarget,
  kDepthStencil
};

enum class plRHIShaderBlobType
{
  kDXIL,
  kSPIRV
};

enum class plRHIResourceType
{
  kUnknown,
  kBuffer,
  kTexture,
  kSampler,
  kAccelerationStructure,
};

enum class plRHITextureType
{
  k1D,
  k2D,
  k3D,
};

namespace plRHIBindFlag
{
  enum
  {
    kRenderTarget = 1 << 1,
    kDepthStencil = 1 << 2,
    kShaderResource = 1 << 3,
    kUnorderedAccess = 1 << 4,
    kConstantBuffer = 1 << 5,
    kIndexBuffer = 1 << 6,
    kVertexBuffer = 1 << 7,
    kAccelerationStructure = 1 << 8,
    kRayTracing = 1 << 9,
    kCopyDest = 1 << 10,
    kCopySource = 1 << 11,
    kShadingRateSource = 1 << 12,
    kShaderTable = 1 << 13,
    kIndirectBuffer = 1 << 14
  };
}

enum class plRHIFillMode
{
  kWireframe,
  kSolid
};

enum class plRHICullMode
{
  kNone,
  kFront,
  kBack,
};

struct plRHIRasterizerDesc
{
  plRHIFillMode fillMode = plRHIFillMode::kSolid;
  plRHICullMode cullMode = plRHICullMode::kBack;
  plInt32 depthBias = 0;

  auto MakeTie() const
  {
    return std::tie(fillMode, cullMode, depthBias);
  }
};

enum class plRHIBlend
{
  kZero,
  kSrcAlpha,
  kInvSrcAlpha,
};

enum class plRHIBlendOp
{
  kAdd,
};

struct plRHIBlendDesc
{
  bool blendEnable = false;
  plRHIBlend blendSrc;
  plRHIBlend blendDest;
  plRHIBlendOp blendOp;
  plRHIBlend blendSrcAlpha;
  plRHIBlend blendDestApha;
  plRHIBlendOp blendOpAlpha;

  auto MakeTie() const
  {
    return std::tie(blendEnable, blendSrc, blendDest, blendOp, blendSrcAlpha, blendDestApha, blendOpAlpha);
  }
};

enum class plRHIComparisonFunc
{
  kNever,
  kLess,
  kEqual,
  kLessEqual,
  kGreater,
  kNotEqual,
  kGreaterEqual,
  kAlways
};

enum class plRHIStencilOp
{
  kKeep,
  kZero,
  kReplace,
  kIncrSat,
  kDecrSat,
  kInvert,
  kIncr,
  kDecr
};

struct plRHIStencilOpDesc
{
  plRHIStencilOp failOp = plRHIStencilOp::kKeep;
  plRHIStencilOp depthFailOp = plRHIStencilOp::kKeep;
  plRHIStencilOp passOp = plRHIStencilOp::kKeep;
  plRHIComparisonFunc func = plRHIComparisonFunc::kAlways;

  auto MakeTie() const
  {
    return std::tie(failOp, depthFailOp, passOp, func);
  }
};

struct plRHIDepthStencilDesc
{
  bool depthTestEnable = true;
  plRHIComparisonFunc depthFunc = plRHIComparisonFunc::kLess;
  bool depthWriteEnable = true;
  bool depthBoundsTestEnable = false;
  bool stencilEnable = false;
  plUInt8 stencilReadMask = 0xff;
  plUInt8 stencilWriteMask = 0xff;
  plRHIStencilOpDesc frontFace = {};
  plRHIStencilOpDesc backFace = {};

  auto MakeTie() const
  {
    return std::tie(depthTestEnable, depthFunc, depthWriteEnable, depthBoundsTestEnable, stencilEnable, stencilReadMask, stencilWriteMask, frontFace, backFace);
  }
};

enum class plRHIShaderType
{
  kUnknown,
  kVertex,
  kPixel,
  kCompute,
  kGeometry,
  kAmplification,
  kMesh,
  kLibrary,
};

struct plRHIViewDesc
{
  plRHIViewType viewType = plRHIViewType::kUnknown;
  plRHIViewDimension dimension = plRHIViewDimension::kUnknown;
  plUInt32 baseMipLevel = 0;
  plUInt32 levelCount = static_cast<plUInt32>(-1);
  plUInt32 baseArrayLayer = 0;
  plUInt32 layerCount = static_cast<plUInt32>(-1);
  plUInt32 planeSlice = 0;
  plUInt64 offset = 0;
  plUInt32 structureStride = 0;
  plUInt64 bufferSize = static_cast<plUInt64>(-1);
  plRHIResourceFormat::Enum bufferFormat = plRHIResourceFormat::UNKNOWN;
  bool bindless = false;

  auto MakeTie() const
  {
    return std::tie(viewType, dimension, baseMipLevel, levelCount, baseArrayLayer, layerCount, planeSlice, offset, structureStride, bufferSize, bufferFormat, bindless);
  }
};

struct plRHIShaderDesc
{
  //plDynamicArray<plUInt8> blob;
  //plSharedPtr<plRHIShaderReflection> reflection;

  plString shaderPath;
  plString entrypoint;
  plRHIShaderType type;
  plString model;
  std::map<plString, plString> define;

  plRHIShaderDesc(const plString& shaderpath, const plString& entrypoint, plRHIShaderType type, const plString& model)
    : shaderPath(shaderPath)
    , entrypoint(entrypoint)
    , type(type)
    , model(model)
  //, blob{}
  //, reflection{}
  {
  }
};

struct plRHIInputLayoutDesc
{
  plUInt32 slot = 0;
  plString semanticName;
  plRHIResourceFormat::Enum format = plRHIResourceFormat::UNKNOWN;
  plUInt32 stride = 0;

  auto MakeTie() const
  {
    return std::tie(slot, semanticName, format, stride);
  }
};

enum class plRHIRenderPassLoadOp
{
  kLoad,
  kClear,
  kDontCare,
};

enum class plRHIRenderPassStoreOp
{
  kStore = 0,
  kDontCare,
};

struct plRHIRenderPassColorDesc
{
  plRHIResourceFormat::Enum format = plRHIResourceFormat::UNKNOWN;
  plRHIRenderPassLoadOp loadOp = plRHIRenderPassLoadOp::kLoad;
  plRHIRenderPassStoreOp storeOp = plRHIRenderPassStoreOp::kStore;

  auto MakeTie() const
  {
    return std::tie(format, loadOp, storeOp);
  }
};

struct plRHIRenderPassDepthStencilDesc
{
  plRHIResourceFormat::Enum format = plRHIResourceFormat::UNKNOWN;
  plRHIRenderPassLoadOp depthLoadOp = plRHIRenderPassLoadOp::kLoad;
  plRHIRenderPassStoreOp depthStoreOp = plRHIRenderPassStoreOp::kStore;
  plRHIRenderPassLoadOp stencilLoadOp = plRHIRenderPassLoadOp::kLoad;
  plRHIRenderPassStoreOp stencilStoreOp = plRHIRenderPassStoreOp::kStore;

  auto MakeTie() const
  {
    return std::tie(format, depthLoadOp, depthStoreOp, stencilLoadOp, stencilStoreOp);
  }
};

struct plRHIRenderPassDesc
{
  std::vector<plRHIRenderPassColorDesc> colors;
  plRHIRenderPassDepthStencilDesc depthStencil;
  plRHIResourceFormat::Enum shadingRateFormat = plRHIResourceFormat::UNKNOWN;
  plUInt32 sampleCount = 1;

  auto MakeTie() const
  {
    return std::tie(colors, depthStencil, shadingRateFormat, sampleCount);
  }
};

class plRHIRenderPass;

struct plRHIFramebufferDesc
{
  plSharedPtr<plRHIRenderPass> renderPass;
  plUInt32 width;
  plUInt32 height;
  std::vector<plSharedPtr<plRHIView>> colors;
  plSharedPtr<plRHIView> depthStencil;
  plSharedPtr<plRHIView> shadingRateImage;

  auto MakeTie() const
  {
    return std::tie(renderPass, width, height, colors, depthStencil, shadingRateImage);
  }
};

struct plRHIGraphicsPipelineDesc
{
  plSharedPtr<plRHIProgram> program;
  plSharedPtr<plRHIBindingSetLayout> layout;
  std::vector<plRHIInputLayoutDesc> input;
  plSharedPtr<plRHIRenderPass> renderPass;
  plRHIDepthStencilDesc depthStencilDesc;
  plRHIBlendDesc blendDesc;
  plRHIRasterizerDesc rasterizerDesc;

  auto MakeTie() const
  {
    return std::tie(program, layout, input, renderPass, depthStencilDesc, blendDesc, rasterizerDesc);
  }
};

struct plRHIComputePipelineDesc
{
  plSharedPtr<plRHIProgram> program;
  plSharedPtr<plRHIBindingSetLayout> layout;

  auto MakeTie() const
  {
    return std::tie(program, layout);
  }
};

enum class plRHIRayTracingShaderGroupType
{
  kGeneral,
  kTrianglesHitGroup,
  kProceduralHitGroup,
};

struct plRHIRayTracingShaderGroup
{
  plRHIRayTracingShaderGroupType type = plRHIRayTracingShaderGroupType::kGeneral;
  plUInt64 general = 0;
  plUInt64 closestHit = 0;
  plUInt64 anyHit = 0;
  plUInt64 intersection = 0;

  auto MakeTie() const
  {
    return std::tie(type, general, closestHit, anyHit, intersection);
  }
};

struct plRHIRayTracingPipelineDesc
{
  plSharedPtr<plRHIProgram> program;
  plSharedPtr<plRHIBindingSetLayout> layout;
  std::vector<plRHIRayTracingShaderGroup> groups;

  auto MakeTie() const
  {
    return std::tie(program, layout, groups);
  }
};

struct plRHIRayTracingShaderTable
{
  plSharedPtr<plRHIResource> resource;
  plUInt64 offset;
  plUInt64 size;
  plUInt64 stride;
};

struct plRHIRayTracingShaderTables
{
  plRHIRayTracingShaderTable raygen;
  plRHIRayTracingShaderTable miss;
  plRHIRayTracingShaderTable hit;
  plRHIRayTracingShaderTable callable;
};

struct plRHIBindKey
{
  plRHIShaderType shaderType = plRHIShaderType::kUnknown;
  plRHIViewType viewType = plRHIViewType::kUnknown;
  plUInt32 slot = 0;
  plUInt32 space = 0;
  plUInt32 count = 0;

  auto MakeTie() const
  {
    return std::tie(shaderType, viewType, slot, space, count);
  }

  bool operator==(const plRHIBindKey& other) const
  {
    return shaderType == other.shaderType &&
           viewType == other.viewType &&
           slot == other.slot &&
           space == other.space &&
           count == other.count;
  }
};

struct plRHIBindingDesc
{
  plRHIBindKey bindKey;
  plSharedPtr<plRHIView> view;

  auto MakeTie() const
  {
    return std::tie(bindKey, view);
  }
};

enum class plRHIReturnType
{
  kUnknown,
  kFloat,
  kUint,
  kInt,
  kDouble,
};

struct plRHIResourceBindingDesc
{
  plString name;
  plRHIViewType type;
  plUInt32 slot;
  plUInt32 space;
  plUInt32 count;
  plRHIViewDimension dimension;
  plRHIReturnType returnType;
  plUInt32 structureStride;
};

enum class plRHIPipelineType
{
  kGraphics,
  kCompute,
  kRayTracing,
};

struct plRHIBufferDesc
{
  plSharedPtr<plRHIResource> res;
  plRHIResourceFormat::Enum format = plRHIResourceFormat::UNKNOWN;
  plUInt32 count = 0;
  plUInt32 offset = 0;
};

enum class plRHIRaytracingInstanceFlags : plUInt32
{
  kNone = 0x0,
  kTriangleCullDisable = 0x1,
  kTriangleFrontCounterclockwise = 0x2,
  kForceOpaque = 0x4,
  kForceNonOpaque = 0x8
};

enum class plRHIRaytracingGeometryFlags
{
  kNone,
  kOpaque,
  kNoDuplicateAnyHitInvocation
};

struct plRHIRaytracingGeometryDesc
{
  plRHIBufferDesc vertex;
  plRHIBufferDesc index;
  plRHIRaytracingGeometryFlags flags = plRHIRaytracingGeometryFlags::kNone;
};

enum class plRHIMemoryType
{
  kDefault,
  kUpload,
  kReadback
};

struct plRHITextureOffset
{
  plInt32 x;
  plInt32 y;
  plInt32 z;
};

struct plRHITextureExtent3D
{
  plUInt32 width;
  plUInt32 height;
  plUInt32 depth;
};

struct plRHIBufferToTextureCopyRegion
{
  plUInt64 bufferOffset;
  plUInt32 bufferRowPitch;
  plUInt32 textureMipLevel;
  plUInt32 textureArrayLayer;
  plRHITextureOffset textureOffset;
  plRHITextureExtent3D textureExtent;
};

struct plRHITextureCreationDesc
{
  plRHITextureType type;
  plUInt32 bindFlags;
  plRHIResourceFormat::Enum format;
  plUInt32 sampleCount;
  int width;
  int height;
  int depth;
  int mipLevls;
};

struct plRHITextureCopyRegion
{
  plRHITextureExtent3D extent;
  plUInt32 srcMipLevel;
  plUInt32 srcArrayLayer;
  plRHITextureOffset srcOffset;
  plUInt32 dstMipLevel;
  plUInt32 dstArrayLayer;
  plRHITextureOffset dstOffset;
};

struct plRHIBufferCopyRegion
{
  plUInt64 srcOffset;
  plUInt64 dstOffset;
  plUInt64 numBytes;
};

struct plRHIRaytracingGeometryInstance
{
  plRHIFloat3X4 transform;
  plUInt32 instanceId : 24;
  plUInt32 instanceMask : 8;
  plUInt32 instanceOffset : 24;
  plRHIRaytracingInstanceFlags flags : 8;
  plUInt64 accelerationStructureHandle;
};

static_assert(sizeof(plRHIRaytracingGeometryInstance) == 64);

struct plRHIResourceBarrierDesc
{
  plSharedPtr<plRHIResource> resource;
  plRHIResourceState stateBefore;
  plRHIResourceState stateAfter;
  plUInt32 baseMipLevel = 0;
  plUInt32 levelCount = 1;
  plUInt32 baseArrayLayer = 0;
  plUInt32 layerCount = 1;
};

enum class plRHIShadingRate : plUInt8
{
  k1x1 = 0,
  k1x2 = 0x1,
  k2x1 = 0x4,
  k2x2 = 0x5,
  k2x4 = 0x6,
  k4x2 = 0x9,
  k4x4 = 0xa,
};

enum class plRHIShadingRateCombiner
{
  kPassthrough = 0,
  kOverride = 1,
  kMin = 2,
  kMax = 3,
  kSum = 4,
};

struct plRHIRaytracingASPrebuildInfo
{
  plUInt64 accelerationStructureSize = 0;
  plUInt64 buildScratchDataSize = 0;
  plUInt64 updateScratchDataSize = 0;
};

enum class plRHIAccelerationStructureType
{
  kTopLevel,
  kBottomLevel,
};

enum class plRHICommandListType
{
  kGraphics,
  kCompute,
  kCopy,
};

struct plRHIClearDesc
{
  std::vector<plColor> colors;
  float depth = 1.0f;
  plUInt8 stencil = 0;
};

enum class plRHICopyAccelerationStructureMode
{
  kClone,
  kCompact,
};

namespace enumClass
{
  enum plRHIBuildAccelerationStructureFlags
  {
    kNone = 0,
    kAllowUpdate = 1 << 0,
    kAllowCompaction = 1 << 1,
    kPreferFastTrace = 1 << 2,
    kPreferFastBuild = 1 << 3,
    kMinimizeMemory = 1 << 4,
  };
}

using plRHIBuildAccelerationStructureFlags = enumClass::plRHIBuildAccelerationStructureFlags;
ENABLE_BITMASK_OPERATORS(plRHIBuildAccelerationStructureFlags);

struct plRHIDrawIndirectCommand
{
  plUInt32 vertexCount;
  plUInt32 instanceCount;
  plUInt32 firstVertex;
  plUInt32 firstInstance;
};

struct plRHIDrawIndexedIndirectCommand
{
  plUInt32 indexCount;
  plUInt32 instanceCount;
  plUInt32 firstIndex;
  plInt32 vertexOffset;
  plUInt32 firstInstance;
};

struct plRHIDispatchIndirectCommand
{
  plUInt32 threadGroupCountX;
  plUInt32 threadGroupCountY;
  plUInt32 threadGroupCountZ;
};

using plRHIIndirectCountType = plUInt32;

constexpr plUInt64 kAccelerationStructureAlignment = 256;

enum class plRHIQueryHeapType
{
  kAccelerationStructureCompactedSize
};

template <typename T>
auto operator<(const T& l, const T& r) -> std::enable_if_t<std::is_same_v<decltype(l.MakeTie() < r.MakeTie()), bool>, bool>
{
  return l.MakeTie() < r.MakeTie();
}

enum class plRHIApiType
{
  kVulkan,
  kDX12,
  kMetal
};

struct plRHIRenderDeviceDesc
{
  plRHIApiType apiType = plRHIApiType::kVulkan;
  bool vsync = true;
  bool roundFps = false;
  plUInt32 requiredGpuIndex = 0;
  plUInt32 frameCount = 3;
};
