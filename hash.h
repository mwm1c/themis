#ifndef __HASH_H
#define __HASH_H

#include "nova.h"

/* the max value of key*/
#define MAX_KEY 10

#define MAX_DETECT_KEY 1000

/* 用来定义是否记录每个进程的详细页面信息 */
#define RECORD_PER_PROCESS_BLOCKNUM


typedef struct detect_hash_node
{
    unsigned long write_count;
    unsigned long block_num;
    struct detect_hash_node * next;
}Detecthashnode,*DetectNode;

typedef struct detect_hash_table
{
    unsigned long node_count;
    unsigned long max_update;
    struct detect_hash_node * next;
}Detecthashtable,*DetectTable;


typedef struct hash_node
{
	/* 进程号 */
    int pid;
	/* 写窗口内总的写次数 */
    unsigned long total_count;
	/* 写窗口内最大的写次数 */
    unsigned long max_count;
	/* 记录页面写情况的哈希表 */
	DetectTable detect_table;
	
	/* 记录当前pid对应的最大的页面写计数（总的） */
	unsigned long max_page_writecount;

	/* 记录写窗口内的页面数量（即计算内存占用） */
	unsigned long page_count;
	
    struct hash_node * next;
}Hashnode,*Node;

typedef struct
{
    Node next;
}Hashtable,*Table;

void init_hash_table(void);

char insert_into_hash_table(int pid,unsigned long block_num);
char update_hash_table(int pid,unsigned long block_num);

//char insert_into_hash_table(int pid);
//char update_hash_table(int pid);
char delete_from_hash_table(int pid);
Node search_from_hash_table(int pid);
void print_hash_table(void);

void init_detect_hash_table(Node node);
void insert_detect_hash_node(Node node, unsigned long block_num);
void print_detect_hash_table(int pid);

/* 空间回收函数 */
void delete_from_detect_hash_table(Node node);
/* 回收除了Table_header的所有空间 */
void free_all_node(void);
/* 回收table在文件系统umount时调用 */
void free_table(void);


/* 增加global的相关内容 */
void init_global_detect_table(void);
void insert_into_global_detect_table(unsigned long block_num);
void delete_global_detect_hash_table(void);
void print_global_detect_hash_table(void);
unsigned long get_global_detect_frequent_update(void);

//unsigned long search_in_suspect_process_list(int pid);


#endif // __HASH_H
