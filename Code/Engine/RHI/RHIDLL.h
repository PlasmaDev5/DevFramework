#pragma once

#include <Foundation/Basics.h>
#include <Foundation/Reflection/Reflection.h>
#include <Foundation/Types/Id.h>
#include <Foundation/Types/RefCounted.h>
#include <Foundation/Types/SharedPtr.h>

// Configure the DLL Import/Export Define
#if PL_ENABLED(PL_COMPILE_ENGINE_AS_DLL)
#  ifdef BUILDSYSTEM_BUILDING_RHI_LIB
#    define PL_RHI_DLL __declspec(dllexport)
#  else
#    define PL_RHI_DLL __declspec(dllimport)
#  endif
#else
#  define PL_RHI_DLL
#endif

class plRHIAdapter;
class plRHIBindingSet;
class plRHIBindingSetLayout;
class plRHICommandList;
class plRHICommandQueue;
class plRHIDevice;
class plRHIFence;
class plRHIFramebuffer;
class plRHIFramebufferBase;
class plRHIInstance;
class plRHIQueryInterface;
class plRHIMemory;
class plRHIPipeline;
class plRHIProgram;
class plRHIProgramBase;
class plRHIQueryHeap;
class plRHIRenderPass;
class plRHIResource;
class plRHIResourceBase;
class plRHIResourceStateTracker;
class plRHIShader;
class plRHIShaderBase;
class plRHIShaderReflection;
class plRHISwapchain;
class plRHIView;

// Handles
namespace plRHI
{
  typedef plGenericId<16, 16> pl16_16Id;
  typedef plGenericId<18, 14> pl18_14Id;
  typedef plGenericId<20, 12> pl20_12Id;
} // namespace plRHI

class plRHIMemoryHandle
{
  PL_DECLARE_HANDLE_TYPE(plRHIMemoryHandle, plRHI::pl20_12Id);

  friend class plRHIDevice;
};

class plRHICommandQueueHandle
{
  PL_DECLARE_HANDLE_TYPE(plRHICommandQueueHandle, plRHI::pl16_16Id);

  friend class plRHIDevice;
};

class plRHISwapChainHandle
{
  PL_DECLARE_HANDLE_TYPE(plRHISwapChainHandle, plRHI::pl16_16Id);

  friend class plRHIDevice;
};

class plRHICommandListHandle
{
  PL_DECLARE_HANDLE_TYPE(plRHICommandListHandle, plRHI::pl18_14Id);

  friend class plRHIDevice;
};

class plRHIFenceHandle
{
  PL_DECLARE_HANDLE_TYPE(plRHIFenceHandle, plRHI::pl20_12Id);

  friend class plRHIDevice;
};

class plRHIResourceHandle
{
  PL_DECLARE_HANDLE_TYPE(plRHIResourceHandle, plRHI::pl18_14Id);

  friend class plRHIDevice;
};

class plRHIViewHandle
{
  PL_DECLARE_HANDLE_TYPE(plRHIViewHandle, plRHI::pl18_14Id);

  friend class plRHIDevice;
};

class plRHIBindingSetLayoutHandle
{
  PL_DECLARE_HANDLE_TYPE(plRHIBindingSetLayoutHandle, plRHI::pl18_14Id);

  friend class plRHIDevice;
};

class plRHIBindingSetHandle
{
  PL_DECLARE_HANDLE_TYPE(plRHIBindingSetHandle, plRHI::pl18_14Id);

  friend class plRHIDevice;
};

class plRHIRenderPassHandle
{
  PL_DECLARE_HANDLE_TYPE(plRHIRenderPassHandle, plRHI::pl20_12Id);

  friend class plRHIDevice;
};

class plRHIFramebufferHandle
{
  PL_DECLARE_HANDLE_TYPE(plRHIFramebufferHandle, plRHI::pl20_12Id);

  friend class plRHIDevice;
};

class plRHIShaderHandle
{
  PL_DECLARE_HANDLE_TYPE(plRHIShaderHandle, plRHI::pl18_14Id);

  friend class plRHIDevice;
};

class plRHIProgramHandle
{
  PL_DECLARE_HANDLE_TYPE(plRHIProgramHandle, plRHI::pl18_14Id);

