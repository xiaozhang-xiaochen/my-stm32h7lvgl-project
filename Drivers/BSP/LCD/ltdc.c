/**
 ****************************************************************************************************
 * @file        ltdc.c
 * @author      正点原子团队(ALIENTEK) / Gemini 优化版
 * @version     V1.4
 * @date        2024-03-14
 * @brief       LTDC (液晶控制器) 驱动代码
 ****************************************************************************************************
 */

#include "./BSP/LCD/ltdc.h"
#include "./BSP/LCD/lcd.h"
#include "./SYSTEM/delay/delay.h"

LTDC_HandleTypeDef  g_ltdc_handle;       /* LTDC 句柄 */
DMA2D_HandleTypeDef g_dma2d_handle;      /* DMA2D 句柄 */

/* 显存切换同步标志位 */
static volatile uint8_t g_wait_for_reload = 0; 

/* ---------------------------------------------------------------------------------
 * 显存缓冲区定义 (定位在外部 SDRAM)
 * ---------------------------------------------------------------------------------
 */
#if !(__ARMCC_VERSION >= 6010050)
    /* 定义两个 1024x600 的缓冲区，映射到 SDRAM */
    uint16_t ltdc_lcd_framebuf[600][1024] __attribute__((at(LTDC_FRAME_BUF_ADDR)));   
    uint16_t ltdc_lcd_framebuf1[600][1024] __attribute__((at(LTDC_FRAME_BUF1_ADDR))); 
#else
    uint16_t ltdc_lcd_framebuf[600][1024] __attribute__((section(".bss.ARM.__at_0XC0000000")));  
    uint16_t ltdc_lcd_framebuf1[600][1024] __attribute__((section(".bss.ARM.__at_0XC0258000"))); 
#endif

uint32_t *g_ltdc_framebuf[2];              
_ltdc_dev lcdltdc;                         

/**
 * @brief       LTDC 开关控制
 */
void ltdc_switch(uint8_t sw)
{
    if (sw) __HAL_LTDC_ENABLE(&g_ltdc_handle);
    else    __HAL_LTDC_DISABLE(&g_ltdc_handle);
}

/**
 * @brief       LTDC 图层显示控制
 */
void ltdc_layer_switch(uint8_t layerx, uint8_t sw)
{
    if (sw) __HAL_LTDC_LAYER_ENABLE(&g_ltdc_handle, layerx);
    else    __HAL_LTDC_LAYER_DISABLE(&g_ltdc_handle, layerx);
    __HAL_LTDC_RELOAD_IMMEDIATE_CONFIG(&g_ltdc_handle);
}

/**
 * @brief       选择图层
 */
void ltdc_select_layer(uint8_t layerx)
{
    lcdltdc.activelayer = layerx;
}

/**
 * @brief       设置显示方向
 */
void ltdc_display_dir(uint8_t dir)
{
    lcdltdc.dir = dir;
    if (dir == 0) { /* 竖屏 */
        lcdltdc.width = lcdltdc.pheight;
        lcdltdc.height = lcdltdc.pwidth;
    } else {        /* 横屏 */
        lcdltdc.width = lcdltdc.pwidth;
        lcdltdc.height = lcdltdc.pheight;
    }
}

/**
 * @brief       画点函数
 */
void ltdc_draw_point(uint16_t x, uint16_t y, uint32_t color)
{ 
    if (lcdltdc.dir == 0) /* 竖屏坐标转换 */
        *(uint16_t *)((uint32_t)g_ltdc_framebuf[lcdltdc.activelayer] + 2 * (lcdltdc.pwidth * (lcdltdc.pheight - x - 1) + y)) = color;
    else
        *(uint16_t *)((uint32_t)g_ltdc_framebuf[lcdltdc.activelayer] + 2 * (lcdltdc.pwidth * y + x)) = color;
}

/**
 * @brief       读点函数
 */
