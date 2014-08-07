/**
  ******************************************************************************
  * @file    spi.c
  * @author  YANDLD
  * @version V2.5
  * @date    2014.3.26
  * @brief   www.beyondcore.net   http://upcmcu.taobao.com 
  * @note    ���ļ�ΪоƬSPIģ��ĵײ㹦�ܺ���
  ******************************************************************************
  */
#include "spi.h"
#include "gpio.h"
#include "common.h"


#if (!defined(SPI_BASES))

    #if     (defined(MK60DZ10))
    #define SPI_BASES {SPI0, SPI1, SPI2}
    #elif   (defined(MK10D5))
    #define SPI_BASES {SPI0}
    #endif

#endif

SPI_Type * const SPI_InstanceTable[] = SPI_BASES;
static SPI_CallBackType SPI_CallBackTable[ARRAY_SIZE(SPI_InstanceTable)] = {NULL};
    
#if (defined(MK60DZ10) || defined(MK40D10) || defined(MK60D10)|| defined(MK10D10) || defined(MK70F12) || defined(MK70F15))
static const struct reg_ops SIM_SPIClockGateTable[] =
{
    {(void*)&(SIM->SCGC6), SIM_SCGC6_SPI0_MASK},
    {(void*)&(SIM->SCGC6), SIM_SCGC6_SPI1_MASK},
    {(void*)&(SIM->SCGC3), SIM_SCGC3_SPI2_MASK},
};
static const IRQn_Type SPI_IRQnTable[] = 
{
    SPI0_IRQn,    SPI1_IRQn,
    SPI2_IRQn,
};

#elif (defined(MK10D5))
static const struct reg_ops SIM_SPIClockGateTable[] =
{
    {(void*)&(SIM->SCGC6), SIM_SCGC6_SPI0_MASK},
};
static const IRQn_Type SPI_IRQnTable[] = 
{
    SPI0_IRQn,
};
#endif


/* Defines constant value arrays for the baud rate pre-scalar and scalar divider values.*/
static const uint32_t s_baudratePrescaler[] = { 2, 3, 5, 7 };
static const uint32_t s_baudrateScaler[] = { 2, 4, 6, 8, 16, 32, 64, 128, 256, 512, 1024, 2048,
                                           4096, 8192, 16384, 32768 };
/*FUNCTION**********************************************************************
 *
 * Function Name : dspi_hal_set_baud
 * Description   : Set the DSPI baud rate in bits per second.
 * This function will take in the desired bitsPerSec (baud rate) and will calculate the nearest
 * possible baud rate without exceeding the desired baud rate, and will return the calculated
 * baud rate in bits-per-second. It requires that the caller also provide the frequency of the
 * module source clock (in Hz).
 *
 *END**************************************************************************/
static uint32_t dspi_hal_set_baud(uint32_t instance, uint8_t whichCtar, uint32_t bitsPerSec, uint32_t sourceClockInHz)                    
{
    uint32_t prescaler, bestPrescaler;
    uint32_t scaler, bestScaler;
    uint32_t dbr, bestDbr;
    uint32_t realBaudrate, bestBaudrate;
    uint32_t diff, min_diff;
    uint32_t baudrate = bitsPerSec;
    /* for master mode configuration, if slave mode detected, return 0*/
    if(!(SPI_InstanceTable[instance]->MCR & SPI_MCR_MSTR_MASK))
    {
        return 0;
    }
    /* find combination of prescaler and scaler resulting in baudrate closest to the */
    /* requested value */
    min_diff = 0xFFFFFFFFU;
    bestPrescaler = 0;
    bestScaler = 0;
    bestDbr = 1;
    bestBaudrate = 0; /* required to avoid compilation warning */

    /* In all for loops, if min_diff = 0, the exit for loop*/
    for (prescaler = 0; (prescaler < 4) && min_diff; prescaler++)
    {
        for (scaler = 0; (scaler < 16) && min_diff; scaler++)
        {
            for (dbr = 1; (dbr < 3) && min_diff; dbr++)
            {
                realBaudrate = ((sourceClockInHz * dbr) /
                                (s_baudratePrescaler[prescaler] * (s_baudrateScaler[scaler])));

                /* calculate the baud rate difference based on the conditional statement*/
                /* that states that the calculated baud rate must not exceed the desired baud rate*/
                if (baudrate >= realBaudrate)
                {
                    diff = baudrate-realBaudrate;
                    if (min_diff > diff)
                    {
                        /* a better match found */
                        min_diff = diff;
                        bestPrescaler = prescaler;
                        bestScaler = scaler;
                        bestBaudrate = realBaudrate;
                        bestDbr = dbr;
                    }
                }
            }
        }
    }

    uint32_t temp;
    /* write the best dbr, prescalar, and baud rate scalar to the CTAR*/
    temp = SPI_InstanceTable[instance]->CTAR[whichCtar];
    temp &= ~(SPI_CTAR_DBR_MASK| SPI_CTAR_PBR_MASK | SPI_CTAR_BR_MASK);
    if((bestDbr-1))
    {
        temp |= SPI_CTAR_DBR_MASK|SPI_CTAR_PBR(bestPrescaler)|SPI_CTAR_BR(bestScaler);
    }
    else
    {
        temp |= SPI_CTAR_PBR(bestPrescaler)|SPI_CTAR_BR(bestScaler);
    }
    SPI_InstanceTable[instance]->CTAR[whichCtar] = temp;
    /* return the actual calculated baud rate*/
    return bestBaudrate;
}