  friend class plRHIDevice;
};

class plRHIPipelineHandle
{
  PL_DECLARE_HANDLE_TYPE(plRHIPipelineHandle, plRHI::pl18_14Id);

  friend class plRHIDevice;
};

class plRHIQueryHeapHandle
{
  PL_DECLARE_HANDLE_TYPE(plRHIQueryHeapHandle, plRHI::pl20_12Id);

  friend class plRHIDevice;
};

struct PL_RHI_DLL plRHIResourceFormat
{
  typedef plUInt32 StorageType;

  enum Enum
  {
    UNKNOWN,
    R32G32B32A32_FLOAT,
    R32G32B32A32_UINT,
    R32G32B32A32_SINT,

    R32G32B32_FLOAT,
    R32G32B32_UINT,
    R32G32B32_SINT,

    R16G16B16A16_FLOAT,
    R16G16B16A16_UNORM,
    R16G16B16A16_UINT,
    R16G16B16A16_SNORM,
    R16G16B16A16_SINT,

    R32G32_FLOAT,
    R32G32_UINT,
    R32G32_SINT,
    R32G8X24_TYPELESS,    // depth (32-bit) + stencil (8-bit) + shader resource (32-bit)
    D32_FLOAT_S8X24_UINT, // depth (32-bit) + stencil (8-bit)

    R10G10B10A2_UNORM,
    R10G10B10A2_UINT,
    R11G11B10_FLOAT,
    R8G8B8A8_UNORM,
    R8G8B8A8_UNORM_SRGB,
    R8G8B8A8_UINT,
    R8G8B8A8_SNORM,
    R8G8B8A8_SINT,
    B8G8R8A8_UNORM,
    B8G8R8A8_UNORM_SRGB,
    R16G16_FLOAT,
    R16G16_UNORM,
    R16G16_UINT,
    R16G16_SNORM,
    R16G16_SINT,
    R32_TYPELESS, // depth (32-bit) + shader resource (32-bit)
    D32_FLOAT,    // depth (32-bit)
    R32_FLOAT,
    R32_UINT,
    R32_SINT,
    R24G8_TYPELESS,    // depth (24-bit) + stencil (8-bit) + shader resource (24-bit)
    D24_UNORM_S8_UINT, // depth (24-bit) + stencil (8-bit)

    R8G8_UNORM,
    R8G8_UINT,
    R8G8_SNORM,
    R8G8_SINT,
    R16_TYPELESS, // depth (16-bit) + shader resource (16-bit)
    R16_FLOAT,
    D16_UNORM, // depth (16-bit)
    R16_UNORM,
    R16_UINT,
    R16_SNORM,
    R16_SINT,

    R8_UNORM,
    R8_UINT,
    R8_SNORM,
    R8_SINT,

    BC1_UNORM,
    BC1_UNORM_SRGB,
    BC2_UNORM,
    BC2_UNORM_SRGB,
    BC3_UNORM,
    BC3_UNORM_SRGB,
    BC4_UNORM,
    BC4_SNORM,
    BC5_UNORM,
    BC5_SNORM,
    BC6H_UF16,
    BC6H_SF16,
    BC7_UNORM,
    BC7_UNORM_SRGB,

    ENUM_COUNT,

    Default = R8G8B8A8_UNORM_SRGB //RGBAUByteNormalizedsRGB
  };

  struct FormatInfo
  {
    plUInt8 BlockSize;
    plVec3U32 BlockExtent;
    plUInt8 Components;
  };

  // General format Meta-Informations:

