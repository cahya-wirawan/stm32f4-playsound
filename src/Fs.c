/*
 * Fs.c
 *
 *  Created on: Apr 26, 2012
 *      Author: Cahya Wirawan
 */
#include <stdio.h>
#include <stdint.h>
#include "Fs.h"

uint32_t FS_ReadFile(FILEINFO *fileInfo, NUMBER_TYPE *buffer, uint32_t *successcount, uint32_t length)
{
#ifdef FIXED_POINT
  f_read(fileInfo, (void *)buffer, (UINT) length, (UINT *) successcount);
#else
  uint32_t i, readLength;
  int16_t localBuffer[2*DSP_FFT_SAMPLE_LENGTH];

  if(length>4*DSP_FFT_SAMPLE_LENGTH)
    return 0;
  f_read(fileInfo, (void *)localBuffer, (UINT) length, (UINT *) successcount);
  readLength = (*successcount)>>1;
  for(i=0; i<readLength; i++){
    buffer[i] = localBuffer[i];
  }
#endif

  return 0;
}

void FS_Seek(FILEINFO *fileInfo,  uint32_t offset)
{
  f_lseek(fileInfo, offset);
}

uint8_t FS_ScanDir (char* path, char fileList[][FS_MAX_FILENAME])
{
  FRESULT res;
  FILINFO fno;
  DIR dir;
  uint8_t fileCounter;
  char *fn;

  fileCounter = 0;
  res = f_opendir(&dir, path);                       /* Open the directory */
  if (res == FR_OK) {
    for (;;) {
      res = f_readdir(&dir, &fno);                   /* Read a directory item */
      if (res != FR_OK || fno.fname[0] == 0) break;  /* Break on error or end of dir */
      if (fno.fname[0] == '.') continue;             /* Ignore dot entry */
      fn = fno.fname;
      if (fno.fattrib & AM_DIR) {                    /* It is a directory */
        continue;
      } else {                                       /* It is a file. */
        printf("%s/%s\n", path, fn);
        sprintf(fileList[fileCounter], "%s/%s", path, fn);
        fileCounter++;
        if(fileCounter==FS_MAX_FILELIST)
          break;
      }
    }
  }

  return fileCounter;
}