/**
 * @brief  ��ʼ��SPIģ��
 * @note ��Ҫ�����������ʹ��
 * @param  SPI_InitStruct :SPI��ʼ�����ýṹ��
 * @retval None
 */
void SPI_Init(SPI_InitTypeDef * SPI_InitStruct)
{

    /* enable clock gate */
    *(uint32_t*)SIM_SPIClockGateTable[SPI_InitStruct->instance].addr |= SIM_SPIClockGateTable[SPI_InitStruct->instance].mask;
    /* let all PCS low when in inactive mode */
    /* stop SPI */
    SPI_InstanceTable[SPI_InitStruct->instance]->MCR |= SPI_MCR_HALT_MASK;
    /* master or slave */
    switch(SPI_InitStruct->mode)
    {
        case kSPI_Master:
            SPI_InstanceTable[SPI_InitStruct->instance]->MCR |= SPI_MCR_MSTR_MASK;
            break;
        case kSPI_Slave:
            SPI_InstanceTable[SPI_InitStruct->instance]->MCR &= ~SPI_MCR_MSTR_MASK;
            break;
        default:
            break;
    }
    /* enable SPI clock */
    SPI_InstanceTable[SPI_InitStruct->instance]->MCR &= ~SPI_MCR_MDIS_MASK;
    /* disable FIFO and clear FIFO flag */
    SPI_InstanceTable[SPI_InitStruct->instance]->MCR |= 
        SPI_MCR_PCSIS_MASK |
        SPI_MCR_HALT_MASK |
        SPI_MCR_CLR_TXF_MASK|
        SPI_MCR_CLR_RXF_MASK|
        SPI_MCR_DIS_TXF_MASK|
        SPI_MCR_DIS_RXF_MASK;
    /* config frame format */
    SPI_FrameConfig(SPI_InitStruct->instance, SPI_InitStruct->ctar, SPI_InitStruct->frameFormat, SPI_InitStruct->dataSize, SPI_InitStruct->bitOrder, SPI_InitStruct->baudrate);
    /* clear all flags */
    SPI_InstanceTable[SPI_InitStruct->instance]->SR = SPI_SR_EOQF_MASK   
            | SPI_SR_TFUF_MASK    
            | SPI_SR_TFFF_MASK 
            | SPI_SR_RFOF_MASK 
            | SPI_SR_RFDF_MASK
            | SPI_SR_TCF_MASK;
    /* launch */
    SPI_InstanceTable[SPI_InitStruct->instance]->MCR &= ~SPI_MCR_HALT_MASK;
}
 
/**
 * @brief  SPI����֡����
 * @note   �ڲ��������û��������
 * @retval None
 */
