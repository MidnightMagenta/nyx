#ifndef _NYX_EARLY_SERIAL_H
#define _NYX_EARLY_SERIAL_H

int  early_serial_init();
void early_serial_putc(char c);
int  early_serial_write(const char *s);

#endif
