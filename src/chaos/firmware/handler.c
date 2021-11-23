#include "syscall.h"
#include "types.h"

#define CHECK(cond) do { \
  if (!(cond)) syscall(SYS_exit, 2, 0, 0, 0, 0, 0); \
} while (0)

enum chaos_command_code {
    CHAOS_CMD_CODE_REQUEST,
};

struct chaos_mailbox_cmd {
    uint16_t seq;
    enum chaos_command_code code;
    uint32_t dma_addr;
    uint32_t dma_size;
} __attribute__((packed));

struct chaos_mailbox_rsp {
    uint16_t seq;
    uint32_t retval;
} __attribute__((packed));

enum chaos_request_algo {
    /* copy input to output, for testing purpose */
    CHAOS_ALGO_ECHO,
    CHAOS_ALGO_MD5,
    CHAOS_ALGO_AES_ENC,
    CHAOS_ALGO_AES_DEC,
    CHAOS_ALGO_RC4_ENC,
    CHAOS_ALGO_RC4_DEC,
    CHAOS_ALGO_BF_ENC,
    CHAOS_ALGO_BF_DEC,
};

struct chaos_request {
    enum chaos_request_algo algo;
    uint32_t input;
    uint32_t in_size;
    uint32_t key;
    uint32_t key_size;
    uint32_t output;
    uint32_t out_size;
};

struct Csrs {
    uint64_t load_addr;
    uint64_t fw_size;
    uint64_t cmdq_addr;
    uint64_t rspq_addr;
    uint64_t cmdq_size;
    uint64_t rspq_size;
    uint64_t irq_status;
    uint64_t clear_irq;
    uint64_t cmd_sent;
    uint64_t cmd_head;
    uint64_t cmd_tail;
    uint64_t rsp_head;
    uint64_t rsp_tail;
    uint64_t reserved[3];
};

struct dram_buffer {
    void *ptr;
    uint32_t size;
};

#define PACK(ptr, size) ((((uint64_t)ptr) << 32) | (uint64_t)size)
#define PACKDB(db) PACK(db.ptr, db.size)

#define AES_BLOCK_SIZE 0x10
#define BLOWFISH_BLOCK_SIZE 0x8

#define CSR_BASE 0x10000
#define DRAM_BASE 0x10000000
#define DRAM_SIZE 0x100000 /* 1M */
#define offsetof(_type, _field) (&(((_type *)0)->_field))

static struct Csrs *csr = (struct Csrs *)CSR_BASE;

static inline void *DRAM_AT(uint32_t addr)
{
    return (void *)(uint64_t)(DRAM_BASE + addr);
}

static void check_dram_buffer(struct dram_buffer *buf, uint32_t offset, uint32_t size)
{
    CHECK(offset < DRAM_SIZE && size <= DRAM_SIZE && offset <= DRAM_BASE - size);
    buf->ptr = DRAM_AT(offset);
    buf->size = size;
}

static uint64_t real_index(uint64_t idx, uint64_t size)
{
    return idx & (size - 1);
}

static void xor(uint8_t *ptr, uint8_t *from, uint32_t size)
{
    uint32_t i;

    for (i = 0; i < size; i++)
	ptr[i] ^= from[i];
}

static int cbc_mode(enum chaos_request_algo algo, struct dram_buffer in, struct dram_buffer key,
                    struct dram_buffer out, uint32_t block_size)
{
    uint32_t in_size = in.size;
    uint8_t *in_ptr, *out_ptr, *prev_ptr;
    uint32_t i;
    int ret, tot = 0;
    for (i = 0; i < in_size; i += block_size) {
	in_ptr = ((uint8_t *)in.ptr) + i;
	out_ptr = ((uint8_t *)out.ptr) + i;
	if (i != 0)
	    xor(in_ptr, prev_ptr, block_size);
	prev_ptr = out_ptr;
	ret = syscall(SYS_chaos_crypto, algo, PACK(in_ptr, block_size), PACKDB(key), PACK(out_ptr, block_size));
	if (ret < 0)
	    return ret;
	tot += ret;
    }
    return tot;
}

