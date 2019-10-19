#pragma once

#include <pe/Defines.hpp>


#if !DISABLE_SIMD

#include <pmmintrin.h>

namespace pe::core::math
{

/// <summary>SIMD intristic dot product of 3D vector.</summary>
__m128 _mm_dot_ps(__m128 a, __m128 b);

/// <summary>SIMD intristic compare two floats with given precission.</summary>
__m128 _mm_cmpf_ps(__m128 a, __m128 b);

}

#endif