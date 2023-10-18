#include "wearleveling.h"

#define TmH 400

extern char should_wear_leveling;
atomic64_t exceed_average = ATOMIC64_INIT(0);
atomic64_t judge_times = ATOMIC64_INIT(0);
atomic64_t max_deviation = ATOMIC64_INIT(400);

extern void get_random_bytes(void *buf, int nbytes);

/* 返回block之间的随机数 */
unsigned long get_random_number(void)
{
	static unsigned long random_num;
	get_random_bytes(&random_num, sizeof(unsigned long));
	random_num = random_num % BLOCKS_MAX;
	/* 返回了静态局部变量 */
	return random_num;
}

/* 获取平均值 */
unsigned long get_global_write_count_base(void)
{
	int i;
	unsigned long sum = 0;
	unsigned long random_num = 0;
	unsigned long average = 0;
	
	for(i = 0; i < SAMPLE_BLOCK_NUM; i++)
	{
		random_num = get_random_number();
		sum += global_blk_write_count[random_num].write_count;
	}
	average = sum / SAMPLE_BLOCK_NUM;
	return average;
}

/* 求平方根 */
unsigned long get_sqrt(unsigned long *number)
{
	unsigned long op, res, one;
	op = *number;
	res = 0;
	one = 1UL << (64 - 2);
	while (one > op)
		one >>= 2;

	while (one != 0) {
		if (op >= res + one) {
			op = op - (res + one);
			res = res +  2 * one;
		}
		res /= 2;
		one /= 4;
	}
	return res;	
}

/* 获取平均值，以及标准差 */
void get_global_write_count_base_2(unsigned long *average, unsigned long *standard_deviation)
{
	int i;
	unsigned long sum = 0;
	unsigned long random_num = 0;
	unsigned long random_arr[SAMPLE_BLOCK_NUM];
	
	for(i = 0; i < SAMPLE_BLOCK_NUM; i++)
	{
		random_num = get_random_number();
		sum += global_blk_write_count[random_num].write_count;
		random_arr[i] = random_num;
	}
	
	*average = sum / SAMPLE_BLOCK_NUM;
	

	for(i = 0; i < SAMPLE_BLOCK_NUM; i++)
	{
		*standard_deviation += ((global_blk_write_count[random_arr[i]].write_count - *average) * (global_blk_write_count[random_arr[i]].write_count - *average));
	}

	*standard_deviation = *standard_deviation / SAMPLE_BLOCK_NUM;
	
	/* 标准差 */
	*standard_deviation = get_sqrt(standard_deviation);
	
}


#if 1
/* 
 * 注释于：2021-2-24
 * 两个阈值tml和TmH，以前有tml是因为平均值的计算问题
 * 现在平均值的计算得到了优化，所以这个函数也需要被优化		
*/
char should_migrate(u64 block_num)
{
	//没有初始化
	unsigned long average,dynamic_deviation;
	char is_in_suspect;
	average = 0;

	// 2020-07-10 写次数没有达到这个值不迁移
	//if(global_blk_write_count[block_num].write_count < DEVIATION_HIGH)
	if(global_blk_write_count[block_num].write_count < atomic64_read(&max_deviation))
	{
		atomic64_set(&exceed_average, 0);
		return 0;
	}

	atomic64_inc(&exceed_average);

	//average = get_global_write_count_base();
	///////////// wwb 20210127 ////////////////
	average = atomic64_read(&count_average);

	if(atomic64_read(&exceed_average) >= 3);
	{
		atomic64_set(&exceed_average, 0);
		if((atomic64_read(&max_deviation)-average)<(DEVIATION_HIGH/2))
		{
			atomic64_add(DEVIATION_HIGH, &max_deviation);
			//atomic64_set(&exceed_average, 0);
			printk("%s: max_deviation is %d average is %d \n", __func__, atomic64_read(&max_deviation), average);
		}
	}

	if((global_blk_write_count[block_num].write_count > (average + DEVIATION_LOW)));
	{
		return 1;
	}
	return 0;
}
#endif

#if 0
/*
 * 2021-2-24
 * 仅使用一个阈值TmH
 */
char should_migrate(u64 block_num)
{
	//没有初始化
	unsigned long average,dynamic_deviation;
	char is_in_suspect;
	average = atomic64_read(&count_average);

	if((global_blk_write_count[block_num].write_count > (average + TmH)));
	{
		return 1;
	}
	return 0;
}
#endif

