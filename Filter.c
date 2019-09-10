#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <windows.h>
#include <math.h>
#include "Filter.h"
#include "Fixed_math32.h"

double filter_float_IIR(double input, double *h, double *DelayBufferf, double accf) {
	//Biquad IIR Filter
	
	accf = h[3] * input + h[4] * DelayBufferf[0] + h[5] * DelayBufferf[1] + h[1] * DelayBufferf[2] + h[2] * DelayBufferf[3];
	accf *= 2;

	DelayBufferf[3] = DelayBufferf[2];
	DelayBufferf[1] = DelayBufferf[0];
	DelayBufferf[0] = input;
	DelayBufferf[2] = accf;

	return accf;
}

int32_t gain2amplif32() {

	float gain = 0;
	double AmplifFactor;
	int32_t AmpFactor32;

	printf("Enter gain in dB (can be negative only): ");
	scanf_s("%f", &gain);
	while (gain >= 0) {
		printf("Please enter NEGATIVE gain only (in dB): ");
		scanf_s("%f", &gain);
	}

	AmplifFactor = pow(10.0, gain / 20.0);
	AmpFactor32 = float2fixed(AmplifFactor);

	return AmpFactor32;
}

//FIR

void CoeffsCalculationFIR(double *hcc) {
	/*
	Low-pass FIR filter design.
	For calculation of coefficients was used Fourier Transform Design Method (without using a window function).
	As the filter length can be used just an ODD number.
	*/
	double h[FLT_LENGTH];
	double hc[FLT_LENGTH + 1];
	int M = (FLT_LENGTH - 1) / 2;	//filter delay value
	double WindowHamm[FLT_LENGTH + 1];
	double sum = 0;
	double sum2 = 0;

	int count = 0;
	double NormFreq = 0.25 * PI;
	h[0] = 2.5;

	for (int i = -M; i <= M; i++) {
		WindowHamm[i + M] = 0.54 + 0.46*cos((i*PI) / M);  // Window calculation
	}

	for (int i = 1; i <= M; i++) { 						// calculation of noncausal filter coefficients
		h[i] = (sin(NormFreq *i)) / i*PI;
	}

	for (int i = 0; i <= M; i++) { 					  // multiplication Wind*H
		h[i] *= WindowHamm[M + i];
	}

	for (int i = M; i >= 1; i--) {			// using the symmetry for other coeffs
		count = M - i;
		hc[count] = h[i];
	}

	hc[M] = h[0];

	for (int i = 1; i <= M; i++) {         // noncausal -> causal coeffs
		count = M + i;
		hc[count] = h[i];
	}
	hc[FLT_LENGTH] = 0.0;

	// Normalisation of coefficients to sum=1;
	printf("\n Normalized Freq = %f \n\n", NormFreq);
	for (int i = 0; i <= FLT_LENGTH; i++) {
		hcc[i] = hc[i];
	}

	
	for (int i = 0; i <= FLT_LENGTH; i++) {
		sum += hcc[i];
	}

	for (int i = 0; i <= FLT_LENGTH; i++) {
		hcc[i] /= sum;
	}
	printf("\n CALCULATED COEFFITIENTS (float type): \n\n");
	for (int i = 0; i <= FLT_LENGTH; i++) {
		printf("h[%d] = %f; \t", i, hcc[i]);
	}
	printf("\n\n");

	for (int i = 0; i <= FLT_LENGTH; i++) {
		sum2 += hcc[i];
	}
	printf("SUM %f", sum2);
}

int CirBufferPut(int16_t NewSamp, int IndexEnd, int16_t *CirBuf) {

	IndexEnd++;
	IndexEnd &= BUFFER_MASK;
	CirBuf[IndexEnd] = NewSamp;

	return IndexEnd;
}

int32_t filter_FIR(int32_t *coeff, int16_t *CirBuf, int16_t pointer) {
	//out Q31

	int64_t acc = 0;
	int32_t Res32;
	int16_t idx;

	for (int r = 0; r <= FLT_LENGTH; r++) {
		idx = (pointer + r) & BUFFER_MASK;
		acc += (int64_t)CirBuf[idx] * coeff[r];
	}

	acc = LeftShift64(acc, 17); 
	Res32 = get_Hi31(acc);

	return Res32;
}


