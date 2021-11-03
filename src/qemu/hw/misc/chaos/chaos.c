/*
 * CHAOS - CryptograpHy AcceleratOr Silicon
 *
 * Copyright (c) 2021 david942j
 */

#include "qemu/osdep.h"

#include <sys/mman.h>
#include <sys/types.h>
#include <unistd.h>

#include "exec/memory.h"
#include "hw/pci/pci.h"

#define PCI_DEVICE_ID_CHAOS 0x7331
#define CHAOS_DEVICE_NAME "chaos"
#define CHAOS(obj) OBJECT_CHECK(ChaosState, obj, CHAOS_DEVICE_NAME)

/* 1 MB */
#define CHAOS_DEVICE_DRAM_SIZE (1 << 20)

typedef struct {
    PCIDevice pdev;
    MemoryRegion mem_csrs, mem_dram;

    int dram_fd;
    void *dram;
} ChaosState;

struct Csrs {
    uint64_t version; /* R */
    uint64_t reset; /* W */
    uint64_t irq_status; /* R */
    uint64_t clear_irq; /* W */
    uint64_t cmd_sent; /* W */
    uint64_t cmd_head; /* R */
    uint64_t cmd_tail; /* W */
    uint64_t rsp_head; /* W */
    uint64_t rsp_tail; /* R */
    uint64_t reserved[7];
};

static void chaos_chip_init(ChaosState *chaos)
{
    int ret;
    int dram_fd = memfd_create("dev-dram", 0);

    g_assert(dram_fd >= 0);
    ret = ftruncate(dram_fd, CHAOS_DEVICE_DRAM_SIZE);
    g_assert(ret == 0);
    chaos->dram_fd = dram_fd;
    chaos->dram = mmap(0, CHAOS_DEVICE_DRAM_SIZE, PROT_READ | PROT_WRITE,
                       MAP_SHARED, dram_fd, 0);
    g_assert(chaos->dram != MAP_FAILED);
}

static void chaos_chip_exit(ChaosState *chaos)
{
    munmap(chaos->dram, CHAOS_DEVICE_DRAM_SIZE);
    close(chaos->dram_fd);
}

static uint64_t chaos_csr_read(void *opaque, hwaddr addr, unsigned size)
{
    uint64_t val = 0;

    return val;
}

static void chaos_csr_write(void *opaque, hwaddr addr, uint64_t val,
                            unsigned size)
{
}

static const MemoryRegionOps chaos_mem_csrs_ops = {
    .read = chaos_csr_read,
    .write = chaos_csr_write,
    .valid = {
        .min_access_size = 8,
        .max_access_size = 8,
    },
    .impl = {
        .min_access_size = 8,
        .max_access_size = 8,
    },
};

static uint64_t chaos_dram_read(void *opaque, hwaddr addr, unsigned size)
{
    ChaosState *chaos = opaque;

    switch (size) {
    case 1:
        return *((uint8_t*)(chaos->dram + addr));
    case 2:
        return *((uint16_t*)(chaos->dram + addr));
    case 4:
        return *((uint32_t*)(chaos->dram + addr));
    case 8:
        return *((uint64_t*)(chaos->dram + addr));
    default:
        return 0;
    }
}

static void chaos_dram_write(void *opaque, hwaddr addr, uint64_t val,
                             unsigned size)
{
    ChaosState *chaos = opaque;

    switch (size) {
    case 1:
        *((uint8_t*)(chaos->dram + addr)) = val;
        break;
    case 2:
        *((uint16_t*)(chaos->dram + addr)) = val;
        break;
    case 4:
        *((uint32_t*)(chaos->dram + addr)) = val;
        break;
    case 8:
        *((uint64_t*)(chaos->dram + addr)) = val;
        break;
    default:
        return;
    }
}

static const MemoryRegionOps chaos_mem_dram_ops = {
    .read = chaos_dram_read,
    .write = chaos_dram_write,
    .valid = {
        .min_access_size = 8,
        .max_access_size = 8,
    },
    .impl = {
        .min_access_size = 8,
        .max_access_size = 8,
    },
};

static void chaos_realize(PCIDevice *pdev, Error **errp)
{
    ChaosState *chaos = CHAOS(pdev);
    uint8_t *config = pdev->config;

    pci_config_set_interrupt_pin(config, 1);

    memory_region_init_io(&chaos->mem_csrs, OBJECT(chaos), &chaos_mem_csrs_ops,
                          chaos, "chaos-csr", sizeof(struct Csrs));
    pci_register_bar(pdev, 0, PCI_BASE_ADDRESS_SPACE_MEMORY, &chaos->mem_csrs);

    memory_region_init_io(&chaos->mem_dram, OBJECT(chaos), &chaos_mem_dram_ops,
                          chaos, "chaos-dram", CHAOS_DEVICE_DRAM_SIZE);
    pci_register_bar(pdev, 1, PCI_BASE_ADDRESS_SPACE_MEMORY, &chaos->mem_dram);

    chaos_chip_init(chaos);
}

static void chaos_exit(PCIDevice *pdev)
{
    ChaosState *chaos = CHAOS(pdev);

    chaos_chip_exit(chaos);
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