struct inode *nova_get_vfs_inode(struct super_block *sb, unsigned long ino)
{
	struct inode *inode;

	inode = iget_locked(sb, ino);
	if (unlikely(!inode))
		return ERR_PTR(-ENOMEM);
	if (!(inode->i_state & I_NEW))
		return inode;
	return NULL;
}


/* 批量修改一个inode table的pi_addr */
char nova_change_pi_addr(struct super_block *sb, u64 begin_ino, int num)
{
	struct nova_inode_info *si;
	struct nova_inode_info_header *sih = NULL;
	struct inode *inode;
	u64 pi_addr;
	int err;
	int i;
	u64 ino;

	for(i = 0; i < num; i++)
	{
		
		ino = begin_ino + i;		
		inode = nova_get_vfs_inode(sb, ino);
		if(inode == NULL)
		{
			printk("nova_change_pi_addr: get inode error ino is %llu \n", ino);
			return -1;
		}

		si = NOVA_I(inode);
		sih = &si->header;

		err = nova_get_inode_address(sb, ino, &pi_addr, 0);
		if (err) {
			nova_dbg("%s: get inode %lu address failed %d\n",
					__func__, ino, err);
			goto fail;
		}

		//直接对pi_addr进行更改
		sih->pi_addr = pi_addr;
	}

	printk("begin_ino is %llu, change pi_addr ok \n", begin_ino);
	return 1;
	
fail:
	printk("fail \n");
	iget_failed(inode);
	return -1;
}

/* 
 * 前台线程主动迁移
 * 这个函数在运行时会遇到bug
 * 暂时还不知道解决办法
 * 在下个函数中对该方法进行重写
 */
char migrate_inode_page(struct super_block *sb, u64 ino)
{
	struct nova_sb_info *sbi = NOVA_SB(sb);
	struct nova_inode *pi;
	unsigned long * inode_page_mapping_table;
	
	unsigned int data_bits;
	unsigned int num_inodes_bits;
	u64 curr;
	u64 migrate;
	unsigned int superpage_count;
	u64 internal_ino;
	int cpuid;
	unsigned int index;
	unsigned long blocknr;
	unsigned long curr_addr;
	unsigned long migrate_addr;
	int allocated;
	u64 begin_ino;

	pi = nova_get_inode_by_ino(sb, NOVA_INODETABLE_INO);
	if(pi == NULL)
	{
		printk("migrate_inode_page error: pi is null \n");
		return -1;
	}

	data_bits = blk_type_to_shift[pi->i_blk_type];
	num_inodes_bits = data_bits - NOVA_INODE_BITS;

	cpuid = ino % sbi->cpus;
	internal_ino = ino / sbi->cpus;


	//mutex_lock(&mapping_table_lock);
	
	mutex_lock(&sbi->inode_page_mapping_table_locks[cpuid]);
	debug_inode_mapping_table("enter lock cpuid is %d\n", cpuid);
	
	inode_page_mapping_table = nova_get_inode_page_mapping_table(sb, cpuid);


	superpage_count = internal_ino >> num_inodes_bits;
	index = internal_ino & ((1 << num_inodes_bits) - 1);

	begin_ino = ino - index * 40;

	//printk("migrate_inode_page: cpuid is %d, internal_ino is %ld, index is %ld, begin_ino is %lu, count is %ld \n", cpuid, internal_ino, index, begin_ino, superpage_count);

	//直接拿页面偏移，这个就是那个页面偏移
	curr = inode_page_mapping_table[superpage_count];
	if (curr == 0)
	{
		printk("migrate_inode_page: curr is 0 \n");
		return -EINVAL;
	}

	curr_addr = (unsigned long)nova_get_block(sb, curr);


/* 分配空间进行迁移 */
	allocated = nova_new_log_blocks(sb, pi, &blocknr, 1, 1);
	if(allocated != 1)
	{
		printk("migrate_inode_page: allocated error \n");
		return allocated;
	}

	migrate = nova_get_block_off(sb, blocknr, NOVA_BLOCK_TYPE_4K);
	migrate_addr = (unsigned long)nova_get_block(sb, migrate);

	//623
	write_count_increase(blocknr);

	//step 1将数据拷贝到新页
	memcpy((unsigned long *)migrate_addr, (unsigned long *)curr_addr, 4096);
	
	//step 2更新mapping table指向
	inode_page_mapping_table[superpage_count] = migrate;
	
	//step 3释放之前的inode page
	//spin_lock(&freelist_lock);
	//insert_freenode(sb, curr);
	//spin_unlock(&freelist_lock);
	//nova_free_log_blocks(sb, pi, (curr >> 12), 1);

	//mutex_unlock(&mapping_table_lock);
	debug_inode_mapping_table("out lock cpuid is %d\n", cpuid);
	mutex_unlock(&sbi->inode_page_mapping_table_locks[cpuid]);
	
	return 0;
}

