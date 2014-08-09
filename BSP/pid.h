#ifndef __PID_H
#define	__PID_H

#include "stdio.h" 
#include "math.h" 
#include "includes.h"

typedef struct PID        //定义PID 的结构体 
{ 
  int SetPoint;
	double Proportion;
	double Integral;
	double Derivative;
	int LastError;            //Error[-1]
	int PrevError;            //Error[-2]
	
	int View_Midline;
	double Proportion_1;
	double Integral_1;
	double Derivative_1;
	int LastError_1;          //Error[-1]
	int PrevError_1;          //Error[-2]
}PID; 

#define P_DATA 2
#define I_DATA 0
#define D_DATA 0

#define P_DATA_1 3
#define I_DATA_1 0
#define D_DATA_1 5

void PID_Init(void);
int PID_Calc(uint8_t pid_flag, int NextPoint, uint8_t Track_Midline);
#endif /* __PID_H */


