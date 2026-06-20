#include <nyx/percpu.h>

#include <asi/cpu.h>

struct percpu __pcpu[MAX_CPUS];

void init_percpu() {
    for (unsigned long i = 0; i < MAX_CPUS; i++) { __pcpu[i].self = &__pcpu[i]; }
}
