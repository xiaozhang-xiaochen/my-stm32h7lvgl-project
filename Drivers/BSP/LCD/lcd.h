/**
 ****************************************************************************************************
 * @file        lcd.h
 * @author      正点原子团队(ALIENTEK)
 * @version     V1.2
 * @date        2024-03-14
 * @brief       通用 LCD 驱动接口 (兼容 MCU 屏与 RGB 屏)
 ****************************************************************************************************
 */

#ifndef __LCD_H
#define _LCD_H

#include "./SYSTEM/sys/sys.h"
#include "./BSP/LCD/ltdc.h"

/* LCD管理结构体 */
typedef struct
{
    uint16_t width;      /* LCD 宽度 */
    uint16_t height;     /* LCD 高度 */
    uint16_t id;         /* LCD ID */
    uint8_t  dir;        /* 横屏或竖屏控制 */
    uint32_t wramcmd;    /* 写 GRAM 指令 (RGB屏不使用) */
    uint32_t setxcmd;    /* 设置 X 坐标指令 (RGB屏不使用) */
    uint32_t setycmd;    /* 设置 Y 坐标指令 (RGB屏不使用) */
} _lcd_dev;

extern _lcd_dev lcddev; /* 管理 LCD 参数 */

/* 常用颜色定义 */
#define WHITE            0xFFFF
#define BLACK            0x0000
#define BLUE             0x001F
#define BRED             0XF81F
#define GRED             0XFFE0
#define GBLUE            0X07FF
#define RED              0xF800
#define MAGENTA          0xF81F
#define GREEN            0x07E0
#define CYAN             0x7FFF
#define YELLOW           0xFFE0
#define BROWN            0XBC40
#define BRRED            0XFC07
#define GRAY             0X8430
#define LGRAY            0XC618 /* 浅灰色 */
#define GRAYBLUE         0X5458 /* 灰蓝色 */
#define LIGHTBLUE        0X7D7C /* 浅蓝色 */
#define LIGHTGREEN       0X841F /* 浅绿色 */
#define LIGHTGRAY        0XEF5B /* 浅灰色 (另选) */
#define LGRAYBLUE        0XA651 /* 浅灰蓝色 */
#define LBBLUE           0X2B12 /* 浅棕蓝色 */

/* 函数声明 */
void lcd_init(void);                                                                    /* 初始化 */
void lcd_display_on(void);                                                              /* 开显示 */
void lcd_display_off(void);                                                             /* 关显示 */
void lcd_clear(uint32_t color);                                                         /* 清屏 */
void lcd_set_cursor(uint16_t x, uint16_t y);                                            /* 设置光标 */
void lcd_draw_point(uint16_t x, uint16_t y, uint32_t color);                            /* 画点 */
uint32_t lcd_read_point(uint16_t x, uint16_t y);                                        /* 读点 */
void lcd_draw_line(uint16_t x1, uint16_t y1, uint16_t x2, uint16_t y2, uint32_t color);  /* 画线 */
void lcd_draw_rectangle(uint16_t x1, uint16_t y1, uint16_t x2, uint16_t y2, uint32_t color); /* 画矩形 */
void lcd_show_char(uint16_t x, uint16_t y, char chr, uint8_t size, uint8_t mode, uint32_t color); /* 显示字符 */
void lcd_show_num(uint16_t x, uint16_t y, uint32_t num, uint8_t len, uint8_t size, uint32_t color); /* 显示数字 */
void lcd_show_string(uint16_t x, uint16_t y, uint16_t width, uint16_t height, uint8_t size, char *p, uint32_t color); /* 显示字符串 */

#endif
