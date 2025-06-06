PT_NULL equ 0;忽略
PT_LOAD equ 1;可加载程序段
PT_DYNAMIC equ 2;动态链接信息
PT_INTERP equ 3;动态加载器名称
PT_NOTE equ 4;一些辅助的附加信息
PT_SHLIB equ 5;保留
PT_PHDR equ 6;程序表头
PT_LOPROC equ 0x70000000
PT_HIPROC equ 0x7fffffff

PT_LOAD_FLAGS_WRITE equ 1
PT_LOAD_FLAGS_READ equ 2
PT_LOAD_FLAGS_EXECUTE equ 4

SHN_UNDEF equ 0
SHN_LORESERVE equ 0xff00
SHN_LOPROC equ 0xff00
SHN_HIPROC equ 0xff1f
SHN_LOOS equ 0xff20