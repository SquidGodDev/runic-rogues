#pragma once

#include <stdint.h>

// Q16.16 fixed-point format
// 16 bits integer (-32768 to 32767), 16 bits fraction (precision 1/65536)
typedef int32_t fixed_t;

#define FP_SHIFT          16
#define FP_ONE            (1 << FP_SHIFT)
#define FP_HALF           (1 << (FP_SHIFT - 1))
#define FP_MASK           (FP_ONE - 1)

// Conversion
#define INT_TO_FP(x)       ((fixed_t)(x) << FP_SHIFT)
#define FLOAT_TO_FP(x)     ((fixed_t)((x) * FP_ONE))
#define FP_TO_INT(x)       ((x) >> FP_SHIFT)
#define FP_TO_INT_ROUND(x) (((x) + FP_HALF) >> FP_SHIFT)
#define FP_TO_FLOAT(x)     ((float)(x) / FP_ONE)

// Arithmetic (FP_MUL uses 64-bit intermediate -> SMULL on ARM)
#define FP_MUL(a, b)       ((fixed_t)(((int64_t)(a) * (int64_t)(b)) >> FP_SHIFT))
// FP_DIV uses float to avoid __aeabi_ldivmod (64-bit integer division).
// Float has 23-bit mantissa, sufficient for Q16.16's 16 fractional bits.
#define FP_DIV(a, b)       ((fixed_t)((float)(a) / (float)(b) * (float)FP_ONE))
