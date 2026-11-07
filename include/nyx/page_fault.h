#ifndef _NYX_PAGE_FAULT_H
#define _NYX_PAGE_FAULT_H

#include <nyx/types.h>

enum pgflt_reason {
    PGFLT_NOT_PRESENT,
    PGFLT_PROT_VIOLATION,
    PGFLT_INSTR_FETCH_NX,
    PGFLT_RESERVED_BIT,
};

struct pgflt_info {
    virt_addr_t       pf_addr;
    enum pgflt_reason pf_reason;
    bool              pf_write;
    bool              pf_user;
    bool              pf_exec;
};

typedef enum {
    PGFLT_NORMAL = 0,
    PGFLT_RETRY,
    PGFLT_SIGSEGV,
    PGFLT_SIGBUS,
    PGFLT_TRY_FIXUP,
    PGFLT_OOM,
    PGFLT_KERNEL_BUG,
} pgflt_result_t;

#endif
