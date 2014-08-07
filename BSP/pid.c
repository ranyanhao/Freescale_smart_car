#include "includes.h"

static PID sPID;
static PID*sptr = &sPID;

void PID_Init(void)
{
  sptr->LastError  = 0;         
	sptr->PrevError  = 0;         
	sptr->Proportion = P_DATA;
	sptr->Integral   = I_DATA;
	sptr->Derivative = D_DATA;
	sptr->SetPoint   = Speed_Set();                   //((200/17.279)*(106/40)*200)*(100/1000);
	
	sptr->LastError_1  = 0;         
	sptr->PrevError_1  = 0;         
	sptr->Proportion_1 = P_DATA_1;
	sptr->Integral_1   = I_DATA_1;
	sptr->Derivative_1 = D_DATA_1;
	sptr->View_Midline   = 64;                  
}	

int PID_Calc(uint8_t pid_flag, int NextPoint, uint8_t Track_Midline)
{ 
  int iError,A,B;                 //当前误差
	int iIncpid;                //增量值
	int iError_1;               //当前误差
	int iIncpid_1;              //增量值
	if(pid_flag==0)
	{
	 iError = sptr->SetPoint - NextPoint;
		A = iError;
	 printf("iError = %d\n",A);
		if(abs(iError) > 20)
		{
		  iIncpid = sptr->Proportion * iError + sptr->Integral * sptr->LastError;
		}
		else
		{
		  iIncpid = sptr->Proportion * iError + sptr->Integral * sptr->LastError + sptr->Derivative * sptr->PrevError; 
		}
	 
	 B = iIncpid; 
	 printf("iIncpid = %d\n",B);
	 sptr->PrevError = sptr->LastError;
	 sptr->LastError = iError;
	 return (iIncpid);
	}
	else
	{
	 iError_1 = Track_Midline - sptr->View_Midline;
	 A = iError_1;
	 printf("iError_1 = %d\n",A);
	
		if(abs(iError_1) < 1)
		{
		  sptr->Proportion_1 = 0;
		}
		else if(abs(iError_1) >= 1 && abs(iError) <= 5)
		{
		  sptr->Proportion_1 = 2;
		}
    else
		{
		  sptr->Proportion_1 = sptr->Proportion_1;
		}			
	 iIncpid_1 = sptr->Proportion_1 * iError_1 + sptr->Derivative_1 * sptr->LastError_1; 
	 B = iIncpid_1; 
	 printf("iIncpid_1 = %d\n",B);
	 sptr->LastError_1 = iError_1;
	 return (iIncpid_1);
	}
}	


