#ifndef MAPPING_TABLE_H
#define MAPPING_TABLE_H

#include "debug.h"

#define TABLE_NUM 512

extern struct mutex mapping_table_lock;
extern struct mutex ** inode_page_locks;


int nova_alloc_block_inode_page_mapping_table(struct super_block *sb);
void nova_delete_inode_page_mapping_tables(struct super_block *sb);

int nova_alloc_block_inode_page_locks(struct super_block *sb);
void nova_delete_inode_page_locks(struct super_block *sb);



#endif
