#ifndef FREELIST__H
#define FREELIST__H

#include "nova.h"
#include "nova_def.h"
#include "debug.h"
#include <linux/vmalloc.h>

#define MAX_NODE 1000

typedef struct freenode
{
    unsigned long addr;
    struct freenode * prev;
    struct freenode * next;
}FreeNode, *FN;

typedef struct freehead
{
    FN node_head;
    FN node_tail;
    unsigned int counts;
}FreeHead, *FH;

extern FH free_header;
extern spinlock_t freelist_lock;


void freehead_init(void);
/* 头插 */
void insert_freenode(struct super_block *sb, unsigned long addr);
void delete_tail(FN node);
void free_all_freenode(void);
void free_freeheader(void);

#endif