uint32_t ltdc_read_point(uint16_t x, uint16_t y)
{ 
    if (lcdltdc.dir == 0)
        return *(uint16_t *)((uint32_t)g_ltdc_framebuf[lcdltdc.activelayer] + 2 * (lcdltdc.pwidth * (lcdltdc.pheight - x - 1) + y));
    else
        return *(uint16_t *)((uint32_t)g_ltdc_framebuf[lcdltdc.activelayer] + 2 * (lcdltdc.pwidth * y + x));
}

/**
 * @brief       矩形单色填充 (使用 DMA2D 硬件加速)
 */
void ltdc_fill(uint16_t sx, uint16_t sy, uint16_t ex, uint16_t ey, uint32_t color)
{ 
    uint32_t psx, psy, pex, pey;
    if (lcdltdc.dir) { psx = sx; psy = sy; pex = ex; pey = ey; }
    else { psx = sy; psy = lcdltdc.pheight - ex - 1; pex = ey; pey = lcdltdc.pheight - sx - 1; }

    uint32_t addr = ((uint32_t)g_ltdc_framebuf[lcdltdc.activelayer] + 2 * (lcdltdc.pwidth * psy + psx));
    uint16_t width = pex - psx + 1;
    uint16_t height = pey - psy + 1;

    SCB_CleanDCache_by_Addr((uint32_t *)addr, height * lcdltdc.pwidth * 2);
    __HAL_RCC_DMA2D_CLK_ENABLE();
    DMA2D->CR = DMA2D_R2M;
    DMA2D->OPFCCR = LTDC_PIXFORMAT;
    DMA2D->OOR = lcdltdc.pwidth - width;
    DMA2D->OMAR = addr;
    DMA2D->NLR = height | (width << 16);
    DMA2D->OCOLR = color;
    DMA2D->CR |= DMA2D_CR_START;
    while ((DMA2D->ISR & DMA2D_FLAG_TC) == 0);
    DMA2D->IFCR |= DMA2D_FLAG_TC;
}

/**
 * @brief       颜色块填充 (使用 DMA2D 硬件加速)
 */
void ltdc_color_fill(uint16_t sx, uint16_t sy, uint16_t ex, uint16_t ey, uint16_t *color)
{
    uint32_t psx, psy, pex, pey;
    if (lcdltdc.dir) { psx = sx; psy = sy; pex = ex; pey = ey; }
    else { psx = sy; psy = lcdltdc.pheight - ex - 1; pex = ey; pey = lcdltdc.pheight - sx - 1; }

    uint32_t addr = ((uint32_t)g_ltdc_framebuf[lcdltdc.activelayer] + 2 * (lcdltdc.pwidth * psy + psx));
    uint16_t width = pex - psx + 1;
    uint16_t height = pey - psy + 1;

    SCB_CleanDCache_by_Addr((uint32_t *)addr, height * lcdltdc.pwidth * 2);
    __HAL_RCC_DMA2D_CLK_ENABLE();
    DMA2D->CR = DMA2D_M2M;
    DMA2D->FGPFCCR = LTDC_PIXFORMAT;
    DMA2D->FGOR = 0;
    DMA2D->OOR = lcdltdc.pwidth - width;
    DMA2D->FGMAR = (uint32_t)color;
    DMA2D->OMAR = addr;
    DMA2D->NLR = height | (width << 16);
    DMA2D->CR |= DMA2D_CR_START;
    while ((DMA2D->ISR & DMA2D_FLAG_TC) == 0);
    DMA2D->IFCR |= DMA2D_FLAG_TC;
}  

/**
 * @brief       清屏
 */
void ltdc_clear(uint32_t color)
{
    ltdc_fill(0, 0, lcdltdc.width - 1, lcdltdc.height - 1, color);
}

/**
 * @brief       时钟配置
 */
