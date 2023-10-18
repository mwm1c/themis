#ifndef __OCCUPY_SPACE__H
#define __OCCUPY_SPACE__H

#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/types.h>
#include <linux/vmalloc.h>

typedef struct space{
	int pid;
	unsigned long size;
	struct space *next;
}Space, *spacePtr;

extern spacePtr spaceTable;

void init_spaceTable(void);
void insert_into_spaceTable(int pid, int size);
void free_spaceNode(void);
void print_spaceInfo(void);
unsigned int get_occupySpace(int pid);



#endif
