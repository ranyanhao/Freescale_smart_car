/**
  ******************************************************************************
  * @file    uart.c
  * @author  YANDLD
  * @version V2.5
  * @date    2014.3.25
  * @brief   www.beyondcore.net   http://upcmcu.taobao.com 
  * @note    ���ļ�ΪоƬUARTģ��ĵײ㹦�ܺ���
  ******************************************************************************
  */
#include "uart.h"
#include "gpio.h"
#include "common.h"

#ifdef UART_USE_STDIO
#if __ICCARM__
#include <yfuns.h>
#endif
#include <stdio.h>
#endif

#if (!defined(GPIO_BASES))

    #if     (defined(MK60DZ10))
    #define UART_BASES {UART0, UART1, UART2, UART3, UART4, UART5}
    #elif   (defined(MK10D5))
    #define UART_BASES {UART0, UART1, UART2}
    #endif

#endif

/* gloabl const table defination */
UART_Type * const UART_InstanceTable[] = UART_BASES;
/* callback function slot */
static UART_CallBackTxType UART_CallBackTxTable[ARRAY_SIZE(UART_InstanceTable)] = {NULL};
static UART_CallBackRxType UART_CallBackRxTable[ARRAY_SIZE(UART_InstanceTable)] = {NULL};
/* special use for printf */
static uint8_t UART_DebugInstance;
/* instance clock gate table */
#if (defined(MK60DZ10) || defined(MK40D10) || defined(MK60D10)|| defined(MK10D10) || defined(MK70F12) || defined(MK70F15))
static const struct reg_ops SIM_UARTClockGateTable[] =
{
    {(void*)&(SIM->SCGC4), SIM_SCGC4_UART0_MASK},
    {(void*)&(SIM->SCGC4), SIM_SCGC4_UART1_MASK},
    {(void*)&(SIM->SCGC4), SIM_SCGC4_UART2_MASK},
    {(void*)&(SIM->SCGC4), SIM_SCGC4_UART3_MASK},
    {(void*)&(SIM->SCGC1), SIM_SCGC1_UART4_MASK},
    {(void*)&(SIM->SCGC1), SIM_SCGC1_UART5_MASK},
};
/* interrupt handler table */
static const IRQn_Type UART_IRQnTable[] = 
{
    UART0_RX_TX_IRQn,
    UART1_RX_TX_IRQn,
    UART2_RX_TX_IRQn,
    UART3_RX_TX_IRQn,
    UART4_RX_TX_IRQn,
    UART5_RX_TX_IRQn,
};
#elif (defined(MK21D5))
static const struct reg_ops SIM_UARTClockGateTable[] =
{
    {(void*)&(SIM->SCGC4), SIM_SCGC4_UART0_MASK},
    {(void*)&(SIM->SCGC4), SIM_SCGC4_UART1_MASK},
    {(void*)&(SIM->SCGC4), SIM_SCGC4_UART2_MASK},
    {(void*)&(SIM->SCGC4), SIM_SCGC4_UART3_MASK},
};
/* interrupt handler table */
static const IRQn_Type UART_IRQnTable[] = 
{
    UART0_RX_TX_IRQn,
    UART1_RX_TX_IRQn,
    UART2_RX_TX_IRQn,
    UART3_RX_TX_IRQn,
};
#elif (defined(MK10D5))
static const struct reg_ops SIM_UARTClockGateTable[] =
{
    {(void*)&(SIM->SCGC4), SIM_SCGC4_UART0_MASK},
    {(void*)&(SIM->SCGC4), SIM_SCGC4_UART1_MASK},
    {(void*)&(SIM->SCGC4), SIM_SCGC4_UART2_MASK},
};
static const IRQn_Type UART_IRQnTable[] = 
{
    UART0_RX_TX_IRQn,
    UART1_RX_TX_IRQn,
    UART2_RX_TX_IRQn,
};
#endif

