%include "boot.inc"
%include "elf.inc"
SECTION loader vstart=LOADER_BASE_ADDR
LOADER_STACK_TOP equ LOADER_BASE_ADDR
jmp loader_start
gdt_addr:
    GDT_BASE: dd 0x00000000
            dd 0x00000000
    CODE_DESC: dd 0x0000FFFF
            dd DESC_CODE_HIGH4
    DATA_STACK_DESC: dd 0x0000FFFF
                    dd DESC_DATA_HIGH4

    VIDEO_DESC: dd 0x80000007
                dd DESC_VIDEO_HIGH4
    GDT_SIZE equ $ - GDT_BASE
    GDT_LIMIT equ GDT_SIZE - 1
    SELECTOR_CODE equ (0x0001 << 3) +TI_GDT+RPL0
    SELECTOR_DATA equ (0x0002 << 3) +TI_GDT+RPL0
    SELECTOR_VIDEO equ (0x0003 << 3) +TI_GDT+RPL0
times 60 dq 0


;以下是 gdt 的指针，前 2 字节是 gdt 界限，后 4 字节是 gdt 起始地址
gdt_ptr  dw GDT_LIMIT
         dd gdt_addr

total_mem_bytes dd 0

ards_buf times 244 db 0
adrs_nr dw 0
loader_start:
    xor ebx,ebx
    mov edx,0x534d4150
    mov di,ards_buf
    .e820_mem_get_loop:
        mov ax,0x0000e820
        mov ecx,20
        int 0x15
        jc .e820_mem_get_fail_try_e801
        add di,cx
        inc word [adrs_nr]
        cmp ebx,0
        jnz .e820_mem_get_loop
    mov cx,[adrs_nr]
    mov ebx,ards_buf
    xor edx,edx
    .find_max_mem_area:
        mov eax,[ebx]
        add eax,[ebx+8]
        add ebx,20
        cmp edx,eax
        jge .next_ards
        mov edx,eax
    .next_ards:
        loop .find_max_mem_area
        jmp .mem_get_ok
    .e820_mem_get_fail_try_e801:
        mov ax,0xe801
        int 0x15
        jc .e801_failed_try_88
        mov cx, 0x400
        mul cx
        shl edx,16
        or edx,eax
        add edx,0x100000
        mov esi,edx
        xor eax, eax
        mov ecx, 0x1000000
        mul ecx
        add esi, eax
        mov edx, esi
        jmp .mem_get_ok
    .e801_failed_try_88:
        mov ah,0x88
        int 0x15
        jc .error_hlt
        and eax,0x0000fffff
        mov cx, 0x400
        mul cx
        shl edx,16
        or edx, eax
        add edx,0x100000
        jmp .mem_get_ok
    .error_hlt:
        mov byte [gs:0xb6],'E'
        mov byte [gs:0xb8], 0x0A
        jmp $
    .mem_get_ok:
        mov [total_mem_bytes],edx

    xor edx, edx
    in al, 0x92
    or al, 0000_0010B
    out 0x92, al

    lgdt [gdt_ptr]

    mov eax, cr0
    or eax, 0x00000001
    mov cr0, eax

    jmp SELECTOR_CODE:p_mode_start

[bits 32]
p_mode_start:
    mov ax, SELECTOR_DATA
    mov ds, ax
    mov es, ax
    mov ss, ax
    mov esp, LOADER_STACK_TOP
    mov ax, SELECTOR_VIDEO
    mov gs, ax
    
    mov eax, KERNEL_START_SECTOR
    mov ebx, KERNEL_BIN_BASE_ADDR
    mov ecx, 200;读入200个扇区
    
    call rd_disk_m_32

    call setup_page;初始化页表
    sgdt [gdt_ptr]

    mov ebx,[gdt_ptr+2]
    or dword [ebx+0x18+4],0xc0000000

    add dword [gdt_ptr+2],0xc0000000
    add esp,0xc0000000
    mov eax, PAGE_DIR_TABLE_POS
    mov cr3, eax

    mov eax, cr0
    or eax,0x80000000
    mov cr0, eax

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


rd_disk_m_32:
    ;保存寄存器
    mov esi,eax
    mov edi,ecx

    mov dx,0x1f2;sector count寄存器
    mov al,cl;扇区数
    out dx,al

    mov eax,esi

    mov dx,0x1f3
    out dx,al

    mov cl,8
    shr eax,cl
    mov dx,0x1f4
    out dx,al

    shr eax,cl
    mov dx,0x1f5
    out dx,al

    shr eax,cl
    and al,0x0f
    or al,0xe0
    mov dx,0x1f6
    out dx,al

    mov dx,0x1f7
    mov al,0x20
    out dx,al

    .not_ready:
        nop
        in al,dx
        and al,0x88

        cmp al,0x08
        jnz .not_ready

        mov eax,edi
        mov edx,256
        mul edx
        mov ecx,eax

        mov dx,0x1f0
    .go_on_read:
        in ax,dx
        mov [ebx],ax
        add ebx,2
        loop .go_on_read
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