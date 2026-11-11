#include "entry_defs.h"
#include <asi/descriptors.h>

.section .init.data
.global idt_data
idt_data:
IDT_DATA_ENTRY 0 0x8 0 GATE_INTERRUPT DPL0 __div_zero_entry
IDT_DATA_ENTRY 1 0x8 0 GATE_INTERRUPT DPL0 __debug_entry
IDT_DATA_ENTRY 2 0x8 0 GATE_INTERRUPT DPL0 __nmi_entry
IDT_DATA_ENTRY 3 0x8 0 GATE_INTERRUPT DPL0 __breakpoint_entry
IDT_DATA_ENTRY 4 0x8 0 GATE_INTERRUPT DPL0 __overflow_entry
IDT_DATA_ENTRY 5 0x8 0 GATE_INTERRUPT DPL0 __bound_range_entry
IDT_DATA_ENTRY 6 0x8 0 GATE_INTERRUPT DPL0 __invalid_opcode_entry
IDT_DATA_ENTRY 7 0x8 0 GATE_INTERRUPT DPL0 __dev_not_avail_entry
IDT_DATA_ENTRY 8 0x8 0 GATE_INTERRUPT DPL0 __double_fault_entry
IDT_DATA_ENTRY 9 0x8 0 GATE_INTERRUPT DPL0 __coprocessor_overrun_entry
IDT_DATA_ENTRY 10 0x8 0 GATE_INTERRUPT DPL0 __ivalid_tss_entry
IDT_DATA_ENTRY 11 0x8 0 GATE_INTERRUPT DPL0 __segment_not_present_entry
IDT_DATA_ENTRY 12 0x8 0 GATE_INTERRUPT DPL0 __stack_segment_fault_entry
IDT_DATA_ENTRY 13 0x8 0 GATE_INTERRUPT DPL0 __general_prot_entry
IDT_DATA_ENTRY 14 0x8 0 GATE_INTERRUPT DPL0 __page_fault_entry
IDT_DATA_ENTRY 15 0x8 0 GATE_INTERRUPT DPL0 __reserved_exception
IDT_DATA_ENTRY 16 0x8 0 GATE_INTERRUPT DPL0 __x87_exception_entry
IDT_DATA_ENTRY 17 0x8 0 GATE_INTERRUPT DPL0 __alignment_check_entry
IDT_DATA_ENTRY 18 0x8 0 GATE_INTERRUPT DPL0 __machine_check_entry
IDT_DATA_ENTRY 19 0x8 0 GATE_INTERRUPT DPL0 __simd_exception_entry
IDT_DATA_ENTRY 20 0x8 0 GATE_INTERRUPT DPL0 __virtualization_exception_entry
IDT_DATA_ENTRY 21 0x8 0 GATE_INTERRUPT DPL0 __control_prot_exception
IDT_DATA_ENTRY 22 0x8 0 GATE_INTERRUPT DPL0 __reserved_exception
IDT_DATA_ENTRY 23 0x8 0 GATE_INTERRUPT DPL0 __reserved_exception
IDT_DATA_ENTRY 24 0x8 0 GATE_INTERRUPT DPL0 __reserved_exception
IDT_DATA_ENTRY 25 0x8 0 GATE_INTERRUPT DPL0 __reserved_exception
IDT_DATA_ENTRY 26 0x8 0 GATE_INTERRUPT DPL0 __reserved_exception
IDT_DATA_ENTRY 27 0x8 0 GATE_INTERRUPT DPL0 __reserved_exception
IDT_DATA_ENTRY 28 0x8 0 GATE_INTERRUPT DPL0 __hypervisor_exception
IDT_DATA_ENTRY 29 0x8 0 GATE_INTERRUPT DPL0 __vmm_com_exception
IDT_DATA_ENTRY 30 0x8 0 GATE_INTERRUPT DPL0 __security_exception
IDT_DATA_ENTRY 31 0x8 0 GATE_INTERRUPT DPL0 __reserved_exception

.macro IDT_DATA_IRQ_ENTRY vector
IDT_DATA_ENTRY \vector 0x8 0 GATE_INTERRUPT DPL0 __irq_entry_\vector
.endm

.altmacro
.set i, 32
.rept 224
IDT_DATA_IRQ_ENTRY %i
.set i, i+1
.endr
.noaltmacro

// null terminator
.byte   0
.short  0
.byte   0
.byte   0
.byte   0
.short  0
.quad   0

