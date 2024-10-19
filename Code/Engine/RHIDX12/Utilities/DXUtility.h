//
// Copyright (c) Microsoft. All rights reserved.
// This code is licensed under the MIT License (MIT).
// THIS CODE IS PROVIDED *AS IS* WITHOUT WARRANTY OF
// ANY KIND, EITHER EXPRESS OR IMPLIED, INCLUDING ANY
// IMPLIED WARRANTIES OF FITNESS FOR A PARTICULAR
// PURPOSE, MERCHANTABILITY, OR NON-INFRINGEMENT.
//
// Developed by Minigraph
//
// Author:  James Stanard
//

#pragma once

#include <Windows.h>
#include <cstdarg>
#include <cstdint>
#include <cstdio>

#include <RHIDX12/RHIDX12DLL.h>
#include <dxgiformat.h>
#include <sal.h>

namespace plDXUtils
{
  DXGI_FORMAT ToDXGIFormat(plRHIResourceFormat::Enum value);

  plRHIResourceFormat::Enum ToEngineFormat(DXGI_FORMAT value);

  void GetSurfaceInfo(
    _In_ size_t width,
    _In_ size_t height,
    _In_ plRHIResourceFormat::Enum format,
    _Out_opt_ size_t* outNumBytes,
    _Out_opt_ size_t* outRowBytes,
    _Out_opt_ size_t* outNumRows);

  DXGI_FORMAT MakeTypelessDepthStencil(DXGI_FORMAT format);
  bool IsTypelessDepthStencil(DXGI_FORMAT format);
  DXGI_FORMAT DepthReadFromTypeless(DXGI_FORMAT format);
  DXGI_FORMAT StencilReadFromTypeless(DXGI_FORMAT format);
  DXGI_FORMAT DepthStencilFromTypeless(DXGI_FORMAT format);
}
