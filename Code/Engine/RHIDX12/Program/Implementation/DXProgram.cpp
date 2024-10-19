#include <RHIDX12/Program/DXProgram.h>
#include <RHIDX12/Device/DXDevice.h>
#include <RHIDX12/View/DXView.h>
#include <RHIDX12/BindingSet/DXBindingSet.h>
#include <deque>
#include <set>
#include <stdexcept>
#include <directx/d3dx12.h>

plDXProgram::plDXProgram(plDXDevice& device, const std::vector<plSharedPtr<plRHIShader>>& shaders)
    : plRHIProgramBase(shaders)
{
}
