// SPDX-License-Identifier: GPL-2.0
/*
 * Expose a file interface for user-space to interact with the chaos device.
 *
 * Copyright (c) 2021 david942j
 */

#include <linux/device.h>
#include <linux/fs.h>
#include <linux/miscdevice.h>

#include "chaos-internal.h"

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

int chaos_fs_init(struct chaos_device *cdev)
{
	int ret;

	cdev->chardev.minor = MISC_DYNAMIC_MINOR;
	cdev->chardev.name = "chaos";
	cdev->chardev.fops = &chaos_fs_ops;
	ret = misc_register(&cdev->chardev);
	if (ret) {
		dev_err(cdev->dev, "device register failed");
		return ret;
	}

	return 0;
}

void chaos_fs_exit(struct chaos_device *cdev)
{
	misc_deregister(&cdev->chardev);
}

