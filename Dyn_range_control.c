#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <windows.h>
#include <math.h>
#include "Fixed_math32.h"
#include "Filter.h"
#include "Dyn_range_control.h"
#include "Wav.h"

//  COMPRESSORS

void Compressor_init_float(Compressor *compress, double SampleRate, char *SettingsFilename, double *Paramethers) {
	//Paramethers[6] reading from the file

	compress->envelope = 0.0001;
	compress->gain = 1;
	compress->input_prev = 0.0001;

	SettingsReading(SettingsFilename, Paramethers);

	compress->CompressorOn = Paramethers[0];
	compress->CompressorType = Paramethers[1];			// 1 - compressor_downward, 2 - compressor_upward
	compress->attack_gain_time = Paramethers[2];
	compress->release_gain_time = Paramethers[3];
	compress->Threshold_dB = Paramethers[4];
	compress->Ratio = Paramethers[5];
}

void compressor_coeffs(Compressor *compress, Compressor_init *comp_init) {
	//All values are in Q31!

	if (compress->Ratio < 1) {
		printf("Ratio can`t be less than 1!\n");
		system("pause");
		exit(1);
	}
	if (compress->Threshold_dB >= 0) {
		printf("Warning: Threshold in dB is positive or equal to 0!\n");
		system("pause");
	}
	if (compress->attack_gain_time < 0) {
		printf("Attack_gain_time can`t be negative!\n");
		system("pause");
		exit(1);
	}
	if (compress->release_gain_time < 0) {
		printf("Release_gain_time can`t be negative!\n");
		system("pause");
		exit(1);
	}

	//for signal envelope
	double attack_sig = 1.0 - exp((double)-1 / ((compress->SampleRate)*0.0000001));
	double release_sig = 1.0 - exp((double)-1 / ((compress->SampleRate)*0.1));

	// for gain envelope
	double attack_gain = 1.0 - exp((double)-1 / (compress->SampleRate * compress->attack_gain_time));
	double release_gain = 1.0 - exp((double)-1 / (compress->SampleRate * compress->release_gain_time));
	////

	double Slope = 1.0 - ((double)1 / compress->Ratio);
	double CT = pow(10.0, ((double)compress->Threshold_dB) / 20);

	comp_init->attack_sig32 = float2fixed(attack_sig);
	comp_init->release_sig32 = float2fixed(release_sig);
	comp_init->attack_gain32 = float2fixed(attack_gain);
	comp_init->release_gain32 = float2fixed(release_gain);
	comp_init->Slope32 = float2fixed(Slope);
	comp_init->CT32 = float2fixed(CT);

	comp_init->CompressorOn = compress->CompressorOn;
	comp_init->CompressorType = compress->CompressorType;

	if (comp_init->CompressorType == 1) {
		comp_init->gain = INT32MAX;
	}
	else {
		comp_init->gain = INT16MAX;
	}
	comp_init->envelope = float2fixed(0.0001);
	comp_init->inp_prev = float2fixed(0.0001);
}

//downward

int32_t compressor(int32_t input, Compressor_init *comp_init)
{
	// input in Q31, out Q31
	
	int32_t coeff_sig, temp1;
	int64_t temp3, temp4;
	int32_t x = input;
	int32_t CTQ15;
	int32_t SlopeQ27, res1, res2, envQ15, gainQ28;
	int64_t G, coeff_gain, out;

	input = abs32(input);
	gainQ28 = RightShift32(comp_init->gain, 3);
	
	if (input >= comp_init->envelope)
		coeff_sig = comp_init->attack_sig32;
	else
		coeff_sig = comp_init->release_sig32;

	temp1 = subs32((INT32MAX), coeff_sig);  //(1.0 - coeff_sig)
	temp3 = mul64(temp1, comp_init->envelope);
	temp3 = adds64(temp3, (1 << 30));	//round
	temp3 = RightShift64(temp3, 31);    //(1.0 - coeff_sig) * (compress->envelope)

	temp4 = mul64(coeff_sig, input);
	temp4 = adds64(temp4, (1 << 30));	//round
	temp4 = RightShift64(temp4, 31);	//coeff_sig * input

	comp_init->envelope = adds64(temp3, temp4);	//compress->envelope = (1.0 - coeff_sig) * (compress->envelope) + coeff_sig * input;

	if (comp_init->envelope > comp_init->CT32) {
		
		CTQ15 = RightShift32(comp_init->CT32, 16);
		SlopeQ27 = RightShift32(comp_init->Slope32, 4);
		
		res1 = Pow(CTQ15, SlopeQ27); //q27 result

		envQ15 = RightShift32(comp_init->envelope, 16);
		res2 = Pow(envQ15, -SlopeQ27); //q27 result

		temp3 = mul64(res1, res2);
		temp3 = adds64(temp3, (1 << 22));//round
		G = RightShift64(temp3, 26);	// -> Q28 
	}
	else G = RightShift32(INT32MAX, 3); // -> Q28

	if (G < gainQ28)
		coeff_gain = comp_init->attack_gain32;
	else
		coeff_gain = comp_init->release_gain32;

	
	temp1 = subs32((INT32MAX), coeff_gain);  //(1 - coeff_gain)

	temp3 = mul64(temp1, comp_init->gain);
	temp3 = adds64(temp3, (1 << 30));	//round
	temp3 = RightShift64(temp3, 31);    //(1 - coeff_gain) * (compress->gain)

	temp4 = mul64(coeff_gain, G);       //Q31 * Q28 = Q59
	temp4 = adds64(temp4, (1 << 27));	//round
	temp4 = RightShift64(temp4, 28);	//coeff_gain * G;
	
	comp_init->gain = adds32(temp4, temp3);		//compress->gain = (1 - coeff_gain) * (compress->gain) + coeff_gain * G; -> Q31
		
	temp4 = mul64(comp_init->gain, x);
	temp4 = adds64(temp4, (1 << 30));	//round
	out = RightShift64(temp4, 31);	 	//coeff_gain * G;

	comp_init->inp_prev = input;

	return out;
}

