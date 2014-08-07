/**
  ******************************************************************************
  * @file    bmp180.c
  * @author  YANDLD
  * @version V2.5
  * @date    2013.12.25
  * @brief   www.beyondcore.net   http://upcmcu.taobao.com 
  ******************************************************************************
  */
#include "bmp180.h"
#include <string.h>

/* BMP180 registers */
#define BMP180_PROM_START_ADDR          0xAA /* E2PROM calibration data start register */
#define BMP180_PROM_DATA_LEN            22   /* E2PROM length */
#define BMP180_CHIP_ID_REG              0xD0 /* Chip ID */
#define BMP180_VERSION_REG              0xD1 /* Version */
#define BMP180_CTRL_MEAS_REG            0xF4 /* Measurements control (OSS[7.6], SCO[5], CTL[4.0] */
#define BMP180_ADC_OUT_MSB_REG          0xF6 /* ADC out MSB  [7:0] */
#define BMP180_ADC_OUT_LSB_REG          0xF7 /* ADC out LSB  [7:0] */
#define BMP180_ADC_OUT_XLSB_REG         0xF8 /* ADC out XLSB [7:3] */
#define BMP180_SOFT_RESET_REG           0xE0 /* Soft reset control */
/* BMP180 Pressure calculation constants */
#define BMP180_PARAM_MG                 3038
#define BMP180_PARAM_MH                -7357
#define BMP180_PARAM_MI                 3791
/* BMP180 Calibration coefficients */
#define BMP180_REG_AC1                  0xAA
#define BMP180_REG_AC2                  0xAC
#define BMP180_REG_AC3                  0xAE
#define BMP180_REG_AC4                  0xB0
#define BMP180_REG_AC5                  0xB2
#define BMP180_REG_AC6                  0xB4
#define BMP180_REG_B1                   0xB6
#define BMP180_REG_B2                   0xB8
#define BMP180_REG_MB                   0xBA
#define BMP180_REG_MC                   0xBC
#define BMP180_REG_MD                   0xBE

__packed struct bmp180_cal
{
    int16_t AC1;
    int16_t AC2;
    int16_t AC3;
    uint16_t AC4;
    uint16_t AC5;
    uint16_t AC6;
    int16_t B1;
    int16_t B2;
    int16_t MB;
    int16_t MC;
    int16_t MD;
    int16_t B5; /* B5 is intermediate variable for pressure compensatation */
};

static struct i2c_device device;
static const uint8_t chip_addr_table[] = {0x77};
static struct bmp180_cal cal;

int bmp180_init(struct i2c_bus* bus)
{
    uint32_t ret = 0;
    /* i2c bus config */
    device.config.baudrate = 400*1000;
    device.config.data_width = 8;
    device.config.mode = 0;
    device.subaddr_len = 1;
    /* attach device to bus */
    ret = i2c_bus_attach_device(bus, &device);
    return ret;
}

/*!
 * @brief dump bmp180 calibration data from bmp180 internal eeporm to struct
 *
 * @param pointer of the bmp180 device struct
 */
static int dump_calibration_data(void)
{
    device.subaddr = BMP180_PROM_START_ADDR;
    if(device.bus->ops->read(&device, (uint8_t *)&cal, BMP180_PROM_DATA_LEN))
    {
        return 1;
    }
    cal.AC1 = SWAP16(cal.AC1);
    cal.AC2 = SWAP16(cal.AC2);
    cal.AC3 = SWAP16(cal.AC3);
    cal.AC4 = SWAP16(cal.AC4);
    cal.AC5 = SWAP16(cal.AC5);
    cal.AC6 = SWAP16(cal.AC6);
    cal.B1 = SWAP16(cal.B1);
    cal.B2 = SWAP16(cal.B2);
    cal.B5 = SWAP16(cal.B5);
    cal.MB = SWAP16(cal.MB);
    cal.MC = SWAP16(cal.MC);
    cal.MD = SWAP16(cal.MD);
    #ifdef LIB_DEBUG
    printf("AC1:%d\r\n", cal.AC1);
    printf("AC2:%d\r\n", cal.AC2);
    printf("AC3:%d\r\n", cal.AC3);
    printf("AC4:%d\r\n", cal.AC4); 
    printf("AC5:%d\r\n", cal.AC5);
    printf("B1:%d\r\n", cal.B1);
    printf("B2:%d\r\n", cal.B2);
    printf("B5:%d\r\n", cal.B5);
    printf("MB:%d\r\n", cal.MB);
    printf("MC:%d\r\n", cal.MC);
    printf("MD:%d\r\n", cal.MD);
    #endif
    return 0;
}

int bmp180_probe(void)
{
    uint32_t i;
    uint8_t buf[1];
    device.subaddr = BMP180_CHIP_ID_REG;
    for(i=0;i<ARRAY_SIZE(chip_addr_table);i++)
    {
        device.chip_addr = chip_addr_table[i];
        if(device.bus->ops->read(&device, buf, sizeof(buf)) == I2C_EOK)
        {
            /* find the device */
            device.chip_addr = chip_addr_table[i];
            /* ID match */
            if(buf[0] == 0x55)
            {
                #if LIB_DEBUG
                printf("bmp180 found!\r\n");
                #endif
                /* init sequence */
                dump_calibration_data();
                return 0; 
            }
        }
    }
    return 1; 
}


