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
#include <linux/spinlock.h>
#include <linux/wait.h>

#include "chaos-core.h"
#include "chaos-dram.h"
#include "chaos-mailbox.h"
#include "chaos.h"

#define MAILBOX_TIMEOUT_MS 1000
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

#define WAITING_RESPONSE -100
#define WRONG_SEQ -101
static int chaos_push_cmd_and_wait(struct chaos_mailbox *mbox, const struct chaos_mailbox_cmd *cmd,
				   u32 *retval)
{
	struct chaos_device *cdev = mbox->cdev;
	const u64 head = CHAOS_READ(cdev, cmd_head);
	u64 tail;
	struct chaos_mailbox_cmd *queue = mbox->cmdq.vaddr;
	const size_t idx = cmd->seq % CHAOS_QUEUE_SIZE;
	u32 val;
	int ret;

	mutex_lock(&mbox->cmdq_lock);
	tail = CHAOS_READ(cdev, cmd_tail);
	if (queue_full(head, tail)) {
		mutex_unlock(&mbox->cmdq_lock);
		return -EBUSY;
	}
	memcpy(&queue[REAL_INDEX(tail)], cmd, sizeof(*cmd));
	CHAOS_WRITE(cdev, cmd_tail, queue_inc(tail));
	mutex_unlock(&mbox->cmdq_lock);

	mbox->responses[idx].seq = cmd->seq;
	mbox->responses[idx].retval = WAITING_RESPONSE;
	CHAOS_WRITE(cdev, cmd_sent, 1);

	ret = wait_event_timeout(mbox->waitq, (val = mbox->responses[idx].retval) != WAITING_RESPONSE,
				 msecs_to_jiffies(MAILBOX_TIMEOUT_MS));
	if (!ret)
		return -ETIMEDOUT;
	if (val == WRONG_SEQ)
		return -ENOMSG;
	*retval = val;
	return 0;
}

struct chaos_mailbox *chaos_mailbox_init(struct chaos_device *cdev)
{
	int ret;
	struct chaos_mailbox *mbox;
	const size_t sz = sizeof(*mbox) + sizeof(*mbox->responses) * CHAOS_QUEUE_SIZE;

	mbox = devm_kzalloc(cdev->dev, sz, GFP_KERNEL);
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
	mbox->responses = (void *)(mbox + 1);
	mutex_init(&mbox->cmdq_lock);
	spin_lock_init(&mbox->rspq_lock);
	init_waitqueue_head(&mbox->waitq);
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

void chaos_mailbox_handle_irq(struct chaos_mailbox *mbox)
{
	struct chaos_device *cdev = mbox->cdev;
	struct chaos_mailbox_rsp *queue = mbox->rspq.vaddr;
	u64 head = CHAOS_READ(cdev, rsp_head);
	unsigned long flags;

	spin_lock_irqsave(&mbox->rspq_lock, flags);
	while (head != CHAOS_READ(cdev, rsp_tail)) {
		struct chaos_mailbox_rsp rsp = queue[REAL_INDEX(head)];
		const size_t idx = rsp.seq % CHAOS_QUEUE_SIZE;

		if (mbox->responses[idx].seq != rsp.seq)
			mbox->responses[idx].retval = WRONG_SEQ;
		else
			mbox->responses[idx].retval = rsp.retval;
		head = queue_inc(head);
	}
	spin_unlock_irqrestore(&mbox->rspq_lock, flags);
	wake_up(&mbox->waitq);
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
	u32 retval = 0;

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
	if ((int)retval < 0) {
		dev_dbg(mbox->cdev->dev, "%s: fw returns an error: %d", __func__, (int)retval);
		return -EPROTO;
	}
	req->out_size = retval;
	return 0;
}