double compressor_float(double input, Compressor *compress)
{
	double G, coeff_gain, output, coeff_sig;

	if (compress->Ratio < 1) {
		printf("Ratio can`t be less than 1!\n");
		system("pause");
		exit(1);
	}

	//for signal envelope
	double attack_sig = 1.0 - exp((double)-1 / ((compress->SampleRate)*0.0000001));
	double release_sig = 1.0 - exp((double)-1 / ((compress->SampleRate)*0.01));

	// for gain envelope
	double attack_gain = 1.0 - exp((double)-1 / (compress->SampleRate * compress->attack_gain_time));
	double release_gain = 1.0 - exp((double)-1 / (compress->SampleRate * compress->release_gain_time));
	////

	double Slope = 1.0 - ((double)1 / compress->Ratio);
	double CT = pow(10.0, ((double)compress->Threshold_dB) / 20);		// threshold in units
	////

	double x = input;
	input = fabs(input);
	
	if (input >= compress->envelope)
		coeff_sig = attack_sig;
	else
		coeff_sig = release_sig;

	compress->envelope = (1.0 - coeff_sig) * (compress->envelope) + coeff_sig * input;	

	if (compress->envelope > CT) {
		G = pow(CT, Slope) * pow(compress->envelope, (-Slope));
	}
	else G = 1;

	if (G <= compress->gain)
		coeff_gain = attack_gain;
	else 
		coeff_gain = release_gain;

	compress->gain = (1 - coeff_gain) * (compress->gain) + coeff_gain * G;

	output = compress->gain * x;

	compress->input_prev = input;

	return output;
}

//upward
double compressor_upward_float(double input, Compressor *compress)
{
	double G, coeff_gain, output, coeff_sig;
	double Gmax = 50;

	if (compress->Ratio < 1) {
		printf("Ratio can`t be less than 1!\n");
		system("pause");
		exit(1);
	}

	//for signal envelope
	double attack_sig = 1.0 - exp((double)-1 / ((compress->SampleRate)*0.0000001));
	double release_sig = 1.0 - exp((double)-1 / ((compress->SampleRate)*0.1));

	// for gain envelope
	double attack_gain = 1.0 - exp((double)-1 / (compress->SampleRate * compress->attack_gain_time));
	double release_gain = 1.0 - exp((double)-1 / (compress->SampleRate * compress->release_gain_time));
	////

	double Slope = 1.0 - ((double)1 / compress->Ratio);
	double CT = pow(10.0, ((double)compress->Threshold_dB) / 20);		// threshold in units
																		////
	double x = input;
	input = fabs(input);

	if (input >= compress->envelope)
		coeff_sig = attack_sig;
	else
		coeff_sig = release_sig;

	compress->envelope = (1.0 - coeff_sig) * (compress->envelope) + coeff_sig * input;

	if (compress->envelope < CT) {
		G = pow(CT, Slope) * pow(compress->envelope, (-Slope));

		if (G >= Gmax) {
			G = Gmax;
		}
	}
	else G = 1;

	if (G <= compress->gain)
		coeff_gain = attack_gain;
	else
		coeff_gain = release_gain;

	compress->gain = (1 - coeff_gain) * (compress->gain) + coeff_gain * G;

	output = compress->gain * x;

	compress->input_prev = input;

	return output; //G / 64;//compress->gain / 64;//G / 64; //compress->gain / 64;//G/64;
}