// IIR Biquad Design

void BiquadFilterDesignLPF_HPF(double SampleRate, double CutOffFreq, double *h, int FiltType) {
	//Coeffintients are divided by 2 that is compensated inside of filter_IIR_float function

	double A = 1;
	double B = 1.4142135624;
	double C = 1;
	double D = 0;
	double E = 0;
	double F = 1;

	double OmegaC = CutOffFreq * 2 / SampleRate;
	double T, Arg;
	double a2, a1, a0, b2, b1, b0;

	T = 2.0 * tan(OmegaC * PI_2);

	if (FiltType == 1)
	{
		if (A == 0.0 && D == 0.0) // 1 pole case
		{
			Arg = (2.0*B + C*T);
			a2 = 0.0;
			a1 = (-2.0*B + C*T) / Arg;
			a0 = 1.0;

			b2 = 0.0;
			b1 = (-2.0*E + F*T) / Arg * C / F;
			b0 = (2.0*E + F*T) / Arg * C / F;
		}
		else // 2 poles
		{
			Arg = (4.0*A + 2.0*B*T + C*T*T);
			a2 = (4.0*A - 2.0*B*T + C*T*T) / Arg;
			a1 = (2.0*C*T*T - 8.0*A) / Arg;
			a0 = 1.0;

			b2 = (4.0*D - 2.0*E*T + F*T*T) / Arg * C / F;
			b1 = (2.0*F*T*T - 8.0*D) / Arg * C / F;
			b0 = (4 * D + F*T*T + 2.0*E*T) / Arg * C / F;
		}
	}

	if (FiltType == 2)
	{
		if (A == 0.0 && D == 0.0) // 1 pole
		{
			Arg = 2.0*C + B*T;
			a2 = 0.0;
			a1 = (B*T - 2.0*C) / Arg;
			a0 = 1.0;

			b2 = 0.0;
			b1 = (E*T - 2.0*F) / Arg * C / F;
			b0 = (E*T + 2.0*F) / Arg * C / F;
		}
		else  // 2 poles
		{
			Arg = A*T*T + 4.0*C + 2.0*B*T;
			a2 = (A*T*T + 4.0*C - 2.0*B*T) / Arg;
			a1 = (2.0*A*T*T - 8.0*C) / Arg;
			a0 = 1.0;

			b2 = (D*T*T - 2.0*E*T + 4.0*F) / Arg * C / F;
			b1 = (2.0*D*T*T - 8.0*F) / Arg * C / F;
			b0 = (D*T*T + 4.0*F + 2.0*E*T) / Arg * C / F;
		}
	}

	h[0] = 0;
	h[1] = -a1 / 2;
	h[2] = -a2 / 2;
	h[3] = b0 / 2;
	h[4] = b1 / 2;
	h[5] = b2 / 2;
}