  PL_ALWAYS_INLINE static plUInt32 GetFormatStride(plRHIResourceFormat::Enum value);
  PL_ALWAYS_INLINE static plUInt32 GetBlockSize(plRHIResourceFormat::Enum value);
  PL_ALWAYS_INLINE static plVec3U32 GetBlockExtent(plRHIResourceFormat::Enum value);
  PL_ALWAYS_INLINE static bool IsFormatUnorm(plRHIResourceFormat::Enum value);
  PL_ALWAYS_INLINE static bool IsFormatBlockCompressed(plRHIResourceFormat::Enum value);
  PL_ALWAYS_INLINE static bool IsFormatStencilSupport(plRHIResourceFormat::Enum value);
  PL_ALWAYS_INLINE static void GetInfo(plUInt32 width,
    plUInt32 height,
    plRHIResourceFormat::Enum format,
    plUInt32& numBytes,
    plUInt32& rowBytes,
    plUInt32& numRows,
    plUInt32 alignment);
  PL_ALWAYS_INLINE static void GetInfo(plUInt32 width,
    plUInt32 height,
    plRHIResourceFormat::Enum format,
    plUInt32& numBytes,
    plUInt32& rowBytes);

  PL_ALWAYS_INLINE static plUInt32 Align(plUInt32 size, plUInt32 alignment);
};

PL_DECLARE_REFLECTABLE_TYPE(PL_RHI_DLL, plRHIResourceFormat);

PL_ALWAYS_INLINE plUInt32 plRHIResourceFormat::GetFormatStride(plRHIResourceFormat::Enum value)
{
  switch (value)
  {

    case R32G32B32A32_FLOAT:
    case R32G32B32A32_UINT:
    case R32G32B32A32_SINT:
    case BC1_UNORM:
    case BC1_UNORM_SRGB:
    case BC2_UNORM:
    case BC2_UNORM_SRGB:
    case BC3_UNORM:
    case BC3_UNORM_SRGB:
    case BC4_SNORM:
    case BC4_UNORM:
    case BC5_SNORM:
    case BC5_UNORM:
    case BC6H_UF16:
    case BC6H_SF16:
    case BC7_UNORM:
    case BC7_UNORM_SRGB:
      return 16;

    case R32G32B32_FLOAT:
    case R32G32B32_UINT:
    case R32G32B32_SINT:
      return 12;

    case R16G16B16A16_FLOAT:
    case R16G16B16A16_UNORM:
    case R16G16B16A16_UINT:
    case R16G16B16A16_SNORM:
    case R16G16B16A16_SINT:
      return 8;

    case R32G32_FLOAT:
    case R32G32_UINT:
    case R32G32_SINT:
    case R32G8X24_TYPELESS:
    case D32_FLOAT_S8X24_UINT:
      return 8;

    case R10G10B10A2_UNORM:
    case R10G10B10A2_UINT:
    case R11G11B10_FLOAT:
    case R8G8B8A8_UNORM:
    case R8G8B8A8_UNORM_SRGB:
    case R8G8B8A8_UINT:
    case R8G8B8A8_SNORM:
    case R8G8B8A8_SINT:
    case B8G8R8A8_UNORM:
    case B8G8R8A8_UNORM_SRGB:
    case R16G16_FLOAT:
    case R16G16_UNORM:
    case R16G16_UINT:
    case R16G16_SNORM:
    case R16G16_SINT:
    case R32_TYPELESS:
    case D32_FLOAT:
    case R32_FLOAT:
    case R32_UINT:
    case R32_SINT:
    case R24G8_TYPELESS:
    case D24_UNORM_S8_UINT:
      return 4;

    case R8G8_UNORM:
    case R8G8_UINT:
    case R8G8_SNORM:
    case R8G8_SINT:
    case R16_TYPELESS:
    case R16_FLOAT:
    case D16_UNORM:
    case R16_UNORM:
    case R16_UINT:
    case R16_SNORM:
    case R16_SINT:
      return 2;

    case R8_UNORM:
    case R8_UINT:
    case R8_SNORM:
    case R8_SINT:
      return 1;


    default:
      PL_ASSERT_ALWAYS(false, "Unknown format"); // didn't catch format!
      break;
  }

  return 16;
}

PL_ALWAYS_INLINE plUInt32 plRHIResourceFormat::GetBlockSize(plRHIResourceFormat::Enum value)
{
  switch (value)
  {

    case BC1_UNORM:
    case BC1_UNORM_SRGB:
    case BC4_UNORM:
    case BC4_SNORM:
      return 8;

    case BC2_UNORM:
    case BC2_UNORM_SRGB:
    case BC3_UNORM:
    case BC3_UNORM_SRGB:
    case BC5_UNORM:
    case BC5_SNORM:
    case BC6H_UF16:
    case BC6H_SF16:
    case BC7_UNORM:
    case BC7_UNORM_SRGB:
      return 16;


    default:
      PL_ASSERT_ALWAYS(false, "Invalid block format"); // didn't catch format!
      break;
  }

  return 0;
}