int32_t compressor_upward(int32_t input, Compressor_init *comp_init)
{
	// input in Q31, out Q31, Gmax in Q25, gain in Q15withHR, Slope in Q31
	// !!! could have some mistake in the G calculation (like the stair)
	int32_t x = input;
	int32_t CTQ15, SlopeQ15, res1, res2, envQ15, coeff_sig, temp1, coeff_gainQ15;
	int64_t G, coeff_gain, out, temp3, temp4, gainQ25;
	int32_t Gmax = 50 * (1 << 25);

	input = abs32(input);
	gainQ25 = LeftShift64(comp_init->gain, 10);

	if (input >= comp_init->envelope)
		coeff_sig = comp_init->attack_sig32;
	else
		coeff_sig = comp_init->release_sig32;

	temp1 = subs32((INT32MAX), coeff_sig);  //(1.0 - coeff_sig)
	temp3 = mul64(temp1, comp_init->envelope);
	temp3 = adds64(temp3, (1 << 30));		//round
	temp3 = RightShift64(temp3, 31);		//(1.0 - coeff_sig) * (compress->envelope)

	temp4 = mul64(coeff_sig, input);
	temp4 = adds64(temp4, (1 << 30));		//round
	temp4 = RightShift64(temp4, 31);		//coeff_sig * input

	comp_init->envelope = adds32(temp3, temp4);		//compress->envelope = (1.0 - coeff_sig) * (compress->envelope) + coeff_sig * input;

	if (comp_init->envelope < comp_init->CT32) {

		CTQ15 = RightShift32(comp_init->CT32, 16);
		SlopeQ15 = RightShift32(comp_init->Slope32, 16);
		res1 = PowQ15(CTQ15, SlopeQ15);		//q14 result with bits of hr

		envQ15 = RightShift32(comp_init->envelope, 16);
		res2 = PowQ15(envQ15, -(SlopeQ15));	//q14 result with bits of hr

		temp3 = mul64(res1, res2);			// Q28 with hr
		temp3 = adds64(temp3, (1 << 2));	//round
		G = RightShift64(temp3, 3);			// -> Q25 with hr

		if (G >= Gmax) {
			G = Gmax;
		}
	}
	else G = RightShift32(INT32MAX, 6);		// -> Q25

	if (G <= gainQ25)
		coeff_gain = comp_init->attack_gain32;
	else
		coeff_gain = comp_init->release_gain32;

	coeff_gainQ15 = RightShift32(coeff_gain, 16);

	temp1 = subs32((INT16MAX), coeff_gainQ15); //(1 - coeff_gain)				// -> Q15
	temp3 = mul64(temp1, comp_init->gain);										// Q15 * Q15withHR = Q30withHR

	temp4 = mul64(coeff_gainQ15, G);											// Q15 * Q25 = Q40
	temp4 = adds64(temp4, (1 << 8));			//round
	temp4 = RightShift64(temp4, 10);			//coeff_gain * G;				-> Q30

	comp_init->gain = adds64(temp4, temp3);		//compress->gain = (1 - coeff_gain) * (compress->gain) + coeff_gain * G; -> Q30 + hr
	comp_init->gain = RightShift64(comp_init->gain, 15);						// -> Q15hr

	temp4 = mul64(comp_init->gain, x);											// Q31 * Q15withHR = Q46withHR
	temp4 = adds64(temp4, (1 << 14));			//round
	out = RightShift64(temp4, 15);	 			//coeff_gain * G;

	comp_init->inp_prev = input;

	return out;//LeftShift32(comp_init->gain, 10);//LeftShift32(G, 1);//LeftShift32(comp_init->gain,14);
}



//	EXPANDERS
void expander_coeffs(Expander *expand, Expander_init *expan_init) {
	//All values are in Q31, Slope in Q27hr 

	if ((expand->Ratio > 1) || (expand->Ratio <= 0.2)) {																	//
		printf("Ratio has to be less than 1 and bigger than 0.2 for expander!\n");
		system("pause");
		exit(1);
	}
	if (expand->Threshold_dB >= 0) {
		printf("Warning: Threshold in dB is positive or equal to 0!\n");
		system("pause");
	}
	if (expand->attack_gain_time < 0) {
		printf("Attack_gain_time can`t be negative!\n");
		system("pause");
		exit(1);
	}
	if (expand->release_gain_time < 0) {
		printf("Release_gain_time can`t be negative!\n");
		system("pause");
		exit(1);
	}

	//for signal envelope
	double attack_sig = 1.0 - exp((double)-1 / ((expand->SampleRate)*0.000000001));
	double release_sig = 1.0 - exp((double)-1 / ((expand->SampleRate)*0.01));

	// for gain envelope
	double attack_gain = 1.0 - exp((double)-1 / (expand->SampleRate * expand->attack_gain_time));
	double release_gain = 1.0 - exp((double)-1 / (expand->SampleRate * expand->release_gain_time));
	////

	double Slope = 1.0 - ((double)1 / expand->Ratio);
	double CT = pow(10.0, ((double)expand->Threshold_dB) / 20);		

	expan_init->attack_sig32 = float2fixed(attack_sig);
	expan_init->release_sig32 = float2fixed(release_sig);
	expan_init->attack_gain32 = float2fixed(attack_gain);
	expan_init->release_gain32 = float2fixed(release_gain);
	expan_init->Slope32 = Slope * (double)(1 << 27);     
	expan_init->CT32 = float2fixed(CT);
}


double expander_downward_float(double input, Expander *expand)
{
	double G, coeff_gain, output, coeff_sig;

	//for signal envelope
	double attack_sig = 1.0 - exp((double)-1 / ((expand->SampleRate)*0.0000001));
	double release_sig = 1.0 - exp((double)-1 / ((expand->SampleRate)*0.01));

	// for gain envelope
	double attack_gain = 1.0 - exp((double)-1 / (expand->SampleRate * expand->attack_gain_time));
	double release_gain = 1.0 - exp((double)-1 / (expand->SampleRate * expand->release_gain_time));
	////

	double Slope = 1.0 - ((double)1 / expand->Ratio);
	double CT = pow(10.0, ((double)expand->Threshold_dB) / 20);
	double Gmax = 50;

	double x = input;
	input = fabs(input);

	if (input >= expand->envelope)
		coeff_sig = attack_sig;
	else
		coeff_sig = release_sig;

	expand->envelope = (1.0 - coeff_sig) * (expand->envelope) + coeff_sig * input;

	if (expand->envelope < CT) {

		G = pow(CT, Slope) * pow(expand->envelope, (-Slope));
		if (G >= Gmax) {
			G = Gmax;
		}
	}
	else G = 1;

	if (G <= expand->gain)
		coeff_gain = attack_gain;
	else
		coeff_gain = release_gain;

	expand->gain = (1 - coeff_gain) * (expand->gain) + coeff_gain * G;

	output = expand->gain * x;

	expand->input_prev = input;

	return output;
}