void BiquadFilterDesignBPF_Notch(double SampleRate, double CutOffFreq2, double CutOffFreq1, double *h, int FiltType) {

	double A = 0;
	double B = 1.4142135624;
	double C = 1;
	double D = 0;
	double E = 0;
	double F = 1;

	double OmegaC = CutOffFreq1 * 2 / SampleRate;
	double T, Arg, Q;
	double a2, a1, a0, b2, b1, b0;
	double BW = (CutOffFreq2 - CutOffFreq1) * 2 / SampleRate;

	T = 2.0 * tan(OmegaC * PI_2);
	Q = 1.0 + OmegaC;
	if (Q > 1.95)Q = 1.95;       // Q must be < 2
	Q = 0.8 * tan(Q * PI_4);  // This is the correction factor.
	Q = OmegaC / BW / Q;        // This is the corrected Q.

	if (FiltType == 3)
	{
		Arg = 4.0*B*Q + 2.0*C*T + B*Q*T*T;
		a2 = (B*Q*T*T + 4.0*B*Q - 2.0*C*T) / Arg;
		a1 = (2.0*B*Q*T*T - 8.0*B*Q) / Arg;
		a0 = 1.0;

		b2 = (E*Q*T*T + 4.0*E*Q - 2.0*F*T) / Arg * C / F;
		b1 = (2.0*E*Q*T*T - 8.0*E*Q) / Arg * C / F;
		b0 = (4.0*E*Q + 2.0*F*T + E*Q*T*T) / Arg * C / F;
	}

	if (FiltType == 4)
	{
		Arg = 2.0*B*T + C*Q*T*T + 4.0*C*Q;
		a2 = (4.0*C*Q - 2.0*B*T + C*Q*T*T) / Arg;
		a1 = (2.0*C*Q*T*T - 8.0*C*Q) / Arg;
		a0 = 1.0;

		b2 = (4.0*F*Q - 2.0*E*T + F*Q*T*T) / Arg * C / F;
		b1 = (2.0*F*Q*T*T - 8.0*F*Q) / Arg *C / F;
		b0 = (2.0*E*T + F*Q*T*T + 4.0*F*Q) / Arg *C / F;
	}


	h[0] = 0;
	h[1] = -a1;
	h[2] = -a2;
	h[3] = b0;
	h[4] = b1;
	h[5] = b2;
}

//BIQUAD FILTER 

int16_t filter_Biquad16(int16_t cur, int16_t *h16, int16_t *DelayBuffer) {
	// 16 input, 16 states, 16 coeffs
	int32_t acc = 0;
	
	acc = mac16(acc, h16[3], cur);				// h = Q14, cur = Q15, acc = Q29
	acc = mac16(acc, h16[1], DelayBuffer[2]);
	acc = mac16(acc, h16[4], DelayBuffer[0]);
	acc = mac16(acc, h16[2], DelayBuffer[3]);
	acc = mac16(acc, h16[5], DelayBuffer[1]);

	acc = LeftShift32(acc, 2);					// *2 for coeffs  div compensation !!! Q29 -> Q30
												// Q30 -> Q31
	DelayBuffer[3] = DelayBuffer[2];
	DelayBuffer[1] = DelayBuffer[0];
	DelayBuffer[0] = cur;
	DelayBuffer[2] = Round32(acc);				//out Rounded value, get_Hi15 for truncated

	return DelayBuffer[2];
}

int16_t filter_Biquad16_with_NS(int16_t cur, int16_t *h16, int16_t *DelayBuffer) {
	// 16 input, 16 states, 16 coeffs
	// error saved in the DelayBuffer[4]

	int32_t acc = DelayBuffer[4];
	
	acc = mac16(acc, h16[3], cur);				// h = Q14, cur = Q15, acc = Q29
	acc = mac16(acc, h16[1], DelayBuffer[2]);
	acc = mac16(acc, h16[4], DelayBuffer[0]);
	acc = mac16(acc, h16[2], DelayBuffer[3]);
	acc = mac16(acc, h16[5], DelayBuffer[1]);

	DelayBuffer[4] = get_Low14(acc);			// error 

	acc = LeftShift32(acc, 2);					//*2 for coeffs  div compensation !!! Q29 -> Q30
												// Q30 ->Q31

	DelayBuffer[3] = DelayBuffer[2];
	DelayBuffer[1] = DelayBuffer[0];
	DelayBuffer[0] = cur;
	DelayBuffer[2] = get_Hi15(acc);

	return DelayBuffer[2];
}

int16_t filter_Biquad16_16(int16_t cur, int32_t *h32, int16_t *DelayBuffer) {
	// 16 input, 16 states, 32 coeffs

	int64_t acc = 0;

	acc = mac32(acc, h32[3], cur);				// h = Q30, cur = Q15, acc = Q45
	acc = mac32(acc, h32[1], DelayBuffer[2]);
	acc = mac32(acc, h32[4], DelayBuffer[0]);
	acc = mac32(acc, h32[2], DelayBuffer[3]);
	acc = mac32(acc, h32[5], DelayBuffer[1]);

	acc = LeftShift64(acc, 18);					//*2 for coeffs  div compensation !!! 1 Q45 -> Q46
												// 17 Q46 ->Q63

	DelayBuffer[3] = DelayBuffer[2];
	DelayBuffer[1] = DelayBuffer[0];
	DelayBuffer[0] = cur;
	DelayBuffer[2] = Round64_15(acc);			//out Rounded value, for truncated get_Hi15_31

	return DelayBuffer[2];
}


