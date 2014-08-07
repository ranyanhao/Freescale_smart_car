/**
  ******************************************************************************
  * @file    nrf24l01.c
  * @author  YANDLD
  * @version V2.5
  * @date    2013.12.25
  * @brief   www.beyondcore.net   http://upcmcu.taobao.com 
  * @note    ���ļ�ΪNRF24L01����ģ���������֧��̨����Ų����оƬ
  ******************************************************************************
  */
  
#include "nrf24l01.h"
#include "spi_abstraction.h"

#define NRF24L01_DEBUG		0
#if ( NRF24L01_DEBUG == 1 )
#define NRF24L01_TRACE	printf
#else
#define NRF24L01_TRACE(...)
#endif

/* STATUS Bit Fields */
#define STATUS_RX_DR_MASK           0x40u 
#define STATUS_TX_DS_MASK           0x20u
#define STATUS_MAX_RT_MASK          0x10u
#define STATUS_RX_P_NO_SHIFT        1
#define STATUS_RX_P_NO_MASK         0x0Eu
#define STATUS_RX_P_NO(x)           (((uint8_t)(((uint32_t)(x))<<STATUS_RX_P_NO_SHIFT))&STATUS_RX_P_NO_MASK)
#define STATUS_TX_FULL              0x01u
/* CONFIG bit Fields */
#define CONFIG_PRIM_RX_MASK         0x01u
#define CONFIG_PWR_UP_MASK          0x02u
#define CONFIG_CRCO_MASK            0x04u
#define CONFIG_EN_CRC_MASK          0x08u
#define CONFIG_MASK_MAX_RT_MASK     0x10u
#define CONFIG_MASK_TX_DS_MASK      0x20u
#define CONFIG_MASK_RX_DS_MASK      0x40u
/* OBSERVE_TX bit Fields */
#define OBSERVE_TX_ARC_CNT_MASK     0x0Fu
#define OBSERVE_TX_ARC_CNT_SHIFT    0
#define OBSERVE_TX_PLOS_CNT_MASK    0xF0u
#define OBSERVE_TX_PLOS_CNT_SHIFT   4
//********************************************************************************************************************// 
/* SPI(nRF24L01) ָ�� */
#define READ_REG                    0x00   // �����üĴ���
#define WRITE_REG                   0x20   // д���üĴ���
#define RD_RX_PLOAD                 0x61   // ��ȡRX FIFO�е�����
#define WR_TX_PLOAD                 0xA0   // ��TX FIFO��д������
#define FLUSH_TX                    0xE1   // ���TX FIFO�е����� Ӧ���ڷ���ģʽ��
#define FLUSH_RX                    0xE2   // ���RX FIFO�е����� Ӧ���ڽ���ģʽ��
#define REUSE_TX_PL                 0xE3   // ����ʹ����һ����Ч����
#define R_RX_PL_WID                 0x60
#define NOP                         0xFF   // ����
//********************************************************************************************************************// 
/* register define */
#define CONFIG              0x00  //���÷���״̬��CRCУ��ģʽ�Լ����շ�״̬��Ӧ��ʽ
#define EN_AA               0x01  //�Զ�Ӧ��������
#define EN_RXADDR           0x02  //�����ŵ�����
#define SETUP_AW            0x03  //�շ���ַ�������
#define SETUP_RETR          0x04  //�Զ��ط�����������
#define RF_CH               0x05  //����Ƶ���趨
#define RF_SETUP            0x06
#define STATUS              0x07
#define OBSERVE_TX          0x08
#define CD                  0x09
#define RX_ADDR_P0          0x0A
#define RX_ADDR_P1          0x0B
#define RX_ADDR_P2          0x0C
#define RX_ADDR_P3          0x0D
#define RX_ADDR_P4          0x0E
#define RX_ADDR_P5          0x0F
#define TX_ADDR             0x10
#define RX_PW_P0            0x11
#define RX_PW_P1            0x12
#define RX_PW_P2            0x13
#define RX_PW_P3            0x14
#define RX_PW_P4            0x15
#define RX_PW_P5            0x16
#define FIFO_STATUS         0x17
#define PYNPD               0x1C
#define FEATURE             0x1D

static struct spi_device device;
const uint8_t TX_ADDRESS[5]={0x34,0x43,0x10,0x10,0x01}; //���͵�ַ
const uint8_t RX_ADDRESS[5]={0x34,0x43,0x10,0x10,0x01}; //���յ�ַ

