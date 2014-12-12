/**
 ******************************************************************************
 * @file    Audio_playback_and_record/src/waveplayer.c
 * @author  MCD Application Team
 * @version V1.0.0
 * @date    28-October-2011
 * @brief   I2S audio program
 ******************************************************************************
 * @attention
 *
 * THE PRESENT FIRMWARE WHICH IS FOR GUIDANCE ONLY AIMS AT PROVIDING CUSTOMERS
 * WITH CODING INFORMATION REGARDING THEIR PRODUCTS IN ORDER FOR THEM TO SAVE
 * TIME. AS A RESULT, STMICROELECTRONICS SHALL NOT BE HELD LIABLE FOR ANY
 * DIRECT, INDIRECT OR CONSEQUENTIAL DAMAGES WITH RESPECT TO ANY CLAIMS ARISING
 * FROM THE CONTENT OF SUCH FIRMWARE AND/OR THE USE MADE BY CUSTOMERS OF THE
 * CODING INFORMATION CONTAINED HEREIN IN CONNECTION WITH THEIR PRODUCTS.
 *
 * <h2><center>&copy; COPYRIGHT 2011 STMicroelectronics</center></h2>
 ******************************************************************************
 */

/* Includes ------------------------------------------------------------------*/
#include "main.h"
#include "Audio.h"
#include "Sound.h"
#include "ClockCounter.h"

/** @addtogroup STM32F4-Discovery_Audio_Player_Recorder
 * @{
 */

/* Private typedef -----------------------------------------------------------*/
/* Private define ------------------------------------------------------------*/
/* Private macro -------------------------------------------------------------*/
/* Private variables ---------------------------------------------------------*/

//#define WAV_OLDCODE 1

#if defined MEDIA_USB_KEY
extern __IO uint8_t Command_index;
static char* WaveFileName ;
static __IO uint32_t SpeechDataOffset = 0x00;
__IO ErrorCode WaveFileStatus = Unvalid_RIFF_ID;
UINT BytesRead;
WAVE_FormatTypeDef WAVE_Format;
int16_t *audioBuffer;
//int16_t audioBuffer[4096];
uint8_t buffer_switch = 1;
extern FATFS fatfs;
extern FIL file;
extern FIL fileR;
extern DIR dir;
extern FILINFO fno;
extern uint16_t *CurrentPos;
extern USB_OTG_CORE_HANDLE USB_OTG_Core;
#endif

__IO uint32_t XferCplt = 0;
__IO uint8_t volume = 70, AudioPlayStart = 0;
__IO uint32_t WaveCounter;
uint8_t Buffer[6];
__IO uint32_t WaveDataLength = 0;
extern __IO uint8_t Count;
extern __IO uint8_t RepeatState ;
extern __IO uint8_t LED_Toggle;
extern __IO uint8_t PauseResumeStatus ;
extern uint32_t AudioRemSize; 
static __IO uint32_t TimingDelay;

char _WAV_SoundDir[] = "0:/sound";
char _WAV_FileList[FS_MAX_FILELIST][FS_MAX_FILENAME];
uint8_t _WAV_FileCounter;
uint8_t _WAV_CurrentFileNumber;
/* Private function prototypes -----------------------------------------------*/
static void Mems_Config(void);
static void EXTILine_Config(void);
#if defined MEDIA_USB_KEY
static ErrorCode WavePlayer_WaveParsing(uint8_t *header);
#endif

/* Private functions ---------------------------------------------------------*/

/**
 * @brief  Play wave from a mass storge
 * @param  AudioFreq: Audio Sampling Frequency
 * @retval None
 */

