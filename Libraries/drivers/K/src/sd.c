/**
  ******************************************************************************
  * @file    sd.c
  * @author  YANDLD
  * @version V2.5
  * @date    2014.3.24
  * @brief   www.beyondcore.net   http://upcmcu.taobao.com 
  * @note    ���ļ�ΪоƬSDģ��ĵײ㹦�ܺ���
              ֻ֧��SD��SDHC�� ��֧��MMC��
  ******************************************************************************
  */
  
#include "sd.h"
#include "gpio.h"

static uint32_t SD_SendCommand(SD_CommandTypeDef* Command);

//SD����Ϣ
struct sd_card_handler
{
    uint32_t card_type;  
    uint32_t OCR;           //Operation Condition Register �����ѹ��Ϣ�� ��ʼ�����λ ���������Ĵ��� ��Ӧ��ʵ��OCR 39:8 λ
    uint32_t CID[4];        //Card IDentification (CID) register  CID[0-4] ��Ӧ ��ʵ��CID�� 127:8
    uint32_t CSD[4];        //Card-Specific Data register CSD ��¼����������Ҫ��Ϣ CSD[0-4] ��Ӧ ��ʵ��CSD 127:8
    uint16_t RCA;           //����Ե�ַ�Ĵ��� ��HOST�Ϳ�ͨѶ�Ļ���      
    uint32_t CSR[2];        //�����üĴ���
    uint32_t total_block_cnt;
    uint32_t block_size;
};     

static struct sd_card_handler sdh;

/**
 * @brief ����SD��ģ���ͨ���ٶ�
 * @param  baudrate  ��λHz
 * @retval None
 */                                                            
static void SD_SetBaudRate(uint32_t baudrate)
{
	uint32_t pres, div, min, minpres = 0x80, mindiv = 0x0F;
	uint32_t  val,clock;
    CLOCK_GetClockFrequency(kBusClock, &clock);
	//�ҵ�����ķ�Ƶ����
	min = (uint32_t)-1;
	for (pres = 2; pres <= 256; pres <<= 1) 
	{
		for (div = 1; div <= 16; div++) 
		{
			val =pres*div* baudrate - clock;
			if ((val > 0) || (val == 0))
			{
				if (min > val) 
				{
					min = val;
					minpres = pres;
					mindiv = div;
				}
			}
		}
   }
	/* disable SDHC */
	SDHC->SYSCTL &= (~ SDHC_SYSCTL_SDCLKEN_MASK);
	/* set prescaler */
	div = SDHC->SYSCTL & (~ (SDHC_SYSCTL_DTOCV_MASK | SDHC_SYSCTL_SDCLKFS_MASK | SDHC_SYSCTL_DVS_MASK));
	SDHC->SYSCTL = div | (SDHC_SYSCTL_DTOCV(0x0E) | SDHC_SYSCTL_SDCLKFS(minpres >> 1) | SDHC_SYSCTL_DVS(mindiv - 1));
	/* waitting for stabile */
	while (0 == (SDHC->PRSSTAT & SDHC_PRSSTAT_SDSTB_MASK));
	/* enable module */
	SDHC->SYSCTL |= SDHC_SYSCTL_SDCLKEN_MASK;
	SDHC->IRQSTAT |= SDHC_IRQSTAT_DTOE_MASK;
} 

/**
 * @brief ����ڴ濨������״̬
 * @note  �ڲ��������û��������
 * @retval 0:δ���У�1:����
 */      
uint8_t SDHC_is_running(void)
{
    return (0 != (SDHC->PRSSTAT & (SDHC_PRSSTAT_RTA_MASK | SDHC_PRSSTAT_WTA_MASK | SDHC_PRSSTAT_DLA_MASK | SDHC_PRSSTAT_CDIHB_MASK | SDHC_PRSSTAT_CIHB_MASK)));
}   

/**
 * @brief SDģ����ٳ�ʼ������
 * @param  baudrate  :ͨ�Ų�����
 * @retval 0:����  ����:δ��ɳ�ʼ��
 */       
