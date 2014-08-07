/**
  ******************************************************************************
  * @file    pit.c
  * @author  YANDLD
  * @version V2.5
  * @date    2014.3.24
  * @brief   www.beyondcore.net   http://upcmcu.taobao.com 
  * @note    ���ļ�ΪоƬPITģ��ĵײ㹦�ܺ���
  ******************************************************************************
  */
#include "pit.h"
#include "common.h"

//!< Internal vars
static uint32_t fac_us; //!< usDelay Mut
static uint32_t fac_ms; //!< msDelay Mut

static PIT_CallBackType PIT_CallBackTable[4] = {NULL};
static const IRQn_Type PIT_IRQnTable[] = 
{
    PIT0_IRQn,
    PIT1_IRQn,
    PIT2_IRQn,
    PIT3_IRQn,
};

//! @defgroup CHKinetis
//! @{


//! @defgroup PIT
//! @brief PIT API functions
//! @{

 /**
 * @brief  ��ϸ��ʼ��PITģ�� �Ƽ�ʹ��PIT_QuickInit����
 * @code
 *      // ����PIT0ģ���0ͨ����ʱ������Ϊ1ms
 *      PIT_InitTypeDef PIT_InitStruct1; //����һ���ṹ����
 *      PIT_InitStruct1.chl = 0;         //ѡ��0ͨ��
 *      PIT_InitStruct1.timeInUs = 1000  //1ms
 *      PIT_Init(&PIT_InitStruct1);
 * @endcode
 * @param  PIT_InitStruct: pitģ�鹤����������
 * @retval None
 */
void PIT_Init(PIT_InitTypeDef* PIT_InitStruct)
{
    SIM->SCGC6 |= SIM_SCGC6_PIT_MASK;
    /* get clock */
    CLOCK_GetClockFrequency(kBusClock, &fac_us);
    fac_us /= 1000000;
    fac_ms = (fac_us * 1000);
    fac_ms = fac_ms;
    PIT->CHANNEL[PIT_InitStruct->chl].LDVAL = fac_us * PIT_InitStruct->timeInUs;
    PIT->CHANNEL[PIT_InitStruct->chl].TCTRL |= (PIT_TCTRL_TEN_MASK);
    /* enable PIT module */
    PIT->MCR &= ~PIT_MCR_MDIS_MASK;
}

 /**
 * @brief  PITģ����ٳ�ʼ������
 * @code
 *      // ��ʼ��PITģ�� 0 ͨ�� ����100MS�ж� �������ж� ע��ص����� �ڻص������д�ӡ������Ϣ
 *      //�����жϻص�����
 *      static void PIT0_CallBack(void);
 *      //��ʼ��PITģ���0ͨ��������100ms�ж�
 *      PIT_QuickInit(HW_PIT_CH0, 100000);
 *      PIT_CallbackInstall(HW_PIT0_CH0, PIT0_CallBack); //ע��ص�����
 *      PIT_ITDMAConfig(HW_PIT0_CH0, ENABLE);            //�����ж�
 *      //�жϻص�����
 *      static void PIT0_CallBack(void)
 *      {
 *          printf("Enter PIT0 INt\r\n");    
 *      }
 * @endcode
 * @param  chl ͨ����
 *         @arg HW_PIT_CH0 
 *         @arg HW_PIT_CH1
 *         @arg HW_PIT_CH2 
 *         @arg HW_PIT_CH3 
 * @param  timeInUs �����жϵ����� \��λUS
 * @retval None
 */
void PIT_QuickInit(uint8_t chl, uint32_t timeInUs)
{
    PIT_InitTypeDef PIT_InitStruct1;
    PIT_InitStruct1.chl = chl;
    PIT_InitStruct1.timeInUs = timeInUs;
    PIT_Init(&PIT_InitStruct1);
}

 /**
 * @brief  ����PITģ���Ƿ����жϹ���
 * 
 * @code
 *      // ��ʼ��PITģ�� 0 ͨ�� ����100MS�ж� �������ж� ע��ص����� �ڻص������д�ӡ������Ϣ
 *      //�����жϻص�����
 *      static void PIT0_CallBack(void);
 *      //��ʼ��PIT
 *      PIT_QuickInit(HW_PIT_CH0, 100000);
 *      PIT_CallbackInstall(HW_PIT0_CH0, PIT0_CallBack); //ע��ص�����
 *      PIT_ITDMAConfig(HW_PIT0_CH0, ENABLE);            //����0ͨ���ж�
 *      //�жϻص�������д
 *      static void PIT0_CallBack(void)
 *      {
 *          printf("Enter PIT0 INt\r\n");    
 *      }
 * @endcode
 * @param  chl  :ͨ����
 *         @arg HW_PIT0_CH0   :0ͨ��
 *         @arg HW_PIT0_CH1   :1ͨ��
 *         @arg HW_PIT0_CH2   :2ͨ��
 *         @arg HW_PIT0_CH3   :3ͨ��
 * @param  NewState ENABLE ��DISABLE
 * @retval None
 */
