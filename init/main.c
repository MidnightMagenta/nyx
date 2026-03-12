void start_kernel() {
    while (1) { __asm__ volatile("hlt"); }
}