int32_t expander_downward(int32_t input, Expander_init *expan_init, int32_t *envelope, int64_t *gain, int32_t *inp_prev, int32_t Gmax)
{
	// input in Q31, out Q27, Gmax in Q25, gain in Q15withHR, Slope in Q15

	int32_t x = input;
	int32_t CTQ15, SlopeQ15, res1, res2, envQ15, coeff_sig, temp1;
	int64_t G, coeff_gain, out, temp3, temp4, gainQ25;

	input = abs32(input);
	gainQ25 = LeftShift64(*gain, 10);

	if (input >= *envelope)
		coeff_sig = expan_init->attack_sig32;
	else
		coeff_sig = expan_init->release_sig32;

	temp1 = subs32((INT32MAX), coeff_sig);  //(1.0 - coeff_sig)
	temp3 = mul64(temp1, *envelope);
	temp3 = adds64(temp3, (1 << 30));		//round
	temp3 = RightShift64(temp3, 31);		//(1.0 - coeff_sig) * (compress->envelope)

	temp4 = mul64(coeff_sig, input);
	temp4 = adds64(temp4, (1 << 30));		//round
	temp4 = RightShift64(temp4, 31);		//coeff_sig * input

	*envelope = adds64(temp3, temp4);		//compress->envelope = (1.0 - coeff_sig) * (compress->envelope) + coeff_sig * input;

	if (*envelope < expan_init->CT32) {

		CTQ15 = RightShift32(expan_init->CT32, 16);
		SlopeQ15 = RightShift32(expan_init->Slope32, 12);
		res1 = PowQ15(CTQ15, SlopeQ15);		//q14 result with bits of hr

		envQ15 = RightShift32(*envelope, 16);
		res2 = PowQ15(envQ15, -SlopeQ15);	//q14 result with bits of hr

		temp3 = mul64(res1, res2);			// Q28 with hr
		temp3 = adds64(temp3, (1 << 2));	//round
		G = RightShift64(temp3, 3);			// -> Q25 with hr

		if (G >= Gmax) {
			G = Gmax;
		}
	}
	else G = RightShift32(INT32MAX, 6);		// -> Q25

	if (G <= gainQ25)
		coeff_gain = expan_init->attack_gain32;
	else
		coeff_gain = expan_init->release_gain32;


	temp1 = subs64((INT32MAX), coeff_gain); //(1 - coeff_gain)
	temp1 = RightShift32(temp1, 15);				// -> Q16

	temp3 = mul64(temp1, *gain);					// Q16 * Q15withHR = Q31withHR

	temp4 = mul64(coeff_gain, G);					// Q31 * Q25 = Q56
	temp4 = adds64(temp4, (1 << 24));		//round
	temp4 = RightShift64(temp4, 25);		//coeff_gain * G; -> Q31

	*gain = adds64(temp4, temp3);			//compress->gain = (1 - coeff_gain) * (compress->gain) + coeff_gain * G; -> Q31 + hr
	*gain = RightShift64(*gain, 16);				// -> Q15hr

	temp4 = mul64(*gain, x);						// Q31 * Q15withHR = Q46withHR
	temp4 = adds64(temp4, (1 << 18));		//round
	out = RightShift64(temp4, 19);	 		//coeff_gain * G;

	*inp_prev = input;

	return out;
}


double expander_upward_float(double input, Expander *expand)
{
	double G, coeff_gain, output, coeff_sig;

	if (expand->Ratio > 1) {
		printf("Ratio can`t be bigger than 1 for expander!\n");
		system("pause");
		exit(1);
	}

	//for signal envelope
	double attack_sig = 1.0 - exp((double)-1 / ((expand->SampleRate)*0.0000001));
	double release_sig = 1.0 - exp((double)-1 / ((expand->SampleRate)*0.01));

	// for gain envelope
	double attack_gain = 1.0 - exp((double)-1 / (expand->SampleRate * expand->attack_gain_time));
	double release_gain = 1.0 - exp((double)-1 / (expand->SampleRate * expand->release_gain_time));
	////

	double Slope = 1.0 - ((double)1 / expand->Ratio);
	double CT = pow(10.0, ((double)expand->Threshold_dB) / 20);		// threshold in units
	double Gmax = 50;

	double x = input;
	input = fabs(input);

	if (input >= expand->envelope)
		coeff_sig = attack_sig;
	else
		coeff_sig = release_sig;

	expand->envelope = (1.0 - coeff_sig) * (expand->envelope) + coeff_sig * input;

	if (expand->envelope > CT) {
		G = pow(CT, Slope) * pow(expand->envelope, (-Slope));
		if (G >= Gmax) {
			G = Gmax;
		}
	}
	else G = 1;

	if (G <= expand->gain)
		coeff_gain = attack_gain;
	else
		coeff_gain = release_gain;

	expand->gain = (1 - coeff_gain) * (expand->gain) + coeff_gain * G;

	output = expand->gain * x;

	expand->input_prev = input;

	return output;
}

