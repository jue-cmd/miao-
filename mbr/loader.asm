;仅负责进入到保护模式，并创建好页表
%include "boot.inc"
%include "elf.inc"
SECTION loader vstart=LOADER_BASE_ADDR
[bits 16]
LOADER_STACK_TOP equ LOADER_BASE_ADDR
gdt_addr:
    GDT_BASE: dd 0x00000000
            dd 0x00000000
    CODE_DESC: dd 0x0000FFFF
            dd DESC_CODE_HIGH4
    DATA_STACK_DESC: dd 0x0000FFFF
                    dd DESC_DATA_HIGH4

    VIDEO_DESC: 
        dw 0x7FFF        ; Limit 0-15
        dw 0x8000        ; Base 0-15 = 0x8000? 不对，应该是 0xB800
        db 0x0B          ; Base 16-23 = 0x0B
        db 0x92          ; P=1, DPL=0, S=1, Type=0010 (data, read/write)
        db 0xCF          ; G=1, D=1, L=0, AVL=0, Limit 16-19 = 0xF
        db 0x00          ; Base 24-31 = 0x00
    GDT_SIZE equ $ - GDT_BASE
    GDT_LIMIT equ GDT_SIZE - 1

times 60 dq 0
gdt_ptr  dw GDT_LIMIT
         dd gdt_addr

ards_buf times 244 db 0
adrs_nr dw 0
db 'load'
loader_start:
    xor ax, ax
    mov ds, ax
    mov es, ax
    in al, 0x92
    or al, 0000_0010B
    out 0x92, al
    ;mov esi,gdt_ptr
    cli
    lgdt [gdt_ptr]
    
    mov eax, cr0
    or eax, 0x00000001
    mov cr0, eax
    jmp SELECTOR_CODE:p_mode_start

[bits 32]

test_print:
    ; 使用 VIDEO 段选择子
    mov ax, SELECTOR_VIDEO
    mov gs, ax
    ; 在屏幕第一行第一列写字符 'S'（黑底绿字，0x0A = 绿色）
    mov word [gs:0], 0x0A53  ; 0x0A 是属性（绿色），0x53 是 'S' 的 ASCII
    ; 或者写白色字符（0x0F = 白字黑底）
    ; mov word [gs:0], 0x0F53
    ret

test_io_only:
    ; 测试 out 指令
    mov dx, 0x03D4
    mov al, 0x0E
    out dx, al
    ; 测试 in 指令
    mov dx, 0x03D5
    in al, dx
    ret

p_mode_start:
    mov ax, SELECTOR_DATA
    mov ds, ax
    mov es, ax
    mov ss, ax
    mov esp, LOADER_STACK_TOP
    mov ax, SELECTOR_VIDEO
    mov gs, ax
    call setup_page;初始化页表
    sgdt [gdt_ptr]
    mov ebx,[gdt_ptr+2]
    ;or dword [ebx+0x18+4],0xc0000000
    add dword [gdt_ptr+2],0xc0000000
    add esp,0xc0000000
    mov eax, PAGE_DIR_TABLE_POS
    mov cr3, eax
    mov eax, cr0
    or eax, 0x80000000
    mov cr0, eax
    mov ax, SELECTOR_VIDEO
    mov gs, ax
    lgdt [gdt_ptr]
    jmp enter_kernel
enter_kernel:
    call kernel_init
    jmp KERNEL_ENTRY_POINT

kernel_init:
    xor eax,eax
    xor ebx,ebx
    xor ecx,ecx
    xor edx,edx

    ;获取program header 大小
    mov dx,[KERNEL_BIN_BASE_ADDR+42]
    ;获取program header 偏移量
    mov ebx,[KERNEL_BIN_BASE_ADDR+28]

    add ebx,KERNEL_BIN_BASE_ADDR
    mov cx, [KERNEL_BIN_BASE_ADDR+44]

    .search_segment:
        cmp byte[ebx+0],PT_NULL
        je .PTNULL
        push dword[ebx+0x10]
        mov eax,[ebx+0x4]
        add eax,KERNEL_BIN_BASE_ADDR
        push eax
        push dword[ebx+0x8]
        call mem_cpy
        add esp,12
        .PTNULL:
            add ebx,edx
        loop .search_segment    
    ret
setup_page:
    ;清空页目录表
    mov ecx,4096
    mov esi,0
    .clear_page_dir:
        mov byte[PAGE_DIR_TABLE_POS+esi],0
        inc esi
    loop .clear_page_dir

    create_pde:
        mov eax, PAGE_DIR_TABLE_POS
        add eax,0x1000;页表目录为4KB，页表目录和页表紧挨着，所以要加0x1000
        mov ebx,eax   ;ebx为页表的基地址

        ;设置页目录项的属性
        ;      用户属性|写属性|存在属性
        or eax,PG_US_U|PG_RW_W|PG_P
        mov [PAGE_DIR_TABLE_POS+0x0],eax
        mov [PAGE_DIR_TABLE_POS+0xc00],eax
        sub eax, 0x1000
        ;为啥啊！！！
        mov [PAGE_DIR_TABLE_POS+4092],eax;访问页表本身的，实现通过虚拟地址访问页表
    ;设置页表项的属性
    mov ecx, 256
    mov esi, 0
    mov edx, PG_US_U|PG_RW_W|PG_P
    .create_pte:
        mov [ebx+esi*4],edx
        add edx,4096
        inc esi
    loop .create_pte
    mov eax, PAGE_DIR_TABLE_POS
    mov ecx,254
    mov esi,769
    .create_kernel_pde:
        mov [ebx+esi*4],eax
        inc esi
        add eax,0x1000
    loop .create_kernel_pde
    ret

;void mem_cpy(void *src,void *dst,int size)
mem_cpy:
    cld;清除方向标志
    push ebp
    mov ebp,esp
    push ecx

    mov edi,[ebp+8] ;目标地址
    mov esi,[ebp+12] ;源地址
    mov ecx,[ebp+16] ;大小
    rep movsb
    pop ecx
    pop ebp
    ret