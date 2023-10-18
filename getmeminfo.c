#include "getmeminfo.h"

void print_mem_info(Mem_info *o)
{
	printk("MemTotal      : %d\n",o->total);
	printk("MemFree       : %d\n",o->free);
    printk("MemAvailable  : %d\n",o->available);
    printk("MemBuffers    : %d\n",o->buffers);
    printk("MemCached     : %d\n",o->cached);
    printk("MemSwapCached : %d\n",o->swap_cached);
    printk("MemSwapTotal  : %d\n",o->swap_total);
    printk("MemSwapFree   : %d\n",o->swap_free);
	printk("\n");
}

/* 函数存在bug，无法完全解析 */
void get_mem_info(Mem_info *o)
{
    struct file* fpMemInfo = filp_open("/proc/meminfo", O_RDONLY, 0);
    if (NULL == fpMemInfo)
    {
        return ;
    }
    int i = 0;
    int value;
    char name[1024] = {0};
    char line[1024] = {0};
    int nFiledNumber = 2;
    int nMemberNumber = 5;

	mm_segment_t old_fs;
	old_fs = get_fs();
	set_fs(KERNEL_DS);

	int offset = 0;

	vfs_read(fpMemInfo, line, sizeof(line) - 1, &fpMemInfo->f_pos);

#if 0
	sscanf(&line[offset], "%s%u", name, &value);
	if (0 == strcmp(name, "MemTotal:"))
	{
		++i;
		o->total = value;
	}
	else if (0 == strcmp(name, "MemFree:"))
	{
		++i;
		o->free = value;
	}
	else if (0 == strcmp(name, "MemAvailable:"))
	{
		++i;
		o->available = value;
	}
	else if (0 == strcmp(name, "Buffers:"))
	{
		++i;
		o->buffers = value;
	}
	else if (0 == strcmp(name, "Cached:"))
	{
		++i;
		o->cached = value;
	}
	printk("MemTotal      : %d\n\n",o->total);
#endif

#if 1
	while (i <= nFiledNumber)
	{
		sscanf(&line[offset], "%s%u", name, &value);
		offset = strlen(name) + sizeof(value);
		printk("offset is %d, \n",offset);
		
		if (0 == strcmp(name, "MemTotal:"))
		{
			++i;
			o->total = value;
		}
		else if (0 == strcmp(name, "MemFree:"))
		{
			++i;
			o->free = value;
		}
		else if (0 == strcmp(name, "MemAvailable:"))
		{
			++i;
			o->available = value;
		}
		else if (0 == strcmp(name, "Buffers:"))
		{
			++i;
			o->buffers = value;
		}
		else if (0 == strcmp(name, "Cached:"))
		{
			++i;
			o->cached = value;
		}
		else
			++i;
	}

	print_mem_info(o);
#endif


	set_fs(old_fs);
	filp_close(fpMemInfo, NULL);	


#if 0
    while (fgets(line, sizeof(line) - 1, fpMemInfo))
    {
        if (sscanf(line, "%s%u", name, &value) != nFiledNumber)
        {
            continue;
        }
        if (0 == strcmp(name, "MemTotal:"))
        {
            ++i;
            o->total = value;
        }
        else if (0 == strcmp(name, "MemFree:"))
        {
            ++i;
            o->free = value;
        }
        else if (0 == strcmp(name, "MemAvailable:"))
        {
            ++i;
            o->available = value;
        }
        else if (0 == strcmp(name, "Buffers:"))
        {
            ++i;
            o->buffers = value;
        }
        else if (0 == strcmp(name, "Cached:"))
        {
            ++i;
            o->cached = value;
        }
        if (i == nMemberNumber)
        {
            break;
        }
    }
    // system("free");
    // system("cat /proc/meminfo");
    
    printf("MemFree       : %d\n",o->free);
    printf("MemAvailable  : %d\n",o->available);
    printf("MemBuffers    : %d\n",o->buffers);
    printf("MemCached     : %d\n",o->cached);
    printf("MemSwapCached : %d\n",o->swap_cached);
    printf("MemSwapTotal  : %d\n",o->swap_total);
    printf("MemSwapFree   : %d\n",o->swap_free);
#endif

}