void WavePlayBack(uint32_t AudioFreq)
{ 
  /*
  Normal mode description:
  Start playing the audio file (using DMA stream) .
  Using this mode, the application can run other tasks in parallel since 
  the DMA is handling the Audio Transfer instead of the CPU.
  The only task remaining for the CPU will be the management of the DMA 
  Transfer Complete interrupt or the Half Transfer Complete interrupt in 
  order to load again the buffer and to calculate the remaining data.  
  Circular mode description:
  Start playing the file from a circular buffer, once the DMA is enabled it 
  always run. User has to fill periodically the buffer with the audio data 
  using Transfer complete and/or half transfer complete interrupts callbacks 
  (EVAL_AUDIO_TransferComplete_CallBack() or EVAL_AUDIO_HalfTransfer_CallBack()...
  In this case the audio data file is smaller than the DMA max buffer 
  size 65535 so there is no need to load buffer continuously or manage the 
  transfer complete or Half transfer interrupts callbacks. */  

  AUDIO_PlaybackBuffer_Status Sound_BufferStatus;

  printf("WavePlayBack start\n");
  /* Start playing */

  audioBuffer = Sound_Get_AudioBuffer();
  Sound_Init();
  AudioPlayStart = 1;
  RepeatState =0;

  /* Initialize wave player (Codec, DMA, I2C) */
  WavePlayerInit(AudioFreq);
  AudioRemSize   = 0;

  STOPWATCH_START
  /* Get Data from USB Key */
  f_lseek(&fileR, WaveCounter);
  //f_read (&fileR, audioBuffer, _MAX_SS, &BytesRead);
  STOPWATCH_STOP
  CycleCounter_Print(0,0,0);

  /* Start playing wave */
  Audio_MAL_Play((uint32_t)audioBuffer, 2*_MAX_SS);
  LED_Toggle = 6;
  PauseResumeStatus = 2;
  Count = 0;

  while(HCD_IsDeviceConnected(&USB_OTG_Core))
  {
    /* Test on the command: Playing */
    if (Command_index == 0)
    {

      /* wait for DMA transfer complete */
      while((AUDIO_PlaybackBuffer_GetStatus(0) == FULL) &&  HCD_IsDeviceConnected(&USB_OTG_Core))
      {
#ifdef WAV_OLDCODE
        if (PauseResumeStatus == 0)
        {
          /* Pause Playing wave */
          LED_Toggle = 0;
          WavePlayerPauseResume(PauseResumeStatus);
          PauseResumeStatus = 2;
        }
        else if (PauseResumeStatus == 1)
        {
          LED_Toggle = 6;
          /* Resume Playing wave */
          WavePlayerPauseResume(PauseResumeStatus);
          PauseResumeStatus = 2;
        }
#else
        if(PauseResumeStatus != 2)
        {
          WAV_OpenNextFile();
          f_lseek(&fileR, WaveCounter);
          Sound_Init();
          PauseResumeStatus = 2;
        }
#endif
      }

      //#define SOUND_DIRECT_TEST
#ifndef	SOUND_DIRECT_TEST
      Sound_FillBuffer(&fileR);
#else
      Sound_BufferStatus = AUDIO_PlaybackBuffer_GetStatus(0);
      if(Sound_BufferStatus & LOW_EMPTY)
      {
        f_read (&fileR, audioBuffer, _MAX_SS, &BytesRead);
        Sound_BufferStatus = AUDIO_PlaybackBuffer_GetStatus(LOW_EMPTY);
      }
      if(Sound_BufferStatus & HIGH_EMPTY)
      {
        f_read (&fileR, audioBuffer+_MAX_SS/2, _MAX_SS, &BytesRead);
        Sound_BufferStatus = AUDIO_PlaybackBuffer_GetStatus(HIGH_EMPTY);
      }
#endif
    }
    else
    {
      WavePlayerStop();
      WaveDataLength = 0;
      RepeatState =0;
      break;
    }
    if(WaveDataLength == 0) {
      WaveDataLength = WAVE_Format.DataSize;
      f_lseek(&fileR, 44);
    }
  }
#if defined PLAY_REPEAT_OFF 
  RepeatState = 1;
  WavePlayerStop();
  if (Command_index == 0)
    LED_Toggle = 4;
#else 
  LED_Toggle = 7;
  RepeatState = 0;
  AudioPlayStart = 0;
  WavePlayerStop();
#endif

}

/**
 * @brief  Pause or Resume a played wave
 * @param  state: if it is equal to 0 pause Playing else resume playing
 * @retval None
 */
void WavePlayerPauseResume(uint8_t state)
{ 
  EVAL_AUDIO_PauseResume(state);
}

/**
 * @brief  Configure the volune
 * @param  vol: volume value
 * @retval None
 */
