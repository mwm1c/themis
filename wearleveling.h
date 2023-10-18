#ifndef WEAR_LEVELING_H
#define WEAR_LEVELING_H

#include "nova.h"
#include "writecount.h"
#include "mappingtable.h"
#include "hash.h"
#include "debug.h"
#include "freelist.h"

#include <linux/types.h>

/* 定义采样的BLOCK数量 */
#define SAMPLE_BLOCK_NUM 100

/* 允许的页面差距 */
#define DEVIATION_HIGH 400

#define DEVIATION_LOW 200

/* judge related */
unsigned long get_random_number(void);
unsigned long get_global_write_count_base(void);
void get_global_write_count_base_2(unsigned long *average, unsigned long *standard_deviation);
char should_migrate(u64 block_num);

/* migrate related */
char nova_change_pi_addr(struct super_block *sb, u64 begin_ino, int num);
char migrate_inode_page(struct super_block *sb, u64 ino);
char migrate_inode_table_page(struct super_block *sb,unsigned long * inode_table);
void scan_inode_table(struct super_block *sb);
char active_migrate_inode_table_page(struct super_block *sb, u64 viraddr);
char active_migrate_journal_page(struct super_block *sb,struct ptr_pair *pair, u64 viraddr);
char active_migrate_inode_log_page(struct super_block *sb, struct nova_inode * pinode, u64 last_page, u64 curr, int flag);


#endif

