#ifndef WRITE_COUNT__H
#define WRITE_COUNT__H
#include "nova.h"
/* file */
#include <linux/uaccess.h>


/* time */
#include <linux/time.h>
#include <linux/jiffies.h>
/* pid */
//#include <asm-generic/current.h>
#include <linux/sched.h>

#include "hash.h"
#include "wearleveling.h"
#include "debug.h"

#include "detect.h"
#include "average.h"


extern unsigned long * block_write_count;
extern spinlock_t block_write_count_lock;


void init_block_write_count(void);
void print_block_write_count(void);
void write_block_write_count_to_file(void);
void write_global_blk_write_count_to_file(void);
void write_global_blk_write_count_to_file_with_zero(void);
int test_get_process_id(void);
/* inode页面写计数 */
void inode_page_count_increase(struct super_block * sb, u64 viraddr);
void inode_page_count_increase_with_migrate(struct super_block * sb, u64 viraddr);
void write_count_increase(unsigned long block_num);

static inline void data_block_write_count_increase(unsigned long block_num)
{
	write_count_increase(block_num);
}

static inline void log_block_write_count_increase(unsigned long block_num)
{
	write_count_increase(block_num);
}

static inline void journal_block_write_count_increase(unsigned long block_num)
{
	write_count_increase(block_num);
}

static inline u64 get_block_number_by_virt(struct super_block *sb,u64 viraddr)
{
	struct nova_super_block *ps = nova_get_super(sb);
	u64 block = viraddr ? (viraddr - (unsigned long)(void *)ps) : 0;
	if(block)
	{
		//4KB pagesize
		return block >> 12;
	}
	return 0;
}

#endif