char active_migrate_inode_table_page(struct super_block *sb, u64 viraddr)
{
	struct nova_inode *pi = (struct nova_inode *)viraddr;
	u64 ino = pi->nova_ino;
	u64 block_num = get_block_number_by_virt(sb, viraddr);

	if(ino < NOVA_NORMAL_INODE_START)
		return 0;
	
	if(should_migrate(block_num))
	{
		//printk("WWB: enter active_migrate_inode_table_page \n");
		migrate_inode_page(sb, ino);
	}
	return 0;
}

/* 迁移journal页面，journal页面会造成大量的写操作 */
char active_migrate_journal_page(struct super_block *sb,struct ptr_pair *pair, u64 viraddr)
{
	u64 block_num = viraddr >> 12;
	//取出低十二位
	u64 tail,head;
	int allocated;
	u64 curr, curr_addr, migrate, migrate_addr;
	unsigned long blocknr;
	struct nova_inode *pi = nova_get_inode_by_ino(sb, NOVA_INODETABLE_INO);

	if(should_migrate(block_num))
	{
		tail = pair->journal_tail;
		head = pair->journal_head;

		//printk("before head is %lx, tail is %lx \n",tail,head);


		allocated = nova_new_log_blocks(sb, pi, &blocknr, 1, 1);
		if(allocated != 1)
		{
			printk("migrate_inode_page: allocated error \n");
			return allocated;
		}

		migrate = nova_get_block_off(sb, blocknr, NOVA_BLOCK_TYPE_4K);
		migrate_addr = (unsigned long)nova_get_block(sb, migrate);


		curr = nova_get_block_off(sb, block_num, NOVA_BLOCK_TYPE_4K);
		curr_addr = (unsigned long)nova_get_block(sb, curr);

		//623
		write_count_increase(blocknr);

		/* step 1 将页面赋值到新地址 */
		memcpy((unsigned long *)migrate_addr, (unsigned long *)curr_addr, 4096);

		tail = tail & (4096 - 1);
		head = head & (4096 - 1);

		/* step 2 更新head和tail */
		pair->journal_head = migrate + head;
		pair->journal_tail = migrate + tail;

		/* step 3 释放旧页面 */
		nova_free_log_blocks(sb, pi, block_num, 1);
		
		//printk("after head is %lx, tail is %lx \n",pair->journal_head,pair->journal_tail);
	}
	return 0;
}

/* 迁移inode log页面 */
char active_migrate_inode_log_page(struct super_block *sb, struct nova_inode * pinode, u64 last_page, u64 curr, int flag)
{
	return;

	u64 block_num = curr >> 12;
	u64 curr_addr, migrate, migrate_addr;
	unsigned long blocknr;
	int allocated;
	struct nova_inode *pi = nova_get_inode_by_ino(sb, NOVA_INODETABLE_INO);

	if(should_migrate(block_num))
	{
		allocated = nova_new_log_blocks(sb, pi, &blocknr, 1, 1);
		if(allocated != 1)
		{
			printk("migrate_inode_page: allocated error \n");
			return allocated;
		}
		
		migrate = nova_get_block_off(sb, blocknr, NOVA_BLOCK_TYPE_4K);
		migrate_addr = (unsigned long)nova_get_block(sb, migrate);

		curr = nova_get_block_off(sb, block_num, NOVA_BLOCK_TYPE_4K);
		curr_addr = (unsigned long)nova_get_block(sb, curr);

		/* step 1 拷贝对应的数据 */
		memcpy((unsigned long *)migrate_addr, (unsigned long *)curr_addr, 4096);

		/* step 2 修改上一个页面的指针让它指向新页面 */
		if(flag == 1)
		{
			pinode->log_head = migrate;
			nova_flush_buffer(&pinode->log_head, CACHELINE_SIZE, 1);
		}
		else
			nova_set_next_page_address(sb, (struct nova_inode_log_page *)last_page, migrate, 1);
		

		/* step 3 释放掉对应的页面 */
		nova_free_log_blocks(sb, pinode, block_num, 1);

		printk("active_migrate_inode_log_page: end \n");
		
	}

	return 0;
}

