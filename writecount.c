#include "writecount.h"
#include <linux/types.h>

/* 栈空间有限 */
/* wwb add 2020-7-3增加写计数的自旋锁 */
spinlock_t block_write_count_lock;

unsigned long * block_write_count = NULL;
unsigned long max_update = 0;

atomic64_t write_traffic = ATOMIC64_INIT(0);

unsigned long nova_base_address = 0;


void init_block_write_count(void)
{
	int i;
	//初始化block_write_count_lock
	spin_lock_init(&block_write_count_lock);
	
	block_write_count = (unsigned long *)vmalloc(sizeof(unsigned long) * BLOCKS_MAX);
	
	global_blk_write_count = (BWC)vmalloc(sizeof(BlkWriteCount) * BLOCKS_MAX); 
	for(i = 0; i < BLOCKS_MAX; i++)
	{
		block_write_count[i] = 0;
		global_blk_write_count[i].write_count = 0;
		global_blk_write_count[i].write_back_threshold = 0;
	}
}


void write_global_blk_write_count_to_file(void)
{
	struct file * file = NULL;
	mm_segment_t old_fs;
	char buf[64];
	int i;
	memset(buf, 0 , 64);
	file = filp_open("global_blk_write_count.txt", O_CREAT | O_RDWR | O_APPEND, 0644);
	if(IS_ERR(file))
	{
		printk("WWB: open file error \n");
		return;
	}
	for(i = 0; i < BLOCKS_MAX; i++)
	{
		if(global_blk_write_count[i].write_count > 0)
		{
			snprintf(buf, sizeof(buf), "%d\t %lu\n", i, global_blk_write_count[i].write_count);
			old_fs = get_fs();
			set_fs(KERNEL_DS);
			vfs_write(file, (char *)buf, strlen(buf), &(file->f_pos));
			set_fs(old_fs);
			memset(buf, 0 , 64);
		}
	}
	//vfree(global_blk_write_count);
}


void write_global_blk_write_count_to_file_with_zero(void)
{
	struct file * file = NULL;
	mm_segment_t old_fs;
	char buf[64];
	int i;
	memset(buf, 0 , 64);
	file = filp_open("global_blk_write_count_with_zero.txt", O_CREAT | O_RDWR | O_APPEND, 0644);
	if(IS_ERR(file))
	{
		printk("WWB: open file error \n");
		return;
	}
	for(i = 0; i < BLOCKS_MAX; i++)
	{
		snprintf(buf, sizeof(buf), "%d\t %lu\n", i, global_blk_write_count[i].write_count);
		old_fs = get_fs();
		set_fs(KERNEL_DS);
		vfs_write(file, (char *)buf, strlen(buf), &(file->f_pos));
		set_fs(old_fs);
		memset(buf, 0 , 64);
	}
	vfree(global_blk_write_count);
}


void write_block_write_count_to_file(void)
{
	struct file * file = NULL;
	mm_segment_t old_fs;
	char buf[64];
	int i;
	memset(buf, 0 , 64);
	file = filp_open("block_write_count.txt", O_CREAT | O_RDWR | O_APPEND, 0644);
	if(IS_ERR(file))
	{
		printk("WWB: open file error \n");
		return;
	}
	for(i = 0; i < BLOCKS_MAX; i++)
	{
		if(block_write_count[i] > 0)
		{
			snprintf(buf, sizeof(buf), "%d\t %lu\n", i, block_write_count[i]);
			old_fs = get_fs();
			set_fs(KERNEL_DS);
			vfs_write(file, (char *)buf, strlen(buf), &(file->f_pos));
			set_fs(old_fs);
			memset(buf, 0 , 64);
		}
	}
	vfree(block_write_count);
}


int test_get_process_id(void)
{
	int curr_pid = current -> pid;
	printk("WWB: task name: %s, task id: %d\n", current->comm, current->pid);
	return curr_pid;
}


void write_count_increase(unsigned long block_num)
{
	char is_malicious;
	if(block_num >= BLOCKS_MAX)
	{
		printk("WWB: write blocks overflow \n");
		return;
	}
	spin_lock(&block_write_count_lock);
	#ifdef MIGRATE_MALICIOUS_APP
	is_malicious = is_in_malicious_process_list(current->pid);
	if(is_malicious)
	{
		if(global_blk_write_count[block_num].write_back_threshold >= WB_THRESHOLD)
		{
			global_blk_write_count[block_num].write_count ++;
			global_blk_write_count[block_num].write_back_threshold = 0;
		}
		global_blk_write_count[block_num].write_back_threshold ++;
		goto OUT;
	}
	#endif
	global_blk_write_count[block_num].write_count++;
		
OUT:
	block_write_count[block_num]++;
	/* 全局的global_max_update永远是整个系统中页面磨损最大的那个值 */
	if(global_max_update < block_write_count[block_num])
		global_max_update = block_write_count[block_num];
	
	spin_unlock(&block_write_count_lock);

	//////////////// wwb 20210127 ///////////////////
	atomic64_inc(&count_hand);
	if(atomic64_read(&count_hand) >= BLOCKS_MAX)
	{
		atomic64_inc(&count_average);
		atomic64_set(&count_hand, 0);
	}
	/////////////////////////////////////////////////

	record_one_sec_write_info(current->pid, block_num);
}


/* new function */
void inode_page_count_increase(struct super_block * sb, u64 viraddr)
{

	u64 block_num = get_block_number_by_virt(sb, viraddr);
	write_count_increase(block_num);
}

/* new function */
void inode_page_count_increase_with_migrate(struct super_block * sb, u64 viraddr)
{

	u64 block_num = get_block_number_by_virt(sb, viraddr);
	
	u64 ino = ((struct nova_inode *)viraddr)->nova_ino;

	write_count_increase(block_num);

	if(ino < NOVA_NORMAL_INODE_START)
		return;

	if(should_migrate(block_num))
		migrate_inode_page(sb,ino);
}