#ifdef UART_USE_STDIO
#ifdef __CC_ARM // MDK Support
struct __FILE 
{ 
	int handle;
	/* Whatever you require here. If the only file you are using is */ 
	/* standard output using printf() for debugging, no file handling */ 
	/* is required. */ 
}; 
/* FILE is typedef�� d in stdio.h. */ 
FILE __stdout;
FILE __stdin;
int fputc(int ch,FILE *f)
{
	UART_WriteByte(UART_DebugInstance, ch);
	return ch;
}

int fgetc(FILE *f)
{
    uint16_t ch;
    while(UART_ReadByte(UART_DebugInstance, &ch));
    return (ch & 0xFF);
}
#elif __ICCARM__ /* IAR support */
size_t __write(int handle, const unsigned char * buffer, size_t size)
{
    size_t nChars = 0;
    if (buffer == 0)
    {
        /* This means that we should flush internal buffers.  Since we*/
        /* don't we just return.  (Remember, "handle" == -1 means that all*/
        /* handles should be flushed.)*/
        return 0;
    }
    /* This function only writes to "standard out" and "standard err",*/
    /* for all other file handles it returns failure.*/
    if ((handle != _LLIO_STDOUT) && (handle != _LLIO_STDERR))
    {
        return _LLIO_ERROR;
    }
    /* Send data.*/
    while (size--)
    {
        UART_WriteByte(UART_DebugInstance, *buffer++);
        ++nChars;
    }
    return nChars;
}

size_t __read(int handle, unsigned char * buffer, size_t size)
{
    size_t nChars = 0;
    uint16_t ch = 0;
    if (buffer == 0)
    {
        /* This means that we should flush internal buffers.  Since we*/
        /* don't we just return.  (Remember, "handle" == -1 means that all*/
        /* handles should be flushed.)*/
        return 0;
    }
    /* This function only writes to "standard out" and "standard err",*/
    /* for all other file handles it returns failure.*/
    if ((handle != _LLIO_STDIN) && (handle != _LLIO_STDERR))
    {
        return _LLIO_ERROR;
    }
    /* read data.*/
    while (size--)
    {
        while(UART_ReadByte(UART_DebugInstance, &ch));
        *buffer++ = (char)ch & 0xFF;
        ++nChars;
    }
    return nChars;
}


#endif /* comiler support */

#else /* DO NOT USE STDIO */
static void UART_putstr(uint32_t instance, const char *str)
{
    while(*str != '\0')
    {
        UART_WriteByte(instance, *str++);
    }
}

static void printn(unsigned int n, unsigned int b)
{
    static char *ntab = "0123456789ABCDEF";
    unsigned int a, m;
    if (n / b)
    {
        a = n / b;
        printn(a, b);  
    }
    m = n % b;
    UART_WriteByte(UART_DebugInstance, ntab[m]);
}

int UART_printf(const char *fmt, ...)
{
    char c;
    unsigned int *adx = (unsigned int*)(void*)&fmt + 1;
_loop:
    while((c = *fmt++) != '%')
    {
        if (c == '\0') return 0;
        UART_WriteByte(UART_DebugInstance, c);
    }
    c = *fmt++;
    if (c == 'd' || c == 'l')
    {
        printn(*adx, 10);
    }
    if (c == 'o' || c == 'x')
    {
        printn(*adx, c=='o'? 8:16 );
    }
    if (c == 's')
    {
        UART_putstr(UART_DebugInstance, (char*)*adx);
    }
    adx++;
    goto _loop;
    return 0;
}
#endif /*end of UART_USE_STDIO */

//! @defgroup CHKinetis
//! @{

//! @defgroup UART
//! @brief UART API functions
//! @{

