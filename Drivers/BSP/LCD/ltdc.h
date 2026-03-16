/**
 ****************************************************************************************************
 * @file        ltdc.h
 * @author      正点原子团队(ALIENTEK) / Gemini 优化版
 * @version     V1.2
 * @date        2024-03-14
 * @brief       LTDC (液晶控制器) 驱动代码
 * @license     Copyright (c) 2020-2032, 广州市星翼电子科技有限公司
 ****************************************************************************************************
 * @attention
 *
 * 优化说明:
 * 1. 适配 1024*600 RGB 屏幕
 * 2. 显存完全定位在外部 SDRAM，释放内部 SRAM
 * 3. 增加双缓冲切换接口
 * 4. 全中文 UTF-8 注释，解决乱码问题
 *
 ****************************************************************************************************
 */

#ifndef _LTDC_H
#define _LTDC_H

#include "./SYSTEM/sys/sys.h"

/* LCD LTDC核心参数结构体 */
typedef struct  
{
    uint32_t pwidth;      /* 物理宽度 (面板实际像素宽度) */
    uint32_t pheight;     /* 物理高度 (面板实际像素高度) */
    uint16_t hsw;         /* 水平同步宽度 */
    uint16_t vsw;         /* 垂直同步宽度 */
    uint16_t hbp;         /* 水平后廊 */
    uint16_t vbp;         /* 垂直后廊 */
    uint16_t hfp;         /* 水平前廊 */
    uint16_t vfp;         /* 垂直前廊 */
    uint8_t activelayer;  /* 当前操作图层: 0 / 1 */
    uint8_t dir;          /* 显示方向: 0-横屏; 1-竖屏 (由软件算法实现) */
    uint16_t width;       /* 逻辑宽度 (根据方向变化) */
    uint16_t height;      /* 逻辑高度 (根据方向变化) */
    uint32_t pixsize;     /* 每个像素占用的字节数 */
}_ltdc_dev; 

extern _ltdc_dev lcdltdc;                   /* LTDC 管理结构体 */
extern LTDC_HandleTypeDef g_ltdc_handle;    /* LTDC 句柄 */
extern DMA2D_HandleTypeDef g_dma2d_handle;  /* DMA2D 句柄 */

/******************************************************************************************/
/* LTDC 引脚及背光控制定义 */

#define LTDC_BL_GPIO_PORT               GPIOB
#define LTDC_BL_GPIO_PIN                GPIO_PIN_5
#define LTDC_BL_GPIO_CLK_ENABLE()       do{ __HAL_RCC_GPIOB_CLK_ENABLE(); }while(0)

#define LTDC_DE_GPIO_PORT               GPIOF
#define LTDC_DE_GPIO_PIN                GPIO_PIN_10
#define LTDC_DE_GPIO_CLK_ENABLE()       do{ __HAL_RCC_GPIOF_CLK_ENABLE(); }while(0)

#define LTDC_VSYNC_GPIO_PORT            GPIOI
#define LTDC_VSYNC_GPIO_PIN             GPIO_PIN_9
#define LTDC_VSYNC_GPIO_CLK_ENABLE()    do{ __HAL_RCC_GPIOI_CLK_ENABLE(); }while(0)

#define LTDC_HSYNC_GPIO_PORT            GPIOI
#define LTDC_HSYNC_GPIO_PIN             GPIO_PIN_10
#define LTDC_HSYNC_GPIO_CLK_ENABLE()    do{ __HAL_RCC_GPIOI_CLK_ENABLE(); }while(0)

#define LTDC_CLK_GPIO_PORT              GPIOG
#define LTDC_CLK_GPIO_PIN               GPIO_PIN_7
#define LTDC_CLK_GPIO_CLK_ENABLE()      do{ __HAL_RCC_GPIOG_CLK_ENABLE(); }while(0)

/* 默认像素格式定义 (LVGL 常用 RGB565) */
#define LTDC_PIXFORMAT                  LTDC_PIXEL_FORMAT_RGB565

/* 显存大小计算 (1024 * 600 * 2 字节 = 1.17MB) */
#define LTDC_FRAME_BUF_SIZE             (1024 * 600 * 2)

/* 显存起始物理地址 (SDRAM) */
#define LTDC_FRAME_BUF_ADDR             0XC0000000  
/* 第二缓冲区地址 (双缓冲使用) */
#define LTDC_FRAME_BUF1_ADDR            (LTDC_FRAME_BUF_ADDR + LTDC_FRAME_BUF_SIZE)

/* 背光控制宏 */
#define LTDC_BL(x)   do{ x ? \
                      HAL_GPIO_WritePin(LTDC_BL_GPIO_PORT, LTDC_BL_GPIO_PIN, GPIO_PIN_SET) : \
                      HAL_GPIO_WritePin(LTDC_BL_GPIO_PORT, LTDC_BL_GPIO_PIN, GPIO_PIN_RESET); \
                     }while(0)

/******************************************************************************************/
/* 驱动函数接口声明 */

void ltdc_switch(uint8_t sw);                                                                                   /* LTDC 控制开关 */
void ltdc_layer_switch(uint8_t layerx, uint8_t sw);                                                             /* 图层开关控制 */
void ltdc_select_layer(uint8_t layerx);                                                                         /* 选择当前操作图层 */
void ltdc_display_dir(uint8_t dir);                                                                             /* 设置显示方向 */
void ltdc_draw_point(uint16_t x, uint16_t y, uint32_t color);                                                   /* 画点函数 */
uint32_t ltdc_read_point(uint16_t x, uint16_t y);                                                               /* 读点颜色函数 */
void ltdc_fill(uint16_t sx, uint16_t sy, uint16_t ex, uint16_t ey, uint32_t color);                             /* 矩形填充 (DMA2D加速) */
void ltdc_color_fill(uint16_t sx, uint16_t sy, uint16_t ex, uint16_t ey, uint16_t *color);                      /* 块颜色填充 (DMA2D加速) */
void ltdc_clear(uint32_t color);                                                                                /* 清屏函数 */
uint8_t ltdc_clk_set(uint32_t pll3n, uint32_t pll3m, uint32_t pll3r);                                           /* LTDC 时钟配置 */
void ltdc_layer_window_config(uint8_t layerx, uint16_t sx, uint16_t sy, uint16_t width, uint16_t height);       /* 窗口位置配置 */
void ltdc_layer_parameter_config(uint8_t layerx, uint32_t bufaddr, uint8_t pixformat, uint8_t alpha, uint8_t alpha0, uint8_t bfac1, uint8_t bfac2, uint32_t bkcolor); /* 图层参数配置 */
uint16_t ltdc_panelid_read(void);                                                                               /* 读取 LCD ID */
void ltdc_init(void);                                                                                           /* 驱动初始化 */
void ltdc_switch_buffer(uint8_t buffer_idx);                                                                    /* 显存缓冲区切换 (双缓冲核心) */

#endif 