int16_t filter_Biquad16_16_with_NS(int16_t cur, int32_t *h32, int16_t *DelayBuffer, int32_t *err) {
	// 16 input, 16 states, 32 coeffs
	//error saved in the err

	int64_t acc = *err;

	acc = mac32(acc, h32[3], cur);				// h = Q30, cur = Q15, acc = Q45
	acc = mac32(acc, h32[1], DelayBuffer[2]);
	acc = mac32(acc, h32[4], DelayBuffer[0]);
	acc = mac32(acc, h32[2], DelayBuffer[3]);
	acc = mac32(acc, h32[5], DelayBuffer[1]);

	*err = get_Low30(acc);						//err

	acc = LeftShift64(acc, 18);					//*2 for coeffs  div compensation !!! Q45 -> Q46
												// Q46 ->Q63

	DelayBuffer[3] = DelayBuffer[2];
	DelayBuffer[1] = DelayBuffer[0];
	DelayBuffer[0] = cur;
	DelayBuffer[2] = get_Hi15_31(acc);
		
	return DelayBuffer[2];
}

int16_t filter_Biquad32(int16_t cur, int32_t *h32, int32_t *DelayBuffer) {
	// 16 input, 32 states, 32 coeffs

	int16_t out;
	int32_t Cur;
	int64_t acc = 0;

	Cur = LeftShift32((int32_t)cur, 16);	
	
	acc = mac32(acc, h32[3], Cur);				// h = Q30, cur = Q31, acc = Q 61
	acc = mac32(acc, h32[1], DelayBuffer[2]);
	acc = mac32(acc, h32[4], DelayBuffer[0]);
	acc = mac32(acc, h32[2], DelayBuffer[3]);
	acc = mac32(acc, h32[5], DelayBuffer[1]);

	acc = LeftShift64(acc, 2);					//*2 for coeffs  div compensation !!! Q61 -> Q62
												// Q62 -> Q63
	DelayBuffer[3] = DelayBuffer[2];
	DelayBuffer[1] = DelayBuffer[0];
	DelayBuffer[0] = Cur;
	DelayBuffer[2] = Round64(acc);				//out Rounded value, for truncated get_Hi31

	out = Round64_15(acc);						//out
	return out;
}

int16_t filter_Biquad32_with_NS(int16_t cur, int32_t *h32, int32_t *DelayBuffer) {
	// 16 input, 32 states, 32 coeffs
	//error saved in the DelayBuffer[4]

	int16_t out;
	int32_t Cur;
	int64_t acc = 0;

	Cur = LeftShift32((int32_t)cur, 16);
	acc = (int64_t)DelayBuffer[4];

	acc = mac32(acc, h32[3], Cur);				// h = Q30, cur = Q31, acc = Q 61
	acc = mac32(acc, h32[1], DelayBuffer[2]);
	acc = mac32(acc, h32[4], DelayBuffer[0]);
	acc = mac32(acc, h32[2], DelayBuffer[3]);
	acc = mac32(acc, h32[5], DelayBuffer[1]);

	DelayBuffer[4] = get_Low30(acc);

	acc = LeftShift64(acc, 2);					//*2 for coeffs  div compensation !!! Q61 -> Q62
												// Q62 -> Q63
	DelayBuffer[3] = DelayBuffer[2];
	DelayBuffer[1] = DelayBuffer[0];
	DelayBuffer[0] = Cur;
	DelayBuffer[2] = get_Hi31(acc);

	out = Round64_15(acc);						//out
	return out;
}

