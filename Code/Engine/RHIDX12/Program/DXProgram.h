#pragma once
#include <RHI/Program/ProgramBase.h>
#include <RHI/Shader/Shader.h>
#include <set>
#include <vector>
#include <directx/d3d12.h>
#include <wrl.h>
using namespace Microsoft::WRL;

class plDXDevice;

class plDXProgram : public plRHIProgramBase
{
public:
    plDXProgram(plDXDevice& device, const std::vector<plSharedPtr<plRHIShader>>& shaders);
};
