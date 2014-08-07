#include "dip_switch.h"

uint32_t dip_state(void)
{
  return (PTC->PDIR);
}	


uint32_t Speed_Set(void)
{
	uint32_t state;
	uint32_t speed;
	uint32_t switch_state;
	switch_state = dip_state();
	state = (switch_state & 0x0000f000)>>12;
  speed = 31*state + 50;    //最小速度1m/s,拨码每刻度增加0.1m/s  310
	return (speed);
}

uint32_t Install_angle_change(void)
{
  uint32_t state;
	uint32_t Angle;
	uint32_t switch_state;
	switch_state = dip_state();
	state = (switch_state & 0x000f0000)>>16;
	if(state > 7)
	{
	  Angle = 45 + (state - 7);  //1000--1111 每增加1角度加1度
	}
	else
	{
	 Angle = 45 - state;        //0000--0111 每增加1角度减1度
	}
   return(Angle);
}