/**
 * @brief  ��ʼ��UARTģ�� 
 * @note   �û����Լ��������ŵĸ�������
 * @code
 *      //ʹ��UART0ģ�� ʹ��115200�����ʽ���ͨ��
 *    UART_InitTypeDef UART_InitStruct1;      //����һ���ṹ����
 *    UART_InitStruct1.instance = HW_UART0;   //ѡ��UART0ģ��
 *    UART_InitStruct1.baudrate = 115200;     //����ͨ���ٶ�Ϊ115200
 *    UART_InitStruct1.parityMode = kUART_ParityDisabled; //У��λ��ֹ
 *    UART_InitStruct1.bitPerChar = kUART_8BitsPerChar;   //ÿ֡8bit
 *    UART_Init(&UART_InitStruct1);
 * @endcode
 * @param  UART_InitTypeDef: ���ڹ������ô洢�ṹ��
 *         instance      :оƬ���ڶ˿�
 *         @arg HW_UART0 :оƬ��UART0�˿�
 *         @arg HW_UART1 :оƬ��UART1�˿�
 *         @arg HW_UART2 :оƬ��UART2�˿�
 *         @arg HW_UART3 :оƬ��UART3�˿�
 *         @arg HW_UART4 :оƬ��UART4�˿�
 *         @arg HW_UART5 :оƬ��UART5�˿�
 * @param  baudrate  :����ͨѶ��������
 * @retval None
 */
void UART_Init(UART_InitTypeDef* UART_InitStruct)
{
    uint16_t sbr;
    uint8_t brfa; 
    uint32_t clock;
    static bool is_fitst_init = true;
	/* param check */
    assert_param(IS_UART_ALL_INSTANCE(UART_InitStruct->instance));
    /* enable clock gate */
    *((uint32_t*) SIM_UARTClockGateTable[UART_InitStruct->instance].addr) |= SIM_UARTClockGateTable[UART_InitStruct->instance].mask;
    /* disable Tx Rx first */
    UART_InstanceTable[UART_InitStruct->instance]->C2 &= ~((UART_C2_TE_MASK)|(UART_C2_RE_MASK));
    /* get clock */
    CLOCK_GetClockFrequency(kBusClock, &clock);
    if((UART_InitStruct->instance == 0) || (UART_InitStruct->instance == 1))
    {
        CLOCK_GetClockFrequency(kCoreClock, &clock); /* UART0 UART1 are use core clock */
    }
    sbr = (uint16_t)((clock)/((UART_InitStruct->baudrate)*16));
    brfa = (32*clock/((UART_InitStruct->baudrate)*16)) - 32*sbr;
    /* config baudrate */
    UART_InstanceTable[UART_InitStruct->instance]->BDH |= UART_BDH_SBR(sbr>>8); 
    UART_InstanceTable[UART_InitStruct->instance]->BDL = UART_BDL_SBR(sbr); 
    UART_InstanceTable[UART_InitStruct->instance]->C4 |= UART_C4_BRFA(brfa);
    /* parity */
    switch(UART_InitStruct->parityMode)
    {
        case kUART_ParityDisabled: /* standard N 8 N 1*/
            UART_InstanceTable[UART_InitStruct->instance]->C1 &= ~UART_C1_PE_MASK;
            UART_InstanceTable[UART_InitStruct->instance]->C1 &= ~UART_C1_M_MASK;
            break;
        case kUART_ParityEven:/* 8 bit data + 1bit parity */
            UART_InstanceTable[UART_InitStruct->instance]->C1 |= UART_C1_PE_MASK;
            UART_InstanceTable[UART_InitStruct->instance]->C1 &= ~UART_C1_PT_MASK;
            break;
        case kUART_ParityOdd:
            UART_InstanceTable[UART_InitStruct->instance]->C1 |= UART_C1_PE_MASK;
            UART_InstanceTable[UART_InitStruct->instance]->C1 |= UART_C1_PT_MASK;
            break;
        default:
            break;
    }
    /* bit per char */
    /* note: Freescale's bit size config in register are including parity bit! */
    switch(UART_InitStruct->bitPerChar)
    {
        case kUART_8BitsPerChar:
            if(UART_InstanceTable[UART_InitStruct->instance]->C1 & UART_C1_PE_MASK)
            {
                /* parity is enabled it's actually 9bit*/
                UART_InstanceTable[UART_InitStruct->instance]->C1 |= UART_C1_M_MASK;
                UART_InstanceTable[UART_InitStruct->instance]->C4 &= ~UART_C4_M10_MASK;    
            }
            else
            {
                UART_InstanceTable[UART_InitStruct->instance]->C1 &= ~UART_C1_M_MASK;
                UART_InstanceTable[UART_InitStruct->instance]->C4 &= ~UART_C4_M10_MASK;    
            }
            break;
        case kUART_9BitsPerChar:
            if(UART_InstanceTable[UART_InitStruct->instance]->C1 & UART_C1_PE_MASK)
            {
                /* parity is enabled it's actually 10 bit*/
                UART_InstanceTable[UART_InitStruct->instance]->C1 |= UART_C1_M_MASK;
                UART_InstanceTable[UART_InitStruct->instance]->C4 |= UART_C4_M10_MASK;  
            } 
            else
            {
                UART_InstanceTable[UART_InitStruct->instance]->C1 |= UART_C1_M_MASK;
                UART_InstanceTable[UART_InitStruct->instance]->C4 &= ~UART_C4_M10_MASK;      
            }
            break;
        default:
            break;
    }
    UART_InstanceTable[UART_InitStruct->instance]->S2 &= ~UART_S2_MSBF_MASK; /* LSB */
    /* enable Tx Rx */
    UART_InstanceTable[UART_InitStruct->instance]->C2 |= ((UART_C2_TE_MASK)|(UART_C2_RE_MASK));
    /* link debug instance */
    
    /* if it's first initalized ,link getc and putc to it */
    if(is_fitst_init)
    {
        UART_DebugInstance = UART_InitStruct->instance;
    }
    is_fitst_init = false;
}