uint32_t SD_QuickInit(uint32_t baudrate)
{
    uint32_t retry = 0;
    SD_InitTypeDef SD_InitStruct1;
    SD_InitStruct1.baudrate = baudrate;
    /* init pinmux */
    PORT_PinMuxConfig(HW_GPIOE, 0, kPinAlt4); /* ESDHC.D1  */
    PORT_PinMuxConfig(HW_GPIOE, 1, kPinAlt4); /* ESDHC.D0  */
    PORT_PinMuxConfig(HW_GPIOE, 2, kPinAlt4); /* ESDHC.CLK */
    PORT_PinMuxConfig(HW_GPIOE, 3, kPinAlt4); /* ESDHC.CMD */
    PORT_PinMuxConfig(HW_GPIOE, 4, kPinAlt4); /* ESDHC.D3  */
    PORT_PinMuxConfig(HW_GPIOE, 5, kPinAlt4); /* ESDHC.D2  */
    PORT_PinPullConfig(HW_GPIOE, 0, kPullUp);
    PORT_PinPullConfig(HW_GPIOE, 1, kPullUp);
    PORT_PinPullConfig(HW_GPIOE, 3, kPullUp);
    PORT_PinPullConfig(HW_GPIOE, 4, kPullUp);
    PORT_PinPullConfig(HW_GPIOE, 5, kPullUp);
    #if 0
    PORTE->PCR[0] =  (PORT_PCR_MUX(4) | PORT_PCR_PS_MASK | PORT_PCR_PE_MASK | PORT_PCR_DSE_MASK);    /* ESDHC.D1  */
    PORTE->PCR[1] =  (PORT_PCR_MUX(4) | PORT_PCR_PS_MASK | PORT_PCR_PE_MASK | PORT_PCR_DSE_MASK);    /* ESDHC.D0  */
    PORTE->PCR[2] =  (PORT_PCR_MUX(4) | PORT_PCR_DSE_MASK);                                          /* ESDHC.CLK */
    PORTE->PCR[3] =  (PORT_PCR_MUX(4) | PORT_PCR_PS_MASK | PORT_PCR_PE_MASK | PORT_PCR_DSE_MASK);    /* ESDHC.CMD */
    PORTE->PCR[4] =  (PORT_PCR_MUX(4) | PORT_PCR_PS_MASK | PORT_PCR_PE_MASK | PORT_PCR_DSE_MASK);    /* ESDHC.D3  */
    PORTE->PCR[5] =  (PORT_PCR_MUX(4) | PORT_PCR_PS_MASK | PORT_PCR_PE_MASK | PORT_PCR_DSE_MASK);    /* ESDHC.D2  */
    #endif
    while(SD_Init(&SD_InitStruct1) && (retry < 10))
    {
        retry++;
    }
    if(retry == 10)
    {
        return retry;
    }
    return 0;
}

/**
 * @brief SDģ���ʼ�����ã�4�ߴ���
 * @note  �������SDģ��ĳ�ʼ�����ã��������������
 * @param  SD_InitStruct  :SDģ�����ýṹ��
 * @retval ESDHC_OK:����  ESDHC_ERROR_INIT_FAILED:δ��ɳ�ʼ��
 */   