int32_t expander_upward(int32_t input, Expander_init *expan_init, int32_t *envelope, int64_t *gain, int32_t *inp_prev)
{
	// input in Q31, out Q27 + hr because of value bigger than 1!!!

	int32_t coeff_sig, temp1;
	int64_t temp3, temp4;
	int32_t x = input;
	int32_t CTQ15, out;
	int32_t res1, res2, envQ15, gainQ28;
	int64_t G, coeff_gain;

	input = abs32(input);
	gainQ28 = RightShift32(*gain, 3);

	if (input >= *envelope)
		coeff_sig = expan_init->attack_sig32;
	else
		coeff_sig = expan_init->release_sig32;

	temp1 = subs32((INT32MAX), coeff_sig);  //(1.0 - coeff_sig)
	temp3 = mul64(temp1, *envelope);
	temp3 = adds64(temp3, (1 << 30));		//round
	temp3 = RightShift64(temp3, 31);		//(1.0 - coeff_sig) * (compress->envelope)

	temp4 = mul64(coeff_sig, input);
	temp4 = adds64(temp4, (1 << 30));		//round
	temp4 = RightShift64(temp4, 31);		//coeff_sig * input

	*envelope = adds64(temp3, temp4);		//compress->envelope = (1.0 - coeff_sig) * (compress->envelope) + coeff_sig * input;

	if (*envelope > expan_init->CT32) {

		CTQ15 = RightShift32(expan_init->CT32, 16);
		res1 = Pow(CTQ15, expan_init->Slope32); //q27 result

		envQ15 = RightShift32(*envelope, 16);
		res2 = Pow(envQ15, -(expan_init->Slope32)); //q27 result

		temp3 = mul64(res1, res2);
		temp3 = adds64(temp3, (1 << 25));//round
		G = RightShift64(temp3, 26);	// -> Q28 
	}
	else G = RightShift32(INT32MAX, 3); // -> Q28

	if (G < gainQ28)
		coeff_gain = expan_init->attack_gain32;
	else
		coeff_gain = expan_init->release_gain32;


	temp1 = subs32((INT32MAX), coeff_gain);  //(1 - coeff_gain)

	temp3 = mul64(temp1, *gain);											  //Q31 * Q15 = Q46
	temp3 = adds64(temp3, (1 << 20));	//round
	temp3 = RightShift64(temp3, 21);    //(1 - coeff_gain) * (compress->gain) //->Q25

	temp4 = mul64(coeff_gain, G);											  //Q31 * Q28 = Q59
	temp4 = adds64(temp4, (1 << 33));	//round
	temp4 = RightShift64(temp4, 34);	//coeff_gain * G;					  //-> Q25

	*gain = adds64(temp4, temp3);		//compress->gain = (1 - coeff_gain) * (compress->gain) + coeff_gain * G; -> Q25 
	*gain = RightShift64(*gain, 10);										  // -> Q15

	temp4 = mul64(*gain, x);												  // Q15 * Q31 = Q46
	temp4 = adds64(temp4, (1 << 14));	//round
	out = RightShift64(temp4, 19);	 	//coeff_gain * G;					  // Q27 + hr
	
	*inp_prev = input;

	return out; // Q27 + hr
}


// LIMITER

int CirBufferPut_double(double NewSamp, int IndexEnd, double *CirBuf) {  //double

	IndexEnd++;
	IndexEnd &= LAB_BUFFER_MASK;
	CirBuf[IndexEnd] = NewSamp;

	return IndexEnd;
}

double limiter_float(double input, Limiter *limit)
{
	double G, coeff_gain, coeff_sig;
	
	//for signal envelope
	double attack_sig = 1.0 - exp((double)-1 / ((limit->SampleRate)*0.00001));
	double release_sig = 1.0 - exp((double)-1 / ((limit->SampleRate)*0.01));

	// for gain envelope
	double attack_gain = 1.0 - exp((double)-1 / (limit->SampleRate * limit->attack_gain_time));
	double release_gain = 1.0 - exp((double)-1 / (limit->SampleRate * limit->release_gain_time));
	////

	double CT = pow(10.0, ((double)limit->Threshold_dB) / 20);
	
	double x = input;
	input = fabs(input);
	
	if (input >= limit->envelope)
		coeff_sig = attack_sig;
	else
		coeff_sig = release_sig;

	limit->envelope = (1.0 - coeff_sig) * (limit->envelope) + coeff_sig * input;

	if (limit->envelope > CT) {
		G = CT * pow(limit->envelope, -1);
	}
	else G = 1;

	if (G <= limit->gain)
		coeff_gain = attack_gain;
	else
		coeff_gain = release_gain;

	limit->gain = (1 - coeff_gain) * (limit->gain) + coeff_gain * G;

	//output = limit->gain * x;

	limit->input_prev = input;

	return limit->gain;
}

double limiter_float_laf(double curf, int *IndexEnd, int *IndexCur, double *lab_signal, double *MaxBufferValue, Limiter *limit) {
	double outf;

	*IndexEnd = CirBufferPut_double(curf, *IndexEnd, lab_signal);

	for (int r = 0; r < LAB_BUFFER_SIZE; r++) {
		if (lab_signal[r] >= *MaxBufferValue) {
			*MaxBufferValue = lab_signal[r];
		}
	}

	limit->gain = limiter_float(*MaxBufferValue, limit);

	*IndexCur = *IndexEnd + 1;
	*IndexCur &= LAB_BUFFER_MASK;
	outf = limit->gain * lab_signal[*IndexCur];

	return outf;
}