static int write_register(uint8_t addr, uint8_t value)
{
    uint8_t buf[1];
    device.subaddr = addr;
    buf[0] = value;
    if(device.bus->ops->write(&device, buf, 1))
    {
        return 1;
    }
    return 0;
}

static int read_register(uint8_t addr, uint8_t * value)
{
    uint8_t buf[1];
    device.subaddr = addr;
    if(device.bus->ops->read(&device, buf, 1))
    {
        return 1;
    }
    *value = buf[0];
    return 0;
}

/*!
 * @brief start bmp180 conversion
 *
 * @param pointer of the bmp180 device struct
 * @param conversion mode: 
 *        @arg BMP180_T_MEASURE: temperature conversion
 *        @arg BMP180_P0_MEASURE: pressure conversion, oversampling:0
 *        @arg BMP180_P1_MEASURE: pressure conversion, oversampling:1
 *        @arg BMP180_P2_MEASURE: pressure conversion, oversampling:2
 */
int bmp180_start_conversion(uint8_t cmd)
{
    return write_register(BMP180_CTRL_MEAS_REG, cmd); 
}


/*!
 * @brief read raw temperature data
 *
 * @param pointer of the bmp180 device struct
 * @param data pointer
 */
static int read_raw_temperature(int32_t * data)
{
    uint8_t buf[2];
    device.subaddr = BMP180_ADC_OUT_MSB_REG;
    if(device.bus->ops->read(&device, buf, sizeof(buf)))
    {
        return 1;
    }
    *data = (buf[0] << 8) + buf[1];
    return 0;
}

/*!
 * @brief read raw perssure data
 *
 * @param pointer of the bmp180 device struct
 * @param data pointer
 * @param oversampling rate data pointer
 */
static int read_raw_pressure(int32_t * data, uint8_t *oss)
{
    uint8_t buf[3];
    uint8_t reg1;
    /* read oss */
    read_register(BMP180_CTRL_MEAS_REG, &reg1);
    *oss = reg1 >> 6;
    device.subaddr = BMP180_ADC_OUT_MSB_REG;
    if(device.bus->ops->read(&device, buf, sizeof(buf)))
    {
        return 1;
    }
    *data = (buf[0] << 16) + (buf[1] << 8) + buf[2];
    *data >>= (8 - (*oss));
    return 0;
}

/*!
 * @brief tell if bmp180 is in conversion
 *
 * @param pointer of the bmp180 device struct
 * @retval 1:busy 0:idle
 */
static int is_conversion_busy(void)
{
    uint8_t reg1;
    read_register(BMP180_CTRL_MEAS_REG, &reg1);
    if(reg1 & (1 << 5)) /* busy */
    {
        return 1;
    }
    return 0;
}

/*!
 * @brief read real temperature data
 *
 * calculate according to bmp180 data sheet
 *
 * @param pointer of the bmp180 device struct
 * @param pointer of temperature, in 10x, eg:156 = 15.6C
 */
int bmp180_read_temperature(int32_t * temperature)
{
    int32_t raw_temperature;
    int32_t x1, x2;
    if(is_conversion_busy())
    {
        return 1;
    }
    if(read_raw_temperature(&raw_temperature))
    {
        return 2;
    }
    x1 = (raw_temperature - cal.AC6) * cal.AC5 >> 15;
    x2 = ((int32_t)cal.MC << 11) / (x1 + cal.MD);
    cal.B5 = x1 + x2;
    *temperature = ((x1 + x2) + 8) >> 4;
    return 0;
}

/*!
 * @brief read real pressure data
 *
 * calculate according to bmp180 data sheet
 *
 * @param pointer of the bmp180 device struct
 * @param pointer of pressure, in pa
 */
int bmp180_read_pressure(int32_t * pressure)
{
    int32_t raw_pressure;
    uint8_t oss;
    if(is_conversion_busy())
    {
        return 1;
    }
    /* read raw pressure */
    if(read_raw_pressure(&raw_pressure, &oss))
    {
        return 2;
    }
    /* temperature compensation */
    uint32_t b4, b7;
    int32_t x1, x2, x3, b3, b6;
    int32_t result;
    b6 = cal.B5 - 4000;
        
    x1 = (cal.B2 * (b6 * b6 >> 12)) >> 11;
    x2 = cal.AC2 * b6 >> 11;
    x3 = x1 + x2;

    b3 = ((((int32_t)cal.AC1 * 4 + x3) << oss) + 2) >> 2;
    x1 = cal.AC3 * b6 >> 13;
    x2 = (cal.B1 * (b6 * b6 >> 12)) >> 16;
    x3 = (x1 + x2 + 2) >> 2;
    b4 = (cal.AC4 * (uint32_t)(x3 + 32768)) >> 15;
    b7 = (uint32_t)(raw_pressure - b3) * (50000 >> oss);

    if (b7 < 0x80000000)
    {
        result = (b7 << 1) / b4;
    }
    else
    {
        result = (b7 / b4) << 1;
    }
    x1 = (result >> 8) * (result >> 8);
    x1 = (x1 * 3038) >> 16;
    x2 = (-7357 * result) >> 16;
    result += ((x1 + x2 + 3791) >> 4);
    *pressure = result;
    return 0;
}

