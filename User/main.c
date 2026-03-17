/**
 ****************************************************************************************************
 * @file        main.c
 * @author      正点原子(ALIENTEK) / Gemini 整合版
 * @version     V1.5
 * @date        2024-03-17
 * @brief       LVGL v9.x + FatFs + NAND 集成验证版
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
#include "ff.h"

/* FreeRTOS 包含 */
#include "FreeRTOS.h"
#include "task.h"

/* LVGL 包含 */
#include "lvgl.h"
#include "lv_port_disp.h"
#include "lv_port_fs.h" /* 引入我们刚才写的 FS 端口 */
#include "lv_demos.h"

/* 任务优先级 */
#define START_TASK_PRIO         1
#define LED_TASK_PRIO           2
#define LVGL_TASK_PRIO          3
#define NAND_TASK_PRIO          4

/* 任务堆栈大小 */
#define START_STK_SIZE          1024
#define LED_STK_SIZE            128
#define LVGL_STK_SIZE          	4096 
#define NAND_STK_SIZE           2048 /* 进一步增加堆栈以支持双重测试 */

/* 任务句柄 */
TaskHandle_t StartTask_Handler;
TaskHandle_t LEDTask_Handler;
TaskHandle_t LVGLTask_Handler;
TaskHandle_t NANDTask_Handler;

/* 全局变量 */
FATFS g_nand_fs; /* 全局文件系统对象 */
char g_nand_status[50] = "NAND: Initializing...";

/* 任务函数原型声明 */
void start_task(void *pvParameters);
void led_task(void *pvParameters);
void lvgl_task(void *pvParameters);
void nand_test_task(void *pvParameters);
void lv_fs_test(void); /* LVGL 文件系统验证函数 */

/**
 * @brief       获取系统毫秒数 (用于 LVGL 心跳)
 */
static uint32_t lv_tick_get_cb(void)
{
    return xTaskGetTickCount();
}

int main(void)
{
    sys_cache_enable();                     
    HAL_Init();                             
    sys_stm32_clock_init(192, 5, 2, 4);     
    delay_init(480);                        
    usart_init(115200);                     
    mpu_memory_protection();                
    led_init();                             
    sdram_init();                           
    lcd_init();                             

    /* 创建开始任务 */
    xTaskCreate((TaskFunction_t )start_task,            
                (const char*    )"start_task",          
                (uint16_t       )START_STK_SIZE,        
                (void*          )NULL,                  
                (UBaseType_t    )START_TASK_PRIO,       
                (TaskHandle_t*  )&StartTask_Handler);   

    vTaskStartScheduler();

    while (1);
}

void start_task(void *pvParameters)
{
    taskENTER_CRITICAL();           

    /* 1. 硬件层初始化 */
    if (ftl_init() == 0) {
        /* 2. 挂载文件系统 (必须在 lv_port_fs_init 之前或过程中确保挂载) */
        if (f_mount(&g_nand_fs, "0:", 1) == FR_OK) {
            strcpy(g_nand_status, "NAND & FatFs: OK");
        } else {
            strcpy(g_nand_status, "FatFs: Mount Failed");
        }
    } else {
        strcpy(g_nand_status, "NAND: Init Failed!");
    }

    /* 3. LVGL 核心初始化 */
    lv_init();                          
    lv_tick_set_cb(lv_tick_get_cb);     
    lv_port_disp_init();                
    
    /* 4. LVGL 文件系统驱动初始化 */
    lv_port_fs_init(); 

    lv_demo_stress();

    /* 创建任务 */
    xTaskCreate((TaskFunction_t )led_task, "led_task", LED_STK_SIZE, NULL, LED_TASK_PRIO, &LEDTask_Handler);
    xTaskCreate((TaskFunction_t )lvgl_task, "lvgl_task", LVGL_STK_SIZE, NULL, LVGL_TASK_PRIO, &LVGLTask_Handler);
    xTaskCreate((TaskFunction_t )nand_test_task, "nand_task", NAND_STK_SIZE, NULL, NAND_TASK_PRIO, &NANDTask_Handler);

    vTaskDelete(StartTask_Handler); 
    taskEXIT_CRITICAL();            
}

void led_task(void *pvParameters)
{
    while (1) {
        LED0_TOGGLE();
        vTaskDelay(500);
    }
}

void lvgl_task(void *pvParameters)
{
    while (1) {
        lv_timer_handler();             
        vTaskDelay(5);                  
    }
}

/**
 * @brief       LVGL 文件系统 API 验证测试
 */
void lv_fs_test(void)
{
    lv_fs_file_t f;
    lv_fs_res_t res;
    uint32_t bw, br;
    const char * test_msg = "LVGL FS Bridge Test Successful!";
    char read_buf[64] = {0};

    printf("\r\n[LVGL-FS] Starting Bridge Test (Drive S:)...\r\n");

    /* 写入测试: S: 映射到 0: */
    res = lv_fs_open(&f, "S:/lv_test.txt", LV_FS_MODE_WR);
    if(res != LV_FS_RES_OK) {
        printf("[LVGL-FS] Error: Open for write failed (%d)\r\n", res);
        return;
    }
    
    lv_fs_write(&f, test_msg, strlen(test_msg), &bw);
    lv_fs_close(&f);
    printf("[LVGL-FS] Write completed.\r\n");

    /* 读取测试 */
    res = lv_fs_open(&f, "S:/lv_test.txt", LV_FS_MODE_RD);
    if(res != LV_FS_RES_OK) {
        printf("[LVGL-FS] Error: Open for read failed (%d)\r\n", res);
        return;
    }

    res = lv_fs_read(&f, read_buf, sizeof(read_buf)-1, &br);
    if(res == LV_FS_RES_OK) {
        printf("[LVGL-FS] Read Data: %s\r\n", read_buf);
        if(strcmp(read_buf, test_msg) == 0) {
            printf("[LVGL-FS] Verification: PASSED! Drive 'S' is working.\r\n");
        } else {
            printf("[LVGL-FS] Verification: FAILED! Content mismatch.\r\n");
        }
    }
    lv_fs_close(&f);
}

void nand_test_task(void *pvParameters)
{
    /* 等待系统稳定 */
    vTaskDelay(1000);

    printf("\r\n--- Combined NAND & FileSystem Test ---\r\n");

    /* 1. 先进行原生 FatFs 测试 */
    FIL file;
    FRESULT res;
    UINT bw;
    printf("[FatFs] Internal Write Test...\r\n");
    res = f_open(&file, "0:/sys_boot.log", FA_CREATE_ALWAYS | FA_WRITE);
    if (res == FR_OK) {
        f_write(&file, "System Boot OK\n", 15, &bw);
        f_close(&file);
        printf("[FatFs] Internal Test OK.\r\n");
    } else {
        printf("[FatFs] Internal Test Failed (%d). Check if NAND is formatted!\r\n", res);
    }

    /* 2. 执行 LVGL 文件系统桥接测试 */
    lv_fs_test();

    printf("--- Test Sequence Completed ---\r\n");

    vTaskDelete(NULL); 
}

void vApplicationStackOverflowHook(TaskHandle_t xTask, char *pcTaskName) { while (1); }
void vApplicationMallocFailedHook(void) { while (1); }