uint8_t SD_Init(SD_InitTypeDef* SD_InitStruct)
{
	uint32_t delay_cnt = 0;
	uint8_t result;  //�洢�������ؽ��
	uint8_t i=0;
	uint8_t hc;     //�Ƿ�ΪSDHC��־
	SD_CommandTypeDef SD_CommandStruct1;
	SIM->SCGC3 |= SIM_SCGC3_SDHC_MASK;
	//��λSDHC ���ó�ʱʱ��
	SDHC->SYSCTL = SDHC_SYSCTL_RSTA_MASK | SDHC_SYSCTL_SDCLKFS(0x80); 
	//�ȴ���λ���
	while(SDHC->SYSCTL & SDHC_SYSCTL_RSTA_MASK){};
	//��ʼ��SDHC��ؼĴ���	
	SDHC->VENDOR = 0;
	SDHC->BLKATTR = SDHC_BLKATTR_BLKCNT(1) | SDHC_BLKATTR_BLKSIZE(512); //Ĭ�ϴ���1��Block ÿ��Block 512�ֽ�
	SDHC->PROCTL = SDHC_PROCTL_EMODE(2)| SDHC_PROCTL_D3CD_MASK;        //LSB��ʽ   ʹ��DATA3 ��⿨�����Ƴ�
	SDHC->WML = SDHC_WML_RDWML(1) | SDHC_WML_WRWML(1);                 //�����������
	//����SDHCģ���ͨ������
	SD_SetBaudRate(SD_InitStruct->baudrate);
	while (SDHC->PRSSTAT & (SDHC_PRSSTAT_CIHB_MASK | SDHC_PRSSTAT_CDIHB_MASK)) ;									
    //���SDHCģ����жϱ�־
	SDHC->IRQSTAT = 0xFFFF;
	/* enable irq status */
	SDHC->IRQSTATEN = SDHC_IRQSTATEN_DEBESEN_MASK 
                    | SDHC_IRQSTATEN_DCESEN_MASK 
                    | SDHC_IRQSTATEN_DTOESEN_MASK 
                    | SDHC_IRQSTATEN_CIESEN_MASK 
                    | SDHC_IRQSTATEN_CEBESEN_MASK 
                    | SDHC_IRQSTATEN_CCESEN_MASK 
                    | SDHC_IRQSTATEN_CTOESEN_MASK 
                    | SDHC_IRQSTATEN_BRRSEN_MASK 
                    | SDHC_IRQSTATEN_BWRSEN_MASK 
                    | SDHC_IRQSTATEN_CRMSEN_MASK
                    | SDHC_IRQSTATEN_TCSEN_MASK 
                    | SDHC_IRQSTATEN_CCSEN_MASK; 								 
	for(delay_cnt = 0; delay_cnt < 10000; delay_cnt++) {};
	//80��ʱ�����ڵĳ�ʼ��
	SDHC->SYSCTL |= SDHC_SYSCTL_INITA_MASK;
	while (SDHC->SYSCTL & SDHC_SYSCTL_INITA_MASK){}; //�ȴ���ʼ�����
	//--------------���¿�ʼSD����ʼ�� �����Э��---------------------------
	//��ʼSD����ʼ������ --------------------------------
	//˵�� �CCMD0 -> CMD8 -> while(CMD55+ACMD41) ->CMD2 -> CMD3 ->CMD9
	//            -> CMD7(ѡ�п�)-> CMD16(���ÿ��С)->(CMD55+ACMD6)����λ4��λ��
	//---------------------------��ʽ��ʼ------------------------------  now Let's begin !
	//CMD0  ʹ���п�����IDLE
	SD_CommandStruct1.COMMAND = ESDHC_CMD0;
	SD_CommandStruct1.ARGUMENT = 0;
	SD_CommandStruct1.BLOCKS = 0;
	result = SD_SendCommand(&SD_CommandStruct1);
	if(result != ESDHC_OK) return ESDHC_ERROR_INIT_FAILED;      //�����򷵻�
		
	//CMD8  �ж���V1.0����V2.0�Ŀ�
	SD_CommandStruct1.COMMAND = ESDHC_CMD8;
	SD_CommandStruct1.ARGUMENT =0x000001AA;
	SD_CommandStruct1.BLOCKS = 0;
	result = SD_SendCommand(&SD_CommandStruct1);
	if (result > 0)  //CMD8����Ӧ  ���������ͨ��
	{
		result = ESDHC_ERROR_INIT_FAILED;
	}
	if (result == 0) //SDHC ��
	{
		hc = true;  					
	}
	//��������55+ACDM41 ֱ����׼����
	do 
	{
		//��ʱ									 
		for(delay_cnt=0;delay_cnt<10000;delay_cnt++);
		i++;   
		SD_CommandStruct1.COMMAND = ESDHC_CMD55;
		SD_CommandStruct1.ARGUMENT =0;
        SD_CommandStruct1.BLOCKS = 0;
	  result = SD_SendCommand(&SD_CommandStruct1);
		
		SD_CommandStruct1.COMMAND = ESDHC_ACMD41;
		if(hc)
		{
			SD_CommandStruct1.ARGUMENT = 0x40300000;
		}
		else
		{
			SD_CommandStruct1.ARGUMENT = 0x00300000;
		}
		result = SD_SendCommand(&SD_CommandStruct1);
	}while ((0 == (SD_CommandStruct1.RESPONSE[0] & 0x80000000)) && (i < 30));	
	//CMD2 ȡCID
	SD_CommandStruct1.COMMAND = ESDHC_CMD2;
	SD_CommandStruct1.ARGUMENT = 0;
	SD_CommandStruct1.BLOCKS = 0;
	result = SD_SendCommand(&SD_CommandStruct1);
	if(result != ESDHC_OK) return ESDHC_ERROR_INIT_FAILED;    
	sdh.CID[0] = SD_CommandStruct1.RESPONSE[0];
	sdh.CID[1] = SD_CommandStruct1.RESPONSE[1];
	sdh.CID[2] = SD_CommandStruct1.RESPONSE[2];
	sdh.CID[3] = SD_CommandStruct1.RESPONSE[3];
	
	//CMD3 ȡRCA
	SD_CommandStruct1.COMMAND = ESDHC_CMD3;
	SD_CommandStruct1.ARGUMENT = 0;
	SD_CommandStruct1.BLOCKS = 0;
	result = SD_SendCommand(&SD_CommandStruct1);
	if(result != ESDHC_OK) return ESDHC_ERROR_INIT_FAILED;   
    sdh.RCA = SD_CommandStruct1.RESPONSE[0]>>16;
	//CMD9 ȡCSD
	SD_CommandStruct1.COMMAND = ESDHC_CMD9;
	SD_CommandStruct1.ARGUMENT = sdh.RCA<<16;
	SD_CommandStruct1.BLOCKS = 0;
	result = SD_SendCommand(&SD_CommandStruct1);
	if(result != ESDHC_OK) return ESDHC_ERROR_INIT_FAILED;  
	sdh.CSD[0] = SD_CommandStruct1.RESPONSE[0];
	sdh.CSD[1] = SD_CommandStruct1.RESPONSE[1];
	sdh.CSD[2] = SD_CommandStruct1.RESPONSE[2];
	sdh.CSD[3] = SD_CommandStruct1.RESPONSE[3];
	
	//CMD7 ѡ�п�
	SD_CommandStruct1.COMMAND = ESDHC_CMD7;
	SD_CommandStruct1.ARGUMENT = sdh.RCA<<16;
	SD_CommandStruct1.BLOCKS = 0;
	result = SD_SendCommand(&SD_CommandStruct1);
	if(result != ESDHC_OK) return ESDHC_ERROR_INIT_FAILED;  
	//CMD16 ���ÿ��С
	SD_CommandStruct1.COMMAND = ESDHC_CMD16;
	SD_CommandStruct1.ARGUMENT = 512;
	SD_CommandStruct1.BLOCKS = 0;
	result = SD_SendCommand(&SD_CommandStruct1);
	if(result != ESDHC_OK) return ESDHC_ERROR_INIT_FAILED;  
	
	//CMD55 ʹ��ACMD����
	SD_CommandStruct1.COMMAND = ESDHC_CMD55;
	SD_CommandStruct1.ARGUMENT = sdh.RCA<<16;
	SD_CommandStruct1.BLOCKS = 0;
	result = SD_SendCommand(&SD_CommandStruct1);
	if(result != ESDHC_OK) return ESDHC_ERROR_INIT_FAILED;  
	//ACMD6 �޸�SD��ͨѶλ��
	SD_CommandStruct1.COMMAND = ESDHC_ACMD6;
	SD_CommandStruct1.ARGUMENT = 2;
	SD_CommandStruct1.BLOCKS = 0;
	result = SD_SendCommand(&SD_CommandStruct1);//�޸�SD��λ4λͨѶλ��	
	if(result != ESDHC_OK) return ESDHC_ERROR_INIT_FAILED;  
	 //���� Kinetis�� SDIOģ��λ4��ģʽ
	SDHC->PROCTL &= (~SDHC_PROCTL_DTW_MASK);
	SDHC->PROCTL |=  SDHC_PROCTL_DTW(ESDHC_PROCTL_DTW_4BIT);
	//�жϿ�����
	if((sdh.CSD[3]>>22)&0x03)
	{
        sdh.card_type = SD_CARD_TYPE_SDHC;
	}
	else
	{
        sdh.card_type = SD_CARD_TYPE_SD;
	}
	//�����������⣬���Ƴ��ж�----δ�ɹ�
	/*
	NVIC_EnableIRQ(SDHC_IRQn);  //����CM4���ϵ��ж���
	SDHC->IRQSTATEN |= SDHC_IRQSTATEN_CINTSEN_MASK | SDHC_IRQSTATEN_CRMSEN_MASK | SDHC_IRQSTATEN_CINSEN_MASK; //����SDHC�жϿ����� ���Ƴ�
	SDHC->IRQSIGEN |=SDHC_IRQSIGEN_CINTIEN_MASK | SDHC_IRQSIGEN_CINSIEN_MASK | SDHC_IRQSIGEN_CRMIEN_MASK;
	*/
    //˳����ʼ������
	return ESDHC_OK;	
}