void SPI_FrameConfig(uint32_t instance, uint32_t ctar, SPI_FrameFormat_Type frameFormat, uint8_t dataSize, uint8_t bitOrder, uint32_t baudrate)
{
    uint32_t clock;
    /* data size */
    SPI_InstanceTable[instance]->CTAR[ctar] &= ~SPI_CTAR_FMSZ_MASK;
    SPI_InstanceTable[instance]->CTAR[ctar] |= SPI_CTAR_FMSZ(dataSize-1);
    /* bit order */
    switch(bitOrder)
    {
        case kSPI_MSBFirst:
            SPI_InstanceTable[instance]->CTAR[ctar] &= ~SPI_CTAR_LSBFE_MASK;
            break;
        case kSPI_LSBFirst:
            SPI_InstanceTable[instance]->CTAR[ctar] |= SPI_CTAR_LSBFE_MASK;
            break;
        default:
            break;
    }
    /* frame format */
    switch(frameFormat)
    {
        case kSPI_CPOL0_CPHA0:
            SPI_InstanceTable[instance]->CTAR[ctar] &= ~SPI_CTAR_CPOL_MASK;
            SPI_InstanceTable[instance]->CTAR[ctar] &= ~SPI_CTAR_CPHA_MASK;
            break;
        case kSPI_CPOL0_CPHA1:
            SPI_InstanceTable[instance]->CTAR[ctar] &= ~SPI_CTAR_CPOL_MASK;
            SPI_InstanceTable[instance]->CTAR[ctar] |= SPI_CTAR_CPHA_MASK;
            break;   
        case kSPI_CPOL1_CPHA0:
            SPI_InstanceTable[instance]->CTAR[ctar] |= SPI_CTAR_CPOL_MASK;
            SPI_InstanceTable[instance]->CTAR[ctar] &= ~SPI_CTAR_CPHA_MASK;
            break;  
        case kSPI_CPOL1_CPHA1:
            SPI_InstanceTable[instance]->CTAR[ctar] |= SPI_CTAR_CPOL_MASK;
            SPI_InstanceTable[instance]->CTAR[ctar] |= SPI_CTAR_CPHA_MASK;  
            break;  
        default:
            break;
    }
    /* set SPI clock, SPI use Busclock */
    CLOCK_GetClockFrequency(kBusClock, &clock);
    dspi_hal_set_baud(instance, ctar, baudrate, clock);
}

/**
 * @brief  ���ٳ�ʼ��SPIģ��
 * @code
 *     //ʹ��SPI��1ģ��SCK-PE02 SOUT-PE01 SIN-PE03 ͨ���ٶ�Ϊ48000hz ���Ժ���λ����0 
 *     SPI_QuickInit(SPI1_SCK_PE02_SOUT_PE01_SIN_PE03, kSPI_CPOL0_CPHA0, 48000);
 * @endcode
 * @param  MAP :SPIͨ�ſ�����������Ԥ���壬���spi.h�ļ�
 * @param  frameFormat: SPIͨ��ʱ����λ�ͼ��ԵĹ�ϵ
 *         @arg kSPI_CPOL0_CPHA0
 *         @arg kSPI_CPOL1_CPHA0
 *         @arg kSPI_CPOL0_CPHA1
 *         @arg kSPI_CPOL1_CPHA1
 * @param  baudrate :SPIͨ���ٶ�����
 * @retval None
 */
uint32_t SPI_QuickInit(uint32_t MAP, SPI_FrameFormat_Type frameFormat, uint32_t baudrate)
{
    uint32_t i;
    QuickInit_Type * pq = (QuickInit_Type*)&(MAP);
    SPI_InitTypeDef SPI_InitStruct1;
    SPI_InitStruct1.baudrate = baudrate;
    SPI_InitStruct1.frameFormat = (SPI_FrameFormat_Type)frameFormat;
    SPI_InitStruct1.dataSize = 8;
    SPI_InitStruct1.instance = pq->ip_instance;
    SPI_InitStruct1.mode = kSPI_Master;
    SPI_InitStruct1.bitOrder = kSPI_MSBFirst;
    SPI_InitStruct1.ctar = HW_CTAR0;
    /* init pinmux */
    for(i = 0; i < pq->io_offset; i++)
    {
        PORT_PinMuxConfig(pq->io_instance, pq->io_base + i, (PORT_PinMux_Type) pq->mux); 
    }
    /* init moudle */
    SPI_Init(&SPI_InitStruct1);
    return pq->ip_instance;
}

/**
 * @brief  SPIģ�� �жϺ�DMA��������
 * @code
 *     //ʹ��SPI��1ģ�鷢������ж�
 *     SPI_ITDMAConfig(HW_SPI1, kSPI_IT_TCF);
 * @endcode
 * @param  instance :SPIͨ��ģ��� HW_SPI0~2
 * @param  SPI_ITDMAConfig_Type: SPI�ж�����
 *         @arg kSPI_IT_TCF_Disable  :�ر��ж�
 *         @arg kSPI_IT_TCF          :������������ж�
 * @param  baudrate :SPIͨ���ٶ�����
 * @retval None
 */
void SPI_ITDMAConfig(uint32_t instance, SPI_ITDMAConfig_Type config)
{
    switch(config)
    {
        case kSPI_IT_TCF_Disable: 
            SPI_InstanceTable[instance]->RSER &= ~SPI_RSER_TCF_RE_MASK;
            NVIC_DisableIRQ(SPI_IRQnTable[instance]);
            break;
        case kSPI_IT_TCF: 
            NVIC_EnableIRQ(SPI_IRQnTable[instance]);
            SPI_InstanceTable[instance]->RSER |= SPI_RSER_TCF_RE_MASK;
            break;
        default:
            break;
    }
}