/**
 * @brief  ���ڷ���һ���ֽ�
 * @note   ����ʽ���� ֻ�з������Ż᷵��
 * @code
 *      //ʹ��UART0ģ�� ��������0x5A
 *    UART_WriteByte(HW_UART0, 0x5A);
 * @endcode
 * @param  instance      :оƬ���ڶ˿�
 *         @arg HW_UART0 :оƬ��UART0�˿�
 *         @arg HW_UART1 :оƬ��UART1�˿�
 *         @arg HW_UART2 :оƬ��UART2�˿�
 *         @arg HW_UART3 :оƬ��UART3�˿�
 *         @arg HW_UART4 :оƬ��UART4�˿�
 *         @arg HW_UART5 :оƬ��UART5�˿�
 * @param  ch: ��Ҫ���͵�һ�ֽ�����
 * @retval None
 */
void UART_WriteByte(uint32_t instance, uint16_t ch)
{
	/* param check */
    assert_param(IS_UART_ALL_INSTANCE(instance));
    /* config ninth bit */
    uint8_t ninth_bit = (ch >> 8) & 0x01U;
    (ninth_bit)?(UART_InstanceTable[instance]->C3 |= UART_C3_T8_MASK):(UART_InstanceTable[instance]->C3 &= ~UART_C3_T8_MASK);
    while(!(UART_InstanceTable[instance]->S1 & UART_S1_TDRE_MASK));
    UART_InstanceTable[instance]->D = (uint8_t)(ch & 0xFF);
}


/**
 * @brief  UART����һ���ֽ�
 * @note   ������ʽ���� ��������
 * @code
 *      //����UART0ģ�������
 *      uint8_t data; //����������洢���յ�����
 *      UART_ReadByte(HW_UART0, &data);
 * @endcode
 * @param  instance      :оƬ���ڶ˿�
 *         @arg HW_UART0 :оƬ��UART0�˿�
 *         @arg HW_UART1 :оƬ��UART1�˿�
 *         @arg HW_UART2 :оƬ��UART2�˿�
 *         @arg HW_UART3 :оƬ��UART3�˿�
 *         @arg HW_UART4 :оƬ��UART4�˿�
 *         @arg HW_UART5 :оƬ��UART5�˿�
 * @param  ch: ���յ�������ָ��
 * @retval 0:�ɹ����յ�����  ��0:û�н��յ�����
 */
