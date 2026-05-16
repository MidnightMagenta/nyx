#ifndef __ASI_I8253_H
#define __ASI_I8253_H

#define PIT_MODE 0x43
#define PIT_CH0  0x40
#define PIT_CH2  0x42

#define PIT_MODE_RATE_GENERATOR 2
#define PIT_MODE_SQUARE_WAVE    3
#define PIT_MODE_SW_STROBE      4
#define PIT_MODE_HW_STROBE      5

void pit_configure_chan(int chan, unsigned int mode, unsigned int divisor);
void pit_set_frequency(unsigned int hz);

#endif