/**
 * @brief  ע���жϻص�����
 * @param  instance :SPIͨ��ģ��� HW_SPI0~2
 * @param  AppCBFun: �ص�����ָ�����
 * @retval None
 * @note ���ڴ˺����ľ���Ӧ�������Ӧ��ʵ��
 */
void SPI_CallbackInstall(uint32_t instance, SPI_CallBackType AppCBFun)
{
    if(AppCBFun != NULL)
    {
        SPI_CallBackTable[instance] = AppCBFun;
    }
}

/**
 * @brief  SPI��дһ�ֽ�����
 * @code
 *     //ʹ��SPI��1ģ���1Ƭѡ�ź�дһ�ֽڵ�����0x55��Ƭѡ�ź����Ϊѡ��״̬
 *    SPI_ReadWriteByte(HW_SPI1, HW_CTAR0, 0x55, 1, kSPI_PCS_ReturnInactive);
 * @endcode
 * @param  instance :SPIͨ��ģ��� HW_SPI0~2
 * @param  ctar :SPIͨ��ͨ��ѡ��
 *          @arg HW_CTAR0  :0ͨ��
 *          @arg HW_CTAR1  :1ͨ��
 * @param  data    : Ҫ���͵�һ�ֽ�����
 * @param  CSn     : Ƭѡ�źŶ˿�ѡ��
 * @param  csState : Ƭѡ�ź�����״̬
 *          @arg kSPI_PCS_ReturnInactive  :�����ѡ��״̬
 *          @arg kSPI_PCS_KeepAsserted    :��󱣳�δѡ��״̬
 * @retval ��ȡ��������
 */
uint16_t SPI_ReadWriteByte(uint32_t instance,uint32_t ctar, uint16_t data, uint16_t CSn, uint16_t csState)
{
    uint16_t read_data;
	SPI_InstanceTable[instance]->PUSHR = (((uint32_t)(((csState))<<SPI_PUSHR_CONT_SHIFT))&SPI_PUSHR_CONT_MASK) 
            | SPI_PUSHR_CTAS(ctar)      
            | SPI_PUSHR_PCS(1<<CSn)
            | SPI_PUSHR_TXDATA(data);
    if(!(SPI_InstanceTable[instance]->RSER & SPI_RSER_TCF_RE_MASK)) // if it is polling mode
    {
        /* wait for transfer complete */
        while(!(SPI_InstanceTable[instance]->SR & SPI_SR_TCF_MASK)){};
        /* clear flag */
        SPI_InstanceTable[instance]->SR |= SPI_SR_TCF_MASK;
    }
    read_data = (uint16_t)SPI_InstanceTable[instance]->POPR;
    return read_data;
}

/**
 * @brief  �жϴ��������
 * @param  SPI0_IRQHandler :оƬ��SPI0ģ���жϺ������
 *         SPI1_IRQHandler :оƬ��SPI1ģ���жϺ������
 *         SPI2_IRQHandler :оƬ��SPI2ģ���жϺ������
 * @note �����ڲ������ж��¼�����
 */
void SPI0_IRQHandler(void)
{
    SPI_InstanceTable[HW_SPI0]->SR |= SPI_SR_TCF_MASK ;
    if(SPI_CallBackTable[HW_SPI0])
    {
        SPI_CallBackTable[HW_SPI0]();
    }
}

void SPI1_IRQHandler(void)
{
    SPI_InstanceTable[HW_SPI1]->SR |= SPI_SR_TCF_MASK ;
    if(SPI_CallBackTable[HW_SPI1])
    {
        SPI_CallBackTable[HW_SPI1]();
    }
}

void SPI2_IRQHandler(void)
{
    SPI_InstanceTable[HW_SPI2]->SR |= SPI_SR_TCF_MASK ;
    if(SPI_CallBackTable[HW_SPI2])
    {
        SPI_CallBackTable[HW_SPI2]();
    }
}


/*

static const QuickInit_Type SPI_QuickInitTable[] =
{
    { 0, 2, 2, 5, 3, 0}, //SPI0_SCK_PC05_SOUT_PC06_SIN_PC07 2
    { 0, 3, 2, 1, 3, 0}, //SPI0_SCK_PD01_SOUT_PD02_SIN_PD03 2
    { 1, 4, 2, 1, 3, 0}, //SPI1_SCK_PE02_SOUT_PE01_SIN_PE03 2
    { 0, 0, 2,15, 3, 0}, //SPI0_SCK_PA15_SOUT_PA16_SIN_PA17 2
    { 2, 1, 2,21, 3, 0}, //SPI2_SCK_PB21_SOUT_PB22_SIN_PB23 2
    { 2, 3, 2,12, 3, 0}, //SPI2_SCK_PD12_SOUT_PD13_SIN_PD14 2
};
*/
