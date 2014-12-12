/*
 * Fs.h
 *
 *  Created on: Apr 26, 2012
 *      Author: User
 */

#ifndef FS_H_
#define FS_H_

#include <stdint.h>
#include "Number.h"
#include "DSP.h"
#include "ff.h"

#define FILEINFO FIL
#define FS_MAX_FILELIST 10
#define FS_MAX_FILENAME 30

uint32_t FS_ReadFile(FILEINFO *fileInfo, NUMBER_TYPE *buffer, uint32_t *successcount, uint32_t length);
void FS_Seek(FILEINFO *fileInfo,  uint32_t offset);
uint8_t FS_ScanDir (char* path, char fileList[][FS_MAX_FILENAME]);

#endif /* FS_H_ */
