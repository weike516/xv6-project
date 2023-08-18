.text

/*
 * 保存旧线程的寄存器状态，
 * 恢复新线程的寄存器状态。
 */

.globl thread_switch
thread_switch:
/* YOUR CODE HERE */
/* 保存旧线程的寄存器状态到 a0 指向的内存位置 */
sd ra, 0(a0)
sd sp, 8(a0)
sd fp, 16(a0)
sd s1, 24(a0)
sd s2, 32(a0)
sd s3, 40(a0)
sd s4, 48(a0)
sd s5, 56(a0)
sd s6, 64(a0)
sd s7, 72(a0)
sd s8, 80(a0)
sd s9, 88(a0)
sd s10, 96(a0)
sd s11, 104(a0)

/* 恢复新线程的寄存器状态从 a1 指向的内存位置 */
ld sp, 8(a1)
ld fp, 16(a1)
ld s1, 24(a1)
ld s2, 32(a1)
ld s3, 40(a1)
ld s4, 48(a1)
ld s5, 56(a1)
ld s6, 64(a1)
ld s7, 72(a1)
ld s8, 80(a1)
ld s9, 88(a1)
ld s10, 96(a1)
ld s11, 104(a1)

ld ra, 0(a1) /* 设置返回地址为下一个线程的地址 */
ret    /* 返回到 ra */