/**
 * @brief ��SD����һ������
 * @note  һ����������Ϊ512�ֽ�
 * @param  sector  :Ҫ��ȡ��SD��������
 * @param  buffer  :���ݴ洢��ַ
 * @retval ESDHC_OK:����  ����:��ȡ����
 */   															  
uint8_t SD_ReadSingleBlock(uint32_t sector, uint8_t *buffer)
{
    uint16_t results;
    uint32_t	j;
    uint32_t	*ptr = (uint32_t*)buffer;
	SD_CommandTypeDef SD_CommandStruct1;
	if(sdh.card_type == SD_CARD_TYPE_SD) //�������ͨSD�� �ѿ��ַת�����ֽڵ�ַ
	{
		sector = sector<<9;
	}
	 while (SDHC->PRSSTAT & SDHC_PRSSTAT_DLA_MASK){};//�ȴ�DATA�߿���
	 //SDHC->BLKATTR &= (~ SDHC_BLKATTR_BLKCNT_MASK); //�������
	 //SDHC->BLKATTR |= SDHC_BLKATTR_BLKCNT(1);
	 SD_CommandStruct1.COMMAND = ESDHC_CMD17;
	 SD_CommandStruct1.ARGUMENT = sector;
	 SD_CommandStruct1.BLOCKS = 1;
	 SD_CommandStruct1.BLOCKSIZE = 512;
	 results = SD_SendCommand(&SD_CommandStruct1);
	if(results != ESDHC_OK) return ESDHC_ERROR_DATA_TRANSFER;  
	//��ʼ��ȡһ������------------------------------
    //��ȡ����ʱ��ÿ�ζ�ȡ4���ֽ�
    for (j = (512+3)>>2;j!= 0;j--)
    {
        //������ �д������˳�
        if (SDHC->IRQSTAT & (   SDHC_IRQSTAT_DEBE_MASK //Data End Bit Error
                                  | SDHC_IRQSTAT_DCE_MASK  //Data CRC Error
                                  | SDHC_IRQSTAT_DTOE_MASK)) //DataTimeout Error
            {
                SDHC->IRQSTAT |= SDHC_IRQSTAT_DEBE_MASK 
                              | SDHC_IRQSTAT_DCE_MASK 
                              | SDHC_IRQSTAT_DTOE_MASK 
                              | SDHC_IRQSTAT_BRR_MASK; //Buffer Read Ready
                return ESDHC_ERROR_DATA_TRANSFER;
            }
						//�ȴ�����׼����	
            while (0 == (SDHC->PRSSTAT & SDHC_PRSSTAT_BREN_MASK)) {};
						*ptr = SDHC->DATPORT;	  
						ptr++;	//����ȡ�� *ptr++=SDHC->DATPORT;	 ��Ϊ�����BUG
						
    }
    return ESDHC_OK;
}

