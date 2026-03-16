/**

 ****************************************************************************************************

 * @file        nandtester.c

 * @author      正点原子团队(ALIENTEK)

 * @version     V1.0

 * @date        2022-09-06

 * @brief       NAND FLASH 测试代码

 ****************************************************************************************************

 */



#include "string.h"

#include "./BSP/NAND/ftl.h"

#include "./BSP/NAND/nand.h"

#include "FreeRTOS.h"

#include "task.h"



/* 兼容宏：将正点原子的内存管理重定向到 FreeRTOS */

#define mymalloc(x, size)   pvPortMalloc(size)

#define myfree(x, ptr)      vPortFree(ptr)

#define SRAMIN 0 



#include "./SYSTEM/usart/usart.h"

#include "./BSP/NAND/nandtester.h"





/**

 * @brief       向 NAND 某一页写入指定大小的数据

 * @param       pagenum: 页面地址

 * @param       colnum: 页面内偏移地址

 * @param       writebytes: 写入字节数

 * @retval      0,成功; 其他,失败

 */

uint8_t test_writepage(uint32_t pagenum, uint16_t colnum, uint16_t writebytes)

{

    uint8_t *pbuf;

    uint8_t sta = 0;

    uint16_t i = 0;

    pbuf = mymalloc(SRAMIN, 5000);

    if (pbuf == NULL) return 1;



    for (i = 0; i < writebytes; i++)

    { 

        pbuf[i] = i;

    }



    sta = nand_writepage(pagenum, colnum, pbuf, writebytes);

    myfree(SRAMIN, pbuf);



    return sta;

}



/**

 * @brief       读取 NAND 某一页指定大小的数据

 */

uint8_t test_readpage(uint32_t pagenum, uint16_t colnum, uint16_t readbytes)

{

    uint8_t *pbuf;

    uint8_t sta = 0;

    uint16_t i = 0;



    pbuf = mymalloc(SRAMIN, 5000);

    if (pbuf == NULL) return 1;

    

    sta = nand_readpage(pagenum, colnum, pbuf, readbytes);



    if (sta == 0 || sta == NSTA_ECC1BITERR || sta == NSTA_ECC2BITERR)

    {

        printf("read page data is:\r\n");

        for (i = 0; i < readbytes; i++)

        {

            printf("%x ", pbuf[i]);

        }

        printf("\r\nend\r\n");

    }



    myfree(SRAMIN, pbuf);

    return sta;

}



/**

 * @brief       复制页面并写入数据

 */

uint8_t test_copypageandwrite(uint32_t spnum, uint32_t dpnum, uint16_t colnum, uint16_t writebytes)

{

    uint8_t *pbuf;

    uint8_t sta = 0;

    uint16_t i = 0;

    pbuf = mymalloc(SRAMIN, 5000);

    if (pbuf == NULL) return 1;



    for (i = 0; i < writebytes; i++)

    { 

        pbuf[i] = i + 0X80;

    }



    sta = nand_copypage_withwrite(spnum, dpnum, colnum, pbuf, writebytes);

    myfree(SRAMIN, pbuf);



    return sta;

}

 

/**

 * @brief       读取 Spare 区数据

 */

uint8_t test_readspare(uint32_t pagenum, uint16_t colnum, uint16_t readbytes)

{

    uint8_t *pbuf;

    uint8_t sta = 0;

    uint16_t i = 0;

    pbuf = mymalloc(SRAMIN, 512);

    if (pbuf == NULL) return 1;

    

    sta = nand_readspare(pagenum, colnum, pbuf, readbytes);



    if (sta == 0)

    { 

        printf("read spare data is:\r\n");

        for (i = 0; i < readbytes; i++)

        { 

            printf("%x ", pbuf[i]);

        }

        printf("\r\nend\r\n");

    }



    myfree(SRAMIN, pbuf);

    return sta;

}



/**

 * @brief       读取所有块的信息

 */

void test_readallblockinfo(uint32_t sblock)

{

    uint8_t j = 0;

    uint32_t i = 0;

    uint8_t sta;

    uint8_t buffer[5];



    for (i = sblock; i < nand_dev.block_totalnum; i++)

    {

        printf("block %d info:", i);

        sta = nand_readspare(i * nand_dev.block_pagenum, 0, buffer, 5);

        if (sta) printf("failed\r\n");

        for (j = 0; j < 5; j++)

        {

            printf("%x ", buffer[j]);

        }

        printf("\r\n");

    }

}



/******************************************************************************************/

/* FTL 测试函数 */



/**

 * @brief       FTL 写扇区测试

 */

uint8_t test_ftlwritesectors(uint32_t secx, uint16_t secsize, uint16_t seccnt)

{

    uint8_t *pbuf;

    uint8_t sta = 0;

    uint32_t i = 0;

    pbuf = mymalloc(SRAMIN, secsize * seccnt);

    if (pbuf == NULL) return 1;



    for (i = 0; i < secsize * seccnt; i++)

    { 

        pbuf[i] = i;

    }



    sta = ftl_write_sectors(pbuf, secx, secsize, seccnt);

    myfree(SRAMIN, pbuf);



    return sta;

}



/**

 * @brief       FTL 读扇区测试

 */

uint8_t test_ftlreadsectors(uint32_t secx, uint16_t secsize, uint16_t seccnt)

{

    uint8_t *pbuf;

    uint8_t sta = 0;

    uint32_t i = 0;



    pbuf = mymalloc(SRAMIN, secsize * seccnt);   

    if (pbuf == NULL) return 1;

    

    sta = ftl_read_sectors(pbuf, secx, secsize, seccnt);



    if (sta == 0)

    {

        printf("read sec %d data is:\r\n", secx); 

        for (i = 0; i < secsize * seccnt; i++)

        { 

            printf("%x ",pbuf[i]);

        }

        printf("\r\nend\r\n");

    }



    myfree(SRAMIN, pbuf);

    return sta;

}

