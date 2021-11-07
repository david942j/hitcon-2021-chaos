/* SPDX-License-Identifier: GPL-2.0 */
/*
 * Allocator for CHAOS device DRAM.
 *
 * Copyright (c) 2021 david942j
 */

#include <linux/device.h>
#include <linux/err.h>
#include <linux/genalloc.h>

#include "chaos-core.h"
#include "chaos-dram.h"

struct chaos_dram_pool *chaos_dram_init(struct chaos_device *cdev)
{
	int ret;
	struct chaos_dram_pool *dpool = devm_kzalloc(cdev->dev, sizeof(*dpool), GFP_KERNEL);

	if (!dpool)
		return ERR_PTR(-ENOMEM);

	dpool->pool = devm_gen_pool_create(cdev->dev, PAGE_SHIFT, -1, "chaos-dram");
	if (IS_ERR(dpool->pool))
		return (void *)dpool->pool;
	ret = gen_pool_add(dpool->pool, cdev->dram.paddr, cdev->dram.size, -1);
	if (ret)
		return ERR_PTR(ret);

	dpool->res = &cdev->dram;
	dpool->cdev = cdev;
	return dpool;
}

void chaos_dram_exit(struct chaos_dram_pool *dpool)
{
	/* ensure all allocated memory are freed */
	BUG_ON(gen_pool_size(dpool->pool) != gen_pool_avail(dpool->pool));
}

int chaos_dram_alloc(struct chaos_dram_pool *dpool, size_t size, struct chaos_resource *res)
{
	unsigned long paddr = gen_pool_alloc(dpool->pool, size);

	if (!paddr)
		return -ENOSPC;
	res->size = size;
	res->paddr = paddr;
	res->vaddr = dpool->res->vaddr + (paddr - dpool->res->paddr);
	return 0;
}

void chaos_dram_free(struct chaos_dram_pool *dpool, const struct chaos_resource *res)
{
	gen_pool_free(dpool->pool, res->paddr, res->size);
}
