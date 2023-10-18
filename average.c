#include "average.h"

/* 增加两个计数器，加速计算平均值 */
/* 当count_hand达到block数时average加1 */
atomic64_t count_hand = ATOMIC64_INIT(0);
atomic64_t count_average = ATOMIC64_INIT(0);

