/*
 * BRIEF DESCRIPTION
 *
 * Symlink operations
 *
 * Copyright 2015-2016 Regents of the University of California,
 * UCSD Non-Volatile Systems Lab, Andiry Xu <jix024@cs.ucsd.edu>
 * Copyright 2012-2013 Intel Corporation
 * Copyright 2009-2011 Marco Stornelli <marco.stornelli@gmail.com>
 * Copyright 2003 Sony Corporation
 * Copyright 2003 Matsushita Electric Industrial Co., Ltd.
 * 2003-2004 (c) MontaVista Software, Inc. , Steve Longerbeam
 *
 * This program is free software; you can redistribute it and/or modify it
 *
 * This file is licensed under the terms of the GNU General Public
 * License version 2. This program is licensed "as is" without any
 * warranty of any kind, whether express or implied.
 */

#include <linux/fs.h>
#include <linux/namei.h>
#include <linux/version.h>
#include "nova.h"
#include "writecount.h"

int nova_block_symlink(struct super_block *sb, struct nova_inode *pi,
	struct inode *inode, u64 log_block,
	unsigned long name_blocknr, const char *symname, int len)
{
	struct nova_file_write_entry *entry;
	struct nova_inode_info *si = NOVA_I(inode);
	struct nova_inode_info_header *sih = &si->header;
	u64 block;
	u32 time;
	char *blockp;

	/* First copy name to name block */
	block = nova_get_block_off(sb, name_blocknr, NOVA_BLOCK_TYPE_4K);
	blockp = (char *)nova_get_block(sb, block);

	nova_memunlock_block(sb, blockp);
	memcpy_to_pmem_nocache(blockp, symname, len);

	/* wwb add 2020-6-27 */
	u64 block_nums = get_block_number_by_virt(sb,(unsigned long)blockp);
	log_block_write_count_increase(block_nums);
	//printk("WWB: len is %d nova_block_symlink block number is %lu \n",len, block_nums);
	/* wwb end 2020-6-27 */


	blockp[len] = '\0';
	nova_memlock_block(sb, blockp);

	/* Apply a write entry to the start of log page */
	block = log_block;
	entry = (struct nova_file_write_entry *)nova_get_block(sb, block);

	entry->pgoff = 0;
	entry->num_pages = cpu_to_le32(1);
	entry->invalid_pages = 0;
	entry->block = cpu_to_le64(nova_get_block_off(sb, name_blocknr,
							NOVA_BLOCK_TYPE_4K));
	time = current_time(inode).tv_sec;
	entry->mtime = cpu_to_le32(time);
	/* Set entry type after set block */
	nova_set_entry_type(entry, FILE_WRITE);
	entry->size = cpu_to_le64(len + 1);
	nova_flush_buffer(entry, CACHELINE_SIZE, 0);

	mutex_lock(nova_get_specific_inode_page_lock(pi->nova_ino));
	if(pi->nova_ino > 1)
		pi = nova_get_inode_by_ino_or_pi_addr(sb, pi->nova_ino, 0);

	sih->log_pages = 1;
	pi->log_head = block;
	
	mutex_unlock(nova_get_specific_inode_page_lock(pi->nova_ino));

	//nova_update_tail(pi, block + sizeof(struct nova_file_write_entry));
	nova_update_tail_wwb(sb, pi, block + sizeof(struct nova_file_write_entry));

	/* wwb add 2020-6-28 */
	//inode_page_count_increase(inode->i_sb, (unsigned long)pi);
	/* end 2020-6-28 */

	return 0;
}

static int nova_readlink(struct dentry *dentry, char __user *buffer, int buflen)
{
	struct nova_file_write_entry *entry;
	struct inode *inode = dentry->d_inode;
	struct super_block *sb = inode->i_sb;
	struct nova_inode *pi = nova_get_inode(sb, inode);
	char *blockp;

	entry = (struct nova_file_write_entry *)nova_get_block(sb,
							pi->log_head);
	blockp = (char *)nova_get_block(sb, BLOCK_OFF(entry->block));

	return readlink_copy(buffer, buflen, blockp);
}

static const char *nova_get_link(struct dentry *dentry, struct inode *inode, void **cookie)
{
	struct nova_file_write_entry *entry;
	struct super_block *sb = inode->i_sb;
	struct nova_inode *pi = nova_get_inode(sb, inode);
	char *blockp;

	entry = (struct nova_file_write_entry *)nova_get_block(sb,
							pi->log_head);
	blockp = (char *)nova_get_block(sb, BLOCK_OFF(entry->block));

	return blockp;
}

#if LINUX_VERSION_CODE < KERNEL_VERSION(4, 5, 0)
static const char *nova_follow_link(struct dentry *dentry, void **cookie)
{
	struct inode *inode = dentry->d_inode;
	return nova_get_link(dentry, inode, cookie);
}
#endif

const struct inode_operations nova_symlink_inode_operations = {
	.readlink	= nova_readlink,
#if LINUX_VERSION_CODE >= KERNEL_VERSION(4, 5, 0)
	.get_link	= nova_get_link,
#else
	.follow_link	= nova_follow_link,
#endif
	.setattr	= nova_notify_change,
};
