// SPDX-License-Identifier: GPL-2.0
/*
 * Bridge functions between PCI driver and internal core modules.
 *
 * Copyright (c) 2021 david942j
 */

#include <linux/err.h>

#include "chaos-core.h"
#include "chaos-dram.h"
#include "chaos-fs.h"
#include "chaos-mailbox.h"

int chaos_init(struct chaos_device *cdev)
{
	int ret;

	cdev->dpool = chaos_dram_init(cdev);
	if (IS_ERR(cdev->dpool)) {
		ret = PTR_ERR(cdev->dpool);
		dev_err(cdev->dev, "DRAM pool init failed: %d\n", ret);
		return ret;
	}
	cdev->mbox = chaos_mailbox_init(cdev);
	if (IS_ERR(cdev->mbox)) {
		ret = PTR_ERR(cdev->mbox);
		dev_err(cdev->dev, "mailbox init failed: %d\n", ret);
		goto err_dram_exit;
	}
	ret = chaos_fs_init(&cdev->chardev);
	if (ret) {
		dev_err(cdev->dev, "FS init failed: %d", ret);
		goto err_mbox_exit;
	}

	return 0;
err_mbox_exit:
	chaos_mailbox_exit(cdev->mbox);
err_dram_exit:
	chaos_dram_exit(cdev->dpool);
	return ret;
}

void chaos_exit(struct chaos_device *cdev)
{
	chaos_fs_exit(&cdev->chardev);
	chaos_mailbox_exit(cdev->mbox);
	chaos_dram_exit(cdev->dpool);
}
