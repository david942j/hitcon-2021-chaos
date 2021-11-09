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
#include <linux/mm.h>
#include <linux/slab.h>

#include "chaos-core.h"
#include "chaos-dram.h"
#include "chaos-fs.h"
#include "chaos-mailbox.h"
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

	if (size == 0)
		return -EINVAL;
	if (client->buf.size != 0)
		return -EEXIST;
	return chaos_dram_alloc(cdev->dpool, size, &client->buf);
}

static int chaos_ioctl_request(struct chaos_client *client, void __user *arg)
{
	struct chaos_request orig_req, req;
	size_t offset;
	int ret;

	if (client->buf.size == 0)
		return -ENOSPC;
	if (copy_from_user(&req, arg, sizeof(req)))
		return -EFAULT;

	orig_req = req;
	offset = client->buf.paddr - client->cdev->dpool->res->paddr;
	/* adjust buffer offsets */
	if (req.in_size != 0) {
		if (req.input >= client->buf.size)
			return -EINVAL;
		req.input += offset;
	} else {
		req.input = 0;
	}
	if (req.out_size != 0) {
		if (req.output >= client->buf.size)
			return -EINVAL;
		req.output += offset;
	} else {
		req.output = 0;
	}
	ret = chaos_mailbox_request(client->cdev->mbox, &req);
	if (ret)
		return ret;
	orig_req.out_size = req.out_size;
	if (copy_to_user(arg, &orig_req, sizeof(orig_req)))
		return -EFAULT;
	return 0;
}

static long chaos_fs_ioctl(struct file *file, unsigned int cmd, unsigned long arg)
{
	struct chaos_client *client = file->private_data;

	switch (cmd) {
	case CHAOS_ALLOCATE_BUFFER:
		return chaos_ioctl_allocate_buffer(client, arg);
	case CHAOS_REQUEST:
		return chaos_ioctl_request(client, (void __user*)arg);
	default:
		return -ENOTTY;
	}
}

static int chaos_fs_mmap(struct file *file, struct vm_area_struct *vma)
{
	struct chaos_client *client = file->private_data;
	const size_t buf_size = client->buf.size;
	const size_t vma_size = vma->vm_end - vma->vm_start;
	const size_t vma_off = vma->vm_pgoff << PAGE_SHIFT;

	if (buf_size == 0 || vma_size > buf_size || vma_off >= buf_size ||
	    vma_size > buf_size - vma_off)
		return -EINVAL;
	return io_remap_pfn_range(vma, vma->vm_start,
				  (client->buf.paddr + vma_off) >> PAGE_SHIFT, vma_size,
				  vma->vm_page_prot);
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
