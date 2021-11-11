/*
 * CHAOS - CryptograpHy AcceleratOr Silicon
 *
 * Copyright (c) 2021 david942j
 */

#include "qemu/osdep.h"

#include <signal.h>
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

    struct share_mem csr, dram;
    pid_t devpid;
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

// TODO
#define USE_BUILTIN 1

#if USE_BUILTIN

static uint64_t chaos_csr_read(void *opaque, hwaddr addr, unsigned size);
static void chaos_csr_write(void *opaque, hwaddr addr, uint64_t val, unsigned size);
#define READ_CSR(chaos, field) chaos_csr_read(chaos, offsetof(struct Csrs, field), 8)
#define WRITE_CSR(chaos, field, val) chaos_csr_write(chaos, offsetof(struct Csrs, field), val, 8)
enum chaos_command_code {
	CHAOS_CMD_CODE_REQUEST,
};

struct chaos_mailbox_cmd {
    uint32_t seq;
    enum chaos_command_code code;
    uint32_t dma_addr;
    uint32_t dma_size;
};

struct chaos_mailbox_rsp {
    uint32_t seq;
    uint32_t retval;
};

enum chaos_request_algo {
	/* copy input to output, for testing purpose */
	CHAOS_ALGO_ECHO,
	CHAOS_ALGO_MD5,
};

struct chaos_request {
	enum chaos_request_algo algo;
	uint32_t input;
	uint32_t in_size;
	uint32_t output;
	uint32_t out_size;
};

static inline void *VALID_ADDR_SIZE(ChaosState *chaos, uint32_t addr, uint32_t sz) {
    g_assert(sz <= chaos->dram.size);
    g_assert(addr < chaos->dram.size);
    g_assert(addr <= chaos->dram.size - sz);
    return chaos->dram.addr + addr;
}

static uint64_t real_index(uint64_t idx, uint64_t size)
{
    return idx & (size - 1);
}

static inline uint8_t htoi(char c)
{
    return c >= 'a' ? (c - 'a' + 10) : c - '0';
}
static void do_md5(const void *in, uint32_t size, uint8_t *out)
{
    FILE *infile = fopen("/tmp/.chaos", "wb"), *pf;
    char hex[33];
    int i;

    g_assert(size == fwrite(in, 1, size, infile));
    fclose(infile);
    pf = popen("md5sum /tmp/.chaos", "r");
    g_assert(fscanf(pf, "%s", hex) == 1);
    pclose(pf);
    for (i = 0; i < 16; i++)
        out[i] = (htoi(hex[2 * i]) << 4) + htoi(hex[2 * i + 1]);
}

static int handle_cmd_request(ChaosState *chaos, struct chaos_mailbox_cmd *cmd)
{
    struct chaos_request *req;
    void *in, *out;

    // XXX: the implementation here is bad, kernel is able to hack with TOCTOU
    g_assert(cmd->dma_size == sizeof(struct chaos_request));
    req = VALID_ADDR_SIZE(chaos, cmd->dma_addr, cmd->dma_size);
    in = VALID_ADDR_SIZE(chaos, req->input, req->in_size);
    out = VALID_ADDR_SIZE(chaos, req->output, req->out_size);

    switch (req->algo) {
    case CHAOS_ALGO_ECHO:
        memcpy(out, in, req->in_size);
        return req->in_size;
    case CHAOS_ALGO_MD5:
        if (req->out_size < 0x10) {
            // TODO: add logs?
            return -EOVERFLOW;
        }
        do_md5(in, req->in_size, out);
        return 0x10;
    default:
        g_assert(false);
        return 0;
    }
}

static int handle_cmd(ChaosState *chaos, struct chaos_mailbox_cmd *cmd)
{
    g_assert(cmd->code == CHAOS_CMD_CODE_REQUEST);
    return handle_cmd_request(chaos, cmd);
}

static inline uint64_t queue_inc(uint64_t val, uint64_t size)
{
    return (++val) & ((size << 1) - 1);
}

static void push_rsp(ChaosState *chaos, struct chaos_mailbox_rsp *rsp)
{
    struct chaos_mailbox_rsp *queue = chaos->dram.addr + READ_CSR(chaos, rspq_addr);
    const uint64_t rspq_size = READ_CSR(chaos, rspq_size);
    uint64_t tail = READ_CSR(chaos, rsp_tail);

    debug("queue[%ld] = {%u, %u}\n", real_index(tail, rspq_size), rsp->seq, rsp->retval);
    queue[real_index(tail, rspq_size)] = *rsp;
    WRITE_CSR(chaos, rsp_tail, queue_inc(tail, rspq_size));
}

#endif

static void chaos_interrupt_to_device(ChaosState *chaos)
{
#if USE_BUILTIN
    const uint64_t cmdq_size = READ_CSR(chaos, cmdq_size);
    uint64_t head = READ_CSR(chaos, cmd_head);
    uint64_t tail = READ_CSR(chaos, cmd_tail);
    struct chaos_mailbox_cmd *cmdq = chaos->dram.addr + READ_CSR(chaos, cmdq_addr), *cmd;
    struct chaos_mailbox_rsp rsp;

    if (head == tail)
        return;
    cmd = &cmdq[real_index(head, cmdq_size)];
    debug("head = %d, sz = %d, dma = 0x%x, dma_size = 0x%x\n", (int)head, (int)cmdq_size, cmd->dma_addr, cmd->dma_size);
    WRITE_CSR(chaos, cmd_head, queue_inc(head, cmdq_size));
    rsp.retval = handle_cmd(chaos, cmd);
    rsp.seq = cmd->seq;
    push_rsp(chaos, &rsp);
    chaos_raise_irq(chaos);
#else
    // TODO: trigger eventfd to the external dev
#endif
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

static void launch_device(ChaosState *chaos)
{
    // TODO: create a thread for polling eventfd
    pid_t pid = fork();
    if (!pid) {
        close(0);
        close(1);
#ifndef DEBUG
        close(2);
#endif
        dup2(chaos->csr.fd, 3);
        dup2(chaos->dram.fd, 4);
        close(chaos->csr.fd);
        close(chaos->dram.fd);
        const char *const argv[] = { "entry", NULL };
        execv("/home/david942j/hitcon-2021-chaos/src/chaos/entry", (char *const *)argv);
        g_assert(false);
    } else {
        debug("child = %d\n", pid);
        chaos->devpid = pid;
    }
}

static void chaos_chip_init(ChaosState *chaos)
{
    share_mem_init("dev-csr", sizeof(struct Csrs), &chaos->csr);
    share_mem_init("dev-dram", CHAOS_DEVICE_DRAM_SIZE, &chaos->dram);
    launch_device(chaos);
}

static void chaos_chip_exit(ChaosState *chaos)
{
    // XXX: this function is never called on QEMU ends..
    kill(chaos->devpid, SIGKILL);
    wait(NULL);
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
