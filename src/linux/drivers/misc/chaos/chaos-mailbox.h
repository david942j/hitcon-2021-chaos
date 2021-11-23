/* SPDX-License-Identifier: GPL-2.0 */
/*
 * Utilities to communicate with the CHAOS device through mailbox.
 *
 * Copyright (c) 2021 david942j
 */

#ifndef _CHAOS_MAILBOX_H
#define _CHAOS_MAILBOX_H

#include <linux/atomic.h>
#include <linux/mutex.h>
#include <linux/spinlock.h>

#include "chaos-core.h"
#include "chaos.h"

#define CHAOS_QUEUE_SIZE 512
#define CHAOS_CMD_QUEUE_LENGTH (CHAOS_QUEUE_SIZE * sizeof(struct chaos_mailbox_cmd))
#define CHAOS_RSP_QUEUE_LENGTH (CHAOS_QUEUE_SIZE * sizeof(struct chaos_mailbox_rsp))

enum chaos_command_code {
	CHAOS_CMD_CODE_REQUEST,
};

struct chaos_mailbox_cmd {
	uint16_t seq;
	enum chaos_command_code code;
	uint32_t dma_addr;
	uint32_t dma_size;
} __packed;

struct chaos_mailbox_rsp {
	uint16_t seq;
	uint32_t retval;
} __packed;

struct chaos_mailbox {
	struct chaos_resource cmdq, rspq;
	/* lock for accessing cmd / rsp queues */
	struct mutex cmdq_lock;
	spinlock_t rspq_lock;
	struct chaos_mailbox_rsp *responses;
	wait_queue_head_t waitq;
	atomic_t next_seq;
	struct chaos_device *cdev;
};

struct chaos_mailbox *chaos_mailbox_init(struct chaos_device *cdev);
void chaos_mailbox_exit(struct chaos_mailbox *mbox);

void chaos_mailbox_handle_irq(struct chaos_mailbox *mbox);

int chaos_mailbox_request(struct chaos_mailbox *mbox, struct chaos_request *req);

#endif /* _CHAOS_MAILBOX_H */