/**
 * @brief дSD����һ������
 * @note  һ����������Ϊ512�ֽ�
 * @param  sector  :Ҫд���SD��������
 * @param  buffer  :���ݴ洢��ַ
 * @retval ESDHC_OK:����  ����:��ȡ����
 */ 													  
uint8_t SD_WriteSingleBlock(uint32_t sector, const uint8_t *buffer)
{
	uint16_t results;
	uint32_t	j;
    uint32_t	*ptr = (uint32_t*)buffer;
	SD_CommandTypeDef SD_CommandStruct1;
	if(sdh.card_type == SD_CARD_TYPE_SD) //�������ͨSD�� �ѿ��ַת�����ֽڵ�ַ
	{
		sector = sector<<9;
	}
	while (SDHC->PRSSTAT & SDHC_PRSSTAT_DLA_MASK){};//�ȴ�DATA�߿���
	//SDHC->BLKATTR &= (~ SDHC_BLKATTR_BLKCNT_MASK); //�������
	//SDHC->BLKATTR |= SDHC_BLKATTR_BLKCNT(1);
    SD_CommandStruct1.COMMAND = ESDHC_CMD24;
    SD_CommandStruct1.ARGUMENT = sector;
    SD_CommandStruct1.BLOCKS = 1;
    SD_CommandStruct1.BLOCKSIZE = 512;
    results = SD_SendCommand(&SD_CommandStruct1);

    if(results != ESDHC_OK) return ESDHC_ERROR_DATA_TRANSFER;  

	//��ʼд��һ������------------------------------
		
    //��ȡ����ʱ��ÿ�ζ�ȡ4���ֽ�
        for (j = (512)>>2;j!= 0;j--)
        {
            if (SDHC->IRQSTAT & (  SDHC_IRQSTAT_DEBE_MASK //Data End Bit Error
                                  | SDHC_IRQSTAT_DCE_MASK  //Data CRC Error
                                  | SDHC_IRQSTAT_DTOE_MASK)) //DataTimeout Error
					
            {
                SDHC->IRQSTAT |= SDHC_IRQSTAT_DEBE_MASK 
                              | SDHC_IRQSTAT_DCE_MASK 
                              | SDHC_IRQSTAT_DTOE_MASK 
                              | SDHC_IRQSTAT_BWR_MASK; //Buffer Write Ready
                return ESDHC_ERROR_DATA_TRANSFER;
            }
            while (0 == (SDHC->PRSSTAT & SDHC_PRSSTAT_BWEN_MASK)){}; //�ȴ�����׼����
						SDHC->DATPORT = *ptr;	
						ptr++;	//����ȡ�� *ptr++=SDHC->DATPORT;	 ��Ϊ�����BUG
        }
				
	if (SDHC->IRQSTAT & (SDHC_IRQSTAT_DEBE_MASK | SDHC_IRQSTAT_DCE_MASK | SDHC_IRQSTAT_DTOE_MASK))
	{
		SDHC->IRQSTAT |= SDHC_IRQSTAT_DEBE_MASK | SDHC_IRQSTAT_DCE_MASK | SDHC_IRQSTAT_DTOE_MASK;
		results = ESDHC_ERROR_DATA_TRANSFER;
	}
	SDHC->IRQSTAT |= SDHC_IRQSTAT_TC_MASK | SDHC_IRQSTAT_BRR_MASK | SDHC_IRQSTAT_BWR_MASK;
	//�ȴ���׼���ã�����״̬
	do
	{
			SD_CommandStruct1.COMMAND = ESDHC_CMD13;
			SD_CommandStruct1.ARGUMENT = sdh.RCA;
			SD_CommandStruct1.BLOCKS = 0;
			results = SD_SendCommand(&SD_CommandStruct1);
			if(results != ESDHC_OK) return ESDHC_ERROR_DATA_TRANSFER;  

			if (SD_CommandStruct1.RESPONSE[0] & 0xFFD98008)
			{
					//count = 0; // necessary to get real number of written blocks 
					break;
			}

	} while (0x000000900 != (SD_CommandStruct1.RESPONSE[0] & 0x00001F00));
	
	return ESDHC_OK;
}