PL_ALWAYS_INLINE plVec3U32 plRHIResourceFormat::GetBlockExtent(plRHIResourceFormat::Enum value)
{
  static plMap<plRHIResourceFormat::Enum, FormatInfo> s_FormatInfos;
  if (s_FormatInfos.GetCount() == 0)
  {
    s_FormatInfos.Insert(plRHIResourceFormat::R32G32B32A32_FLOAT, FormatInfo{16, plVec3U32(1, 1, 1), 3});
    s_FormatInfos.Insert(plRHIResourceFormat::R32G32B32A32_UINT, FormatInfo{16, plVec3U32(1, 1, 1), 3});
    s_FormatInfos.Insert(plRHIResourceFormat::R32G32B32A32_SINT, FormatInfo{16, plVec3U32(1, 1, 1), 3});

    s_FormatInfos.Insert(plRHIResourceFormat::R32G32B32_FLOAT, FormatInfo{12, plVec3U32(1, 1, 1), 3});
    s_FormatInfos.Insert(plRHIResourceFormat::R32G32B32_UINT, FormatInfo{12, plVec3U32(1, 1, 1), 3});
    s_FormatInfos.Insert(plRHIResourceFormat::R32G32B32_SINT, FormatInfo{12, plVec3U32(1, 1, 1), 3});

    s_FormatInfos.Insert(plRHIResourceFormat::R16G16B16A16_FLOAT, FormatInfo{8, plVec3U32(1, 1, 1), 4});
    s_FormatInfos.Insert(plRHIResourceFormat::R16G16B16A16_UNORM, FormatInfo{8, plVec3U32(1, 1, 1), 4});
    s_FormatInfos.Insert(plRHIResourceFormat::R16G16B16A16_UINT, FormatInfo{8, plVec3U32(1, 1, 1), 4});
    s_FormatInfos.Insert(plRHIResourceFormat::R16G16B16A16_SNORM, FormatInfo{8, plVec3U32(1, 1, 1), 4});
    s_FormatInfos.Insert(plRHIResourceFormat::R16G16B16A16_SINT, FormatInfo{8, plVec3U32(1, 1, 1), 4});

    s_FormatInfos.Insert(plRHIResourceFormat::R32G32_FLOAT, FormatInfo{8, plVec3U32(1, 1, 1), 2});
    s_FormatInfos.Insert(plRHIResourceFormat::R32G32_UINT, FormatInfo{8, plVec3U32(1, 1, 1), 2});
    s_FormatInfos.Insert(plRHIResourceFormat::R32G32_SINT, FormatInfo{8, plVec3U32(1, 1, 1), 2});

    //s_FormatInfos.Insert(plRHIResourceFormat::R32G8X24_TYPELESS, FormatInfo{0, plVec3U32(), 0});
    s_FormatInfos.Insert(plRHIResourceFormat::D32_FLOAT_S8X24_UINT, FormatInfo{5, plVec3U32(1, 1, 1), 2});

    s_FormatInfos.Insert(plRHIResourceFormat::R10G10B10A2_UNORM, FormatInfo{4, plVec3U32(1, 1, 1), 4});
    s_FormatInfos.Insert(plRHIResourceFormat::R10G10B10A2_UINT, FormatInfo{4, plVec3U32(1, 1, 1), 4});

    s_FormatInfos.Insert(plRHIResourceFormat::R11G11B10_FLOAT, FormatInfo{4, plVec3U32(1, 1, 1), 3});
    s_FormatInfos.Insert(plRHIResourceFormat::R8G8B8A8_UNORM, FormatInfo{4, plVec3U32(1, 1, 1), 4});
    s_FormatInfos.Insert(plRHIResourceFormat::R8G8B8A8_UNORM_SRGB, FormatInfo{4, plVec3U32(1, 1, 1), 4});
    s_FormatInfos.Insert(plRHIResourceFormat::R8G8B8A8_UINT, FormatInfo{4, plVec3U32(1, 1, 1), 4});
    s_FormatInfos.Insert(plRHIResourceFormat::R8G8B8A8_SNORM, FormatInfo{4, plVec3U32(1, 1, 1), 4});
    s_FormatInfos.Insert(plRHIResourceFormat::R8G8B8A8_SINT, FormatInfo{4, plVec3U32(1, 1, 1), 4});
    s_FormatInfos.Insert(plRHIResourceFormat::B8G8R8A8_UNORM, FormatInfo{4, plVec3U32(1, 1, 1), 4});
    s_FormatInfos.Insert(plRHIResourceFormat::B8G8R8A8_UNORM_SRGB, FormatInfo{4, plVec3U32(1, 1, 1), 4});
    s_FormatInfos.Insert(plRHIResourceFormat::R16G16_FLOAT, FormatInfo{4, plVec3U32(1, 1, 1), 2});
    s_FormatInfos.Insert(plRHIResourceFormat::R16G16_UNORM, FormatInfo{4, plVec3U32(1, 1, 1), 2});
    s_FormatInfos.Insert(plRHIResourceFormat::R16G16_UINT, FormatInfo{4, plVec3U32(1, 1, 1), 2});
    s_FormatInfos.Insert(plRHIResourceFormat::R16G16_SNORM, FormatInfo{4, plVec3U32(1, 1, 1), 2});
    s_FormatInfos.Insert(plRHIResourceFormat::R16G16_SINT, FormatInfo{4, plVec3U32(1, 1, 1), 4});
    //s_FormatInfos.Insert(plRHIResourceFormat::R32_TYPELESS, FormatInfo{0, plVec3U32(), 0});
    s_FormatInfos.Insert(plRHIResourceFormat::D32_FLOAT, FormatInfo{4, plVec3U32(1, 1, 1), 1});
    s_FormatInfos.Insert(plRHIResourceFormat::R32_FLOAT, FormatInfo{4, plVec3U32(1, 1, 1), 1});
    s_FormatInfos.Insert(plRHIResourceFormat::R32_UINT, FormatInfo{4, plVec3U32(1, 1, 1), 1});
    s_FormatInfos.Insert(plRHIResourceFormat::R32_SINT, FormatInfo{4, plVec3U32(1, 1, 1), 1});
    //s_FormatInfos.Insert(plRHIResourceFormat::R24G8_TYPELESS, FormatInfo{0, plVec3U32(), 0});
    s_FormatInfos.Insert(plRHIResourceFormat::D24_UNORM_S8_UINT, FormatInfo{4, plVec3U32(1, 1, 1), 2});

    s_FormatInfos.Insert(plRHIResourceFormat::R8G8_UNORM, FormatInfo{2, plVec3U32(1, 1, 1), 2});
    s_FormatInfos.Insert(plRHIResourceFormat::R8G8_UINT, FormatInfo{2, plVec3U32(1, 1, 1), 2});
    s_FormatInfos.Insert(plRHIResourceFormat::R8G8_SNORM, FormatInfo{2, plVec3U32(1, 1, 1), 2});
    s_FormatInfos.Insert(plRHIResourceFormat::R8G8_SINT, FormatInfo{2, plVec3U32(1, 1, 1), 2});
    //s_FormatInfos.Insert(plRHIResourceFormat::R16_TYPELESS, FormatInfo{0, plVec3U32(), 0});
    s_FormatInfos.Insert(plRHIResourceFormat::R16_FLOAT, FormatInfo{2, plVec3U32(1, 1, 1), 1});
    s_FormatInfos.Insert(plRHIResourceFormat::D16_UNORM, FormatInfo{2, plVec3U32(1, 1, 1), 1});
    s_FormatInfos.Insert(plRHIResourceFormat::R16_UNORM, FormatInfo{2, plVec3U32(1, 1, 1), 1});
    s_FormatInfos.Insert(plRHIResourceFormat::R16_UINT, FormatInfo{2, plVec3U32(1, 1, 1), 1});
    s_FormatInfos.Insert(plRHIResourceFormat::R16_SNORM, FormatInfo{2, plVec3U32(1, 1, 1), 1});
    s_FormatInfos.Insert(plRHIResourceFormat::R16_SINT, FormatInfo{2, plVec3U32(1, 1, 1), 1});

    s_FormatInfos.Insert(plRHIResourceFormat::R8_UNORM, FormatInfo{1, plVec3U32(1,1,1), 1});
    s_FormatInfos.Insert(plRHIResourceFormat::R8_UINT, FormatInfo{1, plVec3U32(1, 1, 1), 1});
    s_FormatInfos.Insert(plRHIResourceFormat::R8_SNORM, FormatInfo{1, plVec3U32(1, 1, 1), 1});
    s_FormatInfos.Insert(plRHIResourceFormat::R8_SINT, FormatInfo{1, plVec3U32(1, 1, 1), 1});

    s_FormatInfos.Insert(plRHIResourceFormat::BC1_UNORM, FormatInfo{8, plVec3U32(4,4,1), 4});
    s_FormatInfos.Insert(plRHIResourceFormat::BC1_UNORM_SRGB, FormatInfo{8, plVec3U32(4,4,1), 4});
    s_FormatInfos.Insert(plRHIResourceFormat::BC2_UNORM, FormatInfo{16, plVec3U32(4,4,1), 4});
    s_FormatInfos.Insert(plRHIResourceFormat::BC2_UNORM_SRGB, FormatInfo{16, plVec3U32(4, 4, 1), 4});
    s_FormatInfos.Insert(plRHIResourceFormat::BC3_UNORM, FormatInfo{16, plVec3U32(4, 4, 1), 4});
    s_FormatInfos.Insert(plRHIResourceFormat::BC3_UNORM_SRGB, FormatInfo{16, plVec3U32(4, 4, 1), 4});
    s_FormatInfos.Insert(plRHIResourceFormat::BC4_UNORM, FormatInfo{8, plVec3U32(4,4,1), 1});
    s_FormatInfos.Insert(plRHIResourceFormat::BC4_SNORM, FormatInfo{8, plVec3U32(4, 4, 1), 1});
    s_FormatInfos.Insert(plRHIResourceFormat::BC5_UNORM, FormatInfo{16, plVec3U32(4,4,1), 2});
    s_FormatInfos.Insert(plRHIResourceFormat::BC5_SNORM, FormatInfo{16, plVec3U32(4, 4, 1), 2});
    s_FormatInfos.Insert(plRHIResourceFormat::BC6H_UF16, FormatInfo{16, plVec3U32(4,4,1), 3});
    s_FormatInfos.Insert(plRHIResourceFormat::BC6H_SF16, FormatInfo{16, plVec3U32(4,4,1), 3});
    s_FormatInfos.Insert(plRHIResourceFormat::BC7_UNORM, FormatInfo{16, plVec3U32(4,4,1), 3});
    s_FormatInfos.Insert(plRHIResourceFormat::BC7_UNORM_SRGB, FormatInfo{16, plVec3U32(4,4,1), 3});
  }

  if (s_FormatInfos.Contains(value))
  {
    return s_FormatInfos[value].BlockExtent;
  }

  return {};
}

