/*
 * CHAOS - CryptograpHy AcceleratOr Silicon
 *
 * Copyright (c) 2021 david942j
 */

#include "qemu/osdep.h"

#include <signal.h>
#include <sys/eventfd.h>
#include <sys/mman.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

#include "exec/memory.h"
#include "hw/pci/pci.h"

#define DEBUG

#ifdef DEBUG
#define debug(fmt, ...) fprintf(stderr, "%s:%d %s: " fmt, __FILE__, __LINE__, __func__, __VA_ARGS__)
#else
#define debug(...)
#endif

#define PCI_DEVICE_ID_CHAOS 0x7331
#define CHAOS_DEVICE_NAME "chaos"
#define CHAOS(obj) OBJECT_CHECK(ChaosState, obj, CHAOS_DEVICE_NAME)

/* 1 MB */
#define CHAOS_DEVICE_DRAM_SIZE (1 << 20)

struct share_mem {
    int fd;
    size_t size;
    void *addr;
};

typedef struct {
    PCIDevice pdev;
    MemoryRegion mem_csrs, mem_dram;
    QemuThread thread;

    struct share_mem csr, dram;
    pid_t devpid;
    int evtfd_to_dev, evtfd_from_dev;
} ChaosState;

struct Csrs {
    uint64_t version; /* R */
    uint64_t cmdq_addr; /* RW */
    uint64_t rspq_addr; /* RW */
    uint64_t cmdq_size; /* RW */
    uint64_t rspq_size; /* RW */
    uint64_t reset; /* W */
    uint64_t irq_status; /* R */
    uint64_t clear_irq; /* W */
    uint64_t cmd_sent; /* W */
    uint64_t cmd_head; /* R */
    uint64_t cmd_tail; /* W */
    uint64_t rsp_head; /* W */
    uint64_t rsp_tail; /* R */
    uint64_t reserved[3];
};

static void chaos_raise_irq(ChaosState *chaos)
{
    pci_set_irq(&chaos->pdev, 1);
}

static void chaos_lower_irq(ChaosState *chaos)
{
    pci_set_irq(&chaos->pdev, 0);
}

static void chaos_interrupt_to_device(ChaosState *chaos)
{
    {
        uint64_t one = 1;
        ssize_t ret = write(chaos->evtfd_to_dev, &one, sizeof(one));
        g_assert(ret == sizeof(one));
    }
}