#if 0
char migrate_inode_table_page(struct super_block *sb,unsigned long * inode_table)
{
	unsigned long migrate;
	unsigned long migrate_addr;
	unsigned long curr;
	unsigned long curr_addr;
	int allocated;
	unsigned long blocknr;
	struct nova_inode * pi;


	pi = nova_get_inode_by_ino(sb, NOVA_INODETABLE_INO);
	curr = *inode_table;
	curr_addr = (unsigned long)nova_get_block(sb, curr);
	
	allocated = nova_new_log_blocks(sb, pi, &blocknr, 1, 1);
	if(allocated != 1)
	{
		printk("migrate_inode_page: allocated error \n");
		return allocated;
	}

	migrate = nova_get_block_off(sb, blocknr, NOVA_BLOCK_TYPE_4K);
	migrate_addr = (unsigned long)nova_get_block(sb, migrate);

	//step 1将数据拷贝到新页
	memcpy((unsigned long *)migrate_addr, (unsigned long *)curr_addr, 4096);

/* TEST */
	struct nova_inode * pia = (struct nova_inode *)curr_addr;
	struct nova_inode * pib = (struct nova_inode *)migrate_addr;
	printk("pia->nova_ino is %lu; pib->nova_ino is %lu \n", pia->nova_ino, pib->nova_ino);

	//step 2更新mapping table指向
	*inode_table = migrate;
	
	//step 3释放之前的inode page
	nova_free_log_blocks(sb, pi, (curr >> 12), 1);
	
}
/* 
 * 扫描inode table，如果需要进行迁移那么则迁移
 * 对每个CPU的inode table如果达到迁移条件则进行迁移
 */
void scan_inode_table(struct super_block *sb)
{
	struct nova_sb_info *sbi = NOVA_SB(sb);
	unsigned long * inode_page_mapping_table;
	u64 curr;
	int i,j;
	unsigned long block_num;
	unsigned int cpus = sbi->cpus;
	for(i = 0; i < cpus; i++)
	{
		//获取每个CPU的inode_mapping_table
		inode_page_mapping_table = nova_get_inode_page_mapping_table(sb,i);
		for(j = 0; j < TABLE_NUM; j++)
		{
			curr = inode_page_mapping_table[j];
			if(curr != 0)
			{
				block_num = curr >> 12;
				if(should_migrate(block_num))
				{
					printk("before inode_page_mapping_table[j] is %lx \n",inode_page_mapping_table[j]);
					migrate_inode_table_page(sb,&inode_page_mapping_table[j]);
					printk("after inode_page_mapping_table[j] is %lx \n",inode_page_mapping_table[j]);
				}
			}
			else
				break;
		}
	}
	
}
#endif

#if 0
/* 一次修改一个pi_addr */
void nova_change_pi_addr(struct super_block *sb, u64 ino)
{
	struct nova_inode_info *si;
	struct nova_inode_info_header *sih = NULL;
	struct inode *inode;
	u64 pi_addr;
	int err;

	inode = iget_locked(sb, ino);
	if (unlikely(!inode))
		return ERR_PTR(-ENOMEM);

	si = NOVA_I(inode);
	sih = &si->header;

	err = nova_get_inode_address(sb, ino, &pi_addr, 0);
	if (err) {
		nova_dbg("%s: get inode %lu address failed %d\n",
				__func__, ino, err);
		goto fail;
	}

	//直接对pi_addr进行更改
	sih->pi_addr = pi_addr;
	
fail:
	iget_failed(inode);
}


/* 
 * old —— 不会被使用
 * 这个函数比较大，
 * 用于找到inode所在的页面同时分配新页面
 * 要迁移一个inode页面，由于inode table是以
 * 单链表的形式来进行管理，所以需要找到前一个inode table
 */