PL_ALWAYS_INLINE bool plRHIResourceFormat::IsFormatUnorm(plRHIResourceFormat::Enum value)
{
  switch (value)
  {
    case R16G16B16A16_UNORM:
    case R10G10B10A2_UNORM:
    case R8G8B8A8_UNORM:
    case R8G8B8A8_UNORM_SRGB:
    case B8G8R8A8_UNORM:
    case B8G8R8A8_UNORM_SRGB:
    case R16G16_UNORM:
    case D24_UNORM_S8_UINT:
    case R8G8_UNORM:
    case D16_UNORM:
    case R16_UNORM:
    case R8_UNORM:
      return true;
  }

  return false;
}

PL_ALWAYS_INLINE bool plRHIResourceFormat::IsFormatBlockCompressed(plRHIResourceFormat::Enum value)
{
  switch (value)
  {
    case BC1_UNORM:
    case BC1_UNORM_SRGB:
    case BC2_UNORM:
    case BC2_UNORM_SRGB:
    case BC3_UNORM:
    case BC3_UNORM_SRGB:
    case BC4_UNORM:
    case BC4_SNORM:
    case BC5_UNORM:
    case BC5_SNORM:
    case BC6H_UF16:
    case BC6H_SF16:
    case BC7_UNORM:
    case BC7_UNORM_SRGB:
      return true;
  }

  return false;
}