/**
 * @brief ���SD������
 * @retval SD����������λMB
 */ 
uint32_t SD_GetSizeInMB(void)
{
	uint32_t BlockBumber;  //�����
	uint32_t Muti;         //����
	uint32_t BlockLen;     //ÿ�鳤��
	uint32_t Capacity;     //����
	//�������
	if((sdh.CSD[3]>>22)&0x03)
	{
		//------------------------------------------------------------
		//CSD V2.00�汾(SDHC��)
		//���������㹫ʽ
		//memory capacity = (C_SIZE+1) * 512K byte 
		//------------------------------------------------------------
		BlockLen = (sdh.CSD[2]>>24)&0xFF;
		Capacity=((sdh.CSD[1]>>8)&0xFFFFFF)+1;
		Capacity=(Capacity+1)/2;
		return Capacity;
	}
	else
	{
		/*
		CSD V1.00�汾(��ͨSD��)
		���������㹫ʽ  BLOCKNR = (C_SIZE+1) * MULT 
		MULT = 2^(C_SIZE_MULT+2)
		BLOCK_LEN = 2^((C_SIZE_MULT < 8) )
		 ����=BLOCKNR*BLOCK_LEN
		*/
        Muti=(sdh.CSD[1]>>7)&0x7;
        Muti=2<<(Muti+1);
        //�������
        BlockBumber =((sdh.CSD[2]>>0)&0x03);
        BlockBumber=(BlockBumber<<10)+((sdh.CSD[1]>>22)&0x0FFF);
        BlockBumber++;
        BlockBumber=BlockBumber*Muti;   //�õ�����
        BlockLen = (sdh.CSD[2]>>8)&0x0F;//�õ�ÿ���С
        BlockLen = 2<<(BlockLen-1);
        Capacity=BlockBumber*BlockLen;  //�������� ��λByte
        Capacity=Capacity/1024/1024;    //��λMB	
        return Capacity;
	}
}

/**
 * @brief ���IRQSTAT�Ĵ���״̬
 * @note  �ڲ�����
 * @retval SDģ����ж�״̬
 */ 
uint32_t SD_StatusWait (uint32_t  mask)
{
    uint32_t result;
    do
    {
        result = SDHC->IRQSTAT & mask;
    } 
    while (0 == result);
    return result;
}

/**
 * @brief ��SD����������
 * @note  �ڲ�����
 * @param  Command  :SD������ṹ����
 * @retval ESDHC_OK:����  ����:����
 */ 	
static uint32_t SD_SendCommand(SD_CommandTypeDef* Command)
{
    uint32_t xfertyp;
    uint32_t blkattr;
  
    xfertyp = Command->COMMAND;
    if (ESDHC_XFERTYP_CMDTYP_RESUME == ((xfertyp & SDHC_XFERTYP_CMDTYP_MASK) >> SDHC_XFERTYP_CMDTYP_SHIFT))
    {
        /* �ָ������������DPSELλ */
        xfertyp |= SDHC_XFERTYP_DPSEL_MASK;
    }
    if (Command->BLOCKS > 1)
    {
        xfertyp |= SDHC_XFERTYP_DPSEL_MASK;
        blkattr = SDHC_BLKATTR_BLKSIZE(Command->BLOCKSIZE) | SDHC_BLKATTR_BLKCNT(Command->BLOCKS);
        /* ׼���ȡBlockCnt */
        xfertyp |= SDHC_XFERTYP_BCEN_MASK;
        if (Command->BLOCKS > 1)
        {
            //��鴫��
            xfertyp |= SDHC_XFERTYP_MSBSEL_MASK;
        }
    }
    else if(Command->BLOCKS == 1)
    {
        blkattr = SDHC_BLKATTR_BLKSIZE(512) | SDHC_BLKATTR_BLKCNT(1);
    }
    else
	{
		blkattr = SDHC_BLKATTR_BLKSIZE(512) | SDHC_BLKATTR_BLKCNT(0);
	}
    //������Ƴ�״̬
    SDHC->IRQSTAT |= SDHC_IRQSTAT_CRM_MASK;
    //�ȴ�CMD����
    while (SDHC->PRSSTAT & SDHC_PRSSTAT_CIHB_MASK) {};
    //��ʼ������
    SDHC->CMDARG = Command->ARGUMENT;
    SDHC->BLKATTR = blkattr;
    SDHC->DSADDR = 0;
    //��������
    SDHC->XFERTYP = xfertyp;
    //�ȴ���Ӧ
    if (SD_StatusWait (SDHC_IRQSTAT_CIE_MASK | SDHC_IRQSTAT_CEBE_MASK | SDHC_IRQSTAT_CCE_MASK | SDHC_IRQSTAT_CC_MASK) != SDHC_IRQSTAT_CC_MASK)
    {
        SDHC->IRQSTAT |= SDHC_IRQSTAT_CTOE_MASK | SDHC_IRQSTAT_CIE_MASK | SDHC_IRQSTAT_CEBE_MASK | SDHC_IRQSTAT_CCE_MASK | SDHC_IRQSTAT_CC_MASK;
        return ESDHC_ERROR_COMMAND_FAILED;
    }
  
    /* if card removed */
    if (SDHC->IRQSTAT & SDHC_IRQSTAT_CRM_MASK)
    {
        SDHC->IRQSTAT |= SDHC_IRQSTAT_CTOE_MASK | SDHC_IRQSTAT_CC_MASK;
        return ESDHC_ERROR_COMMAND_FAILED;
    }
    /* command time out */
    if (SDHC->IRQSTAT & SDHC_IRQSTAT_CTOE_MASK)
    {
        SDHC->IRQSTAT |= SDHC_IRQSTAT_CTOE_MASK | SDHC_IRQSTAT_CC_MASK;
        return ESDHC_ERROR_COMMAND_TIMEOUT;
    }
    //������͵����л�Ӧ������
    if ((xfertyp & SDHC_XFERTYP_RSPTYP_MASK) != SDHC_XFERTYP_RSPTYP(ESDHC_XFERTYP_RSPTYP_NO))
    {
        Command->RESPONSE[0] = SDHC->CMDRSP[0];
		//������ܵ�����136�ֽڵĳ���Ӧ
        if ((xfertyp & SDHC_XFERTYP_RSPTYP_MASK) == SDHC_XFERTYP_RSPTYP(ESDHC_XFERTYP_RSPTYP_136))
        {
            Command->RESPONSE[1] = SDHC->CMDRSP[1];
            Command->RESPONSE[2] = SDHC->CMDRSP[2];
            Command->RESPONSE[3] = SDHC->CMDRSP[3];
        }
  }
    SDHC->IRQSTAT |= SDHC_IRQSTAT_CC_MASK;
    return ESDHC_OK;
}

