// SPDX-License-Identifier: GPL-2.0
/*
 * Bridge functions between PCI driver and internal core modules.
 *
 * Copyright (c) 2021 david942j
 */

#include <linux/err.h>
#include <linux/firmware.h>
#include <linux/minmax.h>
#include <linux/sizes.h>

#include "chaos-core.h"
#include "chaos-dram.h"
#include "chaos-fs.h"
#include "chaos-mailbox.h"

static int chaos_request_firmware(struct chaos_device *cdev)
{
	const struct firmware *fw;
	struct chaos_resource res;
	const size_t size = SZ_1M;
	uint64_t retval;
	int ret;

	ret = chaos_dram_alloc(cdev->dpool, size, &res);
	if (ret)
		return ret;
	ret = request_firmware_into_buf(&fw, "chaos", cdev->dev, res.vaddr, size);
	if (ret)
		goto out_dram_free;
	CHAOS_WRITE(cdev, fw_size, min(size, fw->size));
	CHAOS_WRITE(cdev, load_addr, CHAOS_DRAM_OFFSET(cdev->dpool, &res));
	retval = CHAOS_READ(cdev, load_addr);
	if (retval & (1ull << 63))
		ret = retval ^ (1ull << 63);
	else
		ret = -ETIMEDOUT; /* bootloader unresponsive */

	release_firmware(fw);
out_dram_free:
	chaos_dram_free(cdev->dpool, &res);
	return ret;
}

int chaos_init(struct chaos_device *cdev)
{
	int ret;

	cdev->dpool = chaos_dram_init(cdev);
	if (IS_ERR(cdev->dpool)) {
		ret = PTR_ERR(cdev->dpool);
		dev_err(cdev->dev, "DRAM pool init failed: %d\n", ret);
		return ret;
	}
	ret = chaos_request_firmware(cdev);
	if (ret) {
		dev_err(cdev->dev, "firmware loading failed: %d\n", ret);
		goto err_dram_exit;
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