PL_ALWAYS_INLINE bool plRHIResourceFormat::IsFormatStencilSupport(plRHIResourceFormat::Enum value)
{
  switch (value)
  {
    case R32G8X24_TYPELESS:
    case D32_FLOAT_S8X24_UINT:
    case R24G8_TYPELESS:
    case D24_UNORM_S8_UINT:
      return true;
  }

  return false;
}


PL_ALWAYS_INLINE void plRHIResourceFormat::GetInfo(plUInt32 width,
  plUInt32 height,
  plRHIResourceFormat::Enum format,
  plUInt32& numBytes,
  plUInt32& rowBytes,
  plUInt32& numRows,
  plUInt32 alignment)
{
  if (IsFormatBlockCompressed(format))
  {
    rowBytes = GetBlockSize(format) * ((width + GetBlockExtent(format).x - 1) / GetBlockExtent(format).x);
    rowBytes = Align(rowBytes, alignment);
    numRows = ((height + GetBlockExtent(format).y - 1) / GetBlockExtent(format).y);
  }
  else
  {
    rowBytes = width * GetFormatStride(format);
    rowBytes = Align(rowBytes, alignment);
    numRows = height;
  }
  numBytes = rowBytes * numRows;
}

PL_ALWAYS_INLINE void plRHIResourceFormat::GetInfo(plUInt32 width,
  plUInt32 height,
  plRHIResourceFormat::Enum format,
  plUInt32& numBytes,
  plUInt32& rowBytes)
{
  plUInt32 numRows = 0;
  plUInt32 alignment = 1;
  return GetInfo(width, height, format, numBytes, rowBytes, numRows, alignment);
}

