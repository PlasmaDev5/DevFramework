#pragma once

#include <Foundation/Math/Frustum.h>

PL_FORCE_INLINE bool plFrustum::Overlaps(const plSimdBBox& object) const
{
  plSimdVec4f center2 = object.m_Min + object.m_Max;
  plSimdVec4f extents = object.GetExtents();

  // We're working with center and extents scaled by two - but the plane equation still works out
  // correctly since we set W = 2 here.
  center2.SetW(plSimdFloat(2.0f));
  extents.SetW(plSimdFloat::MakeZero());

#if PL_SIMD_IMPLEMENTATION == PL_SIMD_IMPLEMENTATION_SSE
  plSimdVec4f minusZero;
  minusZero.Set(-0.0f);
#endif

  for (plUInt32 plane = 0; plane < PLANE_COUNT; ++plane)
  {
    plSimdVec4f equation;
    equation.Load<4>(m_Planes[plane].m_vNormal.GetData());

    // Change signs of extents to match signs of plane normal
    plSimdVec4f maxExtent;

#if PL_SIMD_IMPLEMENTATION == PL_SIMD_IMPLEMENTATION_SSE
    // Specialized for SSE - this is faster than FlipSign for multiple calls since we can preload the constant -0.0f
    maxExtent.m_v = _mm_xor_ps(extents.m_v, _mm_andnot_ps(equation.m_v, minusZero.m_v));
#else
    maxExtent = extents.FlipSign(equation >= plSimdVec4f::MakeZero());
#endif

    // Compute AABB corner which is the furthest along the plane normal
    const plSimdVec4f offset = center2 + maxExtent;

    if (equation.Dot<4>(offset) > plSimdFloat::MakeZero())
    {
      // outside
      return false;
    }
  }

  // inside or intersect
  return true;
}

PL_FORCE_INLINE bool plFrustum::Overlaps(const plSimdBSphere& object) const
{
  // Calculate the minimum distance of the given sphere's center to all frustum planes.
  plSimdFloat xSphere = object.m_CenterAndRadius.x();
  plSimdFloat ySphere = object.m_CenterAndRadius.y();
  plSimdFloat zSphere = object.m_CenterAndRadius.z();
  plSimdVec4f radius;
  radius.Set(object.m_CenterAndRadius.w());

  plSimdVec4f xPlanes0;
  plSimdVec4f yPlanes0;
  plSimdVec4f zPlanes0;
  plSimdVec4f dPlanes0;

  xPlanes0.Load<4>(m_Planes[0].m_vNormal.GetData());
  yPlanes0.Load<4>(m_Planes[1].m_vNormal.GetData());
  zPlanes0.Load<4>(m_Planes[2].m_vNormal.GetData());
  dPlanes0.Load<4>(m_Planes[3].m_vNormal.GetData());

  {
    plSimdMat4f tmp;
    tmp.SetRows(xPlanes0, yPlanes0, zPlanes0, dPlanes0);
    xPlanes0 = -tmp.m_col0;
    yPlanes0 = -tmp.m_col1;
    zPlanes0 = -tmp.m_col2;
    dPlanes0 = -tmp.m_col3;
  }

  plSimdVec4f minDist0;
  minDist0 = dPlanes0 + xPlanes0 * xSphere;
  minDist0 += yPlanes0 * ySphere;
  minDist0 += zPlanes0 * zSphere;

  plSimdVec4f xPlanes1;
  plSimdVec4f yPlanes1;
  plSimdVec4f zPlanes1;
  plSimdVec4f dPlanes1;

  xPlanes1.Load<4>(m_Planes[4].m_vNormal.GetData());
  yPlanes1.Load<4>(m_Planes[5].m_vNormal.GetData());
  zPlanes1.Load<4>(m_Planes[5].m_vNormal.GetData());
  dPlanes1.Load<4>(m_Planes[5].m_vNormal.GetData());

  {
    plSimdMat4f tmp;
    tmp.SetRows(xPlanes1, yPlanes1, zPlanes1, dPlanes1);
    xPlanes1 = -tmp.m_col0;
    yPlanes1 = -tmp.m_col1;
    zPlanes1 = -tmp.m_col2;
    dPlanes1 = -tmp.m_col3;
  }


  plSimdVec4f minDist1 = dPlanes1 + xPlanes1 * xSphere;
  minDist1 += yPlanes1 * ySphere;
  minDist1 += zPlanes1 * zSphere;

  plSimdVec4f minDist = minDist0.CompMin(minDist1);

  // Add the sphere radius to cater for the sphere's extents.
  minDist += radius;

  // If the distance is still less than zero, the sphere is completely "outside" of at least one plane.
  return (minDist < plSimdVec4f::MakeZero()).NoneSet();
}
