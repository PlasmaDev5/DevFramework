#include <RHIShaderCompilerHLSL/MSLConverter.h>
#include <RHIShaderCompilerHLSL/Compiler.h>
#include <spirv_msl.hpp>

plMap<plString, plUInt32> ParseBindings(const spirv_cross::CompilerMSL& compiler)
{
  plMap<plString, plUInt32> mapping;
  spirv_cross::ShaderResources resources = compiler.get_shader_resources();
  auto enumerate_resources = [&](const spirv_cross::SmallVector<spirv_cross::Resource>& resources)
  {
    for (const auto& resource : resources)
    {
      plString name = compiler.get_name(resource.id).c_str();
      plUInt32 index = compiler.get_automatic_msl_resource_binding(resource.id);
      mapping[name] = index;
    }
  };
  enumerate_resources(resources.uniform_buffers);
  enumerate_resources(resources.storage_buffers);
  enumerate_resources(resources.storage_images);
  enumerate_resources(resources.separate_images);
  enumerate_resources(resources.separate_samplers);
  enumerate_resources(resources.atomic_counters);
  enumerate_resources(resources.acceleration_structures);
  return mapping;
}

plString GetMSLShader(const plDynamicArray<plUInt8>& blob, plMap<plString, plUInt32>& mapping)
{
  //PL_ASSERT_DEBUG(blob.GetCount() % sizeof(plUInt32) == 0);
  spirv_cross::CompilerMSL compiler((const plUInt32*)blob.GetData(), blob.GetCount());
  auto options = compiler.get_msl_options();
  options.set_msl_version(2, 3);
  // TODO: Fill options
  compiler.set_msl_options(options);
  plString msl_source = compiler.compile().c_str();
  mapping = ParseBindings(compiler);
  return msl_source;
}

plString GetMSLShader(const plRHIShaderDesc& shader)
{
  plDynamicArray<plUInt8> blob = Compile(shader, plRHIShaderBlobType::kSPIRV);
  plMap<plString, plUInt32> mapping;
  return GetMSLShader(blob, mapping);
}
