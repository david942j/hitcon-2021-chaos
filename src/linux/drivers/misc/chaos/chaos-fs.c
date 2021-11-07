// SPDX-License-Identifier: GPL-2.0
/*
 * Expose a file interface for user-space to interact with the chaos device.
 *
 * Copyright (c) 2021 david942j
 */

#include <linux/device.h>
#include <linux/fs.h>
#include <linux/miscdevice.h>

#include "chaos-core.h"
#include "chaos-fs.h"

static int chaos_fs_open(struct inode *n, struct file *file)
{
	return 0;
}

static long chaos_fs_ioctl(struct file *file, unsigned int cmd, unsigned long arg)
{
	return -ENOTTY;
}

static int chaos_fs_release(struct inode *n, struct file *file)
{
	return 0;
}

static const struct file_operations chaos_fs_ops = {
	.owner = THIS_MODULE,
	.open = chaos_fs_open,
	.unlocked_ioctl = chaos_fs_ioctl,
	.llseek = no_llseek,
	.release = chaos_fs_release,
};

int chaos_fs_init(struct miscdevice *chardev)
{
	chardev->minor = MISC_DYNAMIC_MINOR;
	chardev->name = "chaos";
	chardev->fops = &chaos_fs_ops;
	return misc_register(chardev);
}

void chaos_fs_exit(struct miscdevice *chardev)
{
	misc_deregister(chardev);
}
