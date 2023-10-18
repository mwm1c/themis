#include "mappingtable.h"

/*
 * 双指针重写inode_page_mapping_tables;
 */

struct mutex mapping_table_lock;

struct mutex ** inode_page_locks;

int nova_alloc_block_inode_page_mapping_table(struct super_block *sb)
{
	struct nova_sb_info *sbi = NOVA_SB(sb);
	unsigned long * inode_page_mapping_table;
	int i;

	sbi->inode_page_mapping_tables = kzalloc(sbi->cpus * sizeof(unsigned long *),
							GFP_KERNEL);

	if (!sbi->inode_page_mapping_tables)
		return -ENOMEM;

	for (i = 0; i < sbi->cpus; i++) {
		sbi->inode_page_mapping_tables[i] = kzalloc(TABLE_NUM * sizeof(unsigned long), GFP_KERNEL);
		inode_page_mapping_table = nova_get_inode_page_mapping_table(sb, i);
		DEBUG("sbi->cpus is %d , i is %d nova_alloc_block_inode_page_mapping_table \n",sbi->cpus, i);
	}

	printk("WWB: nova_alloc_block_inode_page_mapping_table \n");

	return 0;
}


int nova_alloc_block_inode_page_locks(struct super_block *sb)
{
	struct nova_sb_info *sbi = NOVA_SB(sb);
	struct mutex * inode_page_lock;
	int i;

	inode_page_locks = kzalloc(sbi->cpus * sizeof(struct mutex *),
							GFP_KERNEL);

	if (!inode_page_locks)
		return -ENOMEM;

	for (i = 0; i < sbi->cpus; i++) {
		inode_page_locks[i] = kzalloc(TABLE_NUM * sizeof(struct mutex), GFP_KERNEL);
		inode_page_lock = nova_get_inode_page_lock(i);
		DEBUG("sbi->cpus is %d , i is %d nova_get_inode_page_lock \n",sbi->cpus, i);
	}

	printk("WWB: nova_alloc_block_inode_page_locks \n");

	return 0;
}



void nova_delete_inode_page_mapping_tables(struct super_block *sb)
{
	struct nova_sb_info *sbi = NOVA_SB(sb);

	int i;
	
	for (i = 0; i < sbi->cpus; i++) {
		kfree(sbi->inode_page_mapping_tables[i]);
		sbi->inode_page_mapping_tables[i] = NULL;
	}

	kfree(sbi->inode_page_mapping_tables);

	sbi->inode_page_mapping_tables = NULL;
}


void nova_delete_inode_page_locks(struct super_block *sb)
{
	struct nova_sb_info *sbi = NOVA_SB(sb);

	int i;
	
	for (i = 0; i < sbi->cpus; i++) {
		kfree(inode_page_locks[i]);
		inode_page_locks[i] = NULL;
	}

	kfree(inode_page_locks);

	inode_page_locks = NULL;
}


#if 0
int nova_alloc_block_inode_page_mapping_table(struct super_block *sb)
{
	struct nova_sb_info *sbi = NOVA_SB(sb);
	unsigned long * inode_page_mapping_table;
	int i;

	sbi->inode_page_mapping_tables = kzalloc(sbi->cpus * sizeof(unsigned long),
							GFP_KERNEL);

	if (!sbi->inode_page_mapping_tables)
		return -ENOMEM;

	for (i = 0; i < sbi->cpus; i++) {
		inode_page_mapping_table = nova_get_inode_page_mapping_table(sb, i);
	}

	return 0;
}


void nova_delete_inode_page_mapping_tables(struct super_block *sb)
{
	struct nova_sb_info *sbi = NOVA_SB(sb);

	kfree(sbi->inode_page_mapping_tables);
	sbi->inode_page_mapping_tables = NULL;
}
#endif


