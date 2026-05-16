#include <asi/cpu.h>
#include "entry_defs.h"

.extern unhandled_exception_handler

.section .text

ENTRY(__div_zero_entry)
    push    $0
    push    $0
    PUSH_REGS
unhandled_common:
    mov     %rsp, %rdi
    call unhandled_exception_handler
    POP_REGS
    add $16, %rsp
    iretq

.macro UNHANDLED_EXCEPTION name vector ecode
.global \name
\name:
.ifeq \ecode
    push $0
.endif
    push $\vector
    PUSH_REGS
    jmp unhandled_common
.endm

UNHANDLED_EXCEPTION __debug_entry 1 0
UNHANDLED_EXCEPTION __nmi_entry 2 0
UNHANDLED_EXCEPTION __breakpoint_entry 3 0
UNHANDLED_EXCEPTION __overflow_entry 4 0
UNHANDLED_EXCEPTION __bound_range_entry 5 0
UNHANDLED_EXCEPTION __invalid_opcode_entry 6 0
UNHANDLED_EXCEPTION __dev_not_avail_entry 7 0
UNHANDLED_EXCEPTION __double_fault_entry 8 1
UNHANDLED_EXCEPTION __coprocessor_overrun_entry 9 0
UNHANDLED_EXCEPTION __ivalid_tss_entry 10 1
UNHANDLED_EXCEPTION __segment_not_present_entry 11 1
UNHANDLED_EXCEPTION __stack_segment_fault_entry 12 1
UNHANDLED_EXCEPTION __general_prot_entry 13 1
UNHANDLED_EXCEPTION __page_fault_entry 14 1
UNHANDLED_EXCEPTION __x87_exception_entry 16 0
UNHANDLED_EXCEPTION __alignment_check_entry 17 1
UNHANDLED_EXCEPTION __machine_check_entry 18 0
UNHANDLED_EXCEPTION __simd_exception_entry 19 0
UNHANDLED_EXCEPTION __virtualization_exception_entry 20 0
UNHANDLED_EXCEPTION __control_prot_exception 21 1
UNHANDLED_EXCEPTION __hypervisor_exception 28 0
UNHANDLED_EXCEPTION __vmm_com_exception 29 1
UNHANDLED_EXCEPTION __security_exception 30 1
UNHANDLED_EXCEPTION __reserved_exception 31 0

