#include "detect.h"
#include "utility.h"
#include <linux/types.h>

/* 
 * 检测模式：
 * 0：不进行检测
 * 1：进行全局检测
 * 2：进行各个pid检测
 */
char detect_mode = 0;
unsigned long time_base = 0;
char change_time_base_flag = 1;
char times_goes_by = 0;
unsigned long last_global_max_update = 0;

/* 用来存储整个NVM中写次数最大的行 */
unsigned long global_max_update = 0;

atomic64_t process_count = ATOMIC64_INIT(0);

/* 设置背包容量全局变量 */
// 初始为2GB
int memoryCap = 256;

/*
  0: suspect;
  1: malicious;
*/
char suspect_or_malicious = 0;

//extern unsigned long max_update;
extern atomic64_t write_traffic;

extern char should_wear_leveling;

extern void get_random_bytes(void *buf, int nbytes);

/* 产生0-SAMPLE_INTERVAL之间的随机数 */
int rand_interval(void)
{
	static unsigned long random_num;
	get_random_bytes(&random_num, sizeof(unsigned long));
	random_num = random_num % SAMPLE_INTERVAL;
	/* 返回了静态局部变量 */
	return (int)random_num;
}

int detect_mode_change(void)
{
	char is_or_no;
	int random_num;
	unsigned long frequent_count,total_count;
	random_num = rand_interval();

	/* 在上一秒已经检测了的情况下避免重复检测，所以如果mode为1，那么直接返回0 */
	if(detect_mode == 1)
	{
		total_count = atomic64_read(&write_traffic);
		printk("total_count is %lu \n",total_count);
		//is_or_no = detect_global_ideal_wear_leveling(total_count);
		is_or_no = detect_global_remaining_ideal_wear_leveling(total_count);
		
		/* 如果是则开始per_pid检测 */
		if(is_or_no)
		{
			printk("begin per-pid detect \n");
			detect_mode = 2;
			return 1;
		}
		atomic64_set(&write_traffic, 0);
		detect_mode = 0;
		return 0;
	}

	if(detect_mode == 2)
	{
		mutex_lock(&process_mutex_lock);
		//record_malice_process();
		mutex_unlock(&process_mutex_lock);
		detect_mode = 0;
		atomic64_set(&process_count, 0);
		return 0;
	}

	/* 开始全局检测，平均每一分钟进行一次全局检测 */
	if(random_num < 20)
	{
		last_global_max_update = global_max_update;
		detect_mode = 1;
	}
	
	else
	{
		detect_mode = 0;
	}
	
	atomic64_set(&write_traffic, 0);
	return 0;
}


void detect_prepare(void)
{
	if(change_time_base_flag == 1)
	{
		time_base = jiffies;
		change_time_base_flag = 0;
	}
	unsigned long jiffies_end = jiffies;
	unsigned long interval = jiffies_end - time_base;
	unsigned long interval_ms = jiffies_to_msecs(interval);
	
	if(interval_ms >= TIME_INTERVAL)
	{
		detect_mode_change();
		change_time_base_flag = 1;
	}
}


/* times：1s内最频繁写入行写入次数，返回值为秒 */
unsigned long life_caculate_frequenct_page(unsigned long times)
{
	if(times >= 1)
		return (MAX_ENDURANCE / times);
	return 0;
}


/* times：1s内最频繁写入行写入次数，返回值为秒 */
int life_caculate_and_judge_malicious(unsigned long times)
{
	if(times < 1)
		return 0;
	unsigned long lifes;
	lifes = (MAX_ENDURANCE / times);

	if(lifes < EXPECT_L_S)
		return 1;
	return 0;
}


/* total times最频繁写入行写入次数 */
unsigned long life_caculate_ideal_leveling(unsigned long total_times)
{

	/* NVM拥有的块数 */
	unsigned long max_num_blocks;
	max_num_blocks = BLOCKS_MAX;
	
	if(total_times >= 1)
		return (max_num_blocks * MAX_ENDURANCE) / total_times;
	
	return 0;
}

