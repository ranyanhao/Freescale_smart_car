/**
  ******************************************************************************
  * @file    w25qxx.c
  * @author  YANDLD
  * @version V2.5
  * @date    2013.12.25
  * @brief   www.beyondcore.net   http://upcmcu.taobao.com 
  ******************************************************************************
  */

#include "spi_abstraction.h"
#include "w25qxx.h"

#define W25X_PAGE_SIZE          (256)      //һҳ�ߴ�
#define W25X_SECTOR_SIZE        (4096)
#define W25X_PBLOCK_SIZE        (64*1024)

//оƬWQ25ָ��
#define W25X_WriteEnable		0x06 
#define W25X_WriteDisable		0x04 
#define W25X_ReadStatusReg		0x05 
#define W25X_ReadStatusReg2		0x35 
#define W25X_WriteStatusReg		0x01 
#define W25X_ReadData			0x03 
#define W25X_FastReadData		0x0B 
#define W25X_FastReadDual		0x3B 
#define W25X_PageProgram		0x02 
#define W25X_BlockErase			0xD8 
#define W25X_SectorErase		0x20 
#define W25X_ChipErase			0xC7 
#define W25X_PowerDown			0xB9 
#define W25X_ReleasePowerDown	0xAB 
#define W25X_DeviceID			0xAB 
#define W25X_ManufactDeviceID	0x90 
#define W25X_JedecDeviceID		0x9F 


#ifndef USE_USER_MEM
uint8_t SPI_FLASH_BUFFER[4096];	
#endif

#define W25QXX_DEBUG		0
#if ( W25QXX_DEBUG == 1 )
#define W25QXX_TRACE	printf
#else
#define W25QXX_TRACE(...)
#endif

struct w25qxx_attr_t
{
    const char* name;
    uint32_t size;
    uint16_t id;
};

//W25оƬ��Ϣ
static const struct w25qxx_attr_t w25qxx_attr_table[] = 
{
    {"W25Q10",    128*1024, 0xEF10},
    {"W25Q20",    256*1024, 0xEF11},
    {"W25Q40",    512*1024, 0xEF12},
    {"W25Q80",   1024*1024, 0xEF13},
    {"W25Q16",   2048*1024, 0xEF14},
    {"W25Q32",   4096*1024, 0xEF15},
    {"W25Q64",   8192*1024, 0xEF16},
    {"W25Q128", 16384*1024, 0xEF17}, 
};

static struct spi_device device;
struct w25qxx_attr_t w25qxx_type;

//оƬ�ϵ�
static int w25qxx_power_up(void)
{
    volatile uint32_t i;
    uint8_t buf[1];
    buf[0] = W25X_ReleasePowerDown;
    spi_write(&device, buf, sizeof(buf), true);
    /* delay 3us */
    for(i=0;i<1000;i++);
    return SPI_EOK;
}

//оƬ��
static uint8_t w25qxx_read_sr2(void)
{
    uint8_t buf[1];
    buf[0] = W25X_ReadStatusReg2;
    spi_write(&device, buf, 1, false); //false = ����Ƭѡ,��������
    spi_read(&device, buf, 1, true);
    return buf[0];
}

static uint8_t w25qxx_read_sr(void)
{
    uint8_t buf[1];
    buf[0] = W25X_ReadStatusReg;
    spi_write(&device, buf, 1, false); //false = ����Ƭѡ,��������
    spi_read(&device, buf, 1, true);
    W25QXX_TRACE("W25QXX BUSY\r\n", buf[0]);
    return buf[0];
}

//оƬдʹ��
static int w25qxx_write_enable(void)
{
    uint8_t buf[1];
    buf[0] = W25X_WriteEnable;
    spi_write(&device, buf, sizeof(buf), true);
    return SPI_EOK;
}
//оƬд
static int w25qxx_write_sr(uint8_t value)
{
    uint8_t buf[2];
    buf[0] = W25X_WriteStatusReg;
    buf[1] = value;
    w25qxx_write_enable();
    spi_write(&device, buf, 2, true); 
    return SPI_EOK;
}
//оƬ̽��
int w25qxx_probe(void)
{
    uint32_t i;
    uint16_t id;
    uint8_t buf[4];
    buf[0] = W25X_ManufactDeviceID;
    buf[1] = 0;
    buf[2] = 0;
    buf[3] = 0;
    /* read id */
    spi_write(&device, buf, 4, false);
    spi_read(&device, buf, 2, true);
    id = ((buf[0]<<8) + buf[1]);
    W25QXX_TRACE("ID:%d\r\n", id);
    //see if we find a match
    for(i = 0; i< ARRAY_SIZE(w25qxx_attr_table);i++)
    {
        if(w25qxx_attr_table[i].id == id)
        {
            // find a match
            w25qxx_type.name = w25qxx_attr_table[i].name;
            w25qxx_type.id = w25qxx_attr_table[i].id;
            w25qxx_type.size = w25qxx_attr_table[i].size;
            w25qxx_power_up();
            buf[0] = w25qxx_read_sr();
            W25QXX_TRACE("SR:0x%X\r\n", buf[0]);
            buf[0] = w25qxx_read_sr2();
            W25QXX_TRACE("SR2:0x%X\r\n", buf[0]);
            // enable full access to all memory regin, something like unlock chip.
            w25qxx_write_sr(0x00);
            return SPI_EOK; 
        }
    }
    return SPI_ERROR;
}

//��ȡоƬ�ߴ�
uint32_t w25qxx_get_size(void)
{
    return w25qxx_type.size;
}
//��ȡоƬΨһID
uint32_t w25qxx_get_id(void)
{
    return w25qxx_type.id;
}
//��ȡоƬ�ͺ�
const char * w25qxx_get_name(void)
{
    return w25qxx_type.name;
}

