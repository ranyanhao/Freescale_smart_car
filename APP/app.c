#include "includes.h"



extern OS_EVENT *Str_Box_1;
extern OS_EVENT *Str_Box_2;

void LED_TASK(void *p_arg)
{
 (void)p_arg;
	while(1)
	{
	  PEout(0)=!PEout(0);
		OSTimeDlyHMSM(0,0,0,500);
	}
}


void SCU_TASK(void *p_arg)
{
	uint32_t* speed;
	uint8_t err;
	int PWM_Duty = 0;
	int B;
  (void)p_arg;  
	LCD_Print(1,2,"speed = 1.0m/s");
	while(1)
	{
		PID_Init();
		speed = OSMboxPend(Str_Box_1,0,&err);   //请求消息；获得当前速度
	  // printf("process_point = %d\n",*speed);
		 
		 PWM_Duty += PID_Calc(0, *speed, 0);
     B = PWM_Duty;
		 printf("PWM = %d\n",B);
		 if(PWM_Duty >= 1000)
		 {
		   FTM_PWM_ChangeDuty(HW_FTM0,HW_FTM_CH0,1000);  /* 0-10000 对应 0-100%占空比 */
		 //	LCD_Print(1,5,"FULL_SPEED");
		 	printf("PWM_Duty = 100 \n");
		 }
		  if(PWM_Duty > 0 && PWM_Duty < 1000)
		 {
		   FTM_PWM_ChangeDuty(HW_FTM0,HW_FTM_CH0,PWM_Duty);  /* 0-10000 对应 0-100%占空比 */
		 //	LCD_Print(1,7,"Part_SPEED");
		 	printf("PWM_Duty = %d\n",PWM_Duty/100);
		 }
		 if (PWM_Duty <= 0)
		 {
		   FTM_PWM_ChangeDuty(HW_FTM0,HW_FTM_CH0,0);  /* 0-10000 对应 0-100%占空比 */
		 	 printf("PWM_Duty = 0\n");
		 }
    OSTimeDlyHMSM(0,0,0,50);	
	}
}




void DIR_TASK(void *p_arg)
{
	uint8_t err;
	double servo_corner,Location_corner,B,D;
	float Servo_pwm_duty,A;
	uint8_t* Track_Midline_value;
	(void)p_arg;
		while(1)
		{
			Track_Midline_value = OSMboxPend(Str_Box_2,0,&err);
			B = *Track_Midline_value;
			printf("Track_value = %f\n",B);
			Location_corner = PID_Calc(1, 0, *Track_Midline_value);
			D = servo_corner;
			printf("servo_corner = %f\n",D);
			Servo_pwm_duty = Servo_pwm(Location_corner);
			A = Servo_pwm_duty;
			printf("Servo_pwm_duty = %f\n",A);
			FTM_PWM_ChangeDuty(HW_FTM1,HW_FTM_CH0,Servo_pwm_duty);	
			OSTimeDlyHMSM(0,0,0,50);
		}
}



void CCD_TASK(void *p_arg)
{
  (void)p_arg;
	uint8_t Track_Midline_value,A;
		CCD_Restet();
	while(1)
	{                    
		CCD_gather(); 
		CCD_Filtering();
		Data_binarization(averaging());
	  Track_Midline_value = Track_Midline();
		A = Track_Midline_value;
		printf("Track_Midline_value = %d\n",A);
		OSMboxPost(Str_Box_2,&Track_Midline_value);
   OSTimeDlyHMSM(0,0,0,30);		
	}
}

