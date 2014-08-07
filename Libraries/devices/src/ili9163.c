/**
  ******************************************************************************
  * @file    ili9163.c
  * @author  YANDLD
  * @version V2.5
  * @date    2013.12.25
  * @brief   www.beyondcore.net   http://upcmcu.taobao.com 
  * @note    ���ļ�ΪNRF24L01����ģ���������֧��̨����Ų����оƬ
  ******************************************************************************
  */



#include "ili9163.h"
#include "spi_abstraction.h"

#define ILI9163_DEBUG		0
#if ( ILI9163_DEBUG == 1 )
#define ILI9163_TRACE	printf
#else
#define ILI9163_TRACE(...)
#endif


#define LCD_X_MAX   (128)
#define LCD_Y_MAX   (160)
#define RST_PORT     HW_GPIOA
#define RS_PORT      HW_GPIOA
#define RST_PIN      (13)
#define RS_PIN       (12)
#define LCD_RST_LOW()       do {PAout(RST_PIN) = 0;} while(0)
#define LCD_RST_HIGH()      do {PAout(RST_PIN) = 1;} while(0)
#define LCD_RS_LOW()        do {PAout(RS_PIN) = 0;} while(0)
#define LCD_RS_HIGH()       do {PAout(RS_PIN) = 1;} while(0)

static struct spi_device device;

static void LCD_COM(uint8_t cmd)
{
    LCD_RS_LOW();
    spi_write(&device, &cmd, 1, true);
}

static void LCD_DATA(uint8_t data)
{
    LCD_RS_HIGH();
    spi_write(&device, &data, 1, true);
}


void ili9163_set_window(int x, int y, int xlen, int ylen)
{
    xlen--;
    ylen--;
    LCD_COM(0x2A);
    LCD_DATA(x>>8);
    LCD_DATA(x);
    LCD_DATA((x+xlen)>>8);
    LCD_DATA((x+xlen));
  
    LCD_COM(0x2B);
    LCD_DATA(y>>8);
    LCD_DATA(y);
    LCD_DATA((y+ylen)>>8);
    LCD_DATA((y+ylen));
}

void ili9163_clear(uint16_t c)
{
	uint32_t i=0;      
	ili9163_set_window(0,0,LCD_X_MAX,LCD_Y_MAX);	 
	LCD_COM(0x2C);     //��ʼд��GRAM	 	  
	for(i = 0 ;i < LCD_X_MAX*LCD_Y_MAX; i++)
	{
        LCD_DATA(c>>8);
        LCD_DATA(c); 
	}
}  

void ili9163_write_pixel(int x, int y, int c)
{
    ili9163_set_window(x,y,1,1);//���ù��λ�� 
	LCD_COM(0x2C);
    LCD_DATA(c>>8);
    LCD_DATA(c);
}


int ili9163_init(spi_bus_t bus, uint32_t cs)
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
    GPIO_QuickInit(RST_PORT, RST_PIN, kGPIO_Mode_OPP);
    GPIO_QuickInit(RS_PORT, RS_PIN, kGPIO_Mode_OPP);
	LCD_RST_LOW();    
    DelayMs(1);
	LCD_RST_HIGH();    
	DelayMs(1);
	LCD_COM(0x11);       		  	//�ر�˯�ߣ���������
	DelayMs(1);
	
	LCD_COM(0x3a);       		  	//ÿ�δ���16λ����(VIPF3-0=0101)��ÿ������16λ(IFPF2-0=101)
	LCD_DATA(0x55);						
 
	LCD_COM(0x26);       		  	
	LCD_DATA(0x04);

	LCD_COM(0xf2);              		//Driver Output Control(1)
	LCD_DATA(0x01);
	
	LCD_COM(0xe0);              		//Driver Output Control(1)
	LCD_DATA(0x3f);
	LCD_DATA(0x25);
	LCD_DATA(0x1c);
	LCD_DATA(0x1e);
	LCD_DATA(0x20);
	LCD_DATA(0x12);
	LCD_DATA(0x2a);
	LCD_DATA(0x90);
	LCD_DATA(0x24);
	LCD_DATA(0x11);
	LCD_DATA(0x00);
	LCD_DATA(0x00);
	LCD_DATA(0x00);
	LCD_DATA(0x00);
	LCD_DATA(0x00);
	 
	LCD_COM(0xe1);              //Driver Output Control(1)
	LCD_DATA(0x20);
	LCD_DATA(0x20);
	LCD_DATA(0x20);
	LCD_DATA(0x20);
	LCD_DATA(0x05);
	LCD_DATA(0x00);
	LCD_DATA(0x15);
	LCD_DATA(0xa7);
	LCD_DATA(0x3d);
	LCD_DATA(0x18);
	LCD_DATA(0x25);
	LCD_DATA(0x2a);
	LCD_DATA(0x2b);
	LCD_DATA(0x2b);  
	LCD_DATA(0x3a);  
	
	LCD_COM(0xb1);              	//������Ļˢ��Ƶ��
	LCD_DATA(0x08);				   	//DIVA=8
	LCD_DATA(0x08);				   	//VPA =8��Լ90Hz
						 
	LCD_COM(0xb4);              	//LCD Driveing control
	LCD_DATA(0x07);				  	//NLA=1,NLB=1,NLC=1
 
 
	LCD_COM(0xc0);              //LCD Driveing control
	LCD_DATA(0x0a);
	LCD_DATA(0x02);
		
	LCD_COM(0xc1);              //LCD Driveing control
	LCD_DATA(0x02);

	LCD_COM(0xc5);              //LCD Driveing control
	LCD_DATA(0x4f);
	LCD_DATA(0x5a);

	LCD_COM(0xc7);              //LCD Driveing control
	LCD_DATA(0x40);
	
	LCD_COM(0x2a);              	//����MCU�ɲ�����LCD�ڲ�RAM��������ʼ����������
	LCD_DATA(0x00);				   	//��������ʼ��ַ0x0000
	LCD_DATA(0x00);					
	LCD_DATA(0x00);				   	//�����������ַ0x007f(127)
	LCD_DATA(0x7f);
 
	LCD_COM(0x2b);              	//����MCU�ɲ�����LCD�ڲ�RAM��������ʼ��������
	LCD_DATA(0x00);				   	//��������ʼ��ַ0x0000
	LCD_DATA(0x00);
	LCD_DATA(0x00);				  	//�����������ַ0x009f(159)
	LCD_DATA(0x9f);

	LCD_COM(0x36);              	//����MPU��DDRAM��Ӧ��ϵ
	LCD_DATA(0xc0);					//MX=1,MY=1

	LCD_COM(0xb7);              	//LCD Driveing control
	LCD_DATA(0x00);				   	//CRL=0
	 
	LCD_COM(0x29);   		  	//������Ļ��ʾ
	LCD_COM(0x2c);   			//����ΪLCD��������/����ģʽ
	ili9163_clear(0x0000);
    return 0;
}