uint8_t WaveplayerCtrlVolume(uint8_t vol)
{ 
  EVAL_AUDIO_VolumeCtl(vol);
  return 0;
}


/**
 * @brief  Stop playing wave
 * @param  None
 * @retval None
 */
void WavePlayerStop(void)
{ 
  EVAL_AUDIO_Stop(CODEC_PDWN_SW);
}

/**
 * @brief  Initializes the wave player
 * @param  AudioFreq: Audio sampling frequency
 * @retval None
 */
int WavePlayerInit(uint32_t AudioFreq)
{ 

  /* MEMS Accelerometre configure to manage PAUSE, RESUME and Controle Volume operation */
  Mems_Config();

  /* EXTI configue to detect interrupts on Z axis click and on Y axis high event */
  EXTILine_Config();

  /* Initialize I2S interface */
  EVAL_AUDIO_SetAudioInterface(AUDIO_INTERFACE_I2S);

  /* Initialize the Audio codec and all related peripherals (I2S, I2C, IOExpander, IOs...) */
  EVAL_AUDIO_Init(OUTPUT_DEVICE_AUTO, volume, AudioFreq );

  return 0;
}

/**
 * @brief  MEMS accelerometre management of the timeout situation.
 * @param  None.
 * @retval None.
 */
uint32_t LIS302DL_TIMEOUT_UserCallback(void)
{
  /* MEMS Accelerometer Timeout error occured */
  while (1)
  {
  }
}

/**
 * @brief  Play wave file from internal Flash
 * @param  None
 * @retval None
 */
uint32_t AudioFlashPlay(uint16_t* pBuffer, uint32_t FullSize, uint32_t StartAdd)
{ 
  EVAL_AUDIO_Play((uint16_t*)pBuffer, (FullSize - StartAdd));
  return 0;
}

/*--------------------------------
Callbacks implementation:
the callbacks prototypes are defined in the stm324xg_eval_audio_codec.h file
and their implementation should be done in the user code if they are needed.
Below some examples of callback implementations.
--------------------------------------------------------*/

/**
 * @brief  Manages the DMA Half Transfer complete interrupt.
 * @param  None
 * @retval None
 */
void EVAL_AUDIO_HalfTransfer_CallBack(uint32_t pBuffer, uint32_t Size)
{
  //printf("0,");
#ifdef AUDIO_MAL_MODE_CIRCULAR
  if (WaveDataLength) WaveDataLength -= _MAX_SS;
  if (WaveDataLength < _MAX_SS) WaveDataLength = 0;
  AUDIO_PlaybackBuffer_SetStatus(LOW_EMPTY);
#endif /* AUDIO_MAL_MODE_CIRCULAR */

  /* Generally this interrupt routine is used to load the buffer when
  a streaming scheme is used: When first Half buffer is already transferred load 
  the new data to the first half of buffer while DMA is transferring data from 
  the second half. And when Transfer complete occurs, load the second half of 
  the buffer while the DMA is transferring from the first half ... */
  /*
  ...........
   */
}

/**
 * @brief  Calculates the remaining file size and new position of the pointer.
 * @param  None
 * @retval None
 */
void EVAL_AUDIO_TransferComplete_CallBack(uint32_t pBuffer, uint32_t Size)
{
  /* Calculate the remaining audio data in the file and the new size
  for the DMA transfer. If the Audio files size is less than the DMA max
  data transfer size, so there is no calculation to be done, just restart
  from the beginning of the file ... */
  /* Check if the end of file has been reached */
  static uint16_t counter=0;
  /*
  if(counter==10)
    printf("1\n");
  else
    printf("1,");
    */
  counter++;
  counter%=10000;
  if (WaveDataLength) WaveDataLength -= _MAX_SS;
  if (WaveDataLength < _MAX_SS) WaveDataLength = 0;
  AUDIO_PlaybackBuffer_SetStatus(HIGH_EMPTY);
}


/**
 * @brief  Manages the DMA FIFO error interrupt.
 * @param  None
 * @retval None
 */
void EVAL_AUDIO_Error_CallBack(void* pData)
{
  /* Stop the program with an infinite loop */
  while (1)
  {}

  /* could also generate a system reset to recover from the error */
  /* .... */
}