int32_t filter_Biquad32_32(int32_t cur, int32_t *h32, int32_t *DelayBuffer) { //without Noise Shape
	// 32 input, 32 states, 32 coeffs

	int64_t acc = 0;

	acc = mac32(acc, h32[3], cur);				//h = Q30, cur = Q31, acc= Q61
	acc = mac32(acc, h32[1], DelayBuffer[2]);
	acc = mac32(acc, h32[4], DelayBuffer[0]);
	acc = mac32(acc, h32[2], DelayBuffer[3]);
	acc = mac32(acc, h32[5], DelayBuffer[1]);

	acc = LeftShift64(acc, 2);					//*2 for coeffs  div compensation !!! Q61 -> Q62
												// Q62 -> Q63

	DelayBuffer[3] = DelayBuffer[2];
	DelayBuffer[1] = DelayBuffer[0];
	DelayBuffer[0] = cur;
	DelayBuffer[2] = Round64(acc);				//out Rounded value, for truncated get_Hi31

	return DelayBuffer[2];
}

int32_t filter_Biquad32_32_with_NS(int32_t cur, int32_t *h32, int32_t *DelayBuffer) {
	// 32 input, 32 states, 32 coeffs
	//error saved in the DelayBuffer[4]
	int64_t acc;

	acc = (int64_t) DelayBuffer[4];

	acc = mac32(acc, h32[3], cur);				//h = Q30, cur = Q31, acc= Q61
	acc = mac32(acc, h32[1], DelayBuffer[2]);
	acc = mac32(acc, h32[4], DelayBuffer[0]);
	acc = mac32(acc, h32[2], DelayBuffer[3]);
	acc = mac32(acc, h32[5], DelayBuffer[1]);

	DelayBuffer[4] = get_Low30(acc);			//err

	acc = LeftShift64(acc, 2);					//*2 for coeffs  div compensation !!! Q61 -> Q62
												// Q62 -> Q63
	DelayBuffer[3] = DelayBuffer[2];
	DelayBuffer[1] = DelayBuffer[0];
	DelayBuffer[0] = cur;
	DelayBuffer[2] = get_Hi31(acc);  //out result

	return DelayBuffer[2];
}

// ALLPASS Filter 

double AllpassDesign(double SampleRate, double CutOffFreq) {
	// first order filter design
	double c;

	c = (tan(PI*CutOffFreq / SampleRate) - 1) / (tan(PI*CutOffFreq / SampleRate) + 1);
	
	return c;
}

double filter_float_Allpass(double input, double c, double *DelayBufferf_allp, int FilterType) {
	// first order
	// FilterType = 1 for LP, 2 = for HP

	double accf = 0;
	double Res;

	accf = c*input;
	accf += DelayBufferf_allp[0];
	accf -= c*DelayBufferf_allp[1];

	DelayBufferf_allp[0] = input;
	DelayBufferf_allp[1] = accf;

	if (FilterType == 1) {
		Res = (input + accf) / 2;  // LPH
	}
	else if (FilterType == 2) {
		Res = (input - accf) / 2;  // HPF   
	}
	else Res = 0;

	return Res;			//out value
}

int16_t filter_Allpass_16(int16_t cur, double c, int32_t *DelayBuffer, int FilterType) {
	// 16 input, 32 states, 32 coeffs
	// FilterType = 1 for LP, 2 = for HP

	int32_t current, c32min, c32plus, c_05;
	int64_t cur64, res;
	int64_t acci = 0;
	int16_t out;

	current = LeftShift32((int32_t)cur, 16);

	c32min = float2fixed(-c / 2);					//div coeff by 2
	c32plus = float2fixed(c / 2);
	c_05 = float2fixed(0.5);

	acci = mac32(acci, c32plus, current);		// c = Q30, cur = Q31, acc = Q61 
	acci = mac32(acci, c_05, DelayBuffer[0]);
	acci = mac32(acci, c32min, DelayBuffer[1]);

	acci = LeftShift64(acci, 2);				//compensation of div ->Q62  //Q62 -> Q63

	DelayBuffer[0] = current;
	DelayBuffer[1] = Round64(acci);

	// processing by formula
	/*cur64 = LeftShift64((int64_t)cur, 48);

	acci = RightShift64(acci, 1);			//dividing by 2 by formulas need
	cur64 = RightShift64(cur64, 1);			//dividing by 2 by formulas need

	if (FilterType == 1) {
		res = adds64(cur64, acci);  // LPH
	}
	else if (FilterType == 2) {
		res = subs64(cur64, acci);  // HPF    -> Q63
	}
	else res = 0;
	*/
	out = Round64_15(acci);
	return out;
}

