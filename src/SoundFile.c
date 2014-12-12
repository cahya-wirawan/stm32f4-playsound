//
//  SoundFile.c
//  SoundWave
//
//  Created by Cahya Wirawan on 1/15/12.
//  Copyright (c) 2012 __MyCompanyName__. All rights reserved.
//

#include <stdio.h>
#include <stdlib.h>
#include "SoundFile.h"
#include "DSP.h"

SNDFILE *_SoundFile_Object;
SF_INFO _SoundFile_Info;

int SoundFile_WriteWav(char *filename, NUMBER_TYPE *buffer, int length, int channels) {   
  if(!buffer)
    return -1;
  // Set file settings, 16bit Mono PCM
  SF_INFO info;
  info.format = SF_FORMAT_WAV | SF_FORMAT_PCM_16;
  info.channels = channels;
  info.samplerate = SOUNDFILE_SAMPLERATE;
  
  // Open sound file for writing
  SNDFILE *sndFile = sf_open(filename, SFM_WRITE, &info);
  if (sndFile == NULL) {
    fprintf(stderr, "Error opening sound file '%s': %s\n", filename, sf_strerror(sndFile));
    return -1;
  }
  
  // Write frames
#ifdef FIXED_POINT
  long writtenFrames = sf_writef_short(sndFile, buffer, length);
#else
  #ifdef NUMBER_TYPE_FLOAT
    long writtenFrames = sf_writef_float(sndFile, buffer, length);
  #else
    long writtenFrames = sf_writef_double(sndFile, buffer, length);
  #endif
#endif
  
  ///free(buffer);
  // Check correct number of frames saved
  if (writtenFrames != length) {
    fprintf(stderr, "Did not write enough frames for source\n");
    sf_close(sndFile);
    return -1;
  }
  
  // Tidy up
  sf_write_sync(sndFile);
  sf_close(sndFile);
  printf("Sound file %s generated\n", filename);
  return 1;
}


NUMBER_TYPE *SoundFile_ReadAll(char *filename, sf_count_t *length) {  
	SF_INFO sndInfo;
  int i;
  
	SNDFILE *sndFile = sf_open(filename, SFM_READ, &sndInfo);
	if (sndFile == NULL) {
		fprintf(stderr, "Error reading source file '%s': %s\n", filename, sf_strerror(sndFile));
		return NULL;
	}
  
	// Check format - 16bit PCM
	if (sndInfo.format != (SF_FORMAT_WAV | SF_FORMAT_PCM_16)) {
		fprintf(stderr, "Input should be 16bit Wav\n");
		sf_close(sndFile);
		return NULL;
	}
  
	// Check channels - mono
	if (sndInfo.channels != 1) {
		fprintf(stderr, "Wrong number of channels\n");
		sf_close(sndFile);
		return NULL;
	}
  
	// Allocate memory
	NUMBER_TYPE *buffer = malloc(sndInfo.frames * sizeof(NUMBER_TYPE));
	if (buffer == NULL) {
		fprintf(stderr, "Could not allocate memory for file\n");
		sf_close(sndFile);
		return NULL;
	}
  
	NUMBER_TYPE *stereoBuffer = malloc(2 * sndInfo.frames * sizeof(NUMBER_TYPE));
	if (stereoBuffer == NULL) {
		fprintf(stderr, "Could not allocate memory for file\n");
		sf_close(sndFile);
		return NULL;
	}
  
	// Load data
#ifdef FIXED_POINT
  *length = SoundFile_Read_Short(sndFile, buffer, sndInfo.frames);
#else
	*length = SoundFile_Read_Float(sndFile, buffer, sndInfo.frames);
#endif
  
	// Check correct number of samples loaded
	if (*length != sndInfo.frames) {
		fprintf(stderr, "Did not read enough frames for source\n");
		sf_close(sndFile);
		free(buffer);
		return NULL;
	}
  
	// Output Info
	printf("Read %lld frames from %s, Sample rate: %d, Length: %fs\n",
         *length, filename, sndInfo.samplerate, (float)*length/sndInfo.samplerate);
	sf_close(sndFile);
  
  for (i=0; i<*length; i++) {
#ifdef FIXED_POINT
    stereoBuffer[2*i] = buffer[i];
#else
    stereoBuffer[2*i] = buffer[i] * DSP_AMP_MAX/2;
#endif
    stereoBuffer[2*i+1] = 0;
  }
  free(buffer);
  
  return stereoBuffer;
}