uint8_t ltdc_clk_set(uint32_t pll3n, uint32_t pll3m, uint32_t pll3r)
{
    RCC_PeriphCLKInitTypeDef periphclk_initure;
    periphclk_initure.PeriphClockSelection = RCC_PERIPHCLK_LTDC;
    periphclk_initure.PLL3.PLL3M = pll3m;
    periphclk_initure.PLL3.PLL3N = pll3n;
    periphclk_initure.PLL3.PLL3P = 2;
    periphclk_initure.PLL3.PLL3Q = 2;
    periphclk_initure.PLL3.PLL3R = pll3r;
    if (HAL_RCCEx_PeriphCLKConfig(&periphclk_initure) == HAL_OK) return 0;
    else return 1;
}

/**
 * @brief       图层参数配置
 */
void ltdc_layer_parameter_config(uint8_t layerx, uint32_t bufaddr, uint8_t pixformat, uint8_t alpha, uint8_t alpha0, uint8_t bfac1, uint8_t bfac2, uint32_t bkcolor)
{
    LTDC_LayerCfgTypeDef playercfg;
    playercfg.WindowX0 = 0;
    playercfg.WindowY0 = 0;
    playercfg.WindowX1 = lcdltdc.pwidth;
    playercfg.WindowY1 = lcdltdc.pheight;
    playercfg.PixelFormat = pixformat;
    playercfg.Alpha = alpha;
    playercfg.Alpha0 = alpha0;
    playercfg.BlendingFactor1 = (uint32_t)bfac1 << 8;
    playercfg.BlendingFactor2 = (uint32_t)bfac2;
    playercfg.FBStartAdress = bufaddr;
    playercfg.ImageWidth = lcdltdc.pwidth;
    playercfg.ImageHeight = lcdltdc.pheight;
    playercfg.Backcolor.Red = (uint8_t)(bkcolor & 0X00FF0000) >> 16;
    playercfg.Backcolor.Green = (uint8_t)(bkcolor & 0X0000FF00) >> 8;
    playercfg.Backcolor.Blue = (uint8_t)bkcolor & 0X000000FF;
    HAL_LTDC_ConfigLayer(&g_ltdc_handle, &playercfg, layerx);
}  

/**
 * @brief       LTDC 初始化
 */
void ltdc_init(void)
{
    /* 1024x600 屏幕参数配置 */
    lcdltdc.pwidth = 1024;
    lcdltdc.pheight = 600;
    lcdltdc.hsw = 20;
    lcdltdc.vsw = 3;
    lcdltdc.hbp = 140;
    lcdltdc.vbp = 20;
    lcdltdc.hfp = 160;
    lcdltdc.vfp = 12;
    ltdc_clk_set(300, 25, 6); 

    lcddev.width = lcdltdc.pwidth;
    lcddev.height = lcdltdc.pheight;
    lcdltdc.pixsize = 2; 
    g_ltdc_framebuf[0] = (uint32_t*)LTDC_FRAME_BUF_ADDR;
    g_ltdc_framebuf[1] = (uint32_t*)LTDC_FRAME_BUF1_ADDR;

    g_ltdc_handle.Instance = LTDC;
    g_ltdc_handle.Init.HSPolarity = LTDC_HSPOLARITY_AL;
    g_ltdc_handle.Init.VSPolarity = LTDC_VSPOLARITY_AL;
    g_ltdc_handle.Init.DEPolarity = LTDC_DEPOLARITY_AL;
    g_ltdc_handle.Init.PCPolarity = LTDC_PCPOLARITY_IPC;
    g_ltdc_handle.Init.HorizontalSync = lcdltdc.hsw - 1;
    g_ltdc_handle.Init.VerticalSync = lcdltdc.vsw - 1;
    g_ltdc_handle.Init.AccumulatedHBP = lcdltdc.hsw + lcdltdc.hbp - 1;
    g_ltdc_handle.Init.AccumulatedVBP = lcdltdc.vsw + lcdltdc.vbp - 1;
    g_ltdc_handle.Init.AccumulatedActiveW = lcdltdc.hsw + lcdltdc.hbp + lcdltdc.pwidth - 1;
    g_ltdc_handle.Init.AccumulatedActiveH = lcdltdc.vsw + lcdltdc.vbp + lcdltdc.pheight - 1;
    g_ltdc_handle.Init.TotalWidth = lcdltdc.hsw + lcdltdc.hbp + lcdltdc.pwidth + lcdltdc.hfp - 1;
    g_ltdc_handle.Init.TotalHeigh = lcdltdc.vsw + lcdltdc.vbp + lcdltdc.pheight + lcdltdc.vfp - 1;
    HAL_LTDC_Init(&g_ltdc_handle);

    ltdc_layer_parameter_config(0, (uint32_t)g_ltdc_framebuf[0], LTDC_PIXFORMAT, 255, 0, 6, 7, 0);
    ltdc_display_dir(1); /* 默认横屏 */
    ltdc_select_layer(0);
    
    /* 恢复引脚并开启背光 */
    LTDC_BL(1); 
    ltdc_clear(0XFFFF);  
}

