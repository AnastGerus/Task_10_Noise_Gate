#define _CRT_SECURE_NO_WARNINGS

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <windows.h>
#include <math.h>
#include "Fixed_math32.h"
#include "Wav.h"
#include "Filter.h"
#include "Dyn_range_control.h"

int main() {
	//NOISE GATE
	double curf, outf;

	FILE *fp = fopen("Sound1.wav", "rb");
	if (fp == NULL) {
		printf("Cannot open the file Sound1.wav");
		system("pause");
		exit(1);
	}
	FILE *fp2 = fopen("Sound2_float.wav", "wb");
	if (fp2 == NULL) {
		printf("Cannot open the file Sound2_float.wav");
		system("pause");
		exit(1);
	}
	FILE *fp3 = fopen("Sound2_fix.wav", "wb");
	if (fp3 == NULL) {
		printf("Cannot open the file Sound2_fix.wav");
		system("pause");
		exit(1);
	}

	HEADER header;
	fread(&header, sizeof(header), 1, fp);
	fwrite(&header, sizeof(header), 1, fp2);
	fwrite(&header, sizeof(header), 1, fp3);

	NoiseGate Noise_gate;
	NoiseGate_init NoiseGate_init;
	Noise_gate.attack_gain_time = 0.00001;
	Noise_gate.release_gain_time = 0.01;
	Noise_gate.Threshold_dB = -3;

	Noise_gate.envelope = 0.0001;
	Noise_gate.gain = 1;
	Noise_gate.input_prev = 0.0001;
	Noise_gate.SampleRate = header.SampleRate;

	NoiseGate_coeffs(&Noise_gate, &NoiseGate_init);

	int16_t buffer[buffer_size];
	memset(buffer, 0, sizeof(int16_t) * buffer_size);
	size_t size_read;
	int16_t cur;
	int32_t current, out_32;

	int16_t bufferf[buffer_size];   //float
	memset(bufferf, 0, sizeof(int16_t) * buffer_size);

	while (1) {
		size_read = fread(buffer, sizeof(int16_t), buffer_size, fp);

		if (size_read == 0) {
			break;
		}
		for (unsigned int i = 0; i < (size_read >> 1); i++) {
			cur = buffer[i * 2];
			current = LeftShift32(cur, 16); //to Q31

			curf = fixed2float16(cur);
			outf = NoiseGate_float(curf, &Noise_gate);
			bufferf[i * 2] = float2fixed16(outf);

			out_32 = noise_gate(current, &NoiseGate_init);
			buffer[i * 2] = RightShift32(out_32, 16);
		}

		fwrite(bufferf, sizeof(int16_t), size_read, fp2); //float
		fwrite(buffer, sizeof(int16_t), size_read, fp3);
	}
	fclose(fp);
	fclose(fp2);
	fclose(fp3);

	return 0;
}
