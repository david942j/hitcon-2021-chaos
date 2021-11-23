#include "handler.h"
#include "syscall.h"
#include "types.h"

static struct Csrs *csr = (struct Csrs *)CSR_BASE;

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
    rsp.seq = cmd->seq;
    struct chaos_request *req = DRAM_AT(cmd->dma_addr);
    uint32_t *in = DRAM_AT(req->input);
    rsp.retval = in[0];
    push_rsp(&rsp);
}
