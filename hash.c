#include "hash.h"

/* 用来做各个PID检测 */
Table Table_header;
/* 用来做全局检测 */
DetectTable global_detect_table;

char should_wear_leveling = 0;

extern atomic64_t process_count;


void init_hash_table(void)
{
    Table init_table = (Table)kmalloc(sizeof(Hashtable) * MAX_KEY, GFP_KERNEL);
    int i;
    for(i = 0; i < MAX_KEY; i++)
    {
        init_table[i].next = NULL;
    }
    Table_header = init_table;
}

char delete_from_hash_table(int pid)
{
    int key = pid % MAX_KEY;
	Node node = NULL;
	Node pre_node = NULL;
    if(Table_header[key].next == NULL)
    {
        printk("cant find the pid %d \n", pid);
        return -1;
    }

    node = Table_header[key].next;

    if(node -> pid == pid)
    {
        Table_header[key].next = node -> next;
        node -> next = NULL;
        kfree(node);
        return 1;
    }

    pre_node = Table_header[key].next;
    while(node != NULL)
    {
        if(node -> pid == pid)
        {
            pre_node -> next = node -> next;
            node -> next = NULL;
            kfree(node);
            return 1;
        }
        pre_node = node;
        node = node -> next;
    }
    printk("cant find the pid %d \n", pid);
	return 0;
}

Node search_from_hash_table(int pid)
{
    int key = pid % MAX_KEY;
	Node node = NULL;

    if(Table_header[key].next == NULL)
    {
        printk("cant find the pid %d \n",pid);
        return NULL;
    }

    node = Table_header[key].next;

    while(node != NULL)
    {
        if(node -> pid == pid)
        {
            return node;
        }
        node = node -> next;
    }

    printk("cant find the pid %d \n",pid);
    return NULL;
}

void print_hash_table(void)
{
    int i;
	Node node = NULL;
    for(i = 0; i < MAX_KEY; i++)
    {
        node = Table_header[i].next;
        if(node == NULL)
            continue;
        while(node != NULL)
        {
            printk("KEY %d; pid is %d, total_count is %lu max_count is %lu \n", \
				i, node -> pid, node -> total_count, node->max_count);
            node = node -> next;
        }
    }
}

/* wwb add 2020-7-4 */
void init_detect_hash_table(Node node)
{
    int i;
    node->detect_table = (DetectTable)kmalloc(sizeof(Detecthashtable) * MAX_DETECT_KEY, GFP_KERNEL);
    for(i = 0; i < MAX_DETECT_KEY; i++)
    {
        node->detect_table[i].next = NULL;
        node->detect_table[i].node_count = 0;
        node->detect_table[i].max_update = 0;
    }
}

char insert_into_hash_table(int pid,unsigned long block_num)
{
    int key = pid % MAX_KEY;
    Node node_head = Table_header[key].next;
	Node node = NULL;

	atomic64_inc(&process_count);
	
    if(node_head == NULL)
    {
        node = (Node)kmalloc(sizeof(Hashnode),GFP_KERNEL);
        if(node == NULL)
            return -1;
        node ->pid = pid;
        node ->max_count = 1;
        node ->total_count = 1;
		node ->page_count = 0;

		node->max_page_writecount = block_write_count[block_num];
		
        node ->next = NULL;

		/* wwb add 2020-7-4 */
        //init_detect_hash_table(node);
        //insert_detect_hash_node(node,block_num);
		/* end 2020-7-4 */

		#ifdef RECORD_PER_PROCESS_BLOCKNUM
        init_detect_hash_table(node);
        insert_detect_hash_node(node,block_num);	
		#endif
		
        Table_header[key].next = node;
        return 1;
    }

    while(node_head->next != NULL)
    {
        node_head = node_head -> next;
    }

    node = (Node)kmalloc(sizeof(Hashnode),GFP_KERNEL);
    if(node == NULL)
    {
        return -1;
    }
    node->pid = pid;
    node->max_count = 1;
    node->total_count = 1;
	node->page_count = 0;

	node->max_page_writecount = block_write_count[block_num];
	
    node->next = NULL;
    node_head->next = node;
	/* wwb add 2020-7-4 */
    //init_detect_hash_table(node);
    //insert_detect_hash_node(node,block_num);
	/* end 2020-7-4 */

#ifdef RECORD_PER_PROCESS_BLOCKNUM
	init_detect_hash_table(node);
	insert_detect_hash_node(node,block_num);	
#endif

	
    return 1;
}


