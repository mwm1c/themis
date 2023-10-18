#include "record.h"

//恶意pid list头部
Malicious malicious_header = NULL;

/* 2021-5-11 */
Suspect suspect_header = NULL;

/* 背包问题中用于存储中间值的数组 */
int **V;


struct mutex global_mutex_lock;
struct mutex process_mutex_lock;
struct mutex detect_prepare_lock;

BWC global_blk_write_count = NULL;

void init_malicious_list_lock()
{
	mutex_init(&global_mutex_lock);
	mutex_init(&process_mutex_lock);
	mutex_init(&detect_prepare_lock);
}

/* 记录恶意pid相关 */
void insert_into_malicious_process(int pid)
{
    Malicious p_node,p_curr;
    p_node = (Malicious)kmalloc(sizeof(MaliciousProcess),GFP_KERNEL);
    p_node->pid = pid;
    p_node->next = NULL;
    if(malicious_header == NULL)
    {
        malicious_header = p_node;
        return;
    }
    p_curr = malicious_header;
    while(p_curr->next != NULL)
    {
        p_curr = p_curr->next;
    }
    p_curr->next = p_node;
}

void delete_from_malicious_process(int pid)
{
    Malicious p_prev,p_curr;
    if(malicious_header == NULL)
    {
        return;
    }

    p_curr = malicious_header;
    p_prev = malicious_header;

    if(malicious_header->pid == pid)
    {
        malicious_header = malicious_header->next;
		kfree(p_curr);
		return;
    }

    while(p_curr != NULL)
    {
        if(p_curr->pid == pid)
        {
            p_prev->next = p_curr->next;
            kfree(p_curr);
            return;
        }
        p_prev = p_curr;
        p_curr = p_curr->next;
    }
}

void print_malicious_process_info(void)
{
    Malicious p_curr = malicious_header;
	if(p_curr == NULL)
		printk("There is no malicious process \n");
    while(p_curr != NULL)
    {
        printk("malicious pid is %d\n",p_curr->pid);
        p_curr = p_curr->next;
    }
}

char is_in_malicious_process_list(int pid)
{
    Malicious p_curr = malicious_header;
    while(p_curr != NULL)
    {
        if(p_curr->pid == pid)
            return 1;
        p_curr = p_curr->next;
    }
    return 0;
}

void free_all_malicious_process_list(void)
{
    Malicious p_curr,p_next;
    p_curr = malicious_header;
    while(p_curr != NULL)
    {
        p_next = p_curr->next;
        kfree(p_curr);
        p_curr = p_next;
    }
	malicious_header = NULL;
}

/* suspect 2021-5-11 */
/* 添加可疑进程的代码 */
insert_into_suspect_process(int pid, unsigned long ac, unsigned long dev, unsigned long pc)
{
    Suspect p_node,p_curr;
    p_node = (Suspect)kmalloc(sizeof(SuspectProcess),GFP_KERNEL);
    p_node->pid = pid;
	p_node->averageCount = ac;
	p_node->deviation = dev;
	p_node->pageCount = pc;
    p_node->next = NULL;
	p_node->prev = NULL;
    if(suspect_header == NULL)
    {
        suspect_header = p_node;
        return;
    }
    p_curr = suspect_header;
    while(p_curr->next != NULL)
    {
        p_curr = p_curr->next;
    }
    p_curr->next = p_node;
	p_node->prev = p_curr;
}

void delete_from_suspect_process(int pid)
{
    Suspect p_prev,p_curr;
    if(suspect_header == NULL)
    {
        return;
    }

	p_curr = suspect_header;
    p_prev = suspect_header;
    if(suspect_header->pid == pid)
    {
        suspect_header = suspect_header->next;
		kfree(p_curr);
		return;
    }

    while(p_curr != NULL)
    {
        if(p_curr->pid == pid)
        {
            p_prev->next = p_curr->next;
            kfree(p_curr);
            return;
        }
        p_prev = p_curr;
        p_curr = p_curr->next;
    }
}

void print_suspect_process_info(void)
{
    Suspect p_curr = suspect_header;
	if(p_curr == NULL)
		printk("There is no Suspect process \n");
    while(p_curr != NULL)
    {
        printk("Suspect: pid is %d, average is %llu, deviation is %llu, pageCount is %llu\n",
			p_curr->pid, p_curr->averageCount, p_curr->deviation, p_curr->pageCount);
        p_curr = p_curr->next;
    }
}

char is_in_suspect_process_list(int pid)
{
    Suspect p_curr = suspect_header;
    while(p_curr != NULL)
    {
        if(p_curr->pid == pid)
            return 1;
        p_curr = p_curr->next;
    }
    return 0;
}

void free_all_suspect_process_list(void)
{
    Suspect p_curr,p_next;
    p_curr = suspect_header;
    while(p_curr != NULL)
    {
        p_next = p_curr->next;
        kfree(p_curr);
        p_curr = p_next;
    }
	suspect_header = NULL;
}

/* 保存动态规划0-1背包的中间解 */
void zeroV(void)
{
	int i,j;
	for(i = 0; i < sizeV; i++)
		for(j = 0; j < sizeV; j++)
			V[i][j] = 0;
}

void initV(void)
{
	int i;
	V = (int **)vmalloc(sizeof(int *) * sizeV);
	for(i = 0; i < sizeV; i++)
		V[i] = (int *)vmalloc(sizeof(int) * sizeV);

	zeroV();
}

void printV(void)
{
	int i, j;
	for(i = 0; i < sizeV; i++)
	{
		for(j = 0; j < sizeV; j++)
			printk("%d ", V[i][j]);
		printk("\n");
	}
}