int16_t filter_Allpass_16_with_NS(int16_t cur, double c, int32_t *DelayBuffer, int FilterType) {
	// 16 input, 32 states, 32 coeffs WITH Noise Shaping
	// FilterType = 1 for LP, 2 = for HP
	// error is saved in DelayBuffer[2] 

	int32_t current, c32min, c32plus, c_05;
	int64_t cur64, res, acci;
	int16_t out;

	acci = (int64_t)DelayBuffer[2];
	
	current = LeftShift32((int32_t)cur, 16);

	c32min = float2fixed(-c / 2);					// div coeff by 2
	c32plus = float2fixed(c / 2);
	c_05 = float2fixed(0.5);

	acci = mac32(acci, c32plus, current);		// c= Q30, cur = Q31, acc = Q61 
	acci = mac32(acci, c_05, DelayBuffer[0]);
	acci = mac32(acci, c32min, DelayBuffer[1]);

	DelayBuffer[2] = get_Low30(acci);			// error saved

	acci = LeftShift64(acci, 2);				//compensation of div ->Q62  //Q62 -> Q63

	DelayBuffer[0] = current;
	DelayBuffer[1] = get_Hi31(acci);

	// processing by formula
	/*cur64 = LeftShift64((int64_t)cur, 48);

	acci = RightShift64(acci, 1);			//dividing by 2 by formulas need
	cur64 = RightShift64(cur64, 1);			//dividing by 2 by formulas need

	if (FilterType == 1) {
		res = adds64(cur64, acci);  // LPH
	}
	else if (FilterType == 2) {
		res = subs64(cur64, acci);  // HPF    -> Q63
	}
	else res = 0;*/

	out = Round32(DelayBuffer[1]);
	return out;
}

int16_t filter_Allpass_16_16(int16_t cur, double c, int16_t *DelayBuffer, int FilterType) {		
	// without Noise Shaping
	// 16 input, 16 states, 16 coeffs
	// FilterType = 1 for LP, 2 = for HP

	int16_t c16min, c16plus, c_05, out;
	int32_t res, cur32;
	int32_t acci = 0;
	
	c16min = float2fixed16(-c / 2);					//div coeff by 2
	c16plus = float2fixed16(c / 2);
	c_05 = float2fixed16(0.5);

	acci = mac16(acci, c16plus, cur);		// coef = Q14, cur = Q15, acc = Q29
	acci = mac16(acci, c_05, DelayBuffer[0]);
	acci = mac16(acci, c16min, DelayBuffer[1]);

	acci = LeftShift32(acci, 2);			//compensation of div ->Q30  //Q30 -> Q31

	DelayBuffer[0] = cur;
	DelayBuffer[1] = Round32(acci);

	// processing by formula
	/*cur32 = LeftShift32((int32_t)cur, 16);

	acci = RightShift32(acci, 1);			//dividing by 2 by formulas need
	cur32 = RightShift32(cur32, 1);			//dividing by 2 by formulas need

	if (FilterType == 1) {
		res = adds32(cur32, acci);  // LPH
	}
	else if (FilterType == 2) {
		res = subs32(cur32, acci);  // HPF    -> Q63
	}
	else res = 0;

	out = Round32(res);*/
	return DelayBuffer[1];
}