uint8_t UART_ReadByte(uint32_t instance, uint16_t *ch)
{
	/* param check */
    assert_param(IS_UART_ALL_INSTANCE(instance));
    uint8_t temp = 0;
    if((UART_InstanceTable[instance]->S1 & UART_S1_RDRF_MASK) != 0)
    {
        /* get ninth bit */
        temp = (UART_InstanceTable[instance]->C3 & UART_C3_R8_MASK) >> UART_C3_R8_SHIFT;
        *ch = temp << 8;
        *ch |= (uint8_t)(UART_InstanceTable[instance]->D);	
        return 0; 		  
    }
    return 1;
}

/**
 * @brief  ����UARTģ����жϻ�DMA����
 * @code
 *      //����UART0ģ�鿪�������жϹ���
 *      UART_ITDMAConfig(HW_UART0, kUART_IT_Rx);
 * @endcode
 * @param  instance      :оƬ���ڶ˿�
 *         @arg HW_UART0 :оƬ��UART0�˿�
 *         @arg HW_UART1 :оƬ��UART1�˿�
 *         @arg HW_UART2 :оƬ��UART2�˿�
 *         @arg HW_UART3 :оƬ��UART3�˿�
 *         @arg HW_UART4 :оƬ��UART4�˿�
 *         @arg HW_UART5 :оƬ��UART5�˿�
 * @param  config: ����ģʽѡ��
 *         @arg kUART_IT_Tx_Disable    :�رշ����ж�
 *         @arg kUART_IT_Rx_Disable    :�رս����ж�
 *         @arg kUART_DMA_Tx_Disable   :�ر�DMA�����ж�
 *         @arg kUART_DMA_Rx_Disable   :�ر�DMA�����ж�
 *         @arg kUART_IT_Tx:
 *         @arg kUART_DMA_Tx:
 *         @arg kUART_IT_Rx:
 *         @arg kUART_DMA_Rx:
 * @retval None
 */
void UART_ITDMAConfig(uint32_t instance, UART_ITDMAConfig_Type config)
{
    /* enable clock gate */
    *((uint32_t*) SIM_UARTClockGateTable[instance].addr) |= SIM_UARTClockGateTable[instance].mask;
    switch(config)
    {
        case kUART_IT_Tx_Disable:
            UART_InstanceTable[instance]->C2 &= ~UART_C2_TIE_MASK;
            break;
        case kUART_IT_Rx_Disable:
            UART_InstanceTable[instance]->C2 &= ~UART_C2_RIE_MASK;
            break;
        case kUART_DMA_Tx_Disable:
            UART_InstanceTable[instance]->C5 &= ~UART_C5_TDMAS_MASK;
            UART_InstanceTable[instance]->C2 &= ~UART_C2_TIE_MASK;
            break;
        case kUART_DMA_Rx_Disable:
            UART_InstanceTable[instance]->C5 &= ~UART_C5_RDMAS_MASK;
            UART_InstanceTable[instance]->C2 &= ~UART_C2_RIE_MASK;
            break;
        case kUART_IT_Tx:
            UART_InstanceTable[instance]->C2 |= UART_C2_TIE_MASK;
            NVIC_EnableIRQ(UART_IRQnTable[instance]);
            break; 
        case kUART_IT_Rx:
            UART_InstanceTable[instance]->C2 |= UART_C2_RIE_MASK;
            NVIC_EnableIRQ(UART_IRQnTable[instance]);
            break;
        case kUART_DMA_Tx:
            UART_InstanceTable[instance]->C2 |= UART_C2_TIE_MASK;
            UART_InstanceTable[instance]->C5 |= UART_C5_TDMAS_MASK;
            break;
        case kUART_DMA_Rx:
            UART_InstanceTable[instance]->C2 |= UART_C2_RIE_MASK;
            UART_InstanceTable[instance]->C5 |= UART_C5_RDMAS_MASK;
            break;
        default:
            break;
    }
}

/**
 * @brief  ע�ᷢ���жϻص�����
 * @param  instance      :оƬ���ڶ˿�
 *         @arg HW_UART0 :оƬ��UART0�˿�
 *         @arg HW_UART1 :оƬ��UART1�˿�
 *         @arg HW_UART2 :оƬ��UART2�˿�
 *         @arg HW_UART3 :оƬ��UART3�˿�
 *         @arg HW_UART4 :оƬ��UART4�˿�
 *         @arg HW_UART5 :оƬ��UART5�˿�
 * @param AppCBFun: �ص�����ָ�����
 * @retval None
 * @note ���ڴ˺����ľ���Ӧ�������Ӧ��ʵ��
 */
