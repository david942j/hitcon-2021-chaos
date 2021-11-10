/* SPDX-License-Identifier: GPL-2.0 */
/*
 * Allocator for CHAOS device DRAM.
 *
 * Copyright (c) 2021 david942j
 */

#ifndef _CHAOS_DRAM_H
#define _CHAOS_DRAM_H

#include <linux/genalloc.h>

#include "chaos-core.h"

#define CHAOS_DRAM_OFFSET(dpool, buf) ((buf)->paddr - (dpool)->res->paddr)

struct chaos_dram_pool {
	struct gen_pool *pool;
	/* constant fields after init */
	struct chaos_device *cdev;
	struct chaos_resource *res;
};

struct chaos_dram_pool *chaos_dram_init(struct chaos_device *cdev);
void chaos_dram_exit(struct chaos_dram_pool *dpool);

/*
 * On success, this function fills @res with allocated paddr and vaddr. @res->size is set to
 * PAGE_ALIGN(@size).
 */
int chaos_dram_alloc(struct chaos_dram_pool *dpool, size_t size, struct chaos_resource *res);
void chaos_dram_free(struct chaos_dram_pool *dpool, const struct chaos_resource *res);

#endif /* _CHAOS_DRAM_H */