/* 进行全局检测 */
int detect_global(unsigned long times, unsigned long total_times)
{
	unsigned long frequent_actual;
	unsigned long global_actual;

	if(times == 0 || total_times == 0)
		return 0;

	frequent_actual = life_caculate_frequenct_page(times);
	global_actual = life_caculate_ideal_leveling(total_times);

	if(frequent_actual == 0 || global_actual == 0)
		return 0;

	//if(frequent_actual < FREQUENT_L)
	if(frequent_actual < EXPECT_L_S)
		return 1;
	if(global_actual < EXPECT_L_S)
		return 1;
	return 0;
}


/* 用来监测理想情况下的磨损均衡 */
int detect_global_ideal_wear_leveling(unsigned long total_times)
{
	unsigned long global_actual;

	if(total_times < 1)
		return 0;

	global_actual = life_caculate_ideal_leveling(total_times);

	if( global_actual == 0)
		return 0;

	if(global_actual < EXPECT_L_S)
		return 1;
	return 0;
}


/* 用来计算没有磨损均衡时的使用寿命 */
int detect_global_without_wear_leveling(unsigned long times)
{
	unsigned long frequent_actual;

	if(times < 1)
		return 0;

	frequent_actual = life_caculate_frequenct_page(times);

	if(frequent_actual == 0)
		return 0;

	if(frequent_actual < EXPECT_L_S)
		return 1;

	return 0;
}


/* 进行各process检测，最好通过链表返回 */
int detect_process(unsigned long times, unsigned long total_times)
{
	unsigned long frequent_actual;
	unsigned long global_actual;

	if(times == 0 || total_times == 0)
		return 0;

	frequent_actual = life_caculate_frequenct_page(times);
	global_actual = life_caculate_ideal_leveling(total_times);

	if(frequent_actual == 0 || global_actual == 0)
		return 0;

	//if(frequent_actual < FREQUENT_L)
	if(frequent_actual < EXPECT_L_S)
		return 1;
	if(global_actual < EXPECT_L_S)
		return 1;
	return 0;
}


/* 用来监测理想情况下的磨损均衡 */
int detect_process_ideal_wear_leveling(unsigned long total_times)
{
	unsigned long global_actual;
	unsigned long total_process;
	unsigned long per_cpu_life = 0;

	if(total_times < 1)
		return 0;

	global_actual = life_caculate_ideal_leveling(total_times);

	if( global_actual == 0)
		return 0;

	total_process = atomic64_read(&process_count);

	//printk("%s: total_count is %lu, total_process is %lu \n", __func__, total_times,total_process);
	
	if(total_process > 0)
	{
		per_cpu_life = (unsigned long)(EXPECT_L_S * total_process);
		//printk("%s: global_actual is %lu, per_cpu_life is %lu \n", __func__, global_actual, per_cpu_life);
		if(global_actual < per_cpu_life)
			return 1;
		return 0;
	}

	if(global_actual < EXPECT_L_S)
		return 1;
	return 0;
}

/* 用来计算没有磨损均衡时的使用寿命 */
int detect_process_without_wear_leveling(unsigned long times)
{
	unsigned long frequent_actual;

	if(times < 1)
		return 0;

	frequent_actual = life_caculate_frequenct_page(times);

	if(frequent_actual == 0)
		return 0;

	if(frequent_actual < EXPECT_L_S)
		return 1;

	return 0;
}

void print_record_process_info(Node node)
{
	int i;
	unsigned long total_count,frequent_count;
	unsigned long page_count = 0;

	while(node != NULL)
	{
		total_count = node->total_count;
		frequent_count = node->max_count;
		page_count = node->page_count;

		printk("PID is %d, totalCount is %llu, frequentCount is %llu, pageCount is %llu \n", 
			node->pid, node->total_count, node->max_count, node->page_count);

		print_detect_hash_table(node->pid);

		printk("\n\n");

	}
}


