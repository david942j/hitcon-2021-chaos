// SPDX-License-Identifier: GPL-2.0
/*
 * Expose a file interface for user-space to interact with the chaos device.
 *
 * Copyright (c) 2021 david942j
 */

#include <linux/device.h>
#include <linux/err.h>
#include <linux/fs.h>
#include <linux/miscdevice.h>
#include <linux/slab.h>

#include "chaos-core.h"
#include "chaos-dram.h"
#include "chaos-fs.h"
#include "chaos.h"

static int chaos_client_init(struct chaos_device *cdev, struct chaos_client *client)
{
	mutex_init(&client->lock);
	client->cdev = cdev;
	return 0;
}

static void chaos_client_exit(struct chaos_client *client)
{
	if (client->buf.size != 0)
		chaos_dram_free(client->cdev->dpool, &client->buf);
}

static int chaos_fs_open(struct inode *n, struct file *file)
{
	int ret;
	struct chaos_device *cdev = container_of(file->private_data, struct chaos_device, chardev);
	struct chaos_client *client = kzalloc(sizeof(*client), GFP_KERNEL);

	if (!client)
		return -ENOMEM;
	ret = chaos_client_init(cdev, client);
	if (ret)
		return ret;
	file->private_data = client;
	return 0;
}

static int chaos_ioctl_allocate_buffer(struct chaos_client *client, size_t size)
{
	struct chaos_device *cdev = client->cdev;

	// TODO: test all error cases
	if (size == 0)
		return -EINVAL;
	if (client->buf.size != 0)
		return -EEXIST;
	return chaos_dram_alloc(cdev->dpool, size, &client->buf);
}

static long chaos_fs_ioctl(struct file *file, unsigned int cmd, unsigned long arg)
{
	struct chaos_client *client = file->private_data;

	switch (cmd) {
	case CHAOS_ALLOCATE_BUFFER:
		return chaos_ioctl_allocate_buffer(client, arg);
	default:
		return -ENOTTY;
	}
}

static int chaos_fs_mmap(struct file *file, struct vm_area_struct *vma)
{
	struct chaos_client *client = file->private_data;

	if (client->buf.size == 0)
		return -EINVAL;
	return 0;
}

static int chaos_fs_release(struct inode *n, struct file *file)
{
	struct chaos_client *client = file->private_data;

	chaos_client_exit(client);
	return 0;
}

static const struct file_operations chaos_fs_ops = {
	.owner = THIS_MODULE,
	.open = chaos_fs_open,
	.unlocked_ioctl = chaos_fs_ioctl,
	.mmap = chaos_fs_mmap,
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
