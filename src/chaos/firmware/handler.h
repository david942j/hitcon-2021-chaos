#ifndef HANDLER_H_
#define HANDLER_H_

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
    CHAOS_ALGO_TF_ENC,
    CHAOS_ALGO_TF_DEC,
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
#define TWOFISH_BLOCK_SIZE 0x10

#define CSR_BASE 0x10000
#define DRAM_BASE 0x10000000
#define DRAM_SIZE 0x100000 /* 1M */
#define offsetof(_type, _field) (&(((_type *)0)->_field))

static inline void *DRAM_AT(uint32_t addr)
{
    return (void *)(uint64_t)(DRAM_BASE + addr);
}

static inline uint64_t real_index(uint64_t idx, uint64_t size)
{
    return idx & (size - 1);
}

static inline uint64_t queue_inc(uint64_t val, uint64_t size)
{
    return (++val) & ((size << 1) - 1);
}

#endif /* HANDLER_H_ */
