#define _CRT_SECURE_NO_WARNINGS

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <windows.h>
#include <math.h>
#include "Fixed_math32.h"
#include "Wav.h"

void HeaderCreation(HEADER *header, int SampleRate, int NumChannels, int BitsPerSample, int NumSamples) {

	header->ChunkID[0] = 'R';
	header->ChunkID[1] = 'I';
	header->ChunkID[2] = 'F';
	header->ChunkID[3] = 'F';
	header->ChunkSize = 36 + (NumSamples * NumChannels * BitsPerSample / 8);
	header->Format[0] = 'W';
	header->Format[1] = 'A';
	header->Format[2] = 'V';
	header->Format[3] = 'E';
	header->Subchunk1ID[0] = 'f';
	header->Subchunk1ID[1] = 'm';
	header->Subchunk1ID[2] = 't';
	header->Subchunk1ID[3] = ' ';
	header->Subchunk1Size = 16;
	header->AudioFormat = 1;
	header->NumChannels = NumChannels;
	header->SampleRate = SampleRate;
	header->ByteRate = SampleRate * NumChannels * BitsPerSample / 8;
	header->BlockAlign = NumChannels * BitsPerSample / 8;
	header->BitsPerSample = BitsPerSample;
	header->Subchunk2ID[0] = 'd';
	header->Subchunk2ID[1] = 'a';
	header->Subchunk2ID[2] = 't';
	header->Subchunk2ID[3] = 'a';
	header->Subchunk2Size = NumSamples * NumChannels * BitsPerSample / 8;
};

//Generation

void SweepGenerator(int f_start, double Amplitude, double *SweepSignal, int SamplesNum, int SampleRate) {
	//f_stop < SampleRate / 2 according to Kotelnikova theorem

	int f_stop = SampleRate/2;
	double delta_f;
	double f_new;

	for (int index = 0; index<SamplesNum; index++) {

		delta_f = (f_stop - f_start) / SamplesNum;

		f_new = f_start + delta_f* index;

		SweepSignal[index] = Amplitude * sin(2 * PI* f_new * index / SampleRate);
	}
}

void ToneGenerator(int f, double Amplitude, int SampleRate, double *ToneSignal, int SamplesNum) {

	for (int index = 0; index<SamplesNum; index++) {
		ToneSignal[index] = Amplitude * sin(2 * PI *f *index / SampleRate);
	}
}

void NoiseGenerator(double Variance, double *Noise, int SamplesNum) {

	int RandMax = RAND_MAX / 2;
	for (int index = 0; index < SamplesNum; index++) {
		Noise[index] = sqrt(Variance) + ((((double)rand()) / RandMax) - 1);
	}
}

void SettingsReading(char *Filename, double *Paramethers) {
	//calculated are all strings!

	FILE *ptrFile = fopen(Filename, "r");
	char mystring[100];
	int count, LineCount = 0;
	char ValueString[20];

	if (ptrFile == NULL) {
		printf("Cannot open the settings file.wav");
		system("pause");
		exit(1);
	}
	else
	{
		while (fgets(mystring, sizeof(mystring), ptrFile) != NULL)	//EOF
		{
			for (count = 0; mystring[count] != '\0'; count++)
			{
				if ((isdigit(mystring[count])) || (mystring[count] == '-') || (mystring[count] == '+')) {
					// Found a number
					for (int i = 0; i < 10; i++) {
						ValueString[i] = mystring[count + i];
					}
					Paramethers[LineCount] = atof(ValueString);
					break;
				}
			}
			LineCount++;
		}
		fclose(ptrFile);
	}
}
