#ifndef __FIX_MATH_H

#define __FIX_MATH_H

#include <stdint.h>

#define BASE32 (0x80000000)
#define INT32MAX ((1LL<<31)-1)
#define INT32MIN (-(1LL<<31))

#define BASE16 (0x8000)
#define INT16MAX (0x7fff)
#define INT16MIN (0x8000)

#define BASE64 (0x8000000000000000)
#define INT64MAX (0x7FFFffffFFFFffff)
#define INT64MIN (0x8000000000000000)

#define FIX05 (1LL<<30)
#define FIX05S (1L<<15)

#define PI 3.14159265358979323846
#define PI_2 1.57079632679
#define PI_4 0.78539816339


int32_t float2fixed(double x);

float fixed2float(int32_t x);

double fixed2float64(int64_t x);

double fixed2float16(int16_t x);

int16_t float2fixed16(double x);


int16_t LeftShift16(int16_t x, int n);

int32_t LeftShift32(int32_t x, int n);

int32_t RightShift32(int32_t x, int n);

int64_t LeftShift64(int64_t x, int n);

int64_t RightShift64(int64_t x, int n);


int64_t SaturationResult32(int64_t x);

int32_t adds32(int32_t A, int32_t B);

int32_t subs32(int32_t A, int32_t B);

int64_t adds64(int64_t A, int64_t B);

int64_t subs64(int64_t A, int64_t B);


int32_t get_Hi31(int64_t x);

int32_t get_Low31(int64_t x);

int32_t get_Low30(int64_t x);

int16_t get_Hi15(int32_t x);

int16_t get_Low15(int32_t x);

int16_t get_Low14(int32_t x);

int16_t get_Hi15_31(int64_t x);


int16_t Round64_15(int64_t x);

int16_t Round32(int64_t x);

int32_t Round62(int64_t x);

int32_t Round64(int64_t x);


int64_t mul64(int32_t A, int32_t B);

int32_t mul32(int32_t A, int32_t B);

int32_t mul16(int16_t A, int16_t B);


int64_t mac32(int64_t acc, int32_t A, int32_t B);

int64_t msub32(int64_t acc, int32_t A, int32_t B);

int32_t mac16(int32_t acc, int16_t A, int16_t B);


int64_t abs64(int64_t x);

int32_t abs32(int32_t x);

int16_t abs16(int16_t x);


double div_double(double x, double y);

int Norm16(uint16_t x);

int16_t div16(int16_t x, int16_t y);

int Norm32(uint32_t x);

int32_t div32(int32_t x, int32_t y);


int Norm16dec(uint32_t x);

int32_t LinearInterpolation(int16_t x, int32_t num1, int32_t num2);

int32_t log2calc(int32_t x32);

int NormQ27(int64_t x);

int32_t Pow(int32_t x32, int32_t y32);

int32_t PowQ15(int32_t x32, int32_t y32);

#endif
