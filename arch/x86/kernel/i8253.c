#include <nyx/types.h>

#include <asi/i8253.h>
#include <asi/io.h>

#define PIT_BASE_FREQ 1193182ul

#define PIT_ACCESS_LOHI 0x30

void pit_configure_chan(int chan, unsigned int mode, unsigned int divisor) {
    u8 cmd = ((chan & 0x3) << 6) | PIT_ACCESS_LOHI | ((mode & 0x7) << 1);

    outb_p(PIT_MODE, cmd);
    outb_p(PIT_CH0 + chan, divisor & 0xFF);
    outb_p(PIT_CH0 + chan, (divisor >> 8) & 0xFF);
}

void pit_set_frequency(unsigned int hz) {
    u16 divisor = PIT_BASE_FREQ / hz;
    pit_configure_chan(0, PIT_MODE_SQUARE_WAVE, divisor);
}
