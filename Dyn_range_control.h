#ifndef __DYN_RANGE_H

#define __DYN_RANGE_H

#include <stdint.h>

#define LAB_BUFFER_SIZE 64			//CirBuf
#define LAB_BUFFER_MASK (LAB_BUFFER_SIZE-1)

//	COMPRESSOR

typedef struct {  //for double
	double Ratio;
	double Threshold_dB;
	double SampleRate;
	double attack_gain_time;
	double release_gain_time;
	double envelope;
	double gain;
	double input_prev;
	int CompressorOn;
	int CompressorType;
} Compressor;

typedef struct {  //values are obtained in 'compressor coeffs' function FIX
	int32_t attack_sig32;
	int32_t release_sig32;
	int32_t attack_gain32;
	int32_t release_gain32;
	int32_t Slope32;
	int32_t CT32;
	int32_t envelope;
	int64_t gain;
	int32_t inp_prev;
	int CompressorOn;
	int CompressorType;
} Compressor_init;

void Compressor_init_float(Compressor *compress, double SampleRate, char *SettingsFilename, double *Paramethers);

void compressor_coeffs(Compressor *compress, Compressor_init *comp_init);

int32_t compressor(int32_t input, Compressor_init *comp_init);

double compressor_float(double input, Compressor *compress);


double compressor_upward_float(double input, Compressor *compress);

int32_t compressor_upward(int32_t input, Compressor_init *comp_init);


//	EXPANDERS

typedef struct {  //for double
	double Ratio;
	double Threshold_dB;
	double SampleRate;
	double attack_gain_time;
	double release_gain_time;
	double envelope;
	double gain;
	double input_prev;
} Expander;

typedef struct {  //values are obtained in 'Expander coeffs' function FIX
	int32_t attack_sig32;
	int32_t release_sig32;
	int32_t attack_gain32;
	int32_t release_gain32;
	int32_t Slope32;
	int32_t CT32;
} Expander_init;

void expander_coeffs(Expander *expand, Expander_init *expan_init);

//expander_DOWNward
double expander_downward_float(double input, Expander *expand);

int32_t expander_downward(int32_t input, Expander_init *expan_init, int32_t *envelope, int64_t *gain, int32_t *inp_prev, int32_t Gmax);

//expander_UPward

double expander_upward_float(double input, Expander *expand);

int32_t expander_upward(int32_t input, Expander_init *expan_init, int32_t *envelope, int64_t *gain, int32_t *inp_prev);


//	LIMITER

typedef struct {  //for double
	double Threshold_dB;
	double SampleRate;
	double attack_gain_time;
	double release_gain_time;
	double envelope;
	double gain;
	double input_prev;
} Limiter;

typedef struct {  //values are obtained in 'Expander coeffs' function FIX
	int32_t attack_sig32;
	int32_t release_sig32;
	int32_t attack_gain32;
	int32_t release_gain32;
	int32_t CT32;
	int64_t envelope;
	int64_t gain;
	int64_t inp_prev;
} Limiter_init;

int CirBufferPut_double(double NewSamp, int IndexEnd, double *CirBuf);

double limiter_float(double input, Limiter *limit);

double limiter_float_laf(double curf, int *IndexEnd, int *IndexCur, double *lab_signal, double *MaxBufferValue, Limiter *limit);


void limiter_coeffs(Limiter *limit, Limiter_init *limit_init);

int CirBufferPut_lab(int32_t NewSamp, int IndexEnd, int32_t *CirBuf);

int64_t limiter(int32_t input, Limiter_init *limit_init);

int32_t limiter_laf(int32_t cur, int *IndexEnd, int *IndexCur, int32_t *lab_signal, int32_t *MaxBufferValue, Limiter_init *limit_init);

// CROSSOVER

typedef struct {  //for double
	double SampleRate;

	double DelayBufferf_1ord[2];
	double DelayBufferf_2ord[2];
	double Allpass_coeff;

	double accf_low;
	double accf_high;
} Crossover;

typedef struct {  //for double
	int32_t DelayBuffer_1ord[3];
	int32_t DelayBuffer_2ord[3];
	int32_t Allpass_coeff;

	int32_t acc_low;
	int32_t acc_high;
} Crossover_init;

void Crossover2bands_init_float(Crossover *cross, double CutOffFreq, double SampleRate);

void Crossover2bands_init(Crossover *cross, Crossover_init *cross_init);

void Crossover2bands_float(double curf, Crossover *cross);

void Crossover2bands(int32_t cur, Crossover_init *cross_init);

void Four_Compressors_init_float(Compressor *compress1, Compressor *compress2, Compressor *compress3, Compressor *compress4, double SampleRate, char *SettingsFilename, double *Paramethers);

		
// NOISE GATE

typedef struct {  //for double
	double Threshold_dB;
	double SampleRate;
	double attack_gain_time;
	double release_gain_time;
	double envelope;
	double gain;
	double input_prev;
} NoiseGate;

typedef struct {  //values are obtained in 'NoiseGate coeffs' function FIX
	int32_t attack_sig32;
	int32_t release_sig32;
	int32_t attack_gain32;
	int32_t release_gain32;
	int32_t CT32;
	int32_t envelope;
	int32_t gain;      //Q15
	int32_t inp_prev;
} NoiseGate_init;

double NoiseGate_float(double input, NoiseGate *Noise_gate);

void NoiseGate_coeffs(NoiseGate *Noise_gate, NoiseGate_init *NoiseGate_init);

int32_t noise_gate(int32_t input, NoiseGate_init *NoiseGate_init);

#endif
