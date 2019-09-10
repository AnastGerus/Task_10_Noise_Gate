#ifndef __WAV_H

#define __WAV_H

#include <stdint.h>

#define buffer_size (1 << 8)				//reading buffer
#define buffer_size_2 (buffer_size << 1)

typedef struct {
	uint8_t ChunkID[4];
	uint32_t ChunkSize;
	uint8_t Format[4];
	uint8_t Subchunk1ID[4];
	uint32_t Subchunk1Size;
	uint16_t AudioFormat;
	uint16_t NumChannels;
	uint32_t SampleRate;
	uint32_t ByteRate;
	uint16_t BlockAlign;
	uint16_t BitsPerSample;
	uint8_t Subchunk2ID[4];
	uint32_t Subchunk2Size;
} HEADER;

void HeaderCreation(HEADER *header, int SampleRate, int NumChannels, int BitsPerSample, int NumSamples);

//Generation

void SweepGenerator(int f_start, double Amplitude, double *SweepSignal, int SamplesNum, int SampleRate);

void ToneGenerator(int f, double Amplitude, int SampleRate, double *ToneSignal, int SamplesNum);

void NoiseGenerator(double Variance, double *Noise, int SamplesNum);


void SettingsReading(char *Filename, double *Paramethers);

#endif
