#ifndef __FILTER_H

#define __FILTER_H

#include <stdint.h>

#define FLT_LENGTH 127        
#define BUFFER_SIZE (FLT_LENGTH+1)			//CirBuf
#define BUFFER_MASK (BUFFER_SIZE-1)

double filter_float_IIR(double input, double *h, double *DelayBufferf, double accf);

int32_t gain2amplif32();

// FIR

void CoeffsCalculationFIR(double *hcc);

int CirBufferPut(int16_t NewSamp, int IndexEnd, int16_t *CirBuf);

int32_t filter_FIR(int32_t *coeff, int16_t *CirBuf, int16_t pointer);

// IIR Biquad Design

void BiquadFilterDesignLPF_HPF(double SampleRate, double CutOffFreq, double *h, int FiltType);

void BiquadFilterDesignBPF_Notch(double SampleRate, double CutOffFreq2, double CutOffFreq1, double *h, int FiltType);

int16_t filter_Biquad16(int16_t cur, int16_t *h16, int16_t *DelayBuffer);

int16_t filter_Biquad16_with_NS(int16_t cur, int16_t *h16, int16_t *DelayBuffer);

int16_t filter_Biquad16_16(int16_t cur, int32_t *h32, int16_t *DelayBuffer);

int16_t filter_Biquad16_16_with_NS(int16_t cur, int32_t *h32, int16_t *DelayBuffer, int32_t *err);

int16_t filter_Biquad32(int16_t cur, int32_t *h32, int32_t *DelayBuffer);

int16_t filter_Biquad32_with_NS(int16_t cur, int32_t *h32, int32_t *DelayBuffer);

int32_t filter_Biquad32_32(int32_t cur, int32_t *h32, int32_t *DelayBuffer);

int32_t filter_Biquad32_32_with_NS(int32_t cur, int32_t *h32, int32_t *DelayBuffer);

// Allpass Filter

double AllpassDesign(double SampleRate, double CutOffFreq);

double filter_float_Allpass(double input, double c, double *DelayBufferf_allp, int FilterType);

int16_t filter_Allpass_16(int16_t cur, double c, int32_t *DelayBuffer, int FilterType);

int16_t filter_Allpass_16_with_NS(int16_t cur, double c, int32_t *DelayBuffer, int FilterType);

int16_t filter_Allpass_16_16(int16_t cur, double c, int16_t *DelayBuffer, int FilterType);

int16_t filter_Allpass_16_16_with_NS(int16_t cur, double c, int16_t *DelayBuffer, int FilterType);

int16_t filter_Allpass_32(int16_t cur, double c, int16_t *DelayBuffer, int FilterType);

int16_t filter_Allpass_32_with_NS(int16_t cur, double c, int16_t *DelayBuffer, int32_t *err, int FilterType);

int32_t filter_Allpass_32_32(int32_t cur, double c, int32_t *DelayBuffer, int FilterType);

int32_t filter_Allpass_32_32_with_NS(int32_t cur, int32_t c, int32_t *DelayBuffer, int FilterType);

#endif
