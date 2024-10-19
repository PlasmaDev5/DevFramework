#pragma once

#include <Foundation/Basics.h>
#include <RHI/RHIDLL.h>

// Configure the DLL Import/Export Define
#if PL_ENABLED(PL_COMPILE_ENGINE_AS_DLL)
#  ifdef BUILDSYSTEM_BUILDING_RHIDX12_LIB
#    define PL_RHIDX12_DLL __declspec(dllexport)
#  else
#    define PL_RHIDX12_DLL __declspec(dllimport)
#  endif
#else
#  define PL_RHIDX12_DLL
#endif