/*
 * main.h
 *
 *  Created on: Apr 23, 2012
 *      Author: User
 */

#ifndef MAIN_H_
#define MAIN_H_

#include <stdio.h>
#include "stm32f4xx.h"
#include "stm32f4_discovery.h"
#include "stm32f4_discovery_audio_codec.h"
#include "stm32f4_discovery_lis302dl.h"
#include "stm32f4xx_it.h"
#include "waveplayer.h"
#include "usb_hcd_int.h"
#include "usbh_usr.h"
#include "usbh_core.h"
#include "usbh_msc_core.h"
#include "pdm_filter.h"

#if !defined (MEDIA_IntFLASH) && !defined (MEDIA_USB_KEY)
 //#define MEDIA_IntFLASH /* Wave file stored in internal flash */
 #define MEDIA_USB_KEY  /* Wave file stored in USB flash */
#endif

#define WAVE_NAME "0:audio.wav"

void TimingDelay_Decrement(void);
void Delay(__IO uint32_t nTime);

#endif /* MAIN_H_ */
