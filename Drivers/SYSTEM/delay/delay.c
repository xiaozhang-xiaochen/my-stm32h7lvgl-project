/**
 ****************************************************************************************************
 * @file        delay.c
 * @author      正点原子(ALIENTEK)
 * @version     V1.2
 * @date        2024-03-01
 * @brief       FreeRTOS 延时函数实现
 ****************************************************************************************************
 */

#include "./SYSTEM/sys/sys.h"
#include "./SYSTEM/delay/delay.h"

static uint32_t g_fac_us = 0;       /* us延时倍乘数 */

#if SYS_SUPPORT_OS
#include "FreeRTOS.h"
#include "task.h"

extern void xPortSysTickHandler(void);

/**
 * @brief     SysTick中断服务函数
 */
void SysTick_Handler(void)
{
    HAL_IncTick();                  /* HAL库时基自增 */
    if (xTaskGetSchedulerState() != taskSCHEDULER_NOT_STARTED) /* 如果OS已启动 */
    {
        xPortSysTickHandler();      /* 调用FreeRTOS节拍处理 */
    }
}
#endif

/**
 * @brief     初始化延时函数
 * @param     sysclk: 系统时钟频率 (MHz)
 */
void delay_init(uint16_t sysclk)
{
    g_fac_us = sysclk;              /* 保存每微秒的计数值 */
}

/**
 * @brief     延时nus
 * @param     nus: 要延时的微秒数
 */
void delay_us(uint32_t nus)
{
    uint32_t ticks;
    uint32_t told, tnow, tcnt = 0;
    uint32_t reload = SysTick->LOAD;        /* LOAD的值 */
    ticks = nus * g_fac_us;                 /* 需要的节拍数 */

    told = SysTick->VAL;                    /* 刚进入时的计数器值 */
    while (1)
    {
        tnow = SysTick->VAL;
        if (tnow != told)
        {
            if (tnow < told)
            {
                tcnt += told - tnow;        /* SYSTICK是一个递减的计数器 */
            }
            else
            {
                tcnt += reload - tnow + told;
            }
            told = tnow;
            if (tcnt >= ticks) 
            {
                break;                      /* 时间超过/等于要延迟的时间,则退出 */
            }
        }
    }
}

/**
 * @brief     延时nms
 * @param     nms: 要延时的毫秒数
 */
void delay_ms(uint16_t nms)
{
#if SYS_SUPPORT_OS
    /* 如果OS正在运行, 并且不在中断中, 则调用OS延时以释放CPU */
    if (xTaskGetSchedulerState() != taskSCHEDULER_NOT_STARTED && !(__get_IPSR()))
    {
        if (nms >= (1000 / configTICK_RATE_HZ))         /* 延时时间大于一个OS节拍 */
        {
            vTaskDelay(nms / (1000 / configTICK_RATE_HZ)); /* FreeRTOS延时 */
        }
        nms %= (1000 / configTICK_RATE_HZ);             /* OS无法提供的极小延时, 后面通过普通方式补齐 */
    }
#endif

    delay_us((uint32_t)(nms * 1000));                   /* 普通方式延时 */
}

/**
 * @brief       HAL库内部调用的延时函数
 * @param       Delay : 要延时的毫秒数
 */
void HAL_Delay(uint32_t Delay)
{
     delay_ms(Delay);
}
