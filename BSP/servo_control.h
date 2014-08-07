#ifndef __SERVO_CONTROL_H_
#define __SERVO_CONTROL_H_
#include "includes.h"

/*µ¥Î»CM*/
#define CCD_H 26
#define Track_Width 46 
//#define Install_angle 45
#define Each_Width 0.21
#define PI 3.1415926

double Servo_Angle(double Location_corner);
float Servo_pwm(double servo_corner);

#endif