void limiter_coeffs(Limiter *limit, Limiter_init *limit_init) {
	//All values are in Q31, attack and release values in Q15

	if (limit->Threshold_dB >= 0) {
		printf("Warning: Threshold in dB is positive or equal to 0!\n");
		system("pause");
	}
	if (limit->attack_gain_time < 0) {
		printf("Attack_gain_time can`t be negative!\n");
		system("pause");
		exit(1);
	}
	if (limit->release_gain_time < 0) {
		printf("Release_gain_time can`t be negative!\n");
		system("pause");
		exit(1);
	}

	//for signal envelope
	double attack_sig = 1.0 - exp((double)-1 / ((limit->SampleRate)*0.000001));
	double release_sig = 1.0 - exp((double)-1 / ((limit->SampleRate)*0.01));

	// for gain envelope
	double attack_gain = 1.0 - exp((double)-1 / (limit->SampleRate * limit->attack_gain_time));
	double release_gain = 1.0 - exp((double)-1 / (limit->SampleRate * limit->release_gain_time));
	////

	double CT = pow(10.0, ((double)limit->Threshold_dB) / 20);		

	limit_init->attack_sig32 = float2fixed16(attack_sig);
	limit_init->release_sig32 = float2fixed16(release_sig);
	limit_init->attack_gain32 = float2fixed16(attack_gain);
	limit_init->release_gain32 = float2fixed16(release_gain);
	limit_init->CT32 = float2fixed(CT);

	limit_init->envelope = float2fixed(0.0001);
	limit_init->gain = (double)(1 << 15);      //Q15
	limit_init->inp_prev = float2fixed(0.0001);
}

int CirBufferPut_lab(int32_t NewSamp, int IndexEnd, int32_t *CirBuf) {

	IndexEnd++;
	IndexEnd &= LAB_BUFFER_MASK;
	CirBuf[IndexEnd] = NewSamp;

	return IndexEnd;
}

int64_t limiter(int32_t input, Limiter_init *limit_init)
{
	// input in Q31 with possible bigger than 1, out Q31, gain in Q15withHR
	int32_t x = input;
	int32_t CTQ27, CTQ14, res2, envQ15, coeff_sig, temp1;
	int64_t G, coeff_gain, temp3, temp4, gainQ25;

	input = abs32(input);
	gainQ25 = LeftShift64(limit_init->gain, 10);
	CTQ27 = RightShift32(limit_init->CT32, 4);

	if (input >= limit_init->envelope)
		coeff_sig = limit_init->attack_sig32;		//Q15
	else
		coeff_sig = limit_init->release_sig32;

	temp1 = subs32((INT16MAX), coeff_sig);		//(1.0 - coeff_sig)										// Q15
	temp3 = mul64(temp1, limit_init->envelope);	//(1.0 - coeff_sig) * (compress->envelope)				// Q15 * Q27 = Q42
	
	temp4 = mul64(coeff_sig, input);																	// Q15 * Q27 = Q42

	limit_init->envelope = adds64(temp3, temp4);		//compress->envelope = (1.0 - coeff_sig) * (compress->envelope) + coeff_sig * input;		//Q42
	limit_init->envelope = RightShift64(limit_init->envelope, 15);					// Q27
	/////////////
	if (limit_init->envelope > CTQ27) {

		CTQ14 = RightShift32(limit_init->CT32, 17);		//q14 result with bits of hr
	
		envQ15 = RightShift32(limit_init->envelope, 12);
		res2 = PowQ15(envQ15, 0xffff8000);				//q14 result with bits of hr

		temp3 = mul64(CTQ14, res2);			// Q28 with hr
		temp3 = adds64(temp3, (1 << 2));	//round
		G = RightShift64(temp3, 3);			// -> Q25 with hr
	}
	else G = RightShift32(INT32MAX, 6);		// -> Q25

	if (G <= gainQ25)
		coeff_gain = limit_init->attack_gain32;
	else
		coeff_gain = limit_init->release_gain32;


	temp1 = subs64((INT16MAX), coeff_gain); //(1 - coeff_gain)	// -> Q15
	temp3 = mul64(temp1, limit_init->gain);						// Q15 * Q15withHR = Q30withHR

	temp4 = mul64(coeff_gain, G);					// Q15 * Q25 = Q40
	temp4 = adds64(temp4, (1 << 9));		//round
	temp4 = RightShift64(temp4, 10);		//coeff_gain * G; -> Q30

	limit_init->gain = adds64(temp4, temp3);			//compress->gain = (1 - coeff_gain) * (compress->gain) + coeff_gain * G; -> Q30 + hr
	limit_init->gain = RightShift64(limit_init->gain, 15);				// -> Q15hr

	limit_init->inp_prev = input;

	return limit_init->gain;
}

int32_t limiter_laf(int32_t cur, int *IndexEnd, int *IndexCur, int32_t *lab_signal, int32_t *MaxBufferValue, Limiter_init *limit_init) {
	//input Q27, out Q31
	int32_t out;
	int64_t temp, gain;

	*IndexEnd = CirBufferPut_lab(cur, *IndexEnd, lab_signal);

	for (int r = 0; r < LAB_BUFFER_SIZE; r++) {
		if (lab_signal[r] >= *MaxBufferValue) {
			*MaxBufferValue = lab_signal[r];
		}
	}

	gain = limiter(cur, limit_init);

	*IndexCur = *IndexEnd + 1;
	*IndexCur &= LAB_BUFFER_MASK;

	temp = mul64(gain, lab_signal[*IndexCur]); //-> Q15 * Q27 = Q42
	temp = adds64(temp, (1 << 10));
	out = RightShift64(temp, 11); //Q31
	
	return out;
}


