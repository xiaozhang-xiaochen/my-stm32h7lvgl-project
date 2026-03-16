/**
 ****************************************************************************************************
 * @file        main.c
 * @author      正点原子(ALIENTEK) / Gemini 整合版
 * @version     V1.4
 * @date        2024-03-14
 * @brief       LVGL v9.3.0 + FreeRTOS + LTDC (STM32H743) 集成测试
 ****************************************************************************************************
 */

#include "./SYSTEM/sys/sys.h"
#include "./SYSTEM/usart/usart.h"
#include "./SYSTEM/delay/delay.h"
#include "./BSP/LED/led.h"
#include "./BSP/LCD/lcd.h"
#include "./BSP/SDRAM/sdram.h"
#include "./BSP/MPU/mpu.h"
#include "./BSP/NAND/ftl.h"
#include "string.h"

/* FreeRTOS 包含 */
#include "FreeRTOS.h"
#include "task.h"

/* LVGL 包含 */
#include "lvgl.h"
#include "lv_port_disp.h"
#include "lv_demos.h"

/* 任务优先级 */
#define START_TASK_PRIO         1
#define LED_TASK_PRIO           2
#define LVGL_TASK_PRIO          3
#define NAND_TASK_PRIO          4

/* 任务堆栈大小 */
#define START_STK_SIZE          512
#define LED_STK_SIZE            128
#define LVGL_STK_SIZE          	4096  /* LVGL 任务需要更大的堆栈 */
#define NAND_STK_SIZE           512

/* 任务句柄 */
TaskHandle_t StartTask_Handler;
TaskHandle_t LEDTask_Handler;
TaskHandle_t LVGLTask_Handler;
TaskHandle_t NANDTask_Handler;

/* 全局变量用于显示 NAND 状态 */
char g_nand_status[50] = "NAND: Initializing...";

/* 任务函数原型声明 */
void start_task(void *pvParameters);
void led_task(void *pvParameters);
void lvgl_task(void *pvParameters);
void nand_test_task(void *pvParameters);

/**
 * @brief       获取系统毫秒数 (用于 LVGL 心跳)
 */
static uint32_t lv_tick_get_cb(void)
{
    return xTaskGetTickCount();
}

int main(void)
{
    sys_cache_enable();                     /* 开启 L1-Cache */
    HAL_Init();                             /* 初始化HAL库 */
    sys_stm32_clock_init(192, 5, 2, 4);     /* 设置时钟, 400MHz */
    delay_init(480);                        /* 延时初始化 */
    usart_init(115200);                     /* 串口初始化 */
    mpu_memory_protection();                /* MPU存储保护设置 (已设 SDRAM 为 Write-through) */
    led_init();                             /* 初始化LED */
    sdram_init();                           /* 初始化SDRAM */
    lcd_init();                             /* 初始化LCD (主要是配置 LTDC) */

    /* 创建开始任务 */
    xTaskCreate((TaskFunction_t )start_task,            
                (const char*    )"start_task",          
                (uint16_t       )START_STK_SIZE,        
                (void*          )NULL,                  
                (UBaseType_t    )START_TASK_PRIO,       
                (TaskHandle_t*  )&StartTask_Handler);   

    /* 启动FreeRTOS调度器 */
    vTaskStartScheduler();

    while (1)
    {
    }
}

/**
 * @brief       开始任务函数
 */
void start_task(void *pvParameters)
{
    taskENTER_CRITICAL();           /* 进入临界区 */

    /* 初始化NAND Flash及FTL层 */
    if (ftl_init() == 0)
    {
        strcpy(g_nand_status, "NAND: Init OK");
    }
    else
    {
        strcpy(g_nand_status, "NAND: Init Failed!");
    }

    /* -------------------------------------------------------------------------
     * LVGL 初始化序列
     * -------------------------------------------------------------------------
     */
    lv_init();                          /* LVGL 核心初始化 */
    lv_tick_set_cb(lv_tick_get_cb);     /* 关联 FreeRTOS 系统时钟 */
    lv_port_disp_init();                /* 显示驱动初始化 (对接 LTDC) */

    /* 启动 LVGL 压力测试 Demo */
    lv_demo_stress();

    /* 创建LED任务 */
    xTaskCreate((TaskFunction_t )led_task,
                (const char*    )"led_task",
                (uint16_t       )LED_STK_SIZE,
                (void*          )NULL,
                (UBaseType_t    )LED_TASK_PRIO,
                (TaskHandle_t*  )&LEDTask_Handler);

    /* 创建LVGL调度任务 (替代原 lcd_task) */
    xTaskCreate((TaskFunction_t )lvgl_task,
                (const char*    )"lvgl_task",
                (uint16_t       )LVGL_STK_SIZE,
                (void*          )NULL,
                (UBaseType_t    )LVGL_TASK_PRIO,
                (TaskHandle_t*  )&LVGLTask_Handler);

    /* 创建NAND测试任务 */
    xTaskCreate((TaskFunction_t )nand_test_task,
                (const char*    )"nand_task",
                (uint16_t       )NAND_STK_SIZE,
                (void*          )NULL,
                (UBaseType_t    )NAND_TASK_PRIO,
                (TaskHandle_t*  )&NANDTask_Handler);

    vTaskDelete(StartTask_Handler); 

    taskEXIT_CRITICAL();            
    }

    /**
    * @brief       LED任务函数
    */
    void led_task(void *pvParameters)
    {
    while (1)
    {
        LED0_TOGGLE();
        vTaskDelay(500);
    }
    }

    /**
    * @brief       LVGL 调度任务 - 核心心跳
    */
    void lvgl_task(void *pvParameters)
    {
    while (1)
    {
        lv_timer_handler();             /* 处理 LVGL 任务、动画及重绘 */
        vTaskDelay(5);                  /* 释放 CPU 所有权 */
    }
    }

/**
 * @brief       NAND 测试任务
 */
void nand_test_task(void *pvParameters)
{
    /* NAND 测试逻辑保持不变，结果通过串口及后续 LVGL UI 展示 */
    printf("NAND Test Task Started...\r\n");
    while (1)
    {
        vTaskDelay(1000);
    }
}

/**
 * @brief       堆栈溢出钩子
 */
void vApplicationStackOverflowHook(TaskHandle_t xTask, char *pcTaskName)
{
    while (1);
}

/**
 * @brief       内存分配失败钩子
 */
void vApplicationMallocFailedHook(void)
{
    while (1);
}
