#include "ccd.h"

uint8_t CCD_original_data[132]={0};
uint8_t CCD_filtering_data[128]={0};



void CCD_gather(void)
{
	  OS_CPU_SR  cpu_sr;
    uint8_t i=0;
		CCD_SI(0);
 	  CCD_CLK(0);
	  CCD_SI(1);
	  CCD_CLK(1);
	  Delay_stick(40);
 	  CCD_SI(0);
		
	  OS_ENTER_CRITICAL();  //  关中断
	  for(i=0;i<128;i++)
	  {
      CCD_CLK(1);
		  Delay_stick(10);
		  CCD_original_data[i+2] = ADC_QuickReadValue(ADC0_SE8_PB0);
		  CCD_CLK(0);
		  Delay_stick(10);
    }
		OS_EXIT_CRITICAL();   //开中断
		
//		for(i=0;i<128;i++)
//		{
//		  printf("CCD_data = %d\n",CCD_data[i]);
//		}
}




void CCD_Filtering(void)      //数据滤波
{
	uint8_t i,j,k,MAX,MIN;
	uint8_t temp[5];
	for(i=0;i<128;i++)
	{
	  for(j=0,k=i;j<5;j++)
		{
		  temp[j] = CCD_original_data[k];
			k = i+j++;
		}
		MAX = MAX_5(temp[0],temp[1],temp[2],temp[3],temp[4]);
		MIN = MIN_5(temp[0],temp[1],temp[2],temp[3],temp[4]);
		if(MAX == CCD_original_data[i+2] || MIN == CCD_original_data[i+2])
		{
		  CCD_filtering_data[i] =  Data_sort(temp);
		}
		else
		{
		  CCD_filtering_data[i] = CCD_original_data[i+2];
		}
	}
}


void Data_binarization(uint8_t average)
{
  uint8_t i;
	for(i=0;i<128;i++)
	{
	  if(CCD_filtering_data[i] >= average)
		{
		 CCD_filtering_data[i] = 0XFF;
		}
		else
		{
		  CCD_filtering_data[i] = 0X00;
		}
	}
}	


uint8_t Data_sort(uint8_t data[5])
{
  uint8_t i,j,temp;
	for(j=0;j<5;j++)
	{
	  for(i=0;i<5-j;i++)
		{
		  if(data[i] > data[i+1])
			{
			  temp = data[i];
				data[i] = data[i+1];
				data[i+1] = temp;
			}
		}
	}
	return (data[3]);
}


uint8_t averaging(void)
{
  uint8_t i,MIN,MAX;
	for(i=0;i<128;i++)
	{
	  MAX = MAX_2(MAX,CCD_filtering_data[i]);
		MIN = MIN_2(MIN,CCD_filtering_data[i]);
	}
//	printf("MAX = %d\n", MAX);
//	printf("MIN = %d\n", MIN);
//	printf("avr = %d\n", (MAX + MIN)/2);
	return ((MAX + MIN)/2);
}	





uint8_t MAX_5(uint8_t data_0,uint8_t data_1,uint8_t data_2,uint8_t data_3,uint8_t data_4)
{
 return MAX_2(MAX_2(MAX_2(MAX_2(data_0,data_1),data_2),data_3),data_4);
}

uint8_t MIN_5(uint8_t data_0,uint8_t data_1,uint8_t data_2,uint8_t data_3,uint8_t data_4)
{
 return MIN_2(MIN_2(MIN_2(MIN_2(data_0,data_1),data_2),data_3),data_4);
}

uint8_t MAX_2(uint8_t a,uint8_t b)
{
	return (a >= b)? a:b; 
}

uint8_t MIN_2(uint8_t a,uint8_t b)
{
 return (a <= b)? a:b;
}

uint8_t Track_Midline(void)
{
	  uint8_t i;
	  uint8_t Left_Side,Right_Side,Track_Midline_value,A,B;
		for(i=0;i<64;i++)
	  {
	   if(CCD_filtering_data[64-i]==0)
		 {
		   Left_Side = 64 - i;
			 break;
		 }
	  	else
			{
			  Left_Side = 0;
			}				
	  }
		
		for(i=0;i<64;i++)
	  {
	   if(CCD_filtering_data[64+i]==0)
		 {
		   Right_Side = 64 + i;
			 break;
		 }
	  	else
			{
			  Right_Side = 127;
			}				
	  }
		A = Left_Side;
		B = Right_Side;
		printf("Left_Side = %d\n",A);
		printf("Right_Side = %d\n",B);
	

		Track_Midline_value = Left_Side+((Right_Side - Left_Side)/2);
		return (Track_Midline_value);
}

