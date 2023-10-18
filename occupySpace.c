#include "occupySpace.h"

#define spaceTableVolume 10
#define byteToM 1048576

spacePtr spaceTable;

void init_spaceTable(void)
{
	spacePtr ptr = vmalloc(sizeof(Space) * spaceTableVolume);
	int i = 0;
	for(i = 0; i < spaceTableVolume; i++)
		ptr[i].next = NULL;

	spaceTable = ptr;
}

void insert_into_spaceTable(int pid, int size)
{
	int key = pid % spaceTableVolume;

	if(spaceTable[key].next == NULL)
	{
		spacePtr node = vmalloc(sizeof(Space));
		node->pid = pid;
		node->size = size;
		node->next = NULL;
		spaceTable[key].next = node;
		return;
	}

	spacePtr curr = spaceTable[key].next;

	/* 末尾的前一个 */
	while(curr->next != NULL)
	{
		if(curr->pid == pid)
		{
			curr->size = size;
			return;
		}
		curr = curr->next;
	}

	if(curr->pid == pid)
	{
		curr->size = size;
		return;
	}

	spacePtr node = vmalloc(sizeof(Space));
	node->pid = pid;
	node->size = size;
	node->next = NULL;

	curr->next = node;	
}


void free_spaceNode(void)
{
	int i = 0;
	for(i = 0; i < spaceTableVolume; i++)
	{
		spacePtr curr = spaceTable[i].next;
		spacePtr next = curr;
		while(curr != NULL)
		{
			next = curr->next;
			vfree(curr);
			curr = next;
		}
	}
	vfree(spaceTable);
	spaceTable = NULL;
}

void print_spaceInfo(void)
{
	int i = 0;
	for(i = 0; i <spaceTableVolume; i++)
	{
		spacePtr curr = spaceTable[i].next;
		while(curr != NULL)
		{
			printk("%s: pid is %d, size is %lu \n", 
				__func__, curr->pid, curr->size);
			curr = curr->next;
		}
	}
}

unsigned int get_occupySpace(int pid)
{
	int key = pid % spaceTableVolume;

	spacePtr curr = spaceTable[key].next;

	while(curr != NULL)
	{
		if(curr->pid == pid)
		{
			return (unsigned int)(curr->size / byteToM);
		}
		curr = curr->next;
	}

	printk("didn't find this pid \n");
	return 0;
}