int SoundFile_Open(char *filename) {  
	_SoundFile_Object = sf_open(filename, SFM_READ, &_SoundFile_Info);
	if (_SoundFile_Object == NULL) {
		fprintf(stderr, "Error reading source file '%s': %s\n", filename, sf_strerror(_SoundFile_Object));
		return 1;
	}
  
	// Check format - 16bit PCM
	if (_SoundFile_Info.format != (SF_FORMAT_WAV | SF_FORMAT_PCM_16)) {
		fprintf(stderr, "Input should be 16bit Wav\n");
		sf_close(_SoundFile_Object);
		return 1;
	}
  
	// Check channels - mono
	if (_SoundFile_Info.channels != 1) {
		fprintf(stderr, "Wrong number of channels\n");
		sf_close(_SoundFile_Object);
		return 1;
	}
  
  return 0;
}

sf_count_t SoundFile_Read(NUMBER_TYPE *buffer, sf_count_t length) {  
  sf_count_t readLength, i;

#ifdef FIXED_POINT
  readLength = SoundFile_Read_Short(_SoundFile_Object, buffer, length);
#else
	readLength = SoundFile_Read_Float(_SoundFile_Object, buffer, length);
#endif
  
	// Check correct number of samples loaded
  if(readLength == 0) {
    sf_seek(_SoundFile_Object, 0, SEEK_SET);    
#ifdef FIXED_POINT
    readLength = SoundFile_Read_Short(_SoundFile_Object, buffer, length);
#else
    readLength = SoundFile_Read_Float(_SoundFile_Object, buffer, length);
#endif
  }
	if (readLength != length) {
		fprintf(stderr, "reach end of file\n");
    //sf_seek(_SoundFile_Object, 0, SEEK_SET);    
    for(i=readLength;i<length;i++){
      buffer[i] = 0;
    }
	}
  
#ifndef FIXED_POINT
  for (i=0; i<length; i++) {    
    buffer[i] = buffer[i] * DSP_AMP_MAX/2;
  }
#endif
  
  return readLength;
}

sf_count_t SoundFile_ReadOffset(NUMBER_TYPE *stereoBuffer, int offset, sf_count_t length) {  
  sf_seek(_SoundFile_Object, offset, SEEK_SET);
  return SoundFile_Read(stereoBuffer, length);
}

void SoundFile_Close(void) {  
  if(_SoundFile_Object) {
    sf_close(_SoundFile_Object);
  }
}

uint32_t SoundFile_Read_Short(SNDFILE *_SoundFile_Object, uint8_t *buffer, uint32_t length) {
	uint32_t readLength;

	FS_ReadFile(_SoundFile_Object, (uint8_t *)buffer, &readLength, length);
	return readLength;
}

uint32_t SoundFile_Read_Float(SNDFILE *_SoundFile_Object, float32_t *buffer, uint32_t length) {
	uint32_t i, readLength;
	uint8_t localBuffer[2*DSP_FFT_SAMPLE_LENGTH];

	if(length>2*DSP_FFT_SAMPLE_LENGTH)
		return 0;
	FS_ReadFile(_SoundFile_Object, (uint8_t *)localBuffer, &readLength, length);
	for(i=0; i<readLength; i++){
		buffer[i] = localBuffer[i] * NUMBER_WAV_SCALE;
	}
	return readLength;
}
