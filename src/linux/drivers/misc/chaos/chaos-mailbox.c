// SPDX-License-Identifier: GPL-2.0
/*
 * Utilities to communicate with the CHAOS device through mailbox.
 *
 * Copyright (c) 2021 david942j
 */

#include <linux/device.h>
#include <linux/err.h>

#include "chaos-core.h"
#include "chaos-dram.h"
#include "chaos-mailbox.h"
#include "chaos.h"

struct chaos_mailbox *chaos_mailbox_init(struct chaos_device *cdev)
{
	int ret;
	struct chaos_mailbox *mbox = devm_kzalloc(cdev->dev, sizeof(*mbox), GFP_KERNEL);

	if (!mbox)
		return ERR_PTR(-ENOMEM);
	ret = chaos_dram_alloc(cdev->dpool, CHAOS_CMD_QUEUE_LENGTH, &mbox->cmdq);
	if (ret)
		return ERR_PTR(ret);
	ret = chaos_dram_alloc(cdev->dpool, CHAOS_RSP_QUEUE_LENGTH, &mbox->rspq);
	if (ret) {
		chaos_dram_free(cdev->dpool, &mbox->cmdq);
		return ERR_PTR(ret);
	}
	CHAOS_WRITE(cdev, cmdq_addr, mbox->cmdq.paddr - cdev->dram.paddr);
	CHAOS_WRITE(cdev, cmdq_size, CHAOS_QUEUE_SIZE);
	CHAOS_WRITE(cdev, rspq_addr, mbox->rspq.paddr - cdev->dram.paddr);
	CHAOS_WRITE(cdev, rspq_size, CHAOS_QUEUE_SIZE);
	mbox->cdev = cdev;
	return mbox;
}

void chaos_mailbox_exit(struct chaos_mailbox *mbox)
{
	chaos_dram_free(mbox->cdev->dpool, &mbox->rspq);
	chaos_dram_free(mbox->cdev->dpool, &mbox->cmdq);
}

int chaos_mailbox_request(struct chaos_mailbox *mbox, struct chaos_request *req)
{
	// TODO
	return 0;
}