/* 会议版本的检测 */
#if 0
void record_malice_process(void)
{
	Table table = Table_header;
	Node node = NULL;
	int i;
	char is_or_no_suspect = 0, is_in_malicious = 0, is_in_suspect = 0,is_malicious = 0;
	char is_or_no_malicious = 0;
	unsigned long difference;
	unsigned long total_count,frequent_count;
	unsigned long page_count = 0;
	for(i = 0; i < MAX_KEY; i++)
	{
		node = table[i].next;
		while(node != NULL)
		{
			total_count = node->total_count;
			frequent_count = node->max_count;
			page_count = node->page_count;

			/* 2021-5-6 打印记录信息 */
			//print_record_process_info(node);


			//2021-5-7 测试均值与方差计算
			//unsigned long average = 0;
			//unsigned long deviation = 0;
			//calculate_average(&average, node);
			//calculate_deviation(average, &deviation, node);
			//printk("average is %llu, deviation is %llu \n", average, deviation);
			//printk("total_count is %llu, page_count is %llu, frequent_count is %llu \n", 
				//total_count, page_count, frequent_count);


			is_or_no_malicious = detect_process_ideal_wear_leveling(total_count);

			is_in_malicious = is_in_malicious_process_list(node->pid);
			/* 插入进入malicious process list，加入多线程要加锁 */
			if(!is_in_malicious)
			{
				insert_into_malicious_process(node->pid);
			}
			node = node->next;
		}
	}
	/* 检查完毕，释放所有node */
	free_all_node();
}
#endif

/*
 * 老版本的空间占用，无法很好衡量对DRAM的真实占用
 */
/*
int knapSack(int capacity)
{
    int len = 0;
    int i,j;
	int weight = 0;
    Suspect curr = suspect_header;

	if(curr == NULL)
		return -1;

    for(i = 1; curr != NULL; i++)
    {
        for(j = 1; j <= capacity; j++)
        {
        	//将占用空间容量修改为M
        	weight = curr->pageCount/256;
            if(j < weight)
            {
                V[i][j] = V[i-1][j];
            }
            else
            {
                V[i][j] = maxValue(V[i-1][j],
                    V[i-1][j - weight] + curr->averageCount * curr->deviation);
            }
        }
        curr = curr->next;
        len++;
    }

	char is_or_no_malicious = 0, is_in_malicious = 0;

    //判断哪些进程被选中
    j = capacity;
    curr = suspect_header;
    for(i = len; i >= 1; i--)
    {
    	printk("i is %d: step 0\n", i);
        if(V[i][j] > V[i-1][j])
        {
        	printk("i is %d: step 0.0\n", i);
        	if(curr == NULL)
    		{
    			printk("%s: curr is null \n", __func__);
				break;
    		}
            printk("PID %d is malicious process\n", curr->pid);

			is_in_malicious = is_in_malicious_process_list(curr->pid);
			
			printk("i is %d: step 1\n", i);
			
			weight = curr->pageCount/256;
            j = j - weight;

			// 插入进入malicious process list，加入多线程要加锁 
			if(!is_in_malicious)
			{
				printk("i is %d: step 2\n", i);
				insert_into_malicious_process(curr->pid);
				memoryCap = memoryCap - weight;
			}
			printk("i is %d, j is %d: step 3\n", i, j);

        }
		printk("i is %d: step 4.0\n", i);
        curr = curr->next;
		printk("i is %d: step 4\n", i);
    }

    return V[len][capacity];
}
*/

/* 基于背包找出最恶意的那些进程 */
void findMalicious(Suspect curr, int i, int j) {				//最优解情况
	int weight = get_occupySpace(curr->pid);
	unsigned long value = curr->averageCount * curr->deviation;

	if (i >= 0) {
        //相等的时候不是这个
		if (V[i][j] == V[i - 1][j]) {
			findMalicious(curr->next, i - 1, j);
		}
        //否则
		else if (j - weight >= 0 && V[i][j] == V[i - 1][j - weight] + value) {
			int is_in_malicious = is_in_malicious_process_list(curr->pid);
			if(!is_in_malicious)
			{
				printk("PID %d is malicious process, weight is %d, value is %lu\n", 
					curr->pid, weight, value);
				insert_into_malicious_process(curr->pid);
				memoryCap = memoryCap - weight;
			}
			findMalicious(curr->next, i - 1, j - weight);
		}
	}
}