// CROSSOVER

void Crossover2bands_init_float(Crossover *cross, double CutOffFreq, double SampleRate) {
	//with AllPass
	cross->SampleRate = SampleRate;

	cross->Allpass_coeff = AllpassDesign(SampleRate, CutOffFreq);

	memset(&cross->DelayBufferf_1ord, 0, sizeof(double) * 2);
	memset(&cross->DelayBufferf_2ord, 0, sizeof(double) * 2);

	cross->accf_low = 0;
	cross->accf_high = 0;
}

void Crossover2bands_init(Crossover *cross, Crossover_init *cross_init) {
	//All values are in Q31!

	cross_init->Allpass_coeff = float2fixed(cross->Allpass_coeff);

	cross_init->acc_high = 0;
	cross_init->acc_low = 0;

	memset(cross_init->DelayBuffer_1ord, 0, sizeof(int32_t) * 3);
	memset(cross_init->DelayBuffer_2ord, 0, sizeof(int32_t) * 3);
}

void Crossover2bands_float(double curf, Crossover *cross) {
	//with AllPass

	cross->accf_low = filter_float_Allpass(curf, cross->Allpass_coeff, cross->DelayBufferf_1ord, 1);
	//cross->accf_low = filter_float_Allpass(cross->accf_low, cross->Allpass_coeff, &cross->DelayBufferf_2ord, 1);

	cross->accf_high = curf - cross->accf_low;
}

void Crossover2bands(int32_t cur, Crossover_init *cross_init) {
	//with AllPass

	cross_init->acc_low = filter_Allpass_32_32_with_NS(cur, cross_init->Allpass_coeff, cross_init->DelayBuffer_1ord, 1);
	//cross_init->acc_low = filter_Allpass_32_32_with_NS(cur, cross_init->Allpass_coeff, &cross_init->DelayBuffer_2ord,  1);

	cross_init->acc_high = subs32(cur, cross_init->acc_low);
}

void Four_Compressors_init_float(Compressor *compress1, Compressor *compress2, Compressor *compress3, Compressor *compress4, double SampleRate, char *SettingsFilename, double *Paramethers) {
	//Paramethers[31];  reading from the file

	compress1->envelope = 0.0001;
	compress1->gain = 1;
	compress1->input_prev = 0.0001;
	compress1->SampleRate = SampleRate;

	compress2->envelope = 0.0001;
	compress2->gain = 1;
	compress2->input_prev = 0.0001;
	compress2->SampleRate = SampleRate;

	compress3->envelope = 0.0001;
	compress3->gain = 1;
	compress3->input_prev = 0.0001;
	compress3->SampleRate = SampleRate;

	compress4->envelope = 0.0001;
	compress4->gain = 1;
	compress4->input_prev = 0.0001;
	compress4->SampleRate = SampleRate;

	SettingsReading(SettingsFilename, Paramethers);

	compress1->CompressorOn = Paramethers[1];
	compress1->CompressorType = Paramethers[2];			// 1 - compressor_downward, 2 - compressor_upward
	compress1->attack_gain_time = Paramethers[3];
	compress1->release_gain_time = Paramethers[4];
	compress1->Threshold_dB = Paramethers[5];
	compress1->Ratio = Paramethers[6];

	compress2->CompressorOn = Paramethers[9];
	compress2->CompressorType = Paramethers[10];			// 1 - compressor_downward, 2 - compressor_upward
	compress2->attack_gain_time = Paramethers[11];
	compress2->release_gain_time = Paramethers[12];
	compress2->Threshold_dB = Paramethers[13];
	compress2->Ratio = Paramethers[14];

	compress3->CompressorOn = Paramethers[17];
	compress3->CompressorType = Paramethers[18];			// 1 - compressor_downward, 2 - compressor_upward
	compress3->attack_gain_time = Paramethers[19];
	compress3->release_gain_time = Paramethers[20];
	compress3->Threshold_dB = Paramethers[21];
	compress3->Ratio = Paramethers[22];

	compress4->CompressorOn = Paramethers[25];
	compress4->CompressorType = Paramethers[26];			// 1 - compressor_downward, 2 - compressor_upward
	compress4->attack_gain_time = Paramethers[27];
	compress4->release_gain_time = Paramethers[28];
	compress4->Threshold_dB = Paramethers[29];
	compress4->Ratio = Paramethers[30];
}

// NOISE GATE 

double NoiseGate_float(double input, NoiseGate *Noise_gate)
{
	double G, coeff_gain, output, coeff_sig;
	double temp1, temp2, temp3;

	//for signal envelope
	double attack_sig = 1.0 - exp((double)-1 / ((Noise_gate->SampleRate)*0.0000001));
	double release_sig = 1.0 - exp((double)-1 / ((Noise_gate->SampleRate)*0.1));

	// for gain envelope
	double attack_gain = 1.0 - exp((double)-1 / (Noise_gate->SampleRate * Noise_gate->attack_gain_time));
	double release_gain = 1.0 - exp((double)-1 / (Noise_gate->SampleRate * Noise_gate->release_gain_time));
	////

	double CT = pow(10.0, ((double)Noise_gate->Threshold_dB) / 20);

	double x = input;
	input = fabs(input);

	if (input >= Noise_gate->envelope)
		coeff_sig = attack_sig;
	else
		coeff_sig = release_sig;

	Noise_gate->envelope = (1.0 - coeff_sig) * (Noise_gate->envelope) + coeff_sig * input;

	if (Noise_gate->envelope < CT) {
		G = 0;
	}
	else G = 1;

	if (G <= Noise_gate->gain)
		coeff_gain = attack_gain;
	else
		coeff_gain = release_gain;

	Noise_gate->gain = (1 - coeff_gain) * (Noise_gate->gain) + coeff_gain * G;

	output = Noise_gate->gain * x;

	Noise_gate->input_prev = input;

	return output; //Noise_gate->gain;
}

