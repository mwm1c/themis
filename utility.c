#include "utility.h"

/* 需要把数据集传递进来 */
void calculate_average(unsigned long *average, Node node)
{
// for check
	//unsigned long sum = 0;
	//unsigned long num = 0;
    //int i,j,count;
    //DetectNode curr;
// for check

	if(node->page_count != 0)
		*average = (node->total_count) / (node->page_count);
	else
		*average = 0;

    //for(i = 0 ; i < MAX_DETECT_KEY; i++)
    //{
    //    count = node->detect_table[i].node_count;
    //    curr = node->detect_table[i].next;
    //    for(j = 0; j < count; j++)
    //    {
    //        sum += curr->write_count;
	//		num++;
    //        curr = curr ->next;
    //    }
    //}

	//if(sum != node->total_count)
	//	printk("sum is %llu, total_count is %llu\n", sum, node->total_count);

	//if(num != node->page_count)
	//	printk("num is %llu, page_count is %llu\n", num, node->page_count);

}

void calculate_deviation(unsigned long average, unsigned long *deviation, Node node)
{
    int i,j,count;
    DetectNode curr;
    for(i = 0 ; i < MAX_DETECT_KEY; i++)
    {
        count = node->detect_table[i].node_count;
        curr = node->detect_table[i].next;
        for(j = 0; j < count; j++)
        {
			*deviation += ((curr->write_count - average) * (curr->write_count - average));
            curr = curr ->next;
        }
    }

	*deviation = *deviation / node->page_count;	
}

int maxValue(int a, int b)
{
	if(a > b)
		return a;
	return b;
}