static void share_mem_init(const char *name, size_t size, struct share_mem *smem)
{
    int ret;
    int fd = memfd_create(name, 0);

    g_assert(fd >= 0);
    ret = ftruncate(fd, size);
    g_assert(ret == 0);
    smem->addr = mmap(0, size, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
    g_assert(smem->addr != MAP_FAILED);
    smem->fd = fd;
    smem->size = size;
}

static void share_mem_exit(struct share_mem *smem)
{
    munmap(smem->addr, smem->size);
    close(smem->fd);
}

static void dup_and_close(int oldfd, int newfd)
{
    dup2(oldfd, newfd);
    close(oldfd);
}

static pid_t devpid;
static void kill_sandbox(void)
{
    if (devpid) {
        kill(devpid, SIGKILL);
        waitpid(devpid, NULL, 0);
    }
}

static void launch_device(ChaosState *chaos)
{
    pid_t pid = fork();
    if (!pid) {
        close(0);
        close(1);
#ifndef DEBUG
        close(2);
#endif
        dup_and_close(chaos->csr.fd, 3);
        dup_and_close(chaos->dram.fd, 4);
        dup_and_close(chaos->evtfd_to_dev, 5);
        dup_and_close(chaos->evtfd_from_dev, 6);
        const char *const argv[] = { "sandbox", NULL };
        execv("/home/david942j/hitcon-2021-chaos/src/chaos/sandbox", (char *const *)argv);
        g_assert(false);
    } else {
        debug("child = %d\n", pid);
        chaos->devpid = pid;
        devpid = pid;
        // chaos_chip_exit() is not called on QEMU ends..
        // A hacky way to prevent the child becoming a zombie.
        atexit(kill_sandbox);
    }
}

/* returns true if event triggered */
static bool wait_and_clear(int evtfd)
{
    fd_set readfds;
    FD_ZERO(&readfds);
    FD_SET(evtfd, &readfds);
    struct timeval tval = { 10, 0 };
    int ret = select(evtfd + 1, &readfds, NULL, NULL, &tval);
    if (ret == 0)
        return false;
    g_assert(ret == 1);
    uint64_t dummy;
    ret = read(evtfd, &dummy, sizeof(dummy));
    g_assert(ret == sizeof(dummy));
    return true;
}

static void *chaos_waiter(void *opaque)
{
    ChaosState *chaos = opaque;
    int status = 0;

    while (1) {
        if (waitpid(chaos->devpid, &status, WNOHANG) == 0) {
            if (wait_and_clear(chaos->evtfd_from_dev))
                chaos_raise_irq(chaos);
        } else if (WIFEXITED(status)) {
            break;
        }
    }
    debug("thread ends status=0x%x\n", status);
    return NULL;
}

static void chaos_chip_init(ChaosState *chaos)
{
    share_mem_init("dev-csr", sizeof(struct Csrs), &chaos->csr);
    share_mem_init("dev-dram", CHAOS_DEVICE_DRAM_SIZE, &chaos->dram);
    chaos->evtfd_to_dev = eventfd(0, 0);
    chaos->evtfd_from_dev = eventfd(0, 0);
    launch_device(chaos);
    qemu_thread_create(&chaos->thread, "chaos-waiter", chaos_waiter, chaos,
                       QEMU_THREAD_JOINABLE);
}

static void chaos_chip_exit(ChaosState *chaos)
{
    kill(chaos->devpid, SIGKILL);
    qemu_thread_join(&chaos->thread);
    close(chaos->evtfd_from_dev);
    close(chaos->evtfd_to_dev);
    share_mem_exit(&chaos->dram);
    share_mem_exit(&chaos->csr);
}

static uint64_t chaos_csr_read(void *opaque, hwaddr addr, unsigned size)
{
    ChaosState *chaos = opaque;

    if (addr >= sizeof(struct Csrs) || (addr & 7))
        return 0;

    debug("[0x%02lx] -> 0x%08lx\n", addr, *(uint64_t *)(chaos->csr.addr + addr));
    return *(uint64_t *)(chaos->csr.addr + addr);
}

static void chaos_csr_write(void *opaque, hwaddr addr, uint64_t val,
                            unsigned size)
{
    ChaosState *chaos = opaque;

    if (addr >= sizeof(struct Csrs) || (addr & 7))
        return;
    debug("[0x%02lx] <- 0x%08lx\n", addr, val);
    switch (addr) {
    case offsetof(struct Csrs, cmd_sent):
        if (val & 1)
            chaos_interrupt_to_device(chaos);
        return;
    case offsetof(struct Csrs, clear_irq):
        return chaos_lower_irq(chaos);
    default:
        *(uint64_t *)(chaos->csr.addr + addr) = val;
    }
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
    void *base = chaos->dram.addr;

    /* debug("[0x%02lx] -> 0x%08lx\n", addr, *(uint64_t *)(chaos->dram.addr + addr)); */
    switch (size) {
    case 1:
        return *((uint8_t*)(base + addr));
    case 2:
        return *((uint16_t*)(base + addr));
    case 4:
        return *((uint32_t*)(base + addr));
    case 8:
        return *((uint64_t*)(base + addr));
    default:
        return 0;
    }
}

static void chaos_dram_write(void *opaque, hwaddr addr, uint64_t val,
                             unsigned size)
{
    ChaosState *chaos = opaque;
    void *base = chaos->dram.addr;

    /* debug("[0x%02lx] <- 0x%08lx\n", addr, val); */
    switch (size) {
    case 1:
        *((uint8_t*)(base + addr)) = val;
        break;
    case 2:
        *((uint16_t*)(base + addr)) = val;
        break;
    case 4:
        *((uint32_t*)(base + addr)) = val;
        break;
    case 8:
        *((uint64_t*)(base + addr)) = val;
        break;
    default:
        return;
    }
}

static const MemoryRegionOps chaos_mem_dram_ops = {
    .read = chaos_dram_read,
    .write = chaos_dram_write,
    .valid = {
        .min_access_size = 1,
        .max_access_size = 8,
    },
    .impl = {
        .min_access_size = 1,
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