/* 2021-5-6 将页面信息也进行记录 */
char update_hash_table(int pid,unsigned long block_num)
{
    int key = pid % MAX_KEY;
	Node node = NULL;

    if(Table_header[key].next == NULL)
    {
        return insert_into_hash_table(pid,block_num);
    }

    node = Table_header[key].next;

    while(node != NULL)
    {
        if(node -> pid == pid)
        {
            node ->total_count ++;

			//insert_detect_hash_node(node,block_num);
			#ifdef RECORD_PER_PROCESS_BLOCKNUM
			insert_detect_hash_node(node,block_num);
			#endif
			
            return 1;
        }
        node = node ->next;
    }
    return insert_into_hash_table(pid,block_num);
}

void insert_detect_hash_node(Node node, unsigned long block_num)
{
    DetectNode detect_node,curr;
    int i,count;
    int key;

    key = block_num % MAX_DETECT_KEY;
    count = node->detect_table[key].node_count;

    /* 如果该key对应的节点没有，直接插入 */
    if(count == 0)
    {
        detect_node = (DetectNode)kmalloc(sizeof(Detecthashnode),GFP_KERNEL);
		
		if(detect_node == NULL)
			return;
		
        detect_node->block_num = block_num;
        detect_node->write_count = 1;
        detect_node->next = NULL;

        node->detect_table[key].next = detect_node;
        node->detect_table[key].node_count = 1;
        node->detect_table[key].max_update = 1;

		node->page_count++;
        return;
    }

    curr = node->detect_table[key].next;
    /* 在hash_table内部，不包含末尾 */
    for(i = 0; i < count - 1; i++)
    {
        /* already in the table */
        if(curr->block_num == block_num)
        {
            curr->write_count++;
            if(curr->write_count > node->detect_table[key].max_update)
            {
                node->detect_table[key].max_update = curr->write_count;
            }
            if(curr->write_count > node->max_count)
                node->max_count = curr->write_count;

			if(node->max_page_writecount < block_write_count[block_num])
				node->max_page_writecount = block_write_count[block_num];

            return;
        }
        curr = curr->next;
    }

	/* 如果刚好是最后一个节点 */
    if(curr->block_num == block_num)
    {
        curr->write_count++;
        if(curr->write_count > node->detect_table[key].max_update)
        {
            node->detect_table[key].max_update = curr->write_count;
        }
		if(curr->write_count > node->max_count)
			node->max_count = curr->write_count;

		if(node->max_page_writecount < block_write_count[block_num])
			node->max_page_writecount = block_write_count[block_num];

        return;
    }

    /* 不在现有链表里面，则插入到尾部 */
    detect_node = (DetectNode)kmalloc(sizeof(Detecthashnode),GFP_KERNEL);
	if(detect_node == NULL)
		return;
	
    detect_node->block_num = block_num;
    detect_node->next = NULL;
    detect_node->write_count = 1;

    curr->next = detect_node;
    node->detect_table[key].node_count++;

	node->page_count++;
}

void print_detect_hash_table(int pid)
{
    int i,j,count;
    DetectNode curr;
    Node node = search_from_hash_table(pid);
    if(node == NULL)
        return;
    for(i = 0 ; i < MAX_DETECT_KEY; i++)
    {
        printk("key i %d, max_update is %lu \n",i, node->detect_table[i].max_update);
        count = node->detect_table[i].node_count;
        curr = node->detect_table[i].next;
        for(j = 0; j < count; j++)
        {
            printk("block number is %lu write count is %lu \n", curr->block_num, curr->write_count);
            curr = curr ->next;
        }
    }
}

/* 删除某个pid对应的所有记录，该函数可以重写，将Node改为pid */
void delete_from_detect_hash_table(Node node)
{
    DetectNode detect_node;
    DetectNode curr_node;
    int i,j,count;
    for(i = 0; i < MAX_DETECT_KEY; i++)
    {
        count = node->detect_table[i].node_count;
        for(j = 0; j < count - 1; j++)
        {
            detect_node = node->detect_table[i].next;
            curr_node = detect_node->next;
            kfree(detect_node);
            detect_node = NULL;
            node->detect_table[i].next = curr_node;
            node->detect_table[i].node_count--;
        }
        kfree(detect_node);
        detect_node = NULL;
        node->detect_table[i].node_count = 0;
        node->detect_table[i].next = NULL;
    }
	kfree(node->detect_table);
}

/* 释放所有空间，除了Table_header的空间 */
void free_all_node(void)
{
    int i,j,k;
    Node node = NULL;
	Node next_node = NULL;
    for(i = 0; i < MAX_KEY; i++)
    {
        node = Table_header[i].next;
        if(node == NULL)
            continue;
        while(node->next != NULL)
        {
            next_node = node->next;
			#ifdef RECORD_PER_PROCESS_BLOCKNUM
			delete_from_detect_hash_table(node);
			#endif
			kfree(node);
			node = next_node;
        }
#ifdef RECORD_PER_PROCESS_BLOCKNUM
		delete_from_detect_hash_table(node);
#endif
        kfree(node);
		Table_header[i].next = NULL;
    }
}

