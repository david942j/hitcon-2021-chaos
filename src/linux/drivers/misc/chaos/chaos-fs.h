/* SPDX-License-Identifier: GPL-2.0 */
/*
 * Expose a file interface for user-space to interact with the chaos device.
 *
 * Copyright (c) 2021 david942j
 */

#ifndef _CHAOS_FS_H
#define _CHAOS_FS_H

#include <linux/miscdevice.h>

int chaos_fs_init(struct miscdevice *chardev);
void chaos_fs_exit(struct miscdevice *chardev);

#endif /* _CHAOS_FS_H */