char migrate_inode_page(struct super_block *sb, u64 ino)
{
	struct nova_sb_info *sbi = NOVA_SB(sb);
	struct nova_inode *pi;
	struct inode_table *inode_table;
	unsigned int data_bits;
	unsigned int num_inodes_bits;
	u64 curr;
	unsigned int page_count;
	u64 internal_ino;
	int cpuid;
	unsigned int index;
	unsigned int i = 0;
	unsigned long blocknr;
	unsigned long curr_addr;
	unsigned long up_addr;
	unsigned long up_off;
	unsigned long up_off_addr;
	unsigned long exchange_off;
	unsigned long exchange_off_addr;
	unsigned long exchange_addr;
	int allocated;

	pi = nova_get_inode_by_ino(sb, NOVA_INODETABLE_INO);	

	//分配新页面
	allocated = nova_new_log_blocks(sb, pi, &blocknr,1, 1);
	if(!allocated)
	{
		printk("WWB:nova_exchange_test, nova_new_log_blocks errors \n");
	}
	exchange_off = nova_get_block_off(sb, blocknr, NOVA_BLOCK_TYPE_4K);
	exchange_addr = (unsigned long)nova_get_block(sb, exchange_off);
	printk("WWB: exchange_off is %lx \n", exchange_off);
	printk("WWB: exchange_addr is %lx \n", exchange_addr);
	
	/*
	 * BLOCK_TYPE的位数，4K为12位
	 * unsigned int blk_type_to_shift[NOVA_BLOCK_TYPE_MAX] = {12, 21, 30};
	 */
	data_bits = blk_type_to_shift[pi->i_blk_type];
	/*
	 * #define NOVA_INODE_BITS	7
	 * 如果data_bits为21，那么此时num_inode_bits为14,2^14个inode
	 * 如果为12，那么num_inode_bits为5，
	 * 那么就是说一个inode_table里面有2^5(32)个inode
	 */
	num_inodes_bits = data_bits - NOVA_INODE_BITS;
	/*
	 * cpuid为inode_number与cpu个数取余，
	 * 这是否表明inode按照这种方式分给各个cpu？
	 * 这有可能，应该是按照这种方式分给各个CPU
	 */
	cpuid = ino % sbi->cpus;
	/*
	 * 然后找出，这个inode，在每个CPU中的位置
	 * 这个位置应该是从0开始~
	 */
	internal_ino = ino / sbi->cpus;

	/* 获取inode_table */
	inode_table = nova_get_inode_table(sb, cpuid);
	/* 由于inode是以2M为链表来组织的，这里是看在链表的第几个节点上 */
	page_count = internal_ino >> num_inodes_bits;
	/* 找到在对应的2M内部的索引，也就是在其中一个链表节点的哪个位置 */
	index = internal_ino & ((1 << num_inodes_bits) - 1);

	curr = inode_table->log_head;
	if (curr == 0)
		return -EINVAL;

	for (i = 0; i < page_count; i++) {
		if (curr == 0)
			return -EINVAL;

		curr_addr = (unsigned long)nova_get_block(sb, curr);
		/* Next page pointer in the last 8 bytes of the superpage */
		curr_addr += 4096 - 8;
		curr = *(u64 *)(curr_addr);
		
		if(i == page_count - 2)
		{
			if(exchange_flag == 0)
			{
				up_off = curr;
				printk("WWB: curr exchange before is %lx \n", curr);

				//the next block addr
				up_addr = (unsigned long)nova_get_block(sb, up_off);

				int inode_number1 = ((struct nova_inode *)up_addr)->nova_ino;
				printk("WWB: inode1 is %d \n", inode_number1);

				printk("WWB: up_addr is %lx \n", up_addr);
			
				//copy inode_table to new page
				memcpy((u64 *)exchange_addr, (u64 *)up_addr, 4096);

				nova_free_log_blocks(sb, pi, up_off, 1);

				int inode_number2 = ((struct nova_inode *)exchange_addr)->nova_ino;
				printk("WWB: inode2 is %d \n", inode_number2);

				curr = exchange_off;
				printk("WWB: curr exchange after is %lx \n", curr);

				//This is right
				memcpy((u64 *)curr_addr, &exchange_off, 8);
				printk("WWB: *(u64 *)up_off_addr  is %lx \n", *(u64 *)curr_addr);
			}
			else
			{
				up_off = curr;
				printk("\n\nWWB: curr is %lx \n", curr);
				up_addr = (unsigned long)nova_get_block(sb, up_off);
				int inode_number1 = ((struct nova_inode *)up_addr)->nova_ino;
				printk("WWB: inode1 is %d \n", inode_number1);
			}
		}
	}
	
	return 0;
	
}
#endif