void PIT_ITDMAConfig(uint8_t chl, PIT_ITDMAConfig_Type config)
{
    SIM->SCGC6 |= SIM_SCGC6_PIT_MASK;
    switch (config)
    {
        case kPIT_IT_Disable:
            //NVIC_DisableIRQ(PIT_IRQnTable[chl]);
            PIT->CHANNEL[chl].TCTRL &= ~PIT_TCTRL_TIE_MASK;
            break;
        case kPIT_IT_TOF:
            NVIC_EnableIRQ(PIT_IRQnTable[chl]);
            PIT->CHANNEL[0].TFLG |= PIT_TFLG_TIF_MASK;
            PIT->CHANNEL[chl].TCTRL |= PIT_TCTRL_TIE_MASK;
            break;
        default:
            break;
    }
}

 /**
 * @brief  PIT��ʱ��Counter�嵽LOADֵ (���¿�ʼ��һ��CountDown��ʱ)
 * @param  chl  :ͨ����
 *         @arg HW_PIT0_CH0   :0ͨ��
 *         @arg HW_PIT0_CH1   :1ͨ��
 *         @arg HW_PIT0_CH2   :2ͨ��
 *         @arg HW_PIT0_CH3   :3ͨ��
 * @retval None
 */
void PIT_ResetCounter(uint8_t chl)
{
    PIT->CHANNEL[chl].TCTRL &= (~PIT_TCTRL_TEN_MASK);
    PIT->CHANNEL[chl].TCTRL |= (PIT_TCTRL_TEN_MASK);
}
 /**
 * @brief  ��ȡCounterֵ
 * @param  chl  :ͨ����
 *         @arg HW_PIT0_CH0   :0ͨ��
 *         @arg HW_PIT0_CH1   :1ͨ��
 *         @arg HW_PIT0_CH2   :2ͨ��
 *         @arg HW_PIT0_CH3   :3ͨ��
 * @retval Counterֵ
 */
uint32_t PIT_GetCounterValue(uint8_t chl)
{
    return PIT->CHANNEL[chl].CVAL;
}


/**
 * @brief  ע���жϻص�����
 * @param  chl  :ͨ����
 *         @arg HW_PIT0_CH0   :0ͨ�����
 *         @arg HW_PIT0_CH1   :1ͨ�����
 *         @arg HW_PIT0_CH2   :2ͨ�����
 *         @arg HW_PIT0_CH3   :3ͨ�����
 * @param AppCBFun: �ص�����ָ�����
 * @retval None
 * @note ���ڴ˺����ľ���Ӧ�������Ӧ��ʵ��
 */
void PIT_CallbackInstall(uint8_t chl, PIT_CallBackType AppCBFun)
{
    SIM->SCGC6 |= SIM_SCGC6_PIT_MASK;
    if(AppCBFun != NULL)
    {
        PIT_CallBackTable[chl] = AppCBFun;
    }
}

//! @}

//! @}

void PIT0_IRQHandler(void)
{
    PIT->CHANNEL[0].TFLG |= PIT_TFLG_TIF_MASK;
    if(PIT_CallBackTable[0])
    {
        PIT_CallBackTable[0]();
    }
}

void PIT1_IRQHandler(void)
{
    PIT->CHANNEL[1].TFLG |= PIT_TFLG_TIF_MASK;
    if(PIT_CallBackTable[1])
    {
        PIT_CallBackTable[1]();
    }
}

void PIT2_IRQHandler(void)
{
    PIT->CHANNEL[2].TFLG |= PIT_TFLG_TIF_MASK;
    if(PIT_CallBackTable[2])
    {
        PIT_CallBackTable[2]();
    }
}

void PIT3_IRQHandler(void)
{
    PIT->CHANNEL[3].TFLG |= PIT_TFLG_TIF_MASK;
    if(PIT_CallBackTable[3])
    {
        PIT_CallBackTable[3]();
    }
}