/**
 * @brief  Get next data sample callback
 * @param  None
 * @retval Next data sample to be sent
 */
uint16_t EVAL_AUDIO_GetSampleCallBack(void)
{
  return 0;
}


#ifndef USE_DEFAULT_TIMEOUT_CALLBACK
/**
 * @brief  Basic management of the timeout situation.
 * @param  None.
 * @retval None.
 */
uint32_t Codec_TIMEOUT_UserCallback(void)
{   
  return (0);
}
#endif /* USE_DEFAULT_TIMEOUT_CALLBACK */
/*----------------------------------------------------------------------------*/

/**
 * @brief  Inserts a delay time.
 * @param  nTime: specifies the delay time length, in 10 ms.
 * @retval None
 */
void Delay(__IO uint32_t nTime)
{
  TimingDelay = nTime;

  while(TimingDelay != 0);
}

/**
 * @brief  Decrements the TimingDelay variable.
 * @param  None
 * @retval None
 */
void TimingDelay_Decrement(void)
{
  if (TimingDelay != 0x00)
  {
    TimingDelay--;
  }
}

#if defined MEDIA_USB_KEY

/**
 * @brief  Start wave player
 * @param  None
 * @retval None
 */
void WavePlayerStart(void)
{
  char path[] = "0:/";

  _WAV_FileCounter = FS_ScanDir(_WAV_SoundDir, _WAV_FileList);

  buffer_switch = 1;

  /* Get the read out protection status */
  if (f_opendir(&dir, _WAV_SoundDir)!= FR_OK)
  {
    while(1)
    {
      STM_EVAL_LEDToggle(LED5);
      Delay(10);
    }
  }
  else
  {
#ifdef WAV_OLDCODE
    uint8_t wavHeader[_MAX_SS];

    WaveFileName = WAVE_NAME;
    /* Open the wave file to be played */
    if (f_open(&fileR, WaveFileName , FA_READ) != FR_OK)
    {
      STM_EVAL_LEDOn(LED5);
      Command_index = 1;
    }
    else
    {
      /* Read data(_MAX_SS byte) from the selected file */
      f_read (&fileR, (void *) wavHeader, _MAX_SS, &BytesRead);

      WaveFileStatus = WavePlayer_WaveParsing((uint8_t *) wavHeader);

      if (WaveFileStatus == Valid_WAVE_File)  /* the .WAV file is valid */
      {
        /* Set WaveDataLenght to the Speech wave length */
        WaveDataLength = WAVE_Format.DataSize;
      }
      else /* Unvalid wave file */
      {
        /* Led Red Toggles in infinite loop */
        while(1)
        {
          STM_EVAL_LEDToggle(LED5);
          Delay(10);
        }
      }
      /* Play the wave */
      WavePlayBack(I2S_AudioFreq_44k);
    }
#else
    WAV_OpenNextFile();
    WavePlayBack(I2S_AudioFreq_44k);
#endif
  }
}

/**
 * @brief  Reset the wave player
 * @param  None
 * @retval None
 */
void WavePlayer_CallBack(void)
{
  /* Reset the wave player variables */
  RepeatState = 0;
  AudioPlayStart =0;
  LED_Toggle = 7;
  PauseResumeStatus = 1;
  WaveDataLength =0;
  Count = 0;

  /* Stops the codec */
  EVAL_AUDIO_Stop(CODEC_PDWN_HW);
  /* LED off */
  STM_EVAL_LEDOff(LED3);
  STM_EVAL_LEDOff(LED4);
  STM_EVAL_LEDOff(LED6);

  /* TIM Interrupts disable */
  TIM_ITConfig(TIM4, TIM_IT_CC1, DISABLE);
  f_mount(0, NULL);
} 

/**
 * @brief  Checks the format of the .WAV file and gets information about
 *   the audio format. This is done by reading the value of a
 *   number of parameters stored in the file header and comparing
 *   these to the values expected authenticates the format of a
 *   standard .WAV  file (44 bytes will be read). If  it is a valid
 *   .WAV file format, it continues reading the header to determine
 *   the  audio format such as the sample rate and the sampled data
 *   size. If the audio format is supported by this application,
 *   it retrieves the audio format in WAVE_Format structure and
 *   returns a zero value. Otherwise the function fails and the
 *   return value is nonzero.In this case, the return value specifies
 *   the cause of  the function fails. The error codes that can be
 *   returned by this function are declared in the header file.
 * @param  None
 * @retval Zero value if the function succeed, otherwise it return
 *         a nonzero value which specifies the error code.
 */
