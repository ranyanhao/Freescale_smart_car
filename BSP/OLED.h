#ifndef __OLED_H_
#define __OLED_H_
#include "includes.h"

 void LCD_Init(void);
 void LCD_CLS(void);
 void LCD_P6x8Str(uint8_t x,uint8_t y,uint8_t ch[]);
 void LCD_P8x16Str(uint8_t x,uint8_t y,uint8_t ch[]);
 void LCD_P14x16Str(uint8_t x,uint8_t y,uint8_t ch[]);
 void LCD_Print(uint8_t x, uint8_t y, uint8_t ch[]);
 void LCD_PutPixel(uint8_t x,uint8_t y);
 void LCD_Rectangle(uint8_t x1,uint8_t y1,uint8_t x2,uint8_t y2,uint8_t gif);
 //void Draw_LQLogo(void);
 void Draw_LibLogo(void);
 void Draw_BMP(uint8_t x0,uint8_t y0,uint8_t x1,uint8_t y1,uint8_t bmp[]); 
 void LCD_Fill(uint8_t dat);

#endif
