/*
 * 用来进行 PRINTK DEBUG
 */
#ifndef W_DEBUG_H
#define W_DEBUG_H

#include "nova.h"
#include "nova_def.h"

/* 定义宏开关 */
//#define DEBUG_W
#ifdef DEBUG_W
#define DEBUG(fmt, args...) printk( KERN_DEBUG "DEBUG_W: " fmt, ## args)
#else
#define DEBUG(fmt, args...)
#endif

//#define DEBUG_EVICT_INODE
#ifdef DEBUG_EVICT_INODE
#define DEBUG_E(fmt, args...) printk( KERN_DEBUG "DEBUG_E: " fmt, ## args)
#else
#define DEBUG_E(fmt, args...)
#endif

//#define DEBUG_MIGATE
#ifdef DEBUG_MIGATE
#define DEBUG_M(fmt, args...) printk( KERN_DEBUG "DEBUG_M: " fmt, ## args)
#else
#define DEBUG_M(fmt, args...)
#endif

#define DEBUG_DEAD_LOCK
#ifdef DEBUG_DEAD_LOCK
#define DEBUG_D(fmt, args...) printk( KERN_DEBUG "DEBUG_D: " fmt, ## args)
#else
#define DEBUG_D(fmt, args...)
#endif

//#define DEBUG_FREE_BLOCKS
#ifdef DEBUG_FREE_BLOCKS
#define DEBUG_F(fmt, args...) printk( KERN_DEBUG "DEBUG_F: " fmt, ## args)
#else
#define DEBUG_F(fmt, args...)
#endif

//#define DEBUG_INODE_MAPPING_TABLE
#ifdef DEBUG_INODE_MAPPING_TABLE
#define debug_inode_mapping_table(fmt, args...) printk( KERN_DEBUG "MAPPING TABLE: " fmt, ## args)
#else
#define debug_inode_mapping_table(fmt, args...)
#endif

#define DEBUG_FREELIST
#ifdef DEBUG_FREELIST
#define debug_freelist(fmt, args...) printk( KERN_DEBUG "FREELIST: " fmt, ## args)
#else
#define debug_freelist(fmt, args...)
#endif


#endif 

