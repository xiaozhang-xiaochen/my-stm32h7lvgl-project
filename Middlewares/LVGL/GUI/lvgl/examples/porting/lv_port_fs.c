/**
 * @file lv_port_fs.c
 * @brief LVGL 文件系统接口对接 FatFs (NAND Flash)
 */

#include "lv_port_fs.h"
#include "../../lvgl.h"
#include "ff.h"
#include <stdio.h>
#include <string.h>

/**********************
 *  STATIC PROTOTYPES
 **********************/
static void fs_init(void);

static void * fs_open(lv_fs_drv_t * drv, const char * path, lv_fs_mode_t mode);
static lv_fs_res_t fs_close(lv_fs_drv_t * drv, void * file_p);
static lv_fs_res_t fs_read(lv_fs_drv_t * drv, void * file_p, void * buf, uint32_t btr, uint32_t * br);
static lv_fs_res_t fs_write(lv_fs_drv_t * drv, void * file_p, const void * buf, uint32_t btw, uint32_t * bw);
static lv_fs_res_t fs_seek(lv_fs_drv_t * drv, void * file_p, uint32_t pos, lv_fs_whence_t whence);
static lv_fs_res_t fs_tell(lv_fs_drv_t * drv, void * file_p, uint32_t * pos_p);

static void * fs_dir_open(lv_fs_drv_t * drv, const char * path);
static lv_fs_res_t fs_dir_read(lv_fs_drv_t * drv, void * rddir_p, char * fn, uint32_t fn_len);
static lv_fs_res_t fs_dir_close(lv_fs_drv_t * drv, void * rddir_p);

/**********************
 *   GLOBAL FUNCTIONS
 **********************/

void lv_port_fs_init(void)
{
    /* 初始化存储设备已经在 main.c 中完成 (ftl_init + f_mount) */
    fs_init();

    /* 注册 LVGL 文件系统驱动 */
    static lv_fs_drv_t fs_drv;
    lv_fs_drv_init(&fs_drv);

    /* 设置盘符为 'S' (Storage) */
    fs_drv.letter = 'S';
    fs_drv.open_cb = fs_open;
    fs_drv.close_cb = fs_close;
    fs_drv.read_cb = fs_read;
    fs_drv.write_cb = fs_write;
    fs_drv.seek_cb = fs_seek;
    fs_drv.tell_cb = fs_tell;

    fs_drv.dir_close_cb = fs_dir_close;
    fs_drv.dir_open_cb = fs_dir_open;
    fs_drv.dir_read_cb = fs_dir_read;

    lv_fs_drv_register(&fs_drv);
}

/**********************
 *   STATIC FUNCTIONS
 **********************/

/* 初始化存储设备 */
static void fs_init(void)
{
    /* 已经在 main.c 的 ftl_init() 中完成 */
}

/**
 * 打开文件
 */
static void * fs_open(lv_fs_drv_t * drv, const char * path, lv_fs_mode_t mode)
{
    FRESULT res;
    BYTE ff_mode = 0;

    if(mode == LV_FS_MODE_WR) ff_mode = FA_WRITE | FA_CREATE_ALWAYS;
    else if(mode == LV_FS_MODE_RD) ff_mode = FA_READ;
    else if(mode == (LV_FS_MODE_WR | LV_FS_MODE_RD)) ff_mode = FA_READ | FA_WRITE | FA_OPEN_ALWAYS;

    /* FatFs 内部盘符是 "0:"，我们需要把 LVGL 传入的路径补全盘符 */
    /* 注意: LVGL 传入的 path 已经去掉了盘符字母 'S'，例如 "/test.txt" */
    char full_path[256];
    sprintf(full_path, "0:%s", path);

    FIL * fp = lv_malloc(sizeof(FIL)); /* 使用 LVGL 内存管理 */
    if(fp == NULL) return NULL;

    res = f_open(fp, full_path, ff_mode);
    if(res == FR_OK) {
        return fp;
    } else {
        lv_free(fp);
        return NULL;
    }
}

/**
 * 关闭文件
 */
static lv_fs_res_t fs_close(lv_fs_drv_t * drv, void * file_p)
{
    f_close((FIL *)file_p);
    lv_free(file_p);
    return LV_FS_RES_OK;
}

/**
 * 读取文件
 */
static lv_fs_res_t fs_read(lv_fs_drv_t * drv, void * file_p, void * buf, uint32_t btr, uint32_t * br)
{
    FRESULT res = f_read((FIL *)file_p, buf, btr, (UINT *)br);
    if(res == FR_OK) return LV_FS_RES_OK;
    else return LV_FS_RES_HW_ERR;
}

/**
 * 写入文件
 */
static lv_fs_res_t fs_write(lv_fs_drv_t * drv, void * file_p, const void * buf, uint32_t btw, uint32_t * bw)
{
    FRESULT res = f_write((FIL *)file_p, buf, btw, (UINT *)bw);
    if(res == FR_OK) return LV_FS_RES_OK;
    else return LV_FS_RES_HW_ERR;
}

/**
 * 文件寻址 (LSEEK)
 */
static lv_fs_res_t fs_seek(lv_fs_drv_t * drv, void * file_p, uint32_t pos, lv_fs_whence_t whence)
{
    FSIZE_t offset = pos;
    FIL * fp = (FIL *)file_p;

    if(whence == LV_FS_SEEK_SET) {
        offset = pos;
    } else if(whence == LV_FS_SEEK_CUR) {
        offset = f_tell(fp) + pos;
    } else if(whence == LV_FS_SEEK_END) {
        offset = f_size(fp) + pos;
    }

    if(f_lseek(fp, offset) == FR_OK) return LV_FS_RES_OK;
    else return LV_FS_RES_HW_ERR;
}

/**
 * 获取当前位置
 */
static lv_fs_res_t fs_tell(lv_fs_drv_t * drv, void * file_p, uint32_t * pos_p)
{
    *pos_p = f_tell((FIL *)file_p);
    return LV_FS_RES_OK;
}

/**
 * 打开目录
 */
static void * fs_dir_open(lv_fs_drv_t * drv, const char * path)
{
    DIR * dp = lv_malloc(sizeof(DIR));
    if(dp == NULL) return NULL;

    char full_path[256];
    sprintf(full_path, "0:%s", path);

    if(f_opendir(dp, full_path) == FR_OK) return dp;
    else {
        lv_free(dp);
        return NULL;
    }
}

/**
 * 读取目录项
 */
static lv_fs_res_t fs_dir_read(lv_fs_drv_t * drv, void * rddir_p, char * fn, uint32_t fn_len)
{
    FRESULT res;
    FILINFO fno;
    DIR * dp = (DIR *)rddir_p;

    res = f_readdir(dp, &fno);
    if(res != FR_OK || fno.fname[0] == 0) return LV_FS_RES_UNKNOWN;

    /* 将文件名拷贝到输出缓存，目录名以 '/' 开头 */
    if(fno.fattrib & AM_DIR) {
        fn[0] = '/';
        strncpy(&fn[1], fno.fname, fn_len - 1);
    } else {
        strncpy(fn, fno.fname, fn_len);
    }

    return LV_FS_RES_OK;
}

/**
 * 关闭目录
 */
static lv_fs_res_t fs_dir_close(lv_fs_drv_t * drv, void * rddir_p)
{
    f_closedir((DIR *)rddir_p);
    lv_free(rddir_p);
    return LV_FS_RES_OK;
}