/**
 * @brief ��SD���Ķ������
 * @note  һ����������Ϊ512�ֽ�
 * @param  sector  :Ҫ��ȡ��SD��������
 * @param  buffer  :���ݴ洢��ַ
 * @param  count   :������ȡ����������
 * @retval ESDHC_OK:����  ����:��ȡ����
 */ 		
uint8_t SD_ReadMultiBlock(uint32_t sector, uint8_t *pbuffer, uint16_t count)
{
	uint32_t i,j;
	uint16_t results;
    uint32_t *ptr = (uint32_t*)pbuffer;
	SD_CommandTypeDef SD_CommandStruct1;
	if(sdh.card_type  == SD_CARD_TYPE_SD) //�������ͨSD�� �ѿ��ַת�����ֽڵ�ַ
	{
		sector = sector<<9;
	}
	SD_CommandStruct1.COMMAND = ESDHC_CMD18;
	SD_CommandStruct1.BLOCKS = count;
	SD_CommandStruct1.BLOCKSIZE = 512;
	SD_CommandStruct1.ARGUMENT = sector;
	results = SD_SendCommand(&SD_CommandStruct1);
	if(results != ESDHC_OK) return ESDHC_ERROR_DATA_TRANSFER;  

	for(i = 0; i < count; i++)
	{
		if (((uint32_t)pbuffer & 0x03) == 0)
		{
			for (j = (512+3)>>2;j!= 0;j--)
			{
				//������ �д������˳�
				if (SDHC->IRQSTAT & (   SDHC_IRQSTAT_DEBE_MASK /* data end bit error */
															| SDHC_IRQSTAT_DCE_MASK  /* data CRC error */
															| SDHC_IRQSTAT_DTOE_MASK)) /* data timeout error */
				{
						SDHC->IRQSTAT |= SDHC_IRQSTAT_DEBE_MASK 
													| SDHC_IRQSTAT_DCE_MASK 
													| SDHC_IRQSTAT_DTOE_MASK 
													| SDHC_IRQSTAT_BRR_MASK; /* buffer read ready */
						return ESDHC_ERROR_DATA_TRANSFER;
				}
				while ((0 == (SDHC->PRSSTAT & SDHC_PRSSTAT_BREN_MASK)) && (SDHC->PRSSTAT & SDHC_PRSSTAT_DLA_MASK)) {};
				*ptr = SDHC->DATPORT;
				 ptr++;
			}
		}
	}
	
	//�ȴ��������
	SD_StatusWait (SDHC_IRQSTAT_TC_MASK);
	if (SDHC->IRQSTAT & (SDHC_IRQSTAT_DEBE_MASK | SDHC_IRQSTAT_DCE_MASK | SDHC_IRQSTAT_DTOE_MASK))
	{
		SDHC->IRQSTAT |= SDHC_IRQSTAT_DEBE_MASK | SDHC_IRQSTAT_DCE_MASK | SDHC_IRQSTAT_DTOE_MASK;
		results = ESDHC_ERROR_DATA_TRANSFER;
	}
	SDHC->IRQSTAT |= SDHC_IRQSTAT_TC_MASK | SDHC_IRQSTAT_BRR_MASK | SDHC_IRQSTAT_BWR_MASK;
	//�ȴ���׼���ã�����״̬
	do
	{
			SD_CommandStruct1.COMMAND = ESDHC_CMD13;
			SD_CommandStruct1.ARGUMENT = sdh.RCA;
			SD_CommandStruct1.BLOCKS = 0;
			results = SD_SendCommand(&SD_CommandStruct1);
			if(results != ESDHC_OK)
            {
                return ESDHC_ERROR_DATA_TRANSFER;  
            }
			if (SD_CommandStruct1.RESPONSE[0] & 0xFFD98008)
			{
					count = 0; /* necessary to get real number of written blocks */
					break;
			}

	} while (0x000000900 != (SD_CommandStruct1.RESPONSE[0] & 0x00001F00));
	return ESDHC_OK;
}

