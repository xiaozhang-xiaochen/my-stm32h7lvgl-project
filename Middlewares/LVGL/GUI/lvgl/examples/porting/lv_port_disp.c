/**
 * @file lv_port_disp.c
 *
 */

/*Copy this file as "lv_port_disp.c" and set this value to "1" to enable content*/
#if 1

/*********************
 *      INCLUDES
 *********************/
#include "lv_port_disp.h"
#include <stdbool.h>
#include "BSP/LCD/ltdc.h"
#include "BSP/LCD/lcd.h"

/*********************
 *      DEFINES
 *********************/
#ifndef MY_DISP_HOR_RES
    #define MY_DISP_HOR_RES    1024
#endif

#ifndef MY_DISP_VER_RES
    #define MY_DISP_VER_RES    600
#endif

#define BYTE_PER_PIXEL (LV_COLOR_FORMAT_GET_SIZE(LV_COLOR_FORMAT_RGB565)) /*will be 2 for RGB565 */

/**********************
 *      TYPEDEFS
 **********************/

/**********************
 *  STATIC PROTOTYPES
 **********************/
static void disp_init(void);
static void disp_flush(lv_display_t * disp, const lv_area_t * area, uint8_t * px_map);

/**********************
 *  STATIC VARIABLES
 **********************/

/**********************
 *      MACROS
 **********************/

/**********************
 *   GLOBAL FUNCTIONS
 **********************/

void lv_port_disp_init(void)
{
    /*-------------------------
     * Initialize your display
     * -----------------------*/
    disp_init();

    /*------------------------------------
     * Create a display and set a flush_cb
     * -----------------------------------*/
    lv_display_t * disp = lv_display_create(MY_DISP_HOR_RES, MY_DISP_VER_RES);
    lv_display_set_flush_cb(disp, disp_flush);

    /* 
     * STM32H743 优化方案: 
     * 使用位于 SDRAM 的全屏双缓冲区，开启 DIRECT 模式实现零拷贝。
     * 显存地址由 ltdc.h 中的 LTDC_FRAME_BUF_ADDR 和 LTDC_FRAME_BUF1_ADDR 定义。
     */
    lv_display_set_buffers(disp, (void*)LTDC_FRAME_BUF_ADDR, (void*)LTDC_FRAME_BUF1_ADDR, 
                           MY_DISP_HOR_RES * MY_DISP_VER_RES * BYTE_PER_PIXEL, 
                           LV_DISPLAY_RENDER_MODE_DIRECT);
}

/**********************
 *   STATIC FUNCTIONS
 **********************/

/*Initialize your display and the required peripherals.*/
static void disp_init(void)
{
    /* 初始化 LTDC 及其外设 */
    ltdc_init();
}

volatile bool disp_flush_enabled = true;

/* Enable updating the screen (the flushing process) when disp_flush() is called by LVGL
 */
void disp_enable_update(void)
{
    disp_flush_enabled = true;
}

/* Disable updating the screen (the flushing process) when disp_flush() is called by LVGL
 */
void disp_disable_update(void)
{
    disp_flush_enabled = false;
}

/*Flush the content of the internal buffer the specific area on the display.
 *`px_map` contains the rendered image as raw pixel map and it should be copied to `area` on the display.
 *'lv_display_flush_ready()' has to be called when it's finished.*/
static void disp_flush(lv_display_t * disp_drv, const lv_area_t * area, uint8_t * px_map)
{
    if(disp_flush_enabled) {
        /* 
         * 优化：仅针对修改区域进行 Cache 刷入，避免全屏刷新导致的高开销。
         * 计算当前刷新区域的字节数并按行进行清理，或者如果区域较大则简单处理。
         */
        int32_t width = lv_area_get_width(area);
        int32_t height = lv_area_get_height(area);
        
        for(int32_t y = area->y1; y <= area->y2; y++) {
            uint16_t * line_start = (uint16_t *)px_map + (y * MY_DISP_HOR_RES) + area->x1;
            SCB_CleanDCache_by_Addr((uint32_t *)line_start, width * 2);
        }

        if(lv_display_flush_is_last(disp_drv)) {
            /* 切换 LTDC 显示缓冲区 (该函数内部已实现 VSYNC 同步) */
            if((uint32_t)px_map == LTDC_FRAME_BUF_ADDR) {
                ltdc_switch_buffer(0);
            } else {
                ltdc_switch_buffer(1);
            }
        }
    }

    /* 重要：通知 LVGL 刷新已完成 */
    lv_display_flush_ready(disp_drv);
}

#else /*Enable this file at the top*/

/*This dummy typedef exists purely to silence -Wpedantic.*/
typedef int keep_pedantic_happy;
#endif