int16_t filter_Allpass_16_16_with_NS(int16_t cur, double c, int16_t *DelayBuffer, int FilterType) {
	// 16 input, 16 states, 16 coeffs WITH Noise Shaping
	// FilterType = 1 for LP, 2 = for HP
	// error is saved in DelayBuffer[2] 

	int16_t c16min, c16plus, c_05, out;
	int32_t cur32, res, acci;
	
	acci = (int64_t)DelayBuffer[2];
	
	c16min = float2fixed16(-c / 2);				// div coeff by 2
	c16plus = float2fixed16(c / 2);
	c_05 = float2fixed16(0.5);

	acci = mac16(acci, c16plus, cur);			// c = Q14, cur = Q15, acc = Q29 
	acci = mac16(acci, c_05, DelayBuffer[0]);
	acci = mac16(acci, c16min, DelayBuffer[1]);

	DelayBuffer[2] = get_Low14(acci);			// error saved

	acci = LeftShift32(acci, 2);				// compensation of div ->Q30  //Q30 -> Q31

	DelayBuffer[0] = cur;
	DelayBuffer[1] = get_Hi15(acci);

	// processing by formula
	/*cur32 = LeftShift32((int32_t)cur, 16);

	acci = RightShift32(acci, 1);			//dividing by 2 by formulas need
	cur32 = RightShift32(cur32, 1);			//dividing by 2 by formulas need

	res = (int32_t)DelayBuffer[2];			// error saved
	
	if (FilterType == 1) {
		res = adds32(res, cur32);  // LPH
		res = adds32(res, acci);
	}
	else if (FilterType == 2) {
		res = adds32(res, acci);  // HPF    
		res = subs32(cur32, res);
	}
	else res = 0;
	
	DelayBuffer[2] = get_Low15(res);		// error saved
	out = RightShift32(res, 16);*/
	
	return DelayBuffer[1];
}

int16_t filter_Allpass_32(int16_t cur, double c, int16_t *DelayBuffer, int FilterType) { 
	// without Noise Shaping
	// 16 input, 16 states, 32 coeffs
	// FilterType = 1 for LP, 2 = for HP

	int32_t c32min, c32plus, c_05;
	int64_t res, cur64;
	int64_t acci = 0;
	int16_t out;

	c32min = float2fixed(-c / 2);					//div coeff by 2
	c32plus = float2fixed(c / 2);
	c_05 = float2fixed(0.5);

	acci = mac32(acci, c32plus, cur);		// coef = Q30, cur = Q15, acc = Q45
	acci = mac32(acci, c_05, DelayBuffer[0]);
	acci = mac32(acci, c32min, DelayBuffer[1]);

	acci = LeftShift64(acci, 18);			//compensation of div ->Q46  //Q46 -> Q63

	DelayBuffer[0] = cur;
	DelayBuffer[1] = Round64_15(acci);

	// processing by formula
	/*cur64 = LeftShift64((int32_t)cur, 48);

	acci = RightShift64(acci, 1);			//dividing by 2 by formulas need
	cur64 = RightShift64(cur64, 1);			//dividing by 2 by formulas need

	if (FilterType == 1) {
		res = adds64(cur64, acci);  // LPH
	}
	else if (FilterType == 2) {
		res = subs64(cur64, acci);  // HPF    -> Q63
	}
	else res = 0;

	out = Round64_15(res);*/

	return DelayBuffer[1];
}

int16_t filter_Allpass_32_with_NS(int16_t cur, double c, int16_t *DelayBuffer, int32_t *err, int FilterType) {
	// 16 input, 16 states, 32 coeffs WITH Noise Shaping
	// FilterType = 1 for LP, 2 = for HP
	// error is saved in err

	int16_t out;
	int32_t c32min, c32plus, c_05;
	int64_t cur64, res, acci;

	acci = *err;

	c32min = float2fixed(-c / 2);					// div coeff by 2
	c32plus = float2fixed(c / 2);
	c_05 = float2fixed(0.5);

	acci = mac32(acci, c32plus, cur);				// c= Q30, cur = Q15, acc = Q45 
	acci = mac32(acci, c_05, DelayBuffer[0]);
	acci = mac32(acci, c32min, DelayBuffer[1]);

	*err = get_Low30(acci);							// error saved

	acci = LeftShift64(acci, 18);				//compensation of div ->Q46  //Q46 -> Q63

	DelayBuffer[0] = cur;
	DelayBuffer[1] = get_Hi15_31(acci);

	// processing by formula
	/*cur64 = LeftShift64((int64_t)cur, 48);

	acci = RightShift64(acci, 1);			//dividing by 2 by formulas need
	cur64 = RightShift64(cur64, 1);			//dividing by 2 by formulas need

	if (FilterType == 1) {
		res = adds64(cur64, acci);  // LPH
	}
	else if (FilterType == 2) {
		res = subs64(cur64, acci);  // HPF   
	}
	else res = 0;

	out = Round64_15(res);*/

	return DelayBuffer[1];
}

