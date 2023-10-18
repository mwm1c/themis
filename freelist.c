#include "freelist.h"

FH free_header = NULL;
spinlock_t freelist_lock;

void freehead_init(void)
{
    free_header = (FH)vmalloc(sizeof(FreeHead));
    free_header->counts = 0;
    free_header->node_head = NULL;
    free_header->node_tail = NULL;

	spin_lock_init(&freelist_lock);
}

/* 头插 */
void insert_freenode(struct super_block *sb, unsigned long addr)
{
    FN tempnode = NULL;
    FN prevnode = NULL;
    FN node = NULL;
	struct nova_inode *pi;
    node = (FN)vmalloc(sizeof(FreeNode));
	if(node == NULL)
	{
		printk("%s: node is NULL",__func__);
		return;
	}
    node->addr = addr;
    node->prev = NULL;
    node->next = NULL;

	pi = nova_get_inode_by_ino(sb, NOVA_INODETABLE_INO);
	if(pi == NULL)
	{
		printk("insert_freenode error: pi is null \n");
		return -1;
	}

    if(free_header->counts == 0)
    {
        free_header->node_head = node;
        free_header->node_tail = node;
        free_header->counts += 1;
        return;
    }

    tempnode = free_header->node_head;

    free_header->node_head = node;
    tempnode->prev = node;
    node->next = tempnode;
    free_header->counts += 1;

    if(free_header->counts >= MAX_NODE)
    {
        prevnode = free_header->node_tail->prev;
		//释放掉inode page
		/* 2021-3-22 写错了这一句 */
		//nova_free_log_blocks(sb, pi, (free_header->node_head->addr >> 12), 1);
		nova_free_log_blocks(sb, pi, (free_header->node_tail->addr >> 12), 1);

        vfree(free_header->node_tail);
        free_header->node_tail = prevnode;
		free_header->node_tail->next = NULL;
        free_header->counts -= 1;
    }
}

void delete_tail(FN node)
{
    //tail指向前一个节点
    free_header->node_tail = node->prev;
    //free当前节点
    vfree(node);
    //置为NULL
    node = NULL;
}

void free_all_freenode(void)
{
    FN node = NULL;
    while(free_header->counts > 0)
    {
        node = free_header->node_head;
        free_header->node_head = node->next;
        free_header->counts--;
        vfree(node);
    }
}

void free_freeheader(void)
{
	vfree(free_header);
}


