/* SPDX-License-Identifier: GPL-2.0 */
/*
 * Expose a file interface for user-space to interact with the chaos device.
 *
 * Copyright (c) 2021 david942j
 */

#ifndef _CHAOS_FS_H
#define _CHAOS_FS_H

#include <linux/miscdevice.h>
#include <linux/mutex.h>

#include "chaos-core.h"

/* Each file descriptor creates one client. */
struct chaos_client {
	struct mutex lock;
	/* fields protected by @lock */

	struct chaos_resource buf;

	/* constant fields */

	struct chaos_device *cdev;
};

int chaos_fs_init(struct miscdevice *chardev);
void chaos_fs_exit(struct miscdevice *chardev);

#endif /* _CHAOS_FS_H */
