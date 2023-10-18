/*
 * 工具类，提供均值和方差的计算	
 *
 * 实现背包算法
 */

#ifndef UITITY_H
#define UITITY_H

#include <linux/kernel.h>
#include "hash.h"

void calculate_average(unsigned long *average, Node node);

void calculate_deviation(unsigned long average, unsigned long *deviation, Node node);

int maxValue(int a, int b);

#endif
