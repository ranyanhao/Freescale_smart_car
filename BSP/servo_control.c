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
	printf("servo_corneraa = %f\n",servo_corner);
 if(servo_corner > 0.000)
 {
   Servo_pwm_duty = (3.000 / 40.000) - (servo_corner*(1.000 / 90.000)) / 20.000;  // 3/40表示0度位置时的占空比
 }
 else if (servo_corner < 0.000)
 {
   Servo_pwm_duty = (3.000 / 40.000) - (servo_corner*(1.000 / 90.000)) / 20.000;
 }
 else
 {
  Servo_pwm_duty = (3.000/ 40.000);
 }
 Servo_pwm_duty = Servo_pwm_duty * 10000;
 printf("Servo_pwm_duty = %f\n",Servo_pwm_duty);
 return (Servo_pwm_duty);
}	
