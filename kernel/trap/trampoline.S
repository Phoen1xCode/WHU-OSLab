# trampoline.S
# 处理从用户空间陷入内核以及从内核返回用户空间的底层代码
# 用户空间和内核空间之间的跳板代码
# 映射在虚拟地址空间的最高页

#include "../include/riscv.h"
#include "../include/memlayout.h"

.section trampsec
.globl trampoline
trampoline:
        .align 4
        .globl uservector
uservector:    
        #
        # 从用户空间陷入时的入口点
        # 此时：
        # - sscratch 指向当前进程的 trapframe
        # - 所有用户寄存器都还保存着用户值
        # - 我们需要切换到内核页表和内核栈
        #
        
        # 交换 a0 和 sscratch
        # 现在 a0 指向 trapframe，sscratch 保存用户的 a0
        csrrw a0, sscratch, a0

        # 保存用户寄存器到 trapframe
        sd ra, 40(a0)
        sd sp, 48(a0)
        sd gp, 56(a0)
        sd tp, 64(a0)
        sd t0, 72(a0)
        sd t1, 80(a0)
        sd t2, 88(a0)
        sd s0, 96(a0)
        sd s1, 104(a0)
        sd a1, 120(a0)
        sd a2, 128(a0)
        sd a3, 136(a0)
        sd a4, 144(a0)
        sd a5, 152(a0)
        sd a6, 160(a0)
        sd a7, 168(a0)
        sd s2, 176(a0)
        sd s3, 184(a0)
        sd s4, 192(a0)
        sd s5, 200(a0)
        sd s6, 208(a0)
        sd s7, 216(a0)
        sd s8, 224(a0)
        sd s9, 232(a0)
        sd s10, 240(a0)
        sd s11, 248(a0)
        sd t3, 256(a0)
        sd t4, 264(a0)
        sd t5, 272(a0)
        sd t6, 280(a0)

        # 从sscratch中保存用户的a0到trapframe p->trampoline->a0
        csrr t0, sscratch
        sd t0, 112(a0)

        # 从 trapframe 加载内核栈指针
        ld sp, 8(a0)

        # 从 trapframe 加载内核页表地址
        ld t1, 0(a0)
        
        # 从 trapframe 加载 usertrap 的地址
        ld t0, 16(a0)

        # 从 trapframe 加载 hartid
        ld tp, 32(a0)

        # 切换到内核页表
        csrw satp, t1
        sfence.vma zero, zero

        # 跳转到 usertrap()
        jr t0

        .globl userret
userret:
        #
        # 从内核返回用户空间
        # 参数：
        # a0: 用户页表的 SATP 值
        # a1: trapframe 的用户虚拟地址
        #
        
        # 切换到用户页表
        csrw satp, a0
        sfence.vma zero, zero

        # 将 trapframe 地址放入 sscratch，供下次陷入使用
        csrw sscratch, a1

        # 从 trapframe 恢复所有用户寄存器，除了 a0
        ld ra, 40(a1)
        ld sp, 48(a1)
        ld gp, 56(a1)
        ld tp, 64(a1)
        ld t0, 72(a1)
        ld t1, 80(a1)
        ld t2, 88(a1)
        ld s0, 96(a1)
        ld s1, 104(a1)
        ld a2, 128(a1)
        ld a3, 136(a1)
        ld a4, 144(a1)
        ld a5, 152(a1)
        ld a6, 160(a1)
        ld a7, 168(a1)
        ld s2, 176(a1)
        ld s3, 184(a1)
        ld s4, 192(a1)
        ld s5, 200(a1)
        ld s6, 208(a1)
        ld s7, 216(a1)
        ld s8, 224(a1)
        ld s9, 232(a1)
        ld s10, 240(a1)
        ld s11, 248(a1)
        ld t3, 256(a1)
        ld t4, 264(a1)
        ld t5, 272(a1)
        ld t6, 280(a1)

        # 最后恢复 a0 和 a1
        ld a0, 112(a1)
        ld a1, 120(a1)

        # 返回用户空间
        sret

        .globl trampoline_end
trampoline_end: