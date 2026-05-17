.section .text
.global kthread_trampoline
.extern kthread_exit

// rdi = kthread entry
kthread_trampoline:
    call *%rdi
    call kthread_exit
