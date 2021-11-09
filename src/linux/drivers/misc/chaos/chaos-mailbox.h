/* SPDX-License-Identifier: GPL-2.0 */
/*
 * Utilities to communicate with the CHAOS device through mailbox.
 *
 * Copyright (c) 2021 david942j
 */

#ifndef _CHAOS_MAILBOX_H
#define _CHAOS_MAILBOX_H

#include "chaos-core.h"
#include "chaos.h"

#define CHAOS_QUEUE_SIZE 512
#define CHAOS_CMD_QUEUE_LENGTH (CHAOS_QUEUE_SIZE * sizeof(struct chaos_mailbox_cmd))
#define CHAOS_RSP_QUEUE_LENGTH (CHAOS_QUEUE_SIZE * sizeof(struct chaos_mailbox_rsp))

struct chaos_mailbox_cmd {
	uint64_t seq;
};

struct chaos_mailbox_rsp {
	uint64_t seq;
	uint64_t retval;
};

struct chaos_mailbox {
	struct chaos_resource cmdq, rspq;
	struct chaos_device *cdev;
};

struct chaos_mailbox *chaos_mailbox_init(struct chaos_device *cdev);
void chaos_mailbox_exit(struct chaos_mailbox *mbox);

int chaos_mailbox_request(struct chaos_mailbox *mbox, struct chaos_request *req);

#endif /* _CHAOS_MAILBOX_H */
