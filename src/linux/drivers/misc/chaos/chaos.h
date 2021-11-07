/* SPDX-License-Identifier: GPL-2.0 */

#ifndef _CHAOS_H
#define _CHAOS_H

#include <linux/ioctl.h>

#define CHAOS_IOC_MAGIC 0xCA

/*
 * Allocates device DRAM for this client.
 * Mmap() can only be called on clients with buffer allocated.
 *
 * Each client should allocate a buffer exact once.
 *
 * Returns 0 on success.
 */
#define CHAOS_ALLOCATE_BUFFER _IOW(CHAOS_IOC_MAGIC, 0, __u64)

#endif /* _CHAOS_H */
