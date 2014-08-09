#include "servo_control.h"

double Servo_Angle(double Location_corner)
{
  double error,x,y,servo_corner;
	uint8_t Install_angle;
	Install_angle = Install_angle_change();
	error = Location_corner;
	y = error * Each_Width; 
	x = CCD_H / tan(90.000 - Install_angle);
  servo_corner = (atan(y / x)) * (180.000 / PI);    //角度与弧度的关系
	return (servo_corner);
}



float Servo_pwm(double servo_corner)
{
	double Servo_pwm_duty;
	if(servo_corner != 0)
	{
	  Servo_pwm_duty = 750 - servo_corner;
	}
	else
	{
	  Servo_pwm_duty = 750;
	}

 return (Servo_pwm_duty);
}	
