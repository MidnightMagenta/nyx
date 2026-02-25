.section .boot.text, "ax"
.global startup_64
startup_64:
    hlt
    jmp startup_64

