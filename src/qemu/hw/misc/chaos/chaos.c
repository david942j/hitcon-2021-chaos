/*
 * CHAOS - CryptograpHy AcceleratOr Silicon
 *
 * Copyright (c) 2021 david942j
 */

#include "qemu/osdep.h"

#include "hw/pci/pci.h"

#define PCI_DEVICE_ID_CHAOS 0x7331
#define CHAOS_DEVICE_NAME "chaos"
#define CHAOS(obj) OBJECT_CHECK(ChaosState, obj, CHAOS_DEVICE_NAME)

typedef struct {
	PCIDevice pdev;
} ChaosState;

static void chaos_realize(PCIDevice *dev, Error **errp)
{
    uint8_t *config = dev->config;

    pci_config_set_interrupt_pin(config, 1);
}

static void chaos_exit(PCIDevice *dev)
{
}

static void chaos_instance_init(Object *obj)
{
}

static void chaos_class_init(ObjectClass *class, void *data)
{
    DeviceClass *dc = DEVICE_CLASS(class);
    PCIDeviceClass *pdc = PCI_DEVICE_CLASS(class);

    pdc->realize = chaos_realize;
    pdc->exit = chaos_exit;
    pdc->vendor_id = PCI_VENDOR_ID_QEMU;
    pdc->device_id = PCI_DEVICE_ID_CHAOS;
    pdc->revision = 1;
    pdc->class_id = PCI_CLASS_OTHERS;
    set_bit(DEVICE_CATEGORY_MISC, dc->categories);
}

static void chaos_register_types(void)
{
    static InterfaceInfo interfaces[] = {
	    { INTERFACE_CONVENTIONAL_PCI_DEVICE },
	    { },
    };
    static const TypeInfo chaos_info = {
	    .name = CHAOS_DEVICE_NAME,
	    .parent = TYPE_PCI_DEVICE,
	    .instance_size = sizeof(ChaosState),
	    .instance_init = chaos_instance_init,
	    .class_init = chaos_class_init,
	    .interfaces = interfaces,
    };

    type_register_static(&chaos_info);
}
type_init(chaos_register_types);
