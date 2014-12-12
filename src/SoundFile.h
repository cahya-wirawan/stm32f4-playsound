//
//  SoundFile.h
//  SoundWave
//
//  Created by Cahya Wirawan on 1/15/12.
//  Copyright (c) 2012 __MyCompanyName__. All rights reserved.
//

#ifndef SoundWave_SoundFile_h
#define SoundWave_SoundFile_h

#include <sndfile.h>
#include "Sound.h"

#define SOUNDFILE_SAMPLERATE 44100
#define SOUNDFILE_CHANNEL 2
#define SOUNDFILE_MAX

int SoundFile_WriteWav(char *filename, NUMBER_TYPE *buffer, int length, int channels);
NUMBER_TYPE *SoundFile_ReadAll(char *filename, sf_count_t *length);
int SoundFile_Open(char *filename);
sf_count_t SoundFile_Read(NUMBER_TYPE *stereoBuffer, sf_count_t length);
sf_count_t SoundFile_ReadOffset(NUMBER_TYPE *stereoBuffer, int offset, sf_count_t length);
uint32_t SoundFile_Read_Short(SNDFILE *_SoundFile_Object, uint8_t *buffer, uint32_t length);
uint32_t SoundFile_Read_Float(SNDFILE *_SoundFile_Object, float32_t *buffer, uint32_t length);
void SoundFile_Close(void);

#endif
