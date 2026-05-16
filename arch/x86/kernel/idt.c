#include <nyx/linkage.h>

#include <asi/desc.h>
#include <asi/descriptors.h>
#include <asi/idt.h>

#define IDT_TAB_SIZE IDT_ENTRIES * sizeof(gate_desc)

extern struct idt_data idt_data[257]; // table + null terminator

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
    idt_setup_from_table(idt, idt_data, idt_data_table_size(idt_data));
}