/* 新版本的空间占用 */
int knapSack(int capacity)
{
    int len = 0;
    int i,j;
	int weight = 0;
	unsigned long value = 0;
    Suspect curr = suspect_header;
	Suspect tail = NULL;

	if(curr == NULL)
		return -1;

	//遍历数量
    for(i = 1; curr != NULL; i++)
    {
    	tail = curr;
		//将占用空间容量修改为M
		weight = get_occupySpace(curr->pid);
		value = curr->averageCount * curr->deviation;
		
		printk("%s: pid is %d, weight is %d, value is %lu, average is %lu, deviation is %lu\n",
			__func__, curr->pid, weight, value, curr->averageCount, curr->deviation);

		//遍历容量
        for(j = 1; j <= capacity; j++)
        {
			//如果放不下
            if(j < weight)
            {
                V[i][j] = V[i-1][j];
            }
            else
            {
            	//能放下时判断是否需要放
                V[i][j] = maxValue(V[i-1][j], V[i-1][j - weight] + value);
            }
        }
        curr = curr->next;
        //获取节点数量
        len++;
    }

	
    //判断哪些进程应该被迁移
    char is_in_malicious = 0;
    j = capacity;
	if(tail->next != NULL)
		printk("%s: tail->next is not null\n", __func__);
    curr = tail;

	
	i = len;
	printk("%s: i is %d\n", __func__, i);
    while(i > 0)
    {
        if(V[i][j] > V[i-1][j])
        {
			is_in_malicious = is_in_malicious_process_list(curr->pid);

			weight = get_occupySpace(curr->pid);
			value = curr->averageCount * curr->deviation;

			// 插入进入malicious process list，加入多线程要加锁 
			if(!is_in_malicious)
			{
				j = j - weight;
				if(j < 0)
				{
					printk("%s: j is %d, less than one\n",__func__,j);
					break;
				}
				printk("PID %d is malicious process, weight is %d, value is %lu\n", 
					curr->pid, weight, value);
				insert_into_malicious_process(curr->pid);
				memoryCap = memoryCap - weight;
			}
        }
		//往前遍历
        curr = curr->prev;
		i--;
    }

/*
	for(i = len; i >= 1; i--)
	{
		weight = get_occupySpace(curr->pid);
		value = curr->averageCount * curr->deviation;

		if(V[i][j] == V[i - 1][j]) {
			continue;
		}
		else if (j - weight >= 0 && V[i][j] == V[i - 1][j - weight] + value) 
		{
			is_in_malicious = is_in_malicious_process_list(curr->pid);
			if(!is_in_malicious)
			{
				printk("PID %d is malicious process, weight is %d, value is %lu\n", 
					curr->pid, weight, value);
				insert_into_malicious_process(curr->pid);
				memoryCap = memoryCap - weight;
			}
			j = j - weight;
		}
		curr = curr->next;
	}
*/

    return V[len][capacity];
}


#if 0
/* 期刊版本检测 */
void record_malice_process(void)
{
	Table table = Table_header;
	Node node = NULL;
	int i;
	char is_or_no_suspect = 0, is_in_suspect = 0;
	char is_or_no_malicious = 0, is_in_malicious = 0;
	/* 总的写流量和最频繁写入内存行的写流量 */
	unsigned long total_count,frequent_count;
	/* 页面数量 */
	unsigned long page_count = 0;
	/* 均值 */
	unsigned long average = 0;
	/* 方差 */
	unsigned long deviation = 0;
	for(i = 0; i < MAX_KEY; i++)
	{
		node = table[i].next;
		while(node != NULL)
		{
			total_count = node->total_count;
			frequent_count = node->max_count;
			page_count = node->page_count;

			/* 这里为可以进程 */
			is_or_no_suspect = detect_process_ideal_wear_leveling(total_count);

			is_in_suspect = is_in_suspect_process_list(node->pid);
			/* 插入进入malicious process list，加入多线程要加锁 */
			if(!is_in_suspect)
			{
				calculate_average(&average, node);
				calculate_deviation(average, &deviation, node);
				/* 记录均值方差之类的 */
				insert_into_suspect_process(node->pid, average, deviation, page_count);
			}
			node = node->next;
		}
	}

	//测试可疑进程的记录
	//print_suspect_process_info();
	//

	/* 在这里按照背包算法确认是否需要迁移 */
	if(memoryCap > 0)
		knapSack(memoryCap);
	zeroV();
	//free_all_suspect_process_list();
	printk("knapSack over, begin free\n");
	
	/* 检查完毕，释放所有node */
	free_all_node();
	printk("free over\n");
}
#endif

