#include "malloc.h"

/**
 ****************************************************************************************************
 * @file        malloc.c
 * @author      ALIENTEK
 * @version     V1.0
 * @date        2022-09-06
 * @brief       内存管理 桩代码 (切换至 FreeRTOS 堆管理)
 ****************************************************************************************************
 */

/* 内存管理初始化 */
void my_mem_init(uint8_t memx)
{
    /* 空函数 */
}

/* 获取内存使用率 */
uint16_t my_mem_perused(uint8_t memx)
{
    return 0;
}

/* 内存申请(内部调用) */
uint32_t my_mem_malloc(uint8_t memx, uint32_t size)
{
    return 0xFFFFFFFF;
}

/* 内存释放(内部调用) */
uint8_t my_mem_free(uint8_t memx, uint32_t offset)
{
    return 0;
}

/* 内存释放(外部调用) */
void myfree(uint8_t memx, void *ptr)
{
    /* 空函数 */
}

/* 内存申请(外部调用) */
void *mymalloc(uint8_t memx, uint32_t size)
{
    return (void *)0;
}

/* 重新分配内存(外部调用) */
void *myrealloc(uint8_t memx, void *ptr, uint32_t size)
{
    return (void *)0;
}

/* 内存设置 */
void my_mem_set(void *s, uint8_t c, uint32_t count)
{
    uint8_t *xs = s;
    while (count--) *xs++ = c;
}

/* 内存复制 */
void my_mem_copy(void *des, void *src, uint32_t n)
{
    uint8_t *xdes = des;
    uint8_t *xsrc = src;
    while (n--) *xdes++ = *xsrc++;
}

/* 结构体定义，为了兼容性保留 */
struct _m_mallco_dev mallco_dev =
{
    my_mem_init,
    my_mem_perused,
    {0},    /* membase */
    {0},    /* memmap */
    {0},    /* memrdy */
};