/* 增加global的相关重写函数 */
void init_global_detect_table(void)
{
    int i;
    DetectTable detect_table = (DetectTable)kmalloc(sizeof(Detecthashtable) * MAX_DETECT_KEY, GFP_KERNEL);
    for(i = 0; i < MAX_DETECT_KEY; i++)
    {
        detect_table[i].max_update = 0;
        detect_table[i].next = NULL;
        detect_table[i].node_count = 0;
    }
    global_detect_table = detect_table;
}

void insert_into_global_detect_table(unsigned long block_num)
{
    DetectNode detect_node = NULL;
	DetectNode curr = NULL;
    int i,count;
    int key;

    key = block_num % MAX_DETECT_KEY;
	
    count = global_detect_table[key].node_count;

    /* first */
    if(count == 0)
    {
        detect_node = (DetectNode)kmalloc(sizeof(Detecthashnode),GFP_KERNEL);
		if(detect_node == NULL)
		{
			printk("kmalloc fail out insert into global detect table  \n");
			return;
		}
        detect_node->block_num = block_num;
        detect_node->write_count = 1;
        detect_node->next = NULL;

        global_detect_table[key].next = detect_node;
        global_detect_table[key].max_update = 1;
        global_detect_table[key].node_count = 1;
        return;
    }

    curr = global_detect_table[key].next;
	
    for(i = 0; i < count - 1; i++)
    {
        if(curr->block_num == block_num)
        {
            curr->write_count++;
            if(curr->write_count > global_detect_table[key].max_update)
            {
                global_detect_table[key].max_update = curr->write_count;
            }
            return;
        }
		
        curr = curr->next;
    }

    if(curr->block_num == block_num)
    {
        curr->write_count++;
        if(curr->write_count > global_detect_table[key].max_update)
        {
            global_detect_table[key].max_update = curr->write_count;
        }
        return;
    }

    detect_node = (DetectNode)kmalloc(sizeof(Detecthashnode),GFP_KERNEL);

	if(detect_node == NULL)
	{
		printk("kmalloc fail out insert into global detect table  \n");
		return;
	}
    detect_node->block_num = block_num;
    detect_node->write_count = 1;
    detect_node->next = NULL;
	
    curr->next = detect_node;
    global_detect_table[key].node_count++;
    return;
}

void delete_global_detect_hash_table(void)
{
    DetectNode detect_node;
    DetectNode curr_node;
    int i,j,count;
    for(i = 0; i < MAX_DETECT_KEY; i++)
    {
        count = global_detect_table[i].node_count;
        for(j = 0; j < count - 1; j++)
        {
            detect_node = global_detect_table[i].next;
            curr_node = detect_node->next;
            kfree(detect_node);
            detect_node = NULL;
            global_detect_table[i].next = curr_node;
            global_detect_table[i].node_count--;
        }
        kfree(detect_node);
        detect_node = NULL;
        global_detect_table[i].node_count = 0;
        global_detect_table[i].next = NULL;
    }
}

void print_global_detect_hash_table(void)
{
    DetectNode curr_node;
    int i,j,count;
    for(i = 0; i < MAX_DETECT_KEY; i++)
    {
        count = global_detect_table[i].node_count;
        curr_node = global_detect_table[i].next;
        for(j = 0; j < count; j++)
        {
            printk("curr_node block num is %d, write_count is %d \n",curr_node->block_num,curr_node->write_count);
            curr_node = curr_node->next;
        }
    }
}

unsigned long get_global_detect_frequent_update(void)
{
    static unsigned long frequent_update = 0;
    int i;
    frequent_update = global_detect_table[0].max_update;
    for(i = 0; i < MAX_DETECT_KEY; i++)
    {
        if(frequent_update < global_detect_table[i].max_update)
            frequent_update = global_detect_table[i].max_update;
    }
    return frequent_update;
}

void free_table(void)
{
	kfree(Table_header);
	Table_header = NULL;
	kfree(global_detect_table);
	global_detect_table = NULL;
}

/*
unsigned long search_in_suspect_process_list(int pid)
{
    Suspect p_curr;
    p_curr = suspect_header;
    while(p_curr != NULL)
    {
		if(p_curr->pid == pid)
			return p_curr->suspect_max_page_count;
    }
	printk("%s error , cant find the suspect process \n",__func__);
	return 0;
}
*/