#if 1
void record_malice_process(void)
{
	Table table = Table_header;
	Node node = NULL;
	int i;
	char is_or_no_suspect = 0, is_in_malicious = 0, is_in_suspect = 0,is_malicious = 0;
	char is_or_no_malicious = 0;
	unsigned long difference;
	unsigned long total_count,frequent_count;
	unsigned int occupyspace;
	for(i = 0; i < MAX_KEY; i++)
	{
		node = table[i].next;
		while(node != NULL)
		{
			total_count = node->total_count;
			frequent_count = node->max_count;

			is_or_no_malicious = detect_process_ideal_wear_leveling(total_count);

			is_in_malicious = is_in_malicious_process_list(node->pid);
			
			occupyspace = get_occupySpace(node->pid);
			
			/* 插入进入malicious process list，加入多线程要加锁 */
			if(!is_in_malicious)
			{
				if(memoryCap > occupyspace)
				{
					insert_into_malicious_process(node->pid);
					memoryCap = memoryCap - occupyspace;
					printk("%s: pid is %d, size is %d\n",__func__, node->pid, occupyspace);
				}
			}
			node = node->next;
		}
	}
	/* 检查完毕，释放所有node */
	free_all_node();
}
#endif

/////////////////////////////////////////// remaining code ////////////////////////////////
/* total_times总的写次数 */
unsigned long life_caculate_remaining_ideal_leveling(unsigned long total_times)
{

	/* NVM拥有的块数 */
	unsigned long max_num_blocks;
	unsigned long average = 0;
	max_num_blocks = BLOCKS_MAX;

	//average = get_global_write_count_base();
	///////// wwb 20210127 ///////////
	average = atomic64_read(&count_average);
	
	if(total_times >= 1)
	{
		//printk("%s: total_times is %lu, average is %lu\n", __func__, total_times, average);
		return (max_num_blocks * (MAX_ENDURANCE-average)) / total_times;
	}
	
	return 0;
}


/* 用来监测理想情况下的磨损均衡 */
int detect_global_remaining_ideal_wear_leveling(unsigned long total_times)
{
	unsigned long global_actual;
	unsigned long remainning_expect = 0;

	if(total_times < 1)
		return 0;

	global_actual = life_caculate_remaining_ideal_leveling(total_times);

	if( global_actual == 0)
		return 0;

	remainning_expect = EXPECT_L_S - pm_use_time;
	if(global_actual < remainning_expect)
	{
		return 1;
	}
	return 0;
}
//////////////////////////////////////////  end ///////////////////////////////////////////


/* 这个用来加到各个写入口进行统计 */
void record_one_sec_write_info(int pid, unsigned long block_num)
{
	//return;
	mutex_lock(&detect_prepare_lock);
	detect_prepare();
	mutex_unlock(&detect_prepare_lock);
	
	if(detect_mode == 0)
		return;

	if(detect_mode == 1)
	{
		atomic64_inc(&write_traffic);
		/*
		mutex_lock(&global_mutex_lock);
		max_update++;
		mutex_unlock(&global_mutex_lock);
		*/
		return;
	}

	/* 这里记录每个进程一秒写窗口的写入量 */
	if(detect_mode == 2)
	{
		mutex_lock(&process_mutex_lock);
		update_hash_table(pid, block_num);
		mutex_unlock(&process_mutex_lock);
	}
}



