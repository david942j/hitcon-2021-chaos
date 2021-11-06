/* SPDX-License-Identifier: GPL-2.0 */
/*
 * Internal interfaces and structures for the CHAOS kernel driver.
 *
 * Copyright (c) 2021 david942j
 */

#ifndef _CHAOS_INTERNAL_H
#define _CHAOS_INTERNAL_H

#include <linux/device.h>
#include <linux/miscdevice.h>
#include <linux/types.h>

struct chaos_resource {
	phys_addr_t paddr;
	void *vaddr;
	size_t size;
};

struct chaos_device {
	struct device *dev;
	struct miscdevice chardev;
	struct chaos_resource csr, dram;
};

int chaos_fs_init(struct chaos_device *cdev);
void chaos_fs_exit(struct chaos_device *cdev);

#endif /* _CHAOS_INTERNAL_H */
