#pragma once

#define PLATFORM_DX11 PL_OFF

#if defined(DX11_SM40_93) || defined(DX11_SM40) || defined(DX11_SM41) || defined(DX11_SM50)
#  undef PLATFORM_SHADER
#  define PLATFORM_SHADER PL_ON

#  undef PLATFORM_DX11
#  define PLATFORM_DX11 PL_ON

// DX11 does not support push constants, so we just emulate them via a normal constant buffer.
#  define BEGIN_PUSH_CONSTANTS(Name) cbuffer Name
#  define END_PUSH_CONSTANTS(Name) ;
#  define GET_PUSH_CONSTANT(Name, Constant) Constant

float plEvaluateAttributeAtSample(float Attribute, uint SampleIndex, uint NumMsaaSamples)
{
  return EvaluateAttributeAtSample(Attribute, SampleIndex);
}

float2 plEvaluateAttributeAtSample(float2 Attribute, uint SampleIndex, uint NumMsaaSamples)
{
  return EvaluateAttributeAtSample(Attribute, SampleIndex);
}

float3 plEvaluateAttributeAtSample(float3 Attribute, uint SampleIndex, uint NumMsaaSamples)
{
  return EvaluateAttributeAtSample(Attribute, SampleIndex);
}

float4 plEvaluateAttributeAtSample(float4 Attribute, uint SampleIndex, uint NumMsaaSamples)
{
  return EvaluateAttributeAtSample(Attribute, SampleIndex);
}

// Custom implementation of HLSL2021 intrinsic function which is not available in SM50 or lower.
// https://github.com/microsoft/DirectXShaderCompiler/wiki/HLSL-2021#logical-operation-short-circuiting-for-scalars
float2 select(bool2 condition, float2 yes, float2 no)
{
  return float2(condition.x ? yes.x : no.x, condition.y ? yes.y : no.y);
}

float3 select(bool3 condition, float3 yes, float3 no)
{
  return float3(condition.x ? yes.x : no.x, condition.y ? yes.y : no.y, condition.z ? yes.z : no.z);
}

float4 select(bool4 condition, float4 yes, float4 no)
{
  return float4(condition.x ? yes.x : no.x, condition.y ? yes.y : no.y, condition.z ? yes.z : no.z, condition.w ? yes.w : no.w);
}

bool and(bool a, bool b)
{
  return a && b;
}

bool2 and(bool2 a, bool2 b)
{
  return bool2(a.x && b.x, a.y && b.y);
}

bool3 and(bool3 a, bool3 b)
{
  return bool3(a.x && b.x, a.y && b.y, a.z && b.z);
}

#endif
