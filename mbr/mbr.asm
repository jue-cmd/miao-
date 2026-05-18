%include "include/boot.inc"

SECTION MBR vstart=0x7c00
    cli
    xor ax, ax
    mov ds, ax
    mov es, ax
    mov ss, ax
    mov sp, 0x7c00
    sti

    mov [BOOT_DRIVE], dl    ; 保存启动驱动器号

    ; 准备加载 loader
    mov bx, LOADER_OFS      ; 偏移
    mov ax, LOADER_SEG      ; 段
    mov es, ax              ; 先把段寄存器填好，AX 随便用
    
    mov eax, LOADER_START_SECTOR ; 最后再给 EAX 赋值，确保它是 0x2
    mov cx, 4               ; 读 4 个扇区
    call rd_disk_m_16

    ; 准备加载 Kernel
    mov bx,  0x0   ; 偏移
    mov ax, KERNEL_BIN_BASE_ADDR>>4        ; 段
    mov es, ax              ; 先把段寄存器填好，AX 随便用
    
    mov eax, KERNEL_START_SECTOR ; 最后再给 EAX 赋值，确保它是 0x2
    mov cx, 128              ; 读 4 个扇区
    call rd_disk_m_16
    ; 查找 magic 'load'
    mov ax, LOADER_SEG
    mov es, ax
    mov bx, LOADER_OFS
    mov cx, 512 * 4
find_loader_magic:
    cmp dword [es:bx], 'load'
    je loader_found
    inc bx
    loop find_loader_magic
    jmp disk_error_halt      

loader_found:

    add bx, 4             
    push bx                
    mov ah, 0x0e         
    mov al, 'S'          
    int 0x10
    
    pop bx                
    push word LOADER_SEG   
    push bx              
    retf               

; -----------------------------------------------------------
; 读取函数 (去掉了点号开头的局部标签，避免歧义)
; -----------------------------------------------------------
rd_disk_m_16:
    pushad

    mov word [dap_sector_count], cx
    mov word [dap_buffer_offset], bx
    mov word [dap_buffer_segment], es
    mov dword [dap_lba_low], eax

    mov ah, 0x42
    mov dl, [BOOT_DRIVE]
    mov si, dap
    int 0x13
    
    jc disk_error_halt
    popad
    ret

disk_error_halt:
    mov ax, 0x0e45 ; 打印 'E'
    int 0x10
    hlt
    jmp disk_error_halt

BOOT_DRIVE: db 0

; Disk Address Packet 结构
align 4
dap:
    dap_size           db 0x10
    dap_reserved       db 0
    dap_sector_count   dw 0
    dap_buffer_offset  dw 0
    dap_buffer_segment dw 0
    dap_lba_low        dd 0
    dap_lba_high       dd 0

; -----------------------------------------------------------
times 510 - ($ - $$) db 0
dw 0xaa55