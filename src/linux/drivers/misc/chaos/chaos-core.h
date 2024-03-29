/* SPDX-License-Identifier: GPL-2.0 */
/*
 * Bridge functions between PCI driver and internal core modules.
 *
 * Copyright (c) 2021 david942j
 */

#ifndef _CHAOS_CORE_H
#define _CHAOS_CORE_H

#include <asm/io.h>
#include <linux/miscdevice.h>
#include <linux/types.h>

struct chaos_dram_pool;
struct chaos_mailbox;

struct chaos_resource {
	phys_addr_t paddr;
	void *vaddr;
	size_t size;
};

struct chaos_device {
	struct device *dev;
	struct miscdevice chardev;
	struct chaos_resource csr, dram;
	struct chaos_dram_pool *dpool;
	struct chaos_mailbox *mbox;
};

struct chaos_csrs {
	uint64_t load_addr;
	uint64_t fw_size;
	uint64_t cmdq_addr;
	uint64_t rspq_addr;
	uint64_t cmdq_size;
	uint64_t rspq_size;
	uint64_t irq_status;
	uint64_t clear_irq;
	uint64_t cmd_sent;
	uint64_t cmd_head;
	uint64_t cmd_tail;
	uint64_t rsp_head;
	uint64_t rsp_tail;
	uint64_t reserved[3];
};

int chaos_init(struct chaos_device *cdev);
void chaos_exit(struct chaos_device *cdev);

#define CHAOS_READ(cdev, field) readq((cdev)->csr.vaddr + offsetof(struct chaos_csrs, field))
#define CHAOS_WRITE(cdev, field, val) \
	writeq((val), (cdev)->csr.vaddr + offsetof(struct chaos_csrs, field))

#endif /* _CHAOS_CORE_H */