static ErrorCode WavePlayer_WaveParsing(uint8_t *header)
{
  uint32_t temp = 0x00;
  uint32_t extraformatbytes = 0;

  /* Read chunkID, must be 'RIFF' */
  temp = ReadUnit((uint8_t*)header, 0, 4, BigEndian);
  if (temp != CHUNK_ID)
  {
    return(Unvalid_RIFF_ID);
  }

  /* Read the file length */
  WAVE_Format.RIFFchunksize = ReadUnit((uint8_t*)header, 4, 4, LittleEndian);

  /* Read the file format, must be 'WAVE' */
  temp = ReadUnit((uint8_t*)header, 8, 4, BigEndian);
  if (temp != FILE_FORMAT)
  {
    return(Unvalid_WAVE_Format);
  }

  /* Read the format chunk, must be'fmt ' */
  temp = ReadUnit((uint8_t*)header, 12, 4, BigEndian);
  if (temp != FORMAT_ID)
  {
    return(Unvalid_FormatChunk_ID);
  }
  /* Read the length of the 'fmt' data, must be 0x10 -------------------------*/
  temp = ReadUnit((uint8_t*)header, 16, 4, LittleEndian);
  if (temp != 0x10)
  {
    extraformatbytes = 1;
  }
  /* Read the audio format, must be 0x01 (PCM) */
  WAVE_Format.FormatTag = ReadUnit((uint8_t*)header, 20, 2, LittleEndian);
  if (WAVE_Format.FormatTag != WAVE_FORMAT_PCM)
  {
    return(Unsupporetd_FormatTag);
  }

  /* Read the number of channels, must be 0x01 (Mono) or 0x02 (Stereo) */
  WAVE_Format.NumChannels = ReadUnit((uint8_t*)header, 22, 2, LittleEndian);

  /* Read the Sample Rate */
  WAVE_Format.SampleRate = ReadUnit((uint8_t*)header, 24, 4, LittleEndian);

  /* Read the Byte Rate */
  WAVE_Format.ByteRate = ReadUnit((uint8_t*)header, 28, 4, LittleEndian);

  /* Read the block alignment */
  WAVE_Format.BlockAlign = ReadUnit((uint8_t*)header, 32, 2, LittleEndian);

  /* Read the number of bits per sample */
  WAVE_Format.BitsPerSample = ReadUnit((uint8_t*)header, 34, 2, LittleEndian);
  if (WAVE_Format.BitsPerSample != BITS_PER_SAMPLE_16)
  {
    return(Unsupporetd_Bits_Per_Sample);
  }
  SpeechDataOffset = 36;
  /* If there is Extra format bytes, these bytes will be defined in "Fact Chunk" */
  if (extraformatbytes == 1)
  {
    /* Read th Extra format bytes, must be 0x00 */
    temp = ReadUnit((uint8_t*)header, 36, 2, LittleEndian);
    if (temp != 0x00)
    {
      return(Unsupporetd_ExtraFormatBytes);
    }
    /* Read the Fact chunk, must be 'fact' */
    temp = ReadUnit((uint8_t*)header, 38, 4, BigEndian);
    if (temp != FACT_ID)
    {
      return(Unvalid_FactChunk_ID);
    }
    /* Read Fact chunk data Size */
    temp = ReadUnit((uint8_t*)header, 42, 4, LittleEndian);

    SpeechDataOffset += 10 + temp;
  }
  /* Read the Data chunk, must be 'data' */
  temp = ReadUnit((uint8_t*)header, SpeechDataOffset, 4, BigEndian);
  SpeechDataOffset += 4;
  if (temp != DATA_ID)
  {
    return(Unvalid_DataChunk_ID);
  }

  /* Read the number of sample data */
  WAVE_Format.DataSize = ReadUnit((uint8_t*)header, SpeechDataOffset, 4, LittleEndian);
  SpeechDataOffset += 4;
  WaveCounter =  SpeechDataOffset;
  return(Valid_WAVE_File);
}

