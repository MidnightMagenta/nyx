#include <nyx/linkage.h>

#include <asi/desc.h>
#include <asi/descriptors.h>
#include <asi/idt.h>
#include <asi/isr_entry.h>

#define IDT_ENTRIES  256
#define IDT_TAB_SIZE IDT_ENTRIES * sizeof(gate_desc)

#define DPL0 0x0
#define DPL3 0x3

#define DEFAULT_STACK 0

#define GATE(_vector, _addr, _ist, _type, _dpl, _segment)                                                              \
    {                                                                                                                  \
            .vector    = _vector,                                                                                      \
            .segment   = _segment,                                                                                     \
            .addr      = _addr,                                                                                        \
            .bits.ist  = _ist,                                                                                         \
            .bits.zero = 0,                                                                                            \
            .bits.type = _type,                                                                                        \
            .bits.dpl  = _dpl,                                                                                         \
            .bits.p    = 1,                                                                                            \
    }

#define INTG(_vector, _addr) GATE(_vector, _addr, 0, GATE_INTERRUPT, DPL0, 0x08)
#define TRPG(_vcetor, _addr) GATE(_vector, _addr, 0, GATE_TRAP, DPL0, 0x08)

extern struct idt_data ISR_TABLE_NAME[256];

gate_desc __page_aligned_bss idt[IDT_ENTRIES];

static void idt_setup_from_table(gate_desc *idt, const struct idt_data *table, int size) {
    gate_desc desc;

    for (; size > 0; table++, size--) {
        idt_init_desc(&desc, table);
        write_idt_entry(idt, table->vector, &desc);
    }
}

static int idt_data_table_size(const struct idt_data *table) {
    int size = 0;
    while (table[size].addr != 0) { size++; }

    return size;
}

void idt_setup_interrupts() {
    idt_setup_from_table(idt, ISR_TABLE_NAME, idt_data_table_size(ISR_TABLE_NAME));
}
