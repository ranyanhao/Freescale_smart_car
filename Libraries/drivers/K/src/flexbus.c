/**
  ******************************************************************************
  * @file    flexbus.c
  * @author  YANDLD
  * @version V2.5
  * @date    2014.3.26
  * @brief   www.beyondcore.net   http://upcmcu.taobao.com 
  * @note    ���ļ�ΪоƬFlexBusģ��ĵײ㹦�ܺ���
  ******************************************************************************
  */
#include "flexbus.h"
#include "common.h"

#if (!defined(MK10D5))

/**
 * @brief  ��ʼ��FlexBusģ��
 * @note   ���������Ӧ���������FlexBus��ʹ������  
 * @param  FLEXBUS_InitStruct :FlexBus��ʼ�����ýṹ�壬���FlexBus.h
 * @retval None
 */
void FLEXBUS_Init(FLEXBUS_InitTypeDef* FLEXBUS_InitStruct)
{
    /* enable clock gate enable seruriy mode */
    SIM->SOPT2 |= SIM_SOPT2_FBSL(3);
    SIM->SCGC7 |= SIM_SCGC7_FLEXBUS_MASK;
    /* div */
    SIM->CLKDIV1 &= ~SIM_CLKDIV1_OUTDIV3_MASK;
    SIM->CLKDIV1 |= SIM_CLKDIV1_OUTDIV3(FLEXBUS_InitStruct->div);
    /* we must set V_MASK in CS0, because CS0.CSMR.V_MASK act as a global CS */
    FB->CS[0].CSMR |= FB_CSMR_V_MASK; 
    /* clear registers */
    FB->CS[FLEXBUS_InitStruct->CSn].CSCR = 0;
    /* base address */
    FB->CS[FLEXBUS_InitStruct->CSn].CSAR = FLEXBUS_InitStruct->baseAddress;
    /* address space */
    FB->CS[FLEXBUS_InitStruct->CSn].CSMR = FB_CSMR_BAM(FLEXBUS_InitStruct->ADSpaceMask) | FB_CSMR_V_MASK;
    /* port size */
    FB->CS[FLEXBUS_InitStruct->CSn].CSCR &= FB_CSCR_PS_MASK;
    FB->CS[FLEXBUS_InitStruct->CSn].CSCR |= FB_CSCR_PS(FLEXBUS_InitStruct->dataWidth);
    /* AutoAcknogement(AA) Config */
    if(FLEXBUS_InitStruct->autoAckMode == kFLEXBUS_AutoAckEnable)
    {
        FB->CS[FLEXBUS_InitStruct->CSn].CSCR |= FB_CSCR_AA_MASK;
    }
    else
    {
        FB->CS[FLEXBUS_InitStruct->CSn].CSCR &= ~FB_CSCR_AA_MASK;
    }
    /* data align */
    if(FLEXBUS_InitStruct->dataAlignMode == kFLEXBUS_DataLeftAligned)
    {
        FB->CS[FLEXBUS_InitStruct->CSn].CSCR &= ~FB_CSCR_BLS_MASK;
    }
    else
    {
        FB->CS[FLEXBUS_InitStruct->CSn].CSCR |= FB_CSCR_BLS_MASK;
    }
    /* byte enable mode */
    if(FLEXBUS_InitStruct->ByteEnableMode == kFLEXBUS_BE_AssertedWrite)
    {
        FB->CS[FLEXBUS_InitStruct->CSn].CSCR &= ~FB_CSCR_BEM_MASK;
    }
    else
    {
        FB->CS[FLEXBUS_InitStruct->CSn].CSCR |= FB_CSCR_BEM_MASK;
    }
    /* assert wait status */
    FB->CS[FLEXBUS_InitStruct->CSn].CSCR &= ~FB_CSCR_WS_MASK;
    FB->CS[FLEXBUS_InitStruct->CSn].CSCR |= FB_CSCR_WS(1);
}


void FLEXBUS_PortMuxConfig(FLEXBUS_PortMultiplexingSelect_Type group, uint32_t config)
{
    /* CS Port Multiplexing Cotrol */
    switch(group)
    {
        case kFLEXBUS_CSPMCR_Group1:
            FB->CSPMCR &= ~FB_CSPMCR_GROUP1_MASK;
            FB->CSPMCR |= FB_CSPMCR_GROUP1(config);
            break;
        case kFLEXBUS_CSPMCR_Group2:
            FB->CSPMCR &= ~FB_CSPMCR_GROUP2_MASK;
            FB->CSPMCR |= FB_CSPMCR_GROUP2(config);
            break;
        case kFLEXBUS_CSPMCR_Group3:
            FB->CSPMCR &= ~FB_CSPMCR_GROUP3_MASK;
            FB->CSPMCR |= FB_CSPMCR_GROUP3(config);
            break;
        case kFLEXBUS_CSPMCR_Group4:
            FB->CSPMCR &= ~FB_CSPMCR_GROUP4_MASK;
            FB->CSPMCR |= FB_CSPMCR_GROUP4(config);
            break;
        case kFLEXBUS_CSPMCR_Group5:
            FB->CSPMCR &= ~FB_CSPMCR_GROUP5_MASK;
            FB->CSPMCR |= FB_CSPMCR_GROUP5(config);
            break;
        default:
            break;
    }
}



#endif



