/**
 * @brief  Reads a number of bytes from the SPI Flash and reorder them in Big
 *         or little endian.
 * @param  NbrOfBytes: number of bytes to read.
 *         This parameter must be a number between 1 and 4.
 * @param  ReadAddr: external memory address to read from.
 * @param  Endians: specifies the bytes endianness.
 *         This parameter can be one of the following values:
 *             - LittleEndian
 *             - BigEndian
 * @retval Bytes read from the SPI Flash.
 */
uint32_t ReadUnit(uint8_t *buffer, uint8_t idx, uint8_t NbrOfBytes, Endianness BytesFormat)
{
  uint32_t index = 0;
  uint32_t temp = 0;

  for (index = 0; index < NbrOfBytes; index++)
  {
    temp |= buffer[idx + index] << (index * 8);
  }

  if (BytesFormat == BigEndian)
  {
    temp = __REV(temp);
  }
  return temp;
}
#endif

/**
 * @brief  configure the mems accelometer
 * @param  None
 * @retval None
 */
static void Mems_Config(void)
{ 
  uint8_t ctrl = 0;

  LIS302DL_InitTypeDef  LIS302DL_InitStruct;
  LIS302DL_InterruptConfigTypeDef LIS302DL_InterruptStruct;

  /* Set configuration of LIS302DL*/
  LIS302DL_InitStruct.Power_Mode = LIS302DL_LOWPOWERMODE_ACTIVE;
  LIS302DL_InitStruct.Output_DataRate = LIS302DL_DATARATE_100;
  LIS302DL_InitStruct.Axes_Enable = LIS302DL_X_ENABLE | LIS302DL_Y_ENABLE | LIS302DL_Z_ENABLE;
  LIS302DL_InitStruct.Full_Scale = LIS302DL_FULLSCALE_2_3;
  LIS302DL_InitStruct.Self_Test = LIS302DL_SELFTEST_NORMAL;
  LIS302DL_Init(&LIS302DL_InitStruct);

  /* Set configuration of Internal High Pass Filter of LIS302DL*/
  LIS302DL_InterruptStruct.Latch_Request = LIS302DL_INTERRUPTREQUEST_LATCHED;
  LIS302DL_InterruptStruct.SingleClick_Axes = LIS302DL_CLICKINTERRUPT_Z_ENABLE;
  LIS302DL_InterruptStruct.DoubleClick_Axes = LIS302DL_DOUBLECLICKINTERRUPT_Z_ENABLE;
  LIS302DL_InterruptConfig(&LIS302DL_InterruptStruct);

  /* Configure Interrupt control register: enable Click interrupt on INT1 and
     INT2 on Z axis high event */
  ctrl = 0x3F;
  LIS302DL_Write(&ctrl, LIS302DL_CTRL_REG3_ADDR, 1);

  /* Enable Interrupt generation on click on Z axis */
  ctrl = 0x50;
  LIS302DL_Write(&ctrl, LIS302DL_CLICK_CFG_REG_ADDR, 1);

  /* Configure Click Threshold on X/Y axis (10 x 0.5g) */
  ctrl = 0xAA;
  LIS302DL_Write(&ctrl, LIS302DL_CLICK_THSY_X_REG_ADDR, 1);

  /* Configure Click Threshold on Z axis (10 x 0.5g) */
  ctrl = 0x0A;
  LIS302DL_Write(&ctrl, LIS302DL_CLICK_THSZ_REG_ADDR, 1);

  /* Enable interrupt on Y axis high event */
  ctrl = 0x4C;
  LIS302DL_Write(&ctrl, LIS302DL_FF_WU_CFG1_REG_ADDR, 1);

  /* Configure Time Limit */
  ctrl = 0x03;
  LIS302DL_Write(&ctrl, LIS302DL_CLICK_TIMELIMIT_REG_ADDR, 1);

  /* Configure Latency */
  ctrl = 0x7F;
  LIS302DL_Write(&ctrl, LIS302DL_CLICK_LATENCY_REG_ADDR, 1);

  /* Configure Click Window */
  ctrl = 0x7F;
  LIS302DL_Write(&ctrl, LIS302DL_CLICK_WINDOW_REG_ADDR, 1);

}