/**
 * @brief дSD���Ķ������
 * @note  һ����������Ϊ512�ֽ�
 * @param  sector  :Ҫд���SD��������
 * @param  buffer  :���ݴ洢��ַ
 * @param  count   :����д���������
 * @retval ESDHC_OK:����  ����:��ȡ����
 */ 	
uint8_t SD_WriteMultiBlock(uint32_t sector,const uint8_t *pbuffer, uint16_t count)
{
	uint32_t i,j;
	uint16_t results;
    uint32_t	*ptr = (uint32_t*)pbuffer;
	SD_CommandTypeDef SD_CommandStruct1;
	if(sdh.card_type  == SD_CARD_TYPE_SD) //�������ͨSD�� �ѿ��ַת�����ֽڵ�ַ
	{
		sector = sector<<9;
	}
	SD_CommandStruct1.COMMAND = ESDHC_CMD25;
	SD_CommandStruct1.BLOCKS = count;
	SD_CommandStruct1.BLOCKSIZE = 512;
	SD_CommandStruct1.ARGUMENT = sector;
	results = SD_SendCommand(&SD_CommandStruct1);
	if(results != ESDHC_OK) 
	{
		return ESDHC_ERROR_DATA_TRANSFER;  
	}

	//��ʼ��������
	for(i=0;i<count;i++)
	{
        for (j = (512)>>2;j!= 0;j--)
        {
            if (SDHC->IRQSTAT & (  SDHC_IRQSTAT_DEBE_MASK //Data End Bit Error
                                  | SDHC_IRQSTAT_DCE_MASK  //Data CRC Error
                                  | SDHC_IRQSTAT_DTOE_MASK)) //DataTimeout Error
					
            {
                SDHC->IRQSTAT |= SDHC_IRQSTAT_DEBE_MASK 
                              | SDHC_IRQSTAT_DCE_MASK 
                              | SDHC_IRQSTAT_DTOE_MASK 
                              | SDHC_IRQSTAT_BWR_MASK; //Buffer Write Ready
                return ESDHC_ERROR_DATA_TRANSFER;
            }
            while (0 == (SDHC->PRSSTAT & SDHC_PRSSTAT_BWEN_MASK)){}; //�ȴ�����׼����
						SDHC->DATPORT = *ptr;	
						ptr++;	//����ȡ�� *ptr++=SDHC->DATPORT;	 ��Ϊ�����BUG
        }
	}
	
	SD_StatusWait (SDHC_IRQSTAT_TC_MASK);

	if (SDHC->IRQSTAT & (SDHC_IRQSTAT_DEBE_MASK | SDHC_IRQSTAT_DCE_MASK | SDHC_IRQSTAT_DTOE_MASK))
	{
		SDHC->IRQSTAT |= SDHC_IRQSTAT_DEBE_MASK | SDHC_IRQSTAT_DCE_MASK | SDHC_IRQSTAT_DTOE_MASK;
		results = ESDHC_ERROR_DATA_TRANSFER;
	}
	SDHC->IRQSTAT |= SDHC_IRQSTAT_TC_MASK | SDHC_IRQSTAT_BRR_MASK | SDHC_IRQSTAT_BWR_MASK;
	//�ȴ���׼���ã�����״̬
	do
	{
			SD_CommandStruct1.COMMAND = ESDHC_CMD13;
			SD_CommandStruct1.ARGUMENT = sdh.RCA;
			SD_CommandStruct1.BLOCKS = 0;
			results = SD_SendCommand(&SD_CommandStruct1);
			if(results != ESDHC_OK) return ESDHC_ERROR_DATA_TRANSFER;  

			if (SD_CommandStruct1.RESPONSE[0] & 0xFFD98008)
			{
					count = 0; // necessary to get real number of written blocks 
					break;
			}

	} while (0x000000900 != (SD_CommandStruct1.RESPONSE[0] & 0x00001F00));
	
	return ESDHC_OK;
}



