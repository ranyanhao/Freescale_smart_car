

#ifndef __OV7620_H__
#define __OV7620_H__

#include <stdint.h>


#define OV7620_W    (320) // ÿ���ж�������
#define OV7620_H    (240) //�߶� �ж�����
extern uint8_t * CCDBuffer[OV7620_H];
//!< Callback Type
typedef void (*OV7620_CallBackType)(void);

//!< API funtctions
void OV7620_Init(void);
void OV7620_CallbackInstall(OV7620_CallBackType AppCBFun);

#endif


