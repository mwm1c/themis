#ifndef DETECT_H
#define DETECT_H

#include "nova.h"
#include "hash.h"
#include "wearleveling.h"
#include "detect_time.h"
#include "record.h"
#include "occupySpace.h"

/* time */
#include <linux/time.h>
#include <linux/jiffies.h>

#include <linux/slab.h>


/* 每个单元的最大写次数PCM:10^7 */
#define MAX_ENDURANCE 10000000

/* NVM的容量，暂时不使用这个，使用sbi->num_blocks */
#define CAPACITY 128

/* 对应的秒数 */
#define D_HOURS 3600
#define D_DAYS 86400
#define D_MOUTHS 2592000
#define D_YEARS 31536000

/* 设定期望的寿命 年         */
#define EXPECT_L 3
#define EXPECT_L_S EXPECT_L*D_YEARS
#define FREQUENT_L (EXPECT_L*D_YEARS)/20

/* 定义采样随机频率，产生0-100的随机数，如果随机数落在一个范围则进行检测 */
#define SAMPLE_INTERVAL 1200

#define TIME_INTERVAL 1000

/* 导入table header */
extern Table Table_header;

extern char detect_mode;
extern unsigned long time_base;
extern char change_time_base_flag;

extern unsigned long global_max_update;


int rand_interval(void);
int detect_mode_change(void);
void detect_prepare(void);

int detect_global(unsigned long times, unsigned long total_times);
int detect_process(unsigned long times, unsigned long total_times);
unsigned long life_caculate_frequenct_page(unsigned long times);
unsigned long life_caculate_ideal_leveling(unsigned long total_times);

int detect_process_ideal_wear_leveling(unsigned long total_times);
int detect_process_without_wear_leveling(unsigned long times);

int detect_global_ideal_wear_leveling(unsigned long total_times);
int detect_global_without_wear_leveling(unsigned long times);

unsigned long life_caculate_remaining_ideal_leveling(unsigned long total_times);
int detect_global_remaining_ideal_wear_leveling(unsigned long total_times);


/* 对各个process进行检测与记录 */
void record_malice_process(void);

/* 加入到各个写入接口来进行1s的记录 */
void record_one_sec_write_info(int pid, unsigned long block_num);

#endif


