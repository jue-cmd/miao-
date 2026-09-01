#ifndef __KERNEL_DEBUG_H
#define __KERNEL_DEBUG_H

void panic_spin(char *filename, int line, const char *func, const char *condition);

void dump_regs(void);
/* 从指定 ebp 起沿栈帧链打印返回地址（需 -fno-omit-frame-pointer） */
void dump_stack_from(unsigned int ebp);
void dump_stack(void);

#define PANIC(...) panic_spin((char *)__FILE__, __LINE__, __func__, __VA_ARGS__)

#ifdef NDEBUG
#define ASSERT(CONDITION) ((void)0)
#else
#define ASSERT(CONDITION)          \
    do                             \
    {                              \
        if (!(CONDITION))          \
        {                          \
            PANIC(#CONDITION);     \
        }                          \
    } while (0)
#endif

#endif