//��ȡ����
int w25qxx_read(uint32_t addr, uint8_t *buf, uint32_t len)
{
    uint8_t buf_send[4];
    buf_send[0] = W25X_ReadData;
    buf_send[1] = (uint8_t)((addr)>>16);
    buf_send[2] = (uint8_t)((addr)>>8);
    buf_send[3] = (uint8_t)addr;
    
    if(spi_write(&device, buf_send, sizeof(buf_send), false))
    {
        return SPI_ERROR;
    }
    spi_read(&device, buf, len, true);
    return SPI_EOK;
}
//дһҳ����
static int w25qxx_write_page(uint32_t addr, uint8_t *buf, uint32_t len)
{
    uint8_t buf_send[4];
    w25qxx_write_enable();
    buf_send[0] = W25X_PageProgram;
    buf_send[1] = (uint8_t)((addr)>>16);
    buf_send[2] = (uint8_t)((addr)>>8);
    buf_send[3] = (uint8_t)addr;
    if(spi_write(&device, buf_send, sizeof(buf_send), false))
    {
        return SPI_ERROR;
    }
    spi_write(&device, buf, len, true);
    /* wait busy */
    while((w25qxx_read_sr() & 0x01) == 0x01);
    return SPI_EOK;
}
//д����
static int w25qxx_write_no_check(uint32_t addr, uint8_t *buf, uint32_t len)  
{ 			 		 
	uint16_t pageremain;	   
    W25QXX_TRACE("w25qxx - write_no_check: addr:%d len:%d\r\n", addr, len);
	pageremain = W25X_PAGE_SIZE-(addr%W25X_PAGE_SIZE); //��ҳʣ����ֽ���		 	    
	if(len <= pageremain) pageremain = len;//������256���ֽ�
	while(1)
	{	   
        if(w25qxx_write_page(addr, buf, pageremain))
        {
            return SPI_ERROR;
        }
		if(len == pageremain)break;//д�������
	 	else //NumByteToWrite>pageremain
		{
			buf += pageremain;
			addr += pageremain;	
			len -= pageremain;			  //��ȥ�Ѿ�д���˵��ֽ���
			if(len > 256)pageremain=256; //һ�ο���д��256���ֽ�
			else pageremain = len; 	  //����256���ֽ���
		}
	}
    return SPI_EOK;
} 
//��������
static int w25qxx_erase_sector(uint32_t addr)
{
    uint8_t buf_send[4];
    addr /= W25X_SECTOR_SIZE;
    addr *= W25X_SECTOR_SIZE; //round addr to N x W25X_SECTOR_SIZE
    buf_send[0] = W25X_SectorErase;
    buf_send[1] = (uint8_t)((addr)>>16);
    buf_send[2] = (uint8_t)((addr)>>8);
    buf_send[3] = (uint8_t)addr;
    w25qxx_write_enable();
    while((w25qxx_read_sr() & 0x01) == 0x01);
    spi_write(&device, buf_send, sizeof(buf_send), true);
    while((w25qxx_read_sr() & 0x01) == 0x01);
    return SPI_EOK;
}
//��������оƬ
int w25qxx_erase_chip(void)
{
    uint8_t buf_send[1]; 
    w25qxx_write_enable();
    while((w25qxx_read_sr() & 0x01) == 0x01);
    buf_send[0] = W25X_ChipErase;
    spi_write(&device, buf_send, sizeof(buf_send), true);
    while((w25qxx_read_sr() & 0x01) == 0x01);
    return SPI_EOK;
}
//��оƬд����
int w25qxx_write(uint32_t addr, uint8_t *buf, uint32_t len)  
{ 
	uint32_t secpos;
	uint16_t secoff;
	uint16_t secremain;	   
 	uint16_t i;    
	uint8_t * mem_pool;
    mem_pool = SPI_FLASH_BUFFER;
 	secpos = addr/4096;//������ַ  
	secoff = addr%4096;//�������ڵ�ƫ��
	secremain = 4096-secoff;//����ʣ��ռ��С   

 	if(len <= secremain)secremain = len;//������4096���ֽ�
	while(1) 
	{	
        w25qxx_read(secpos*4096, mem_pool, 4096); //������������������
		for(i = 0; i < secremain; i++)//У������
		{
			if(mem_pool[secoff+i]!=0XFF)break;//��Ҫ����  	  
		}
		if(i<secremain)//��Ҫ����
		{
            w25qxx_erase_sector(secpos*4096); //�����������
			for(i=0;i<secremain;i++)	   //����ԭ��������
			{
				mem_pool[i+secoff]=buf[i];	  
			}
            w25qxx_write_no_check(secpos*4096, mem_pool,4096); //д���������� 
		}else 
        {
            W25QXX_TRACE("no need to erase -addr:%d\r\n", addr);
            w25qxx_write_no_check(addr, buf, secremain);//д�Ѿ������˵�,ֱ��д������ʣ������. 
        }			   
		if(len == secremain)break;//д�������
		else//д��δ����
		{
			secpos++;//������ַ��1
			secoff=0;//ƫ��λ��Ϊ0 	 

		   	buf += secremain;  //ָ��ƫ��
			addr += secremain;//д��ַƫ��	   
		   	len -= secremain;				//�ֽ����ݼ�
			if(len > 4096) secremain = 4096;	//��һ����������д����
			else secremain = len;			//��һ����������д����
		}	 
	}
    return SPI_EOK;
}
//оƬ��ʼ��
int w25qxx_init(spi_bus_t bus, uint32_t cs)
{
    uint32_t ret;
    device.csn = cs;
    device.config.baudrate = 50*1000*1000;
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