//���Ĵ���
static uint8_t read_reg(uint8_t addr)
{
    uint8_t temp = READ_REG + addr;
    spi_write(&device, &temp, 1, false);
    spi_read(&device, &temp, 1, true);
    return temp;
}
//д�Ĵ���
static void write_reg(uint8_t addr, uint8_t val)
{
    uint8_t temp = WRITE_REG + addr;
    spi_write(&device, &temp, 1, false);
    spi_write(&device, &val, 1, true);
}
//д����
static void write_buffer(uint8_t addr, uint8_t *buf, uint32_t len)
{
    uint8_t temp = WRITE_REG + addr;
    spi_write(&device, &temp, 1, false);
    spi_write(&device, buf, len, true);
}
//������
static void read_buffer(uint8_t addr, uint8_t *buf, uint32_t len)
{
    uint8_t temp = READ_REG + addr;
    spi_write(&device, &temp, 1, false);
    spi_read(&device, buf, len, true);
}
//NRF�豸��⣬�����ý��պͷ��͵�ַ
int nrf24l01_probe(void)
{
    uint8_t i;
    uint8_t buf[5];
    read_buffer(TX_ADDR, buf, 5);
    for(i = 0; i < 5; i++)
    {
        if((buf[i]!= 0) && (buf[i] != 0xFF))
        {
            NRF24L01_CE_LOW();
            /* init sequence */
            write_reg(CONFIG, 0x0F); /* config */
            write_reg(EN_AA, 0x03);/* aa */
            write_reg(EN_RXADDR, 0x03); /* available pipe */
            write_reg(SETUP_AW, 0x03);  /* setup_aw */
            write_reg(SETUP_RETR, 0x07);/* setup_retr */
            write_reg(RF_CH,40);/* RF freq */
            write_reg(RF_SETUP, 0X04);
            write_reg(RX_PW_P0, 0X20);
            write_reg(RX_PW_P1, 0X20);
            write_reg(RX_PW_P2, 0X20);
            write_reg(RX_PW_P3, 0X20);
            write_reg(RX_PW_P4, 0X20);
            write_reg(RX_PW_P5, 0X20);
            write_reg(PYNPD, 0x3F); /* enable dynmic playload whhich means packet len is varible */
            write_reg(FEATURE, 0x07); /* enable dynmic playload whhich means packet len is varible */
            write_buffer(RX_ADDR_P0, (uint8_t*)RX_ADDRESS, 5);
            write_buffer(TX_ADDR, (uint8_t*)TX_ADDRESS, 5);
            return 0;
        }
    }
    return 1;
}
//NRFģ���ʼ��
int nrf24l01_init(spi_bus_t bus, uint32_t cs)
{
    uint32_t ret;
    device.csn = cs;
    device.config.baudrate = 1*1000*1000;
    device.config.data_width = 8;
    device.config.mode = SPI_MODE_0 | SPI_MASTER | SPI_MSB;
    ret = spi_bus_attach_device(bus, &device);
    if(ret)
    {
        return ret;
    }
    else
    {
        ret = spi_config(&device);
    }
    return ret;
}


//�ú�����ʼ��NRF24L01��TXģʽ
//����TX��ַ,дTX���ݿ��,����RX�Զ�Ӧ��ĵ�ַ,���TX��������,ѡ��RFƵ��,�����ʺ�LNA HCURR
//PWR_UP,CRCʹ��
//��CE��ߺ�,������RXģʽ,�����Խ���������		   
//CEΪ�ߴ���10us,����������.

void nrf24l01_set_tx_mode(void)
{
    uint8_t value;
    value = FLUSH_TX;
    spi_write(&device, &value, 1, true);
    NRF24L01_CE_LOW();
    value = read_reg(CONFIG);
    value &= ~CONFIG_PRIM_RX_MASK;
    write_reg(CONFIG, value); /* Set PWR_UP bit, enable CRC(2 length) & Prim:RX. RX_DR enabled.. */
    NRF24L01_CE_HIGH();
}

//�ú�����ʼ��NRF24L01��RXģʽ
//����RX��ַ,дRX���ݿ��,ѡ��RFƵ��,�����ʺ�LNA HCURR
//��CE��ߺ�,������RXģʽ,�����Խ���������	
void nrf24l01_set_rx_mode(void)
{
	uint8_t value;
    /* reflash data */
    value = FLUSH_RX;
    spi_write(&device, &value, 1, true);
	NRF24L01_CE_LOW();
    /* set CONFIG_PRIM_RX_MASK to enable Rx */
    value = read_reg(CONFIG);
    value |= CONFIG_PRIM_RX_MASK;
    write_reg(CONFIG, value);
	NRF24L01_CE_HIGH();
}

//����NRF24L01����һ������
//txbuf:�����������׵�ַ
//����ֵ:�������״��
int nrf24l01_write_packet(uint8_t *buf, uint32_t len)
{
    uint8_t status;
    uint8_t plos;
    /* clear bits */
    status = read_reg(STATUS);
    status |= STATUS_TX_DS_MASK | STATUS_MAX_RT_MASK;
    write_reg(STATUS, status);
    NRF24L01_CE_LOW();
    /* clear PLOS */
    write_reg(RF_CH, 40); 
    /* write data */
    uint8_t value = WR_TX_PLOAD;
    spi_write(&device, &value, 1, false);
    spi_write(&device, buf, len, true);
    NRF24L01_CE_HIGH();
    while(1)
    {
        status = read_reg(STATUS);
        if(status & STATUS_TX_DS_MASK)
        {
            /* send complete */
            status |= STATUS_TX_DS_MASK | STATUS_MAX_RT_MASK;
            write_reg(STATUS, status);
            return 0;
        }
        status = read_reg(OBSERVE_TX);
        plos = (status & OBSERVE_TX_PLOS_CNT_MASK) >> OBSERVE_TX_PLOS_CNT_SHIFT;
        if(plos && (status & STATUS_MAX_RT_MASK))
        {
            status = FLUSH_TX;
            spi_write(&device, &status, 1, true);
            return 1;
        }
    }
}

//NRF��һ֡����
int nrf24l01_read_packet(uint8_t *buf, uint32_t *len)
{
	uint8_t sta,rev_len;
    sta = read_reg(STATUS);
	if(sta & STATUS_RX_DR_MASK)
	{
        /* clear pendign bit */
        sta |= STATUS_RX_DR_MASK;
        write_reg(STATUS, sta);
        /* read len and data */
        rev_len = read_reg(R_RX_PL_WID);
        read_buffer(RD_RX_PLOAD, buf, rev_len);
        *len = rev_len;
        /* if rev_len > 32 which means a error occur, usr FLUSH_RX to clear */
        if(rev_len > 32)
        {
            NRF24L01_TRACE("rev len > 32, error!\r\n");
            sta = FLUSH_RX;
            spi_write(&device, &sta, 1, true);
            *len = 32;
            return 2;
        }
        return 0;
	}
    *len = 0;
    return 1;
}