void NoiseGate_coeffs(NoiseGate *Noise_gate, NoiseGate_init *NoiseGate_init) {
	//All values are in Q31

	if (Noise_gate->Threshold_dB >= 0) {
		printf("Warning: Threshold in dB is positive or equal to 0!\n");
		system("pause");
	}
	if (Noise_gate->attack_gain_time < 0) {
		printf("Attack_gain_time can`t be negative!\n");
		system("pause");
		exit(1);
	}
	if (Noise_gate->release_gain_time < 0) {
		printf("Release_gain_time can`t be negative!\n");
		system("pause");
		exit(1);
	}

	//for signal envelope
	double attack_sig = 1.0 - exp((double)-1 / ((Noise_gate->SampleRate)*0.0000001));
	double release_sig = 1.0 - exp((double)-1 / ((Noise_gate->SampleRate)*0.1));

	// for gain envelope
	double attack_gain = 1.0 - exp((double)-1 / (Noise_gate->SampleRate * Noise_gate->attack_gain_time));
	double release_gain = 1.0 - exp((double)-1 / (Noise_gate->SampleRate * Noise_gate->release_gain_time));
	////

	double CT = pow(10.0, ((double)Noise_gate->Threshold_dB) / 20);		// threshold in units

	NoiseGate_init->attack_sig32 = float2fixed(attack_sig);
	NoiseGate_init->release_sig32 = float2fixed(release_sig);
	NoiseGate_init->attack_gain32 = float2fixed(attack_gain);
	NoiseGate_init->release_gain32 = float2fixed(release_gain);
	NoiseGate_init->CT32 = float2fixed(CT);

	NoiseGate_init->envelope = float2fixed(0.0001);
	NoiseGate_init->gain = (double)(1 << 15);      //Q15
	NoiseGate_init->inp_prev = float2fixed(0.0001);
}

int32_t noise_gate(int32_t input, NoiseGate_init *NoiseGate_init)
{
	// input in Q31, out Q31, gain in Q15withHR
	int32_t x = input;
	int32_t CTQ14, res2, envQ15, coeff_sig, temp1, temp2;
	int64_t G, coeff_gain, out, temp3, temp4, gainQ25;

	input = abs32(input);
	gainQ25 = LeftShift32(NoiseGate_init->gain, 10);

	if (input >= NoiseGate_init->envelope)
		coeff_sig = NoiseGate_init->attack_sig32;
	else
		coeff_sig = NoiseGate_init->release_sig32;

	temp1 = subs32((INT32MAX), coeff_sig);  //(1.0 - coeff_sig)
	temp3 = mul64(temp1, NoiseGate_init->envelope);
	temp3 = adds64(temp3, (1 << 30));		//round
	temp3 = RightShift64(temp3, 31);		//(1.0 - coeff_sig) * (compress->envelope)

	temp4 = mul64(coeff_sig, input);
	temp4 = adds64(temp4, (1 << 30));		//round
	temp4 = RightShift64(temp4, 31);		//coeff_sig * input

	NoiseGate_init->envelope = adds64(temp3, temp4);		//compress->envelope = (1.0 - coeff_sig) * (compress->envelope) + coeff_sig * input;

	if (NoiseGate_init->envelope < NoiseGate_init->CT32) {
		G = 0;								// -> Q25 with hr
	}
	else G = RightShift32(INT32MAX, 6);		// -> Q25

	if (G <= gainQ25)
		coeff_gain = NoiseGate_init->attack_gain32;
	else
		coeff_gain = NoiseGate_init->release_gain32;


	temp1 = subs64((INT32MAX), coeff_gain); //(1 - coeff_gain)
	temp1 = RightShift32(temp1, 15);							// -> Q16

	temp3 = mul64(temp1, NoiseGate_init->gain);					// Q16 * Q15withHR = Q31withHR

	temp4 = mul64(coeff_gain, G);								// Q31 * Q25 = Q56
	temp4 = adds64(temp4, (1 << 24));		//round
	temp4 = RightShift64(temp4, 25);		//coeff_gain * G; -> Q31

	NoiseGate_init->gain = adds64(temp4, temp3); //compress->gain = (1 - coeff_gain) * (compress->gain) + coeff_gain * G; -> Q31 + hr
	NoiseGate_init->gain = RightShift64(NoiseGate_init->gain, 16);// -> Q15hr

	temp4 = mul64(NoiseGate_init->gain, x);							// Q31 * Q15withHR = Q46withHR
	temp4 = adds64(temp4, (1 << 14));		//round
	out = RightShift64(temp4, 15);	 		//coeff_gain * G;

	NoiseGate_init->inp_prev = input;

	return out;
}
