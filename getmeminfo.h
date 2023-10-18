#ifndef MEM_INFO_H
#define MEM_INFO_H

#include <linux/kernel.h>
#include <linux/slab.h>
#include <linux/fs.h>
#include <linux/string.h>
#include <linux/mm.h>
#include <linux/syscalls.h>
#include <asm/unistd.h>
#include <asm/uaccess.h>


/* 定义保存内存使用情况的结构体 */
struct MEM_INFO
{
    unsigned int total;
    unsigned int free;
    unsigned int buffers;
    unsigned int cached;
    unsigned int swap_cached;
    unsigned int swap_total;
    unsigned int swap_free;
    unsigned int available;
};
typedef struct MEM_INFO Mem_info;


void get_mem_info(Mem_info *o);


#endif


