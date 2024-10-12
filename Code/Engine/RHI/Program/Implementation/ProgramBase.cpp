#include <RHI/Program/ProgramBase.h>
#include <deque>

plRHIProgramBase::plRHIProgramBase(const std::vector<plSharedPtr<plRHIShader>>& shaders)
    : m_Shaders(shaders)
{
    for (const auto& shader : m_Shaders)
    {
        m_ShadersByType[shader->GetType()] = shader;
        decltype(auto) bindings = shader->GetBindings();
        m_Bindings.insert(m_Bindings.begin(), bindings.begin(), bindings.end());

        decltype(auto) reflection = shader->GetReflection();
        decltype(auto) shaderEntryPoints = reflection->GetEntryPoints();
        m_EntryPoints.insert(m_EntryPoints.end(), shaderEntryPoints.begin(), shaderEntryPoints.end());
    }
}

bool plRHIProgramBase::HasShader(plRHIShaderType type) const
{
    return m_ShadersByType.count(type);
}

plSharedPtr<plRHIShader> plRHIProgramBase::GetShader(plRHIShaderType type) const
{
    auto it = m_ShadersByType.find(type);
    if (it != m_ShadersByType.end())
    {
        return it->second;
    }
    return {};
}

const std::vector<plSharedPtr<plRHIShader>>& plRHIProgramBase::GetShaders() const
{
    return m_Shaders;
}

const std::vector<plRHIBindKey>& plRHIProgramBase::GetBindings() const
{
    return m_Bindings;
}

const std::vector<plRHIEntryPoint>& plRHIProgramBase::GetEntryPoints() const
{
    return m_EntryPoints;
}
