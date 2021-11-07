// SPDX-License-Identifier: GPL-2.0
/*
 * Linux kernel PCI device driver for CHAOS - CryptograpHy AcceleratOr Silicon
 *
 * Copyright (c) 2021 david942j
 */

#include <linux/device.h>
#include <linux/err.h>
#include <linux/irqreturn.h>
#include <linux/module.h>
#include <linux/pci.h>
#include <linux/types.h>

#include "chaos-core.h"

#define PCI_VENDOR_ID_QEMU 0x1234
#define PCI_DEVICE_ID_CHAOS 0x7331

static irqreturn_t chaos_irq_handler(int irq, void *data)
{
	return IRQ_HANDLED;
}

static int chaos_pci_mem_remap(struct pci_dev *pdev, int bar, struct chaos_resource *cres)
{
	phys_addr_t phys_addr;
	struct resource *res;
	void *mem;
	size_t size;
	unsigned long flags;

	phys_addr = pci_resource_start(pdev, bar);
	if (!phys_addr) {
		pci_err(pdev, "No resource");
		return -ENODEV;
	}

	size = pci_resource_len(pdev, bar);
	res = devm_request_mem_region(&pdev->dev, phys_addr, size, KBUILD_MODNAME);
	if (!res) {
		pci_err(pdev, "Failed to request memory\n");
		return -EBUSY;
	}

	mem = devm_ioremap(&pdev->dev, phys_addr, size);
	if (!mem) {
		pci_err(pdev, "ioremap failed\n");
		return -ENOMEM;
	}

	flags = pci_resource_flags(pdev, bar);
	if (flags & IORESOURCE_IO) {
		pci_err(pdev, "IO mapped PCI devices are not supported\n");
		return -ENOTSUPP;
	}

	cres->paddr = phys_addr;
	cres->vaddr = mem;
	cres->size = size;
	return 0;
}
static int chaos_pci_probe(struct pci_dev *pdev, const struct pci_device_id *id)
{
	int ret;
	struct device *dev = &pdev->dev;
	struct chaos_device *cdev;

	ret = pcim_enable_device(pdev);
	if (ret)
		return ret;
	pci_set_master(pdev);
	cdev = devm_kzalloc(dev, sizeof(*cdev), GFP_KERNEL);
	if (!cdev)
		return -ENOMEM;
	ret = chaos_pci_mem_remap(pdev, 0, &cdev->csr);
	if (ret)
		return ret;
	ret = chaos_pci_mem_remap(pdev, 1, &cdev->dram);
	if (ret)
		return ret;

	cdev->dev = dev;
	ret = devm_request_irq(dev, pdev->irq, chaos_irq_handler, IRQF_SHARED, KBUILD_MODNAME, cdev);
	if (ret) {
		pci_err(pdev, "Failed to request interrupt IRQ: %d\n", pdev->irq);
		return ret;
	}
	ret = chaos_init(cdev);
	if (ret)
		return ret;

	pci_set_drvdata(pdev, cdev);

	return 0;
}

static void chaos_pci_remove(struct pci_dev *pdev)
{
	struct chaos_device *cdev = pci_get_drvdata(pdev);

	chaos_exit(cdev);
}

static const struct pci_device_id chaos_pci_ids[] = {
	{ PCI_DEVICE(PCI_VENDOR_ID_QEMU, PCI_DEVICE_ID_CHAOS) },
	{ 0 },
};

MODULE_DEVICE_TABLE(pci, chaos_pci_ids);
static struct pci_driver chaos_pci_driver = {
	.name = "chaos-pci",
	.id_table = chaos_pci_ids,
	.probe = chaos_pci_probe,
	.remove = chaos_pci_remove,
};
module_pci_driver(chaos_pci_driver);

MODULE_AUTHOR("david942j @ 217");
MODULE_LICENSE("GPL");