/**
 * @brief       LTDC 底层引脚初始化
 */
void HAL_LTDC_MspInit(LTDC_HandleTypeDef *hltdc)
{
    GPIO_InitTypeDef gpio_init_struct;

    __HAL_RCC_LTDC_CLK_ENABLE();
    __HAL_RCC_DMA2D_CLK_ENABLE();
    __HAL_RCC_GPIOB_CLK_ENABLE();
    __HAL_RCC_GPIOF_CLK_ENABLE();
    __HAL_RCC_GPIOG_CLK_ENABLE();
    __HAL_RCC_GPIOH_CLK_ENABLE();
    __HAL_RCC_GPIOI_CLK_ENABLE();

    /* 背光 PB5 */
    gpio_init_struct.Pin = GPIO_PIN_5;
    gpio_init_struct.Mode = GPIO_MODE_OUTPUT_PP;
    gpio_init_struct.Pull = GPIO_PULLUP;
    gpio_init_struct.Speed = GPIO_SPEED_HIGH;
    HAL_GPIO_Init(GPIOB, &gpio_init_struct);

    /* LTDC 引脚配置 (AF14) */
    gpio_init_struct.Mode = GPIO_MODE_AF_PP;
    gpio_init_struct.Pull = GPIO_NOPULL;
    gpio_init_struct.Alternate = GPIO_AF14_LTDC;
    
    gpio_init_struct.Pin = GPIO_PIN_10; HAL_GPIO_Init(GPIOF, &gpio_init_struct);
    gpio_init_struct.Pin = GPIO_PIN_7 | GPIO_PIN_6 | GPIO_PIN_11; HAL_GPIO_Init(GPIOG, &gpio_init_struct);
    gpio_init_struct.Pin = GPIO_PIN_9 | GPIO_PIN_10 | GPIO_PIN_11 | GPIO_PIN_12 | GPIO_PIN_13 | GPIO_PIN_14 | GPIO_PIN_15; HAL_GPIO_Init(GPIOH, &gpio_init_struct);
    gpio_init_struct.Pin = GPIO_PIN_0 | GPIO_PIN_1 | GPIO_PIN_2 | GPIO_PIN_4 | GPIO_PIN_5 | GPIO_PIN_6 | GPIO_PIN_7 | GPIO_PIN_9 | GPIO_PIN_10; HAL_GPIO_Init(GPIOI, &gpio_init_struct);

    /* 先不开启中断，防止未定义中断服务函数导致死机 */
    // HAL_NVIC_EnableIRQ(LTDC_IRQn);
}

/**
 * @brief       显存切换
 */
void ltdc_switch_buffer(uint8_t buffer_idx)
{
    HAL_LTDC_SetAddress(&g_ltdc_handle, (uint32_t)g_ltdc_framebuf[buffer_idx], 0);
    __HAL_LTDC_RELOAD_IMMEDIATE_CONFIG(&g_ltdc_handle); /* 暂时改用立即重载 */
}