int32_t filter_Allpass_32_32(int32_t cur, double c, int32_t *DelayBuffer, int FilterType) {
	// without Noise Shaping
	// 32 input, 32 states, 32 coeffs
	// FilterType = 1 for LP, 2 = for HP

	int32_t c32min, c32plus, c_05, out;
	int64_t res, cur64;
	int64_t acci = 0;

	c32min = float2fixed(-c / 2);					//div coeff by 2
	c32plus = float2fixed(c / 2);
	c_05 = float2fixed(0.5);

	acci = mac32(acci, c32plus, cur);		// coef = Q30, cur = Q31, acc = Q61
	acci = mac32(acci, c_05, DelayBuffer[0]);
	acci = mac32(acci, c32min, DelayBuffer[1]);

	acci = LeftShift64(acci, 2);			//compensation of div ->Q62  //Q62 -> Q63

	DelayBuffer[0] = cur;
	DelayBuffer[1] = Round64(acci);

	// processing by formula
	cur64 = LeftShift64((int64_t)cur, 32);

	acci = RightShift64(acci, 1);			//dividing by 2 by formulas need
	cur64 = RightShift64(cur64, 1);			//dividing by 2 by formulas need

	if (FilterType == 1) {
		res = adds64(cur64, acci);  // LPH
	}
	else if (FilterType == 2) {
		res = subs64(cur64, acci);  // HPF    -> Q63
	}
	else res = 0;

	out = Round64(res);
	return out;//DelayBuffer[1];
}

int32_t filter_Allpass_32_32_with_NS(int32_t cur, int32_t c, int32_t *DelayBuffer, int FilterType) {
	// 32 input, 32 states, 32 coeffs WITH Noise Shaping
	// FilterType = 1 for LP, 2 = for HP
	// error is saved in DelayBuffer[2] 

	int32_t c32min, c32plus, c_05, out;
	int64_t cur64, res, acci;

	acci = (int32_t)DelayBuffer[2];

	c32plus = RightShift32(c, 1);					// div coeff by 2
	c32min = -c32plus;
	c_05 = 0x40000000;

	acci = mac32(acci, c32plus, cur);				// c= Q30, cur = Q31, acc = Q61 
	acci = mac32(acci, c_05, DelayBuffer[0]);
	acci = mac32(acci, c32min, DelayBuffer[1]);

	DelayBuffer[2] = get_Low30(acci);			// error saved

	acci = LeftShift64(acci, 2);				//compensation of div ->Q62  //Q62 -> Q63

	DelayBuffer[0] = cur;
	DelayBuffer[1] = get_Hi31(acci);

	// processing by formula
	cur64 = LeftShift64((int64_t)cur, 32);

	acci = RightShift64(acci, 1);			//dividing by 2 by formulas need
	cur64 = RightShift64(cur64, 1);			//dividing by 2 by formulas need

	res = (int64_t)DelayBuffer[3];  //err2

	if (FilterType == 1) {
		res = adds64(res, cur64);
		res = adds64(res, acci);  // LPH
	}
	else if (FilterType == 2) {
		res = adds64(res, cur64);
		res = subs64(res, acci);  // HPF    
	}
	else res = 0;

	DelayBuffer[3] = get_Low31(res);  //err2
	out = get_Hi31(res);

	return out;
}
