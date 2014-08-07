/**
  ******************************************************************************
  * @file    sram.c
  * @author  YANDLD
  * @version V2.5
  * @date    2014.3.24
  * @brief   www.beyondcore.net   http://upcmcu.taobao.com 
  ******************************************************************************
  */
#include "sram.h"
#include "flexbus.h"
#include "common.h"


#define SRAM_DEBUG		1
#if ( SRAM_DEBUG == 1 )
#include <stdio.h>
#define SRAM_TRACE	printf
#else
#define SRAM_TRACE(...)
#endif

//SRAM��ʼ������
void SRAM_Init(void)
{
    /* set SRAM pinMux */
    SIM->SCGC5 |= (SIM_SCGC5_PORTA_MASK | SIM_SCGC5_PORTB_MASK | SIM_SCGC5_PORTC_MASK | SIM_SCGC5_PORTD_MASK | SIM_SCGC5_PORTE_MASK);
    /* flexbus clock output(optional, use for debug) */
    PORTC->PCR[3] = PORT_PCR_MUX(5)|PORT_PCR_DSE_MASK;          // FB_CLKOUT
    /*control signals */
    PORTB->PCR[19] = PORT_PCR_MUX(5)|PORT_PCR_DSE_MASK;          // FB_OE
    PORTC->PCR[11] = PORT_PCR_MUX(5)|PORT_PCR_DSE_MASK;          // FB_RW
    PORTD->PCR[0] =  PORT_PCR_MUX(5)|PORT_PCR_DSE_MASK;          // CS1
    PORTC->PCR[16] = PORT_PCR_MUX(5)|PORT_PCR_DSE_MASK;          // FB_BE_23_16
    PORTC->PCR[17] = PORT_PCR_MUX(5)|PORT_PCR_DSE_MASK;          // FB_BE_31-24
    /* address signal */
    PORTD->PCR[10] = PORT_PCR_MUX(6)|PORT_PCR_DSE_MASK;         //  FB_A18 
    PORTD->PCR[9] = PORT_PCR_MUX(6)|PORT_PCR_DSE_MASK;          //  FB_A17 
    PORTD->PCR[8] = PORT_PCR_MUX(6)|PORT_PCR_DSE_MASK;          //  FB_A16
    PORTB->PCR[18] = PORT_PCR_MUX(5)|PORT_PCR_DSE_MASK;         //  FB_A15
    PORTC->PCR[0]  = PORT_PCR_MUX(5)|PORT_PCR_DSE_MASK;         //  FB_A14
    PORTC->PCR[1]  = PORT_PCR_MUX(5)|PORT_PCR_DSE_MASK;         //  FB_A13
    PORTC->PCR[2]  = PORT_PCR_MUX(5)|PORT_PCR_DSE_MASK;         //  FB_A12
    PORTC->PCR[4]  = PORT_PCR_MUX(5)|PORT_PCR_DSE_MASK;         //  FB_A11
    PORTC->PCR[5]  = PORT_PCR_MUX(5)|PORT_PCR_DSE_MASK;         //  FB_A10
    PORTC->PCR[6]  = PORT_PCR_MUX(5)|PORT_PCR_DSE_MASK;         //  FB_A9
    PORTC->PCR[7]  = PORT_PCR_MUX(5)|PORT_PCR_DSE_MASK;         //  FB_A8
    PORTC->PCR[8]  = PORT_PCR_MUX(5)|PORT_PCR_DSE_MASK;         //  FB_A7
    PORTC->PCR[9]  = PORT_PCR_MUX(5)|PORT_PCR_DSE_MASK;         //  FB_A6
    PORTC->PCR[10] = PORT_PCR_MUX(5)|PORT_PCR_DSE_MASK;         //  FB_A5
    PORTD->PCR[2]  = PORT_PCR_MUX(5)|PORT_PCR_DSE_MASK;         //  FB_A4
    PORTD->PCR[3]  = PORT_PCR_MUX(5)|PORT_PCR_DSE_MASK;         //  FB_A3
    PORTD->PCR[4]  = PORT_PCR_MUX(5)|PORT_PCR_DSE_MASK;         //  FB_A2
    PORTD->PCR[5]  = PORT_PCR_MUX(5)|PORT_PCR_DSE_MASK;         //  FB_A1
    /* data signal */
    PORTB->PCR[17]  = PORT_PCR_MUX(5)|PORT_PCR_DSE_MASK;        //  FB_AD16
    PORTB->PCR[16]  = PORT_PCR_MUX(5)|PORT_PCR_DSE_MASK;        //  FB_AD17
    PORTB->PCR[11]  = PORT_PCR_MUX(5)|PORT_PCR_DSE_MASK;        //  FB_AD18
    PORTB->PCR[10]  = PORT_PCR_MUX(5)|PORT_PCR_DSE_MASK;        //  FB_AD19
    PORTB->PCR[9]  = PORT_PCR_MUX(5)|PORT_PCR_DSE_MASK;         //  FB_AD20
    PORTB->PCR[8]  = PORT_PCR_MUX(5)|PORT_PCR_DSE_MASK;         //  FB_AD21
    PORTB->PCR[7]  = PORT_PCR_MUX(5)|PORT_PCR_DSE_MASK;         //  FB_AD22
    PORTB->PCR[6]  = PORT_PCR_MUX(5)|PORT_PCR_DSE_MASK;         //  FB_AD23
    PORTC->PCR[15]  = PORT_PCR_MUX(5)|PORT_PCR_DSE_MASK;        //  FB_AD24
    PORTC->PCR[14]  = PORT_PCR_MUX(5)|PORT_PCR_DSE_MASK;        //  FB_AD25
    PORTC->PCR[13]  = PORT_PCR_MUX(5)|PORT_PCR_DSE_MASK;        //  FB_AD26
    PORTC->PCR[12]  = PORT_PCR_MUX(5)|PORT_PCR_DSE_MASK;        //  FB_AD27
    PORTB->PCR[23]  = PORT_PCR_MUX(5)|PORT_PCR_DSE_MASK;        //  FB_AD28
    PORTB->PCR[22]  = PORT_PCR_MUX(5)|PORT_PCR_DSE_MASK;        //  FB_AD29
    PORTB->PCR[21]  = PORT_PCR_MUX(5)|PORT_PCR_DSE_MASK;        //  FB_AD30
    PORTB->PCR[20]  = PORT_PCR_MUX(5)|PORT_PCR_DSE_MASK;        //  FB_AD31 
    /* enable flexbus */
    FLEXBUS_InitTypeDef FLEXBUS_InitStruct;
    FLEXBUS_InitStruct.ADSpaceMask = kFLEXBUS_ADSpace_512KByte; /* �ڴ��ַ��Χ 512K */
    FLEXBUS_InitStruct.autoAckMode = kFLEXBUS_AutoAckEnable; /*�����Զ�Ӧ�� */
    FLEXBUS_InitStruct.CSn = kFLEXBUS_CS1; /*ʹ��CS1Ƭѡ�ź� */
    FLEXBUS_InitStruct.dataAlignMode = kFLEXBUS_DataLeftAligned; /*��������� */
    FLEXBUS_InitStruct.dataWidth = kFLEXBUS_PortSize_16Bit; /*����λ�� 16λ */
    FLEXBUS_InitStruct.baseAddress = SRAM_ADDRESS_BASE; /* ����ַ */
    FLEXBUS_InitStruct.ByteEnableMode = kFLEXBUS_BE_AssertedReadWrite; /* �ڶ�д������ʱ�򶼲��� λʹ���ź� */
    FLEXBUS_InitStruct.div = 0;
    FLEXBUS_Init(&FLEXBUS_InitStruct);
    /* config Flexbus SRAM pinmux */
    FLEXBUS_PortMuxConfig(kFLEXBUS_CSPMCR_Group3, kFLEXBUS_CSPMCR_GROUP3_BE_23_16);
    FLEXBUS_PortMuxConfig(kFLEXBUS_CSPMCR_Group2, kFLEXBUS_CSPMCR_GROUP2_BE_31_24);
    FLEXBUS_PortMuxConfig(kFLEXBUS_CSPMCR_Group1, kFLEXBUS_CSPMCR_GROUP1_CS1);
}

