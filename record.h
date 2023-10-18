#ifndef __RECORD_H
#define __RECORD_H
#include <linux/kernel.h>
#include <linux/slab.h>
#include <linux/mutex.h>
#include <linux/vmalloc.h>

/* 定义是否需要迁移到DRAM */
#define MIGRATE_MALICIOUS_APP
/* 设置回写的阈值 */
#define WB_THRESHOLD 5

/* 可保存 2G 左右的背包内容 */
#define sizeV 2100

typedef struct suspect_process
{
	int pid;
	/* 写窗口内平均写次数 */
	unsigned long averageCount;
	/* 写窗口内页面写次数的方差 */
	unsigned long deviation;
	/* 写窗口内写入的总页面数量 */
	unsigned long pageCount;
	
	struct suspect_process *next;
	struct suspect_process *prev;
}SuspectProcess,*Suspect;


/* 记录恶意磨损进程 */
typedef struct malicious_process
{
    int pid;
    struct malicious_process *next;
}MaliciousProcess,*Malicious;

/* 记录全局写 */
typedef struct blk_write_count
{
	unsigned long write_count;
	char write_back_threshold;
	//int pid;
}BlkWriteCount,*BWC;


extern Malicious malicious_header;

extern Suspect suspect_header;


extern struct mutex global_mutex_lock;
extern struct mutex process_mutex_lock;
extern struct mutex detect_prepare_lock;


extern BWC global_blk_write_count;


/* 用来记录恶意pid */
void init_malicious_list_lock(void);
void insert_into_malicious_process(int pid);
void delete_from_malicious_process(int pid);
void print_malicious_process_info(void);
char is_in_malicious_process_list(int pid);
void free_all_malicious_process_list(void);

/* 记录可疑进程 */
void insert_into_suspect_process(int pid, unsigned long ac, unsigned long dev, unsigned long pc);
void delete_from_suspect_process(int pid);
void print_suspect_process_info(void);
char is_in_suspect_process_list(int pid);
void free_all_suspect_process_list(void);


extern int **V;
void zeroV(void);
void initV(void);
void printV(void);


#endif

