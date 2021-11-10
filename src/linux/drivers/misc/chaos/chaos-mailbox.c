// SPDX-License-Identifier: GPL-2.0
/*
 * Utilities to communicate with the CHAOS device through mailbox.
 *
 * Copyright (c) 2021 david942j
 */

#include <linux/atomic.h>
#include <linux/device.h>
#include <linux/err.h>
#include <linux/mutex.h>

#include "chaos-core.h"
#include "chaos-dram.h"
#include "chaos-mailbox.h"
#include "chaos.h"

/* BUG: should be "& (CHAOS_QUEUE_SIZE - 1)" to prevent OOB */
#define REAL_INDEX(v) ((v) & ~CHAOS_QUEUE_SIZE)

static inline bool queue_full(u64 head, u64 tail)
{
	return (head ^ tail) == CHAOS_QUEUE_SIZE;
}

static inline u64 queue_inc(u64 index)
{
	return (++index) & ((CHAOS_QUEUE_SIZE << 1) - 1);
}

static int chaos_push_cmd(struct chaos_mailbox *mbox, const struct chaos_mailbox_cmd *cmd)
{
	struct chaos_device *cdev = mbox->cdev;
	const u64 head = CHAOS_READ(cdev, cmd_head);
	struct chaos_mailbox_cmd *queue = mbox->cmdq.vaddr;
	u64 tail;

	mutex_lock(&mbox->cmdq_lock);
	tail = CHAOS_READ(cdev, cmd_tail);
	if (queue_full(head, tail)) {
		mutex_unlock(&mbox->cmdq_lock);
		return -EBUSY;
	}
	memcpy(&queue[REAL_INDEX(head)], cmd, sizeof(*cmd));
	CHAOS_WRITE(cdev, cmd_head, queue_inc(head));
	CHAOS_WRITE(cdev, cmd_sent, 1);
	return 0;
}

static int chaos_push_cmd_and_wait(struct chaos_mailbox *mbox, const struct chaos_mailbox_cmd *cmd,
				   u64 *retval)
{
	int ret = chaos_push_cmd(mbox, cmd);

	if (ret)
		return ret;

	return ret;
}

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
	mutex_init(&mbox->cmdq_lock);
	mutex_init(&mbox->rspq_lock);
	atomic_set(&mbox->next_seq, 0);
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
	struct chaos_mailbox_cmd cmd = {
		.seq = atomic_inc_return(&mbox->next_seq),
		.code = CHAOS_CMD_CODE_REQUEST,
	};
	struct chaos_dram_pool *dpool = mbox->cdev->dpool;
	struct chaos_resource buf;
	int ret;
	u64 retval = 0;

	ret = chaos_dram_alloc(dpool, sizeof(*req), &buf);
	if (ret)
		return ret;
	cmd.dma_addr = CHAOS_DRAM_OFFSET(dpool, &buf);
	cmd.dma_size = sizeof(*req);
	memcpy(buf.vaddr, req, sizeof(*req));
	ret = chaos_push_cmd_and_wait(mbox, &cmd, &retval);
	chaos_dram_free(dpool, &buf);
	if (ret)
		return ret;
	/* FW returned an error */
	if ((long long)retval < 0) {
		dev_dbg(mbox->cdev->dev, "%s: fw returns an error: %lld", __func__, (long long)retval);
		return -EPROTO;
	}
	req->out_size = retval;
	return 0;
}
