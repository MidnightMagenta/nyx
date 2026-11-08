#include <asi/exception.h>
#include <asi/traps.h>
#include <nyx/errno.h>
#include <nyx/stddef.h>

extern struct exception_entry __ex_table_start[];
extern struct exception_entry __ex_table_stop[];

const struct exception_entry *search_ex_table(virt_addr_t inst_addr) {
    for (const struct exception_entry *ex = __ex_table_start; ex < __ex_table_stop; ex++) {
        if (ex->insn_addr == inst_addr) { return ex; }
    }

    return NULL;
}

int try_fixup_exception(struct trap_frame *tf) {
    const struct exception_entry *ex = search_ex_table(tf->frame.rip);
    if (!ex) { return -ENOENT; }
    tf->frame.rip = ex->fixup_addr;
    return 0;
}
