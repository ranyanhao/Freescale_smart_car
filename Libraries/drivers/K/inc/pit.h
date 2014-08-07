/**
  ******************************************************************************
  * @file    pit.h
  * @author  YANDLD
  * @version V2.5
  * @date    2014.3.24
  * @brief   www.beyondcore.net   http://upcmcu.taobao.com 
  * @note    ���ļ�ΪоƬPITģ��ĵײ㹦�ܺ���
  ******************************************************************************
  */
#ifndef __CH_LIB_PIT_H__
#define __CH_LIB_PIT_H__

#include "common.h"

#ifdef __cplusplus
 extern "C" {
#endif


//!< hardware instances
#define HW_PIT_CH0   (0x00U) //����PITģ���0ͨ��
#define HW_PIT_CH1   (0x01U) //����PITģ���1ͨ��
#define HW_PIT_CH2   (0x02U) //����PITģ���2ͨ��
#define HW_PIT_CH3   (0x03U) //����PITģ���3ͨ��

//!< PIT CallBack Type
typedef void (*PIT_CallBackType)(void);

//PIT ��ʼ���ṹ
typedef struct
{
    uint8_t   chl;            //PITģ��ͨ��ѡ��
    uint32_t  timeInUs;       //��ʱʱ�䵥λΪus
}PIT_InitTypeDef;

//!< PIT �ж�DMA����
typedef enum
{
    kPIT_IT_Disable,        //�ر��ж�
    kPIT_IT_TOF,            //�����ж�
}PIT_ITDMAConfig_Type;

//!< API functions
void PIT_Init(PIT_InitTypeDef* PIT_InitStruct);
void PIT_QuickInit(uint8_t chl, uint32_t timeInUs);
void PIT_ResetCounter(uint8_t chl);
uint32_t PIT_GetCounterValue(uint8_t chl);
void PIT_CallbackInstall(uint8_t chl, PIT_CallBackType AppCBFun);
void PIT_ITDMAConfig(uint8_t chl, PIT_ITDMAConfig_Type config);

#ifdef __cplusplus
}
#endif

#endif

