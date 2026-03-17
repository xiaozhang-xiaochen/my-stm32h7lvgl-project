/*-----------------------------------------------------------------------*/
/* Low level disk I/O module SKELETON for FatFs     (C)ChaN, 2019        */
/*-----------------------------------------------------------------------*/

#include "ff.h"			/* Obtains integer types */
#include "diskio.h"		/* Declarations of disk functions */
#include "./BSP/NAND/ftl.h"
#include "./BSP/NAND/nand.h"
#include "./SYSTEM/sys/sys.h"

/* Definitions of physical drive number for each drive */
#define DEV_NAND	0	/* Map NAND to physical drive 0 */


/*-----------------------------------------------------------------------*/
/* Get Drive Status                                                      */
/*-----------------------------------------------------------------------*/

DSTATUS disk_status (
	BYTE pdrv		/* Physical drive nmuber to identify the drive */
)
{
	if (pdrv == DEV_NAND) {
		return 0; // Assumed initialized if we reach here without error
	}
	return STA_NOINIT;
}



/*-----------------------------------------------------------------------*/
/* Inidialize a Drive                                                    */
/*-----------------------------------------------------------------------*/

DSTATUS disk_initialize (
	BYTE pdrv				/* Physical drive nmuber to identify the drive */
)
{
	if (pdrv == DEV_NAND) {
		if (ftl_init() == 0) {
			return 0;
		} else {
			return STA_NOINIT;
		}
	}
	return STA_NOINIT;
}



/*-----------------------------------------------------------------------*/
/* Read Sector(s)                                                        */
/*-----------------------------------------------------------------------*/

DRESULT disk_read (
	BYTE pdrv,		/* Physical drive nmuber to identify the drive */
	BYTE *buff,		/* Data buffer to store read data */
	LBA_t sector,	/* Start sector in LBA */
	UINT count		/* Number of sectors to read */
)
{
	if (pdrv == DEV_NAND) {
		if (ftl_read_sectors(buff, sector, 512, count) == 0) {
            // 注意: 如果FTL使用DMA并且buff在D-Cache区域，此处需要调用 SCB_InvalidateDCache_by_Addr
			return RES_OK;
		} else {
			return RES_ERROR;
		}
	}
	return RES_PARERR;
}



/*-----------------------------------------------------------------------*/
/* Write Sector(s)                                                       */
/*-----------------------------------------------------------------------*/

#if FF_FS_READONLY == 0

DRESULT disk_write (
	BYTE pdrv,			/* Physical drive nmuber to identify the drive */
	const BYTE *buff,	/* Data to be written */
	LBA_t sector,		/* Start sector in LBA */
	UINT count			/* Number of sectors to write */
)
{
	if (pdrv == DEV_NAND) {
        // 注意: 如果FTL使用DMA并且buff在D-Cache区域，此处需要调用 SCB_CleanDCache_by_Addr
		if (ftl_write_sectors((uint8_t*)buff, sector, 512, count) == 0) {
			return RES_OK;
		} else {
			return RES_ERROR;
		}
	}
	return RES_PARERR;
}

#endif


/*-----------------------------------------------------------------------*/
/* Miscellaneous Functions                                               */
/*-----------------------------------------------------------------------*/

DRESULT disk_ioctl (
	BYTE pdrv,		/* Physical drive nmuber (0..) */
	BYTE cmd,		/* Control code */
	void *buff		/* Buffer to send/receive control data */
)
{
	DRESULT res = RES_PARERR;

	if (pdrv == DEV_NAND) {
		switch (cmd) {
		case CTRL_SYNC:
			res = RES_OK;
			break;
		case GET_SECTOR_COUNT:
            /* 获取扇区数量: 有效块数 * 每块页数 * 每页有效字节数 / 512 */
			*(DWORD*)buff = ((DWORD)nand_dev.valid_blocknum * nand_dev.block_pagenum * nand_dev.page_mainsize) / 512;
			res = RES_OK;
			break;
		case GET_SECTOR_SIZE:
			*(WORD*)buff = 512;
			res = RES_OK;
			break;
		case GET_BLOCK_SIZE:
			*(DWORD*)buff = 1;
			res = RES_OK;
			break;
		default:
			res = RES_PARERR;
			break;
		}
	}

	return res;
}