/**
 * @brief  Configures EXTI Line0 (connected to PA0 pin) in interrupt mode
 * @param  None
 * @retval None
 */
static void EXTILine_Config(void)
{
  GPIO_InitTypeDef   GPIO_InitStructure;
  NVIC_InitTypeDef   NVIC_InitStructure;
  EXTI_InitTypeDef   EXTI_InitStructure;
  /* Enable GPIOA clock */
  RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOE, ENABLE);
  /* Enable SYSCFG clock */
  RCC_APB2PeriphClockCmd(RCC_APB2Periph_SYSCFG, ENABLE);
  /* Configure PE0 and PE1 pins as input floating */
  GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IN;
  GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_NOPULL;
  GPIO_InitStructure.GPIO_Pin = GPIO_Pin_0|GPIO_Pin_1;
  GPIO_Init(GPIOE, &GPIO_InitStructure);

  /* Connect EXTI Line to PE1 pins */
  SYSCFG_EXTILineConfig(EXTI_PortSourceGPIOE, EXTI_PinSource1);

  /* Configure EXTI Line1 */
  EXTI_InitStructure.EXTI_Line = EXTI_Line1;
  EXTI_InitStructure.EXTI_Mode = EXTI_Mode_Interrupt;
  EXTI_InitStructure.EXTI_Trigger = EXTI_Trigger_Rising;
  EXTI_InitStructure.EXTI_LineCmd = ENABLE;
  EXTI_Init(&EXTI_InitStructure);

  NVIC_PriorityGroupConfig(NVIC_PriorityGroup_3);

  /* Enable and set EXTI Line0 Interrupt to the lowest priority */
  NVIC_InitStructure.NVIC_IRQChannel = EXTI1_IRQn;
  NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 0x00;
  NVIC_InitStructure.NVIC_IRQChannelSubPriority = 0x00;
  NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
  NVIC_Init(&NVIC_InitStructure);
}

#ifdef  USE_FULL_ASSERT

/**
 * @brief  Reports the name of the source file and the source line number
 *   where the assert_param error has occurred.
 * @param  file: pointer to the source file name
 * @param  line: assert_param error line source number
 * @retval None
 */
void assert_failed(uint8_t* file, uint32_t line)
{ 
  /* User can add his own implementation to report the file name and line number,
  ex: printf("Wrong parameters value: file %s on line %d\r\n", file, line) */

  /* Infinite loop */
  while (1)
  {
  }
}
#endif

/**
 * @}
 */


void WAV_OpenNextFile(void)
{
  uint8_t i;
  FRESULT fresult;
  uint8_t wavHeader[_MAX_SS];

  f_close(&fileR);
  for(i = 0; i < _WAV_FileCounter; i++){
    fresult = f_open(&fileR, _WAV_FileList[_WAV_CurrentFileNumber], FA_READ);
    if (fresult != FR_OK) {
      _WAV_CurrentFileNumber = (_WAV_CurrentFileNumber+1)%_WAV_FileCounter;
      continue;
    }
    else {
      f_read (&fileR, (void *) wavHeader, _MAX_SS, &BytesRead);
      WaveFileStatus = WavePlayer_WaveParsing((uint8_t *) wavHeader);
      if (WaveFileStatus == Valid_WAVE_File) {
        WaveDataLength = WAVE_Format.DataSize;
        printf("Reading sound file %s\n", _WAV_FileList[_WAV_CurrentFileNumber]);
        _WAV_CurrentFileNumber = (_WAV_CurrentFileNumber+1)%_WAV_FileCounter;
        break;
      }
      else {
        f_close(&fileR);
        _WAV_CurrentFileNumber = (_WAV_CurrentFileNumber+1)%_WAV_FileCounter;
        continue;
      }
    }
  }
  if(i == _WAV_FileCounter){
    /* Led Red Toggles in infinite loop */
    while(1)
    {
      STM_EVAL_LEDToggle(LED5);
      Delay(10);
    }
  }
}

/******************* (C) COPYRIGHT 2011 STMicroelectronics *****END OF FILE****/