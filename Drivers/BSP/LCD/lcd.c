/**
 ****************************************************************************************************
 * @file        lcd.c
 * @author      正点原子团队(ALIENTEK) / Gemini 优化版
 * @version     V1.2
 * @date        2024-03-14
 * @brief       通用 LCD 驱动实现 (针对 RGB 屏进行了简化)
 ****************************************************************************************************
 */

#include "./BSP/LCD/lcd.h"
#include "./BSP/LCD/lcdfont.h"

_lcd_dev lcddev; /* LCD 参数管理 */

/**
 * @brief       写点函数
 * @param       x, y: 坐标
 * @param       color: 颜色
 */
void lcd_draw_point(uint16_t x, uint16_t y, uint32_t color)
{
    ltdc_draw_point(x, y, color);
}

/**
 * @brief       读点函数
 * @param       x, y: 坐标
 * @retval      颜色值
 */
uint32_t lcd_read_point(uint16_t x, uint16_t y)
{
    return ltdc_read_point(x, y);
}

/**
 * @brief       清屏
 * @param       color: 颜色
 */
void lcd_clear(uint32_t color)
{
    ltdc_clear(color);
}

/**
 * @brief       画线
 */
void lcd_draw_line(uint16_t x1, uint16_t y1, uint16_t x2, uint16_t y2, uint32_t color)
{
    /* 使用经典的 Bresenham 算法画线 */
    uint16_t t;
    int xerr = 0, yerr = 0, delta_x, delta_y, distance;
    int incx, incy, uRow, uCol;
    delta_x = x2 - x1;
    delta_y = y2 - y1;
    uRow = x1;
    uCol = y1;

    if (delta_x > 0) incx = 1;
    else if (delta_x == 0) incx = 0;
    else { incx = -1; delta_x = -delta_x; }

    if (delta_y > 0) incy = 1;
    else if (delta_y == 0) incy = 0;
    else { incy = -1; delta_y = -delta_y; }

    if (delta_x > delta_y) distance = delta_x;
    else distance = delta_y;

    for (t = 0; t <= distance + 1; t++)
    {
        lcd_draw_point(uRow, uCol, color);
        xerr += delta_x;
        yerr += delta_y;
        if (xerr > distance) { xerr -= distance; uRow += incx; }
        if (yerr > distance) { yerr -= distance; uCol += incy; }
    }
}

/**
 * @brief       画矩形
 */
void lcd_draw_rectangle(uint16_t x1, uint16_t y1, uint16_t x2, uint16_t y2, uint32_t color)
{
    lcd_draw_line(x1, y1, x2, y1, color);
    lcd_draw_line(x1, y1, x1, y2, color);
    lcd_draw_line(x1, y2, x2, y2, color);
    lcd_draw_line(x2, y1, x2, y2, color);
}

/**
 * @brief       显示字符 (支持 12/16/24/32 字体)
 */
void lcd_show_char(uint16_t x, uint16_t y, char chr, uint8_t size, uint8_t mode, uint32_t color)
{
    uint8_t temp, t1, t;
    uint8_t y0 = y;
    uint8_t csize = (size / 8 + ((size % 8) ? 1 : 0)) * (size / 2);
    chr = chr - ' '; /* 得到偏移后的值 */

    for (t = 0; t < csize; t++)
    {
        if (size == 12) temp = asc2_1206[chr][t];
        else if (size == 16) temp = asc2_1608[chr][t];
        else if (size == 24) temp = asc2_2412[chr][t];
        else if (size == 32) temp = asc2_3216[chr][t];
        else return;

        for (t1 = 0; t1 < 8; t1++)
        {
            if (temp & 0x80) lcd_draw_point(x, y, color);
            else if (mode == 0) lcd_draw_point(x, y, WHITE); /* 默认白色背景 */
            temp <<= 1;
            y++;
            if (y >= lcddev.height) return;
            if ((y - y0) == size)
            {
                y = y0;
                x++;
                if (x >= lcddev.width) return;
                break;
            }
        }
    }
}

/**
 * @brief       显示字符串
 */
void lcd_show_string(uint16_t x, uint16_t y, uint16_t width, uint16_t height, uint8_t size, char *p, uint32_t color)
{
    uint8_t x0 = x;
    width += x;
    height += y;
    while ((*p <= '~') && (*p >= ' '))
    {
        if (x >= width) { x = x0; y += size; }
        if (y >= height) break;
        lcd_show_char(x, y, *p, size, 0, color);
        x += size / 2;
        p++;
    }
}

/**
 * @brief       初始化 LCD
 */
void lcd_init(void)
{
    ltdc_init(); /* 调用 LTDC 初始化 */
    lcddev.id = 0X7016;
    lcddev.width = lcdltdc.width;
    lcddev.height = lcdltdc.height;
}
