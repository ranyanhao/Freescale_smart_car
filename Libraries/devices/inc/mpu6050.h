/**
  ******************************************************************************
  * @file    mpu6050.h
  * @author  YANDLD
  * @version V2.5
  * @date    2013.12.25
  * @brief   www.beyondcore.net   http://upcmcu.taobao.com 
  ******************************************************************************
  */
#ifndef __MPU6050_H__
#define __MPU6050_H__

#include "i2c_abstraction.h"

#ifndef ARRAY_SIZE
#define ARRAY_SIZE(x)	(sizeof(x) / sizeof((x)[0]))
#endif

//!< API function
int mpu6050_init(struct i2c_bus* bus);
int mpu6050_probe(void);
int mpu6050_read_accel(int16_t* x, int16_t* y, int16_t* z);
int mpu6050_read_gyro(int16_t* x, int16_t* y, int16_t* z);



#endif