void UART_CallbackTxInstall(uint32_t instance, UART_CallBackTxType AppCBFun)
{
	/* param check */
    assert_param(IS_UART_ALL_INSTANCE(instance));
    /* enable clock gate */
    *((uint32_t*) SIM_UARTClockGateTable[instance].addr) |= SIM_UARTClockGateTable[instance].mask;
    if(AppCBFun != NULL)
    {
        UART_CallBackTxTable[instance] = AppCBFun;
    }
}

/**
 * @brief  ע������жϻص�����
 * @param  instance      :оƬ���ڶ˿�
 *         @arg HW_UART0 :оƬ��UART0�˿�
 *         @arg HW_UART1 :оƬ��UART1�˿�
 *         @arg HW_UART2 :оƬ��UART2�˿�
 *         @arg HW_UART3 :оƬ��UART3�˿�
 *         @arg HW_UART4 :оƬ��UART4�˿�
 *         @arg HW_UART5 :оƬ��UART5�˿�
 * @param AppCBFun: �ص�����ָ�����
 * @retval None
 * @note ���ڴ˺����ľ���Ӧ�������Ӧ��ʵ��
 */
void UART_CallbackRxInstall(uint32_t instance, UART_CallBackRxType AppCBFun)
{
	/* param check */
    assert_param(IS_UART_ALL_INSTANCE(instance));
    /* enable clock gate */
    *((uint32_t*) SIM_UARTClockGateTable[instance].addr) |= SIM_UARTClockGateTable[instance].mask;
    if(AppCBFun != NULL)
    {
        UART_CallBackRxTable[instance] = AppCBFun;
    }
}

 /**
 * @brief  ���ڿ��ٻ����ú���
 * @code
 *      // ��ʼ�� UART4 ����: 115200-N-8-N-1, Tx:PC15 Rx:PC14
 *      GPIO_QuickInit(UART4_RX_PC14_TX_PC15, 115200);
 * @endcode
 * @param  MAP  : ����������������ͼ
 *         ���� UART1_RX_PE01_TX_PE00 ��ʹ�ô���1��PTE1/PTE0����
 * @param  baudrate: ������ 9600 115200...
 * @retval UARTģ���
 */
uint8_t UART_QuickInit(uint32_t MAP, uint32_t baudrate)
{
    uint8_t i;
    UART_InitTypeDef UART_InitStruct1;
    QuickInit_Type * pq = (QuickInit_Type*)&(MAP);
    UART_InitStruct1.baudrate = baudrate;
    UART_InitStruct1.instance = pq->ip_instance;
    UART_InitStruct1.parityMode = kUART_ParityDisabled;
    UART_InitStruct1.bitPerChar = kUART_8BitsPerChar;
    /* init pinmux */
    for(i = 0; i < pq->io_offset; i++)
    {
        PORT_PinMuxConfig(pq->io_instance, pq->io_base + i, (PORT_PinMux_Type) pq->mux); 
    }
    /* init UART */
    UART_Init(&UART_InitStruct1);
    return pq->ip_instance;
}

//! @}

//! @}

/**
 * @brief  �жϴ��������
 * @param  UART0_RX_TX_IRQHandler :оƬ��UART0�˿��жϺ������
 *         UART1_RX_TX_IRQHandler :оƬ��UART1�˿��жϺ������
 *         UART2_RX_TX_IRQHandler :оƬ��UART2�˿��жϺ������
 *         UART3_RX_TX_IRQHandler :оƬ��UART3�˿��жϺ������
 *         UART4_RX_TX_IRQHandler :оƬ��UART4�˿��жϺ������
 *         UART5_RX_TX_IRQHandler :оƬ��UART5�˿��жϺ������
 * @note �����ڲ������ж��¼�����
 */