//�Բ��Գ�����sram��д���ݣ�Ȼ��������ݣ���֤���ݵ���ȷ��
uint32_t SRAM_SelfTest(void)
{
    uint32_t i;
    uint32_t address,size;
    address = SRAM_ADDRESS_BASE;
    size = SRAM_SIZE;
    SRAM_TRACE("memtest,address: 0x%08X size: 0x%08X\r\n", address, size);
    
    /**< 8bit test */
    {
        uint8_t * p_uint8_t = (uint8_t *)address;
        for(i=0; i<size/sizeof(uint8_t); i++)
        {
            *p_uint8_t++ = (uint8_t)i;
        }

        p_uint8_t = (uint8_t *)address;
        for(i=0; i<size/sizeof(uint8_t); i++)
        {
            if( *p_uint8_t != (uint8_t)i )
            {
                printf("8bit test fail @ 0x%08X\r\nsystem halt!!!!!",(uint32_t)p_uint8_t);
                while(1);
            }
            p_uint8_t++;
        }
        printf("8bit test pass!!\r\n");
    }
    
    /**< 16bit test */
    {
        uint16_t * p_uint16_t = (uint16_t *)address;
        for(i=0; i<size/sizeof(uint16_t); i++)
        {
            *p_uint16_t++ = (uint16_t)i;
        }

        p_uint16_t = (uint16_t *)address;
        for(i=0; i<size/sizeof(uint16_t); i++)
        {
            if( *p_uint16_t != (uint16_t)i )
            {
                printf("16bit test fail @ 0x%08X\r\nsystem halt!!!!!",(uint32_t)p_uint16_t);
                while(1);
            }
            p_uint16_t++;
        }
        printf("16bit test pass!!\r\n");
    }

    /**< 32bit test */
    {
        uint32_t * p_uint32_t = (uint32_t *)address;
        for(i=0; i<size/sizeof(uint32_t); i++)
        {
            *p_uint32_t++ = (uint32_t)i;
        }

        p_uint32_t = (uint32_t *)address;
        for(i=0; i<size/sizeof(uint32_t); i++)
        {
            if( *p_uint32_t != (uint32_t)i )
            {
                printf("32bit test fail @ 0x%08X\r\nsystem halt!!!!!",(uint32_t)p_uint32_t);
                while(1);
            }
            p_uint32_t++;
        }
        printf("32bit test pass!!\r\n");
    }

    /**< 32bit Loopback test */
    {
        uint32_t * p_uint32_t = (uint32_t *)address;
        for(i=0; i<size/sizeof(uint32_t); i++)
        {
            *p_uint32_t  = (uint32_t)p_uint32_t;
            *p_uint32_t++;
        }

        p_uint32_t = (uint32_t *)address;
        for(i=0; i<size/sizeof(uint32_t); i++)
        {
            if( *p_uint32_t != (uint32_t)p_uint32_t )
            {
                printf("32bit Loopback test fail @ 0x%08X", (uint32_t)p_uint32_t);
                printf(" data:0x%08X \r\n", (uint32_t)*p_uint32_t);
                printf("system halt!!!!!");
                while(1);
            }
            p_uint32_t++;
        }
        printf("32bit Loopback test pass!!\r\n");
    }
    
    return  0;
}