PL_ALWAYS_INLINE plUInt32 plRHIResourceFormat::Align(plUInt32 size, plUInt32 alignment)
{
  return (size + (alignment - 1)) & ~(alignment - 1);
}

struct PL_RHI_DLL plRHIFloat3X4
{
  union
  {
    struct
    {
      float _11, _12, _13, _14;
      float _21, _22, _23, _24;
      float _31, _32, _33, _34;
    };
    float m[3][4];
    float f[12];
  };

  plRHIFloat3X4() = default;

  plRHIFloat3X4(const plRHIFloat3X4&) = default;
  plRHIFloat3X4& operator=(const plRHIFloat3X4&) = default;

  plRHIFloat3X4(plRHIFloat3X4&&) = default;
  plRHIFloat3X4& operator=(plRHIFloat3X4&&) = default;

  constexpr plRHIFloat3X4(float m00, float m01, float m02, float m03,
    float m10, float m11, float m12, float m13,
    float m20, float m21, float m22, float m23)
    : _11(m00)
    , _12(m01)
    , _13(m02)
    , _14(m03)
    , _21(m10)
    , _22(m11)
    , _23(m12)
    , _24(m13)
    , _31(m20)
    , _32(m21)
    , _33(m22)
    , _34(m23)
  {
  }
  explicit plRHIFloat3X4(const float* pArray);

  float operator()(size_t Row, size_t Column) const { return m[Row][Column]; }
  float& operator()(size_t Row, size_t Column) { return m[Row][Column]; }
};

inline plRHIFloat3X4::plRHIFloat3X4(
  const float* pArray)
{
  PL_ASSERT_ALWAYS(pArray != nullptr, "The float pointer passed should not be null.");

  m[0][0] = pArray[0];
  m[0][1] = pArray[1];
  m[0][2] = pArray[2];
  m[0][3] = pArray[3];

  m[1][0] = pArray[4];
  m[1][1] = pArray[5];
  m[1][2] = pArray[6];
  m[1][3] = pArray[7];

  m[2][0] = pArray[8];
  m[2][1] = pArray[9];
  m[2][2] = pArray[10];
  m[2][3] = pArray[11];
}