void UART0_RX_TX_IRQHandler(void)
{
    uint16_t ch;
    /* Tx */
    if((UART_InstanceTable[HW_UART0]->S1 & UART_S1_TDRE_MASK) && (UART_InstanceTable[HW_UART0]->C2 & UART_C2_TIE_MASK))
    {
        if(UART_CallBackTxTable[HW_UART0])
        {
            UART_CallBackTxTable[HW_UART0](&ch);
        }
        UART_InstanceTable[HW_UART0]->D = (uint8_t)ch;
    }
    /* Rx */
    if((UART_InstanceTable[HW_UART0]->S1 & UART_S1_RDRF_MASK) && (UART_InstanceTable[HW_UART0]->C2 & UART_C2_RIE_MASK))
    {
        ch = (uint8_t)UART_InstanceTable[HW_UART0]->D;
        if(UART_CallBackRxTable[HW_UART0])
        {
            UART_CallBackRxTable[HW_UART0](ch);
        }    
    }
}

void UART1_RX_TX_IRQHandler(void)
{
    uint16_t ch;
    /* Tx */
    if((UART_InstanceTable[HW_UART1]->S1 & UART_S1_TDRE_MASK) && (UART_InstanceTable[HW_UART1]->C2 & UART_C2_TIE_MASK))
    {
        if(UART_CallBackTxTable[HW_UART1])
        {
            UART_CallBackTxTable[HW_UART1](&ch);
        }
        UART_InstanceTable[HW_UART1]->D = (uint8_t)ch;
    }
    /* Rx */
    if((UART_InstanceTable[HW_UART1]->S1 & UART_S1_RDRF_MASK) && (UART_InstanceTable[HW_UART1]->C2 & UART_C2_RIE_MASK))
    {
        ch = (uint8_t)UART_InstanceTable[HW_UART1]->D;
        if(UART_CallBackRxTable[HW_UART1])
        {
            UART_CallBackRxTable[HW_UART1](ch);
        }    
    }
}

#if (!defined(MK10D5))
void UART2_RX_TX_IRQHandler(void)
{
    uint16_t ch;
    /* Tx */
    if((UART_InstanceTable[HW_UART2]->S1 & UART_S1_TDRE_MASK) && (UART_InstanceTable[HW_UART2]->C2 & UART_C2_TIE_MASK))
    {
        if(UART_CallBackTxTable[HW_UART2])
        {
            UART_CallBackTxTable[HW_UART2](&ch);
        }
        UART_InstanceTable[HW_UART2]->D = (uint8_t)ch;
    }
    /* Rx */
    if((UART_InstanceTable[HW_UART2]->S1 & UART_S1_RDRF_MASK) && (UART_InstanceTable[HW_UART2]->C2 & UART_C2_RIE_MASK))
    {
        ch = (uint8_t)UART_InstanceTable[HW_UART2]->D;
        if(UART_CallBackRxTable[HW_UART2])
        {
            UART_CallBackRxTable[HW_UART2](ch);
        }    
    }
}

void UART3_RX_TX_IRQHandler(void)
{
    uint16_t ch;
    /* Tx */
    if((UART_InstanceTable[HW_UART3]->S1 & UART_S1_TDRE_MASK) && (UART_InstanceTable[HW_UART3]->C2 & UART_C2_TIE_MASK))
    {
        if(UART_CallBackTxTable[HW_UART3])
        {
            UART_CallBackTxTable[HW_UART3](&ch);
        }
        UART_InstanceTable[HW_UART3]->D = (uint8_t)ch;
    }
    /* Rx */
    if((UART_InstanceTable[HW_UART3]->S1 & UART_S1_RDRF_MASK) && (UART_InstanceTable[HW_UART3]->C2 & UART_C2_RIE_MASK))
    {
        ch = (uint8_t)UART_InstanceTable[HW_UART3]->D;
        if(UART_CallBackRxTable[HW_UART3])
        {
            UART_CallBackRxTable[HW_UART3](ch);
        }    
    }
}

