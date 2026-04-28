#ifndef _ASI_I8259_H
#define _ASI_I8259_H

#define PIC_MASTER_CMD  0x20
#define PIC_MASTER_IMR  0x21
#define PIC_MASTER_ISR  PIC_MASTER_CMD
#define PIC_MASTER_POLL PIC_MASTER_ISR
#define PIC_MASTER_OCW3 PIC_MASTER_ISR
#define PIC_SLAVE_CMD   0xa0
#define PIC_SLAVE_IMR   0xa1

void i8259_init();
void i8259_mask_all();
void i8259_mask(unsigned int irq);
void i8259_unmask(unsigned int irq);
void i8259_eoi(unsigned int irq);

#endif
