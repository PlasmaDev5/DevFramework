#include <RHIDX12/RHIDX12PCH.h>

#define INITGUID

#ifndef _WIN32
#  include <wsl/winadapter.h>
#endif

#include <directx/d3d12.h>
#include <directx/dxcore.h>

PL_STATICLINK_LIBRARY(RHIDX12)
{
  if (bReturn)
    return;
}