void UART4_RX_TX_IRQHandler(void)
{
    uint16_t ch;
    /* Tx */
    if((UART_InstanceTable[HW_UART4]->S1 & UART_S1_TDRE_MASK) && (UART_InstanceTable[HW_UART4]->C2 & UART_C2_TIE_MASK))
    {
        if(UART_CallBackTxTable[HW_UART4])
        {
            UART_CallBackTxTable[HW_UART4](&ch);
        }
        UART_InstanceTable[HW_UART4]->D = (uint8_t)ch;
    }
    /* Rx */
    if((UART_InstanceTable[HW_UART4]->S1 & UART_S1_RDRF_MASK) && (UART_InstanceTable[HW_UART4]->C2 & UART_C2_RIE_MASK))
    {
        ch = (uint8_t)UART_InstanceTable[HW_UART4]->D;
        if(UART_CallBackRxTable[HW_UART4])
        {
            UART_CallBackRxTable[HW_UART4](ch);
        }    
    }
}

#if (defined(MK70F12)|| defined(MK70F15))
void UART5_RX_TX_IRQHandler(void)
{
    uint16_t ch;
    /* Tx */
    if((UART_InstanceTable[HW_UART5]->S1 & UART_S1_TDRE_MASK) && (UART_InstanceTable[HW_UART5]->C2 & UART_C2_TIE_MASK))
    {
        if(UART_CallBackTxTable[HW_UART5])
        {
            UART_CallBackTxTable[HW_UART5](&ch);
        }
        UART_InstanceTable[HW_UART5]->D = (uint8_t)ch;
    }
    /* Rx */
    if((UART_InstanceTable[HW_UART5]->S1 & UART_S1_RDRF_MASK) && (UART_InstanceTable[HW_UART5]->C2 & UART_C2_RIE_MASK))
    {
        ch = (uint8_t)UART_InstanceTable[HW_UART5]->D;
        if(UART_CallBackRxTable[HW_UART5])
        {
            UART_CallBackRxTable[HW_UART5](ch);
        }    
    }
}
#endif /* (defined(MK70F12)|| defined(MK70F15)) */

#endif /* (!defined(MK10D5)) */


#if 0
static const QuickInit_Type UART_QuickInitTable[] =
{
    { 1, 4, 3, 0, 2, 0}, //UART1_RX_PE01_TX_PE00
    { 0, 5, 4,18, 2, 0}, //UART0_RX_PF17_TX_PF18 4
    { 3, 4, 3, 4, 2, 0}, //UART3_RX_PE05_TX_PE04 3
    { 5, 5, 4,19, 2, 0}, //UART5_RX_PF19_TX_PF20 4
    { 5, 4, 3, 8, 2, 0}, //UART5_RX_PE09_TX_PE08 3
    { 2, 4, 3,16, 2, 0}, //UART2_RX_PE17_TX_PE16 3
    { 4, 4, 3,24, 2, 0}, //UART4_RX_PE25_TX_PE24 3
    { 0, 0, 2, 1, 2, 0}, //UART0_RX_PA01_TX_PA02 2
    { 0, 0, 3,14, 2, 0}, //UART0_RX_PA15_TX_PA14 3
    { 3, 1, 3,10, 2, 0}, //UART3_RX_PB10_TX_PB11 3
    { 0, 1, 3,16, 2,  0}, //UART0_RX_PB16_TX_PB17 3
    { 1, 2, 3, 3, 2, 0}, //UART1_RX_PC03_TX_PC04 3
    { 4, 2, 3,14, 2, 0}, //UART4_RX_PC14_TX_PC15 3
    { 3, 2, 3,16, 2, 0}, //UART3_RX_PC16_TX_PC17 3
    { 2, 3, 3, 2, 2, 0}, //UART2_RX_PD02_TX_PD03 3
    { 0, 3, 3, 6, 2, 0}, //UART0_RX_PD06_TX_PD07 3
    { 2, 5, 4,13, 2, 0}, //UART2_RX_PF13_TX_PF14 4
    { 5, 3, 3, 8, 2, 0}, //UART5_RX_PD08_TX_PD09 3
};
#endif



