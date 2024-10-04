#pragma once

#include "ParticleSystemConstants.h"
#include "BaseParticleShaderData.h"

struct PL_SHADER_STRUCT plBillboardQuadParticleShaderData
{
  FLOAT3(Position);
  PACKEDHALF2(RotationOffset, RotationSpeed, RotationOffsetAndSpeed);
};

// this is only defined during shader compilation
#if PL_ENABLED(PLATFORM_SHADER)

StructuredBuffer<plBillboardQuadParticleShaderData> particleBillboardQuadData;

#else // C++

static_assert(sizeof(plBillboardQuadParticleShaderData) == 16);

#endif