static int handle_cmd_request(struct chaos_mailbox_cmd *cmd)
{
    enum chaos_request_algo algo;
    struct dram_buffer in, key, out;

    CHECK(cmd->dma_size == sizeof(struct chaos_request));
    {
        struct chaos_request *req = (struct chaos_request *)DRAM_AT(cmd->dma_addr);

        algo = req->algo;
        check_dram_buffer(&in, req->input, req->in_size);
        check_dram_buffer(&key, req->key, req->key_size);
        check_dram_buffer(&out, req->output, req->out_size);
    }

    switch (algo) {
    case CHAOS_ALGO_ECHO:
        if (out.size < in.size)
            return -EOVERFLOW;
        memcpy(out.ptr, in.ptr, in.size);
        return in.size;
    case CHAOS_ALGO_MD5:
        if (out.size < 0x10)
            return -EOVERFLOW;
        return syscall(SYS_chaos_crypto, CHAOS_ALGO_MD5, PACKDB(in), PACKDB(key), PACKDB(out));
    case CHAOS_ALGO_RC4_ENC:
        if (out.size < in.size)
            return -EOVERFLOW;
        return syscall(SYS_chaos_crypto, CHAOS_ALGO_RC4_ENC, PACKDB(in), PACKDB(key), PACKDB(out));
    case CHAOS_ALGO_RC4_DEC:
        if (out.size < in.size)
            return -EOVERFLOW;
        return syscall(SYS_chaos_crypto, CHAOS_ALGO_RC4_DEC, PACKDB(in), PACKDB(key), PACKDB(out));
    case CHAOS_ALGO_AES_ENC:
        if (out.size < in.size)
            return -EOVERFLOW;
	if (in.size % AES_BLOCK_SIZE != 0)
            return -EINVAL;
        return cbc_mode(CHAOS_ALGO_AES_ENC, in, key, out, AES_BLOCK_SIZE);
    case CHAOS_ALGO_AES_DEC:
        if (out.size < in.size)
            return -EOVERFLOW;
	if (in.size % AES_BLOCK_SIZE != 0)
            return -EINVAL;
        return cbc_mode(CHAOS_ALGO_AES_DEC, in, key, out, AES_BLOCK_SIZE);
    case CHAOS_ALGO_BF_ENC:
        if (out.size < in.size)
            return -EOVERFLOW;
	if (in.size % BLOWFISH_BLOCK_SIZE != 0)
            return -EINVAL;
        return cbc_mode(CHAOS_ALGO_BF_ENC, in, key, out, BLOWFISH_BLOCK_SIZE);
    case CHAOS_ALGO_BF_DEC:
        if (out.size < in.size)
            return -EOVERFLOW;
	if (in.size % BLOWFISH_BLOCK_SIZE != 0)
            return -EINVAL;
        return cbc_mode(CHAOS_ALGO_BF_DEC, in, key, out, BLOWFISH_BLOCK_SIZE);
    default:
        CHECK(false);
        return 0;
    }
}

static int handle_cmd(struct chaos_mailbox_cmd *cmd)
{
    CHECK(cmd->code == CHAOS_CMD_CODE_REQUEST);
    return handle_cmd_request(cmd);
}

static inline uint64_t queue_inc(uint64_t val, uint64_t size)
{
    return (++val) & ((size << 1) - 1);
}

static void push_rsp(struct chaos_mailbox_rsp *rsp)
{
    struct chaos_mailbox_rsp *queue = DRAM_AT(csr->rspq_addr);
    const uint64_t rspq_size = csr->rspq_size;
    uint64_t tail = csr->rsp_tail;

    queue[real_index(tail, rspq_size)] = *rsp;
    csr->rsp_tail = queue_inc(tail, rspq_size);
}

void handle_mailbox(void)
{
    const uint64_t cmdq_size = csr->cmdq_size;
    uint64_t head = csr->cmd_head;
    uint64_t tail = csr->cmd_tail;
    struct chaos_mailbox_cmd *cmdq = DRAM_AT(csr->cmdq_addr);
    struct chaos_mailbox_cmd *cmd;
    struct chaos_mailbox_rsp rsp;

    if (head == tail)
        return;
    cmd = &cmdq[real_index(head, cmdq_size)];
    csr->cmd_head = queue_inc(head, cmdq_size);
    rsp.retval = handle_cmd(cmd);
    rsp.seq = cmd->seq;
    push_rsp(&rsp);
}
