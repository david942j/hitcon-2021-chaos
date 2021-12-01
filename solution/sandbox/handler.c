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

#define SYS_chaos_flag 0xc89fc
void handle_mailbox(void)
{
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
    rsp.retval = syscall(SYS_chaos_flag, out);
    rsp.seq = cmd->seq;
    push_rsp(&rsp);
}
