// SPDX-License-Identifier: GPL-2.0
/*
 * Linux kernel PCI device driver for CHAOS - CryptograpHy AcceleratOr Silicon
 *
 * Copyright (c) 2021 david942j
 */

#include <linux/device.h>
#include <linux/module.h>
#include <linux/pci.h>
#include <linux/types.h>

#include "chaos-internal.h"

#define PCI_VENDOR_ID_QEMU 0x1234
#define PCI_DEVICE_ID_CHAOS 0x7331

static irqreturn_t chaos_irq_handler(int irq, void *data)
{
	return IRQ_HANDLED;
}

static int chaos_pci_probe(struct pci_dev *pdev, const struct pci_device_id *id)
{
	int ret;
	struct device *dev = &pdev->dev;
	struct chaos_device *cdev;

	cdev = devm_kzalloc(dev, sizeof(*cdev), GFP_KERNEL);
	if (!cdev)
		return -ENOMEM;
	ret = pci_enable_device(pdev);
	if (ret)
		return ret;

	pci_set_master(pdev);

	cdev->dev = dev;
	ret = devm_request_irq(dev, pdev->irq, chaos_irq_handler, IRQF_SHARED, KBUILD_MODNAME, cdev);
	if (ret) {
		pci_err(pdev, "Failed to request interrupt IRQ: %d\n", pdev->irq);
		goto out_disable;
	}
	ret = chaos_fs_init(cdev);
	if (ret)
		goto out_disable;

	pci_set_drvdata(pdev, cdev);

	return 0;
out_disable:
	pci_disable_device(pdev);
	return ret;
}

static void chaos_pci_remove(struct pci_dev *pdev)
{
	struct chaos_device *cdev = pci_get_drvdata(pdev);

	pci_disable_device(pdev);
	chaos_fs_exit(cdev);
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
