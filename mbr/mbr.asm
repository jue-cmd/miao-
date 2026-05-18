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
    mov eax, LOADER_START_SECTOR
    mov bx, LOADER_OFS
    mov ax, LOADER_SEG
    mov es, ax
    mov cx, 4
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
    jmp disk_error_halt      ; 没找到也跳到错误

loader_found:
    add bx, 4               ; 跳过 'load' 字符串

    ; --- 打印 "Success" 逻辑 ---
    push bx                 ; 保存 bx，防止被打印逻辑破坏
    
    mov ah, 0x0e            ; BIOS 卷帘显示服务 (Teletype Output)
    mov al, 'S'             ; 打印字符 'S'
    int 0x10
    
    pop bx                  ; 恢复真实的 Loader 偏移地址
    ; ---------------------------
    push word LOADER_SEG    ; 将段地址入栈
    push bx                 ; 将计算出的偏移地址入栈
    retf                    ; 执行远返回，跳转到 LOADER_SEG:BX

; -----------------------------------------------------------
; 读取函数 (去掉了点号开头的局部标签，避免歧义)
; -----------------------------------------------------------
rd_disk_m_16:
    pushad
    push ds
    
    xor ax, ax
    mov ds, ax

    mov word [dap_sector_count], cx
    mov word [dap_buffer_offset], bx
    mov word [dap_buffer_segment], es
    mov dword [dap_lba_low], eax

    mov ah, 0x42
    mov dl, [BOOT_DRIVE]
    mov si, dap
    int 0x13
    
    jc disk_error_halt

    pop ds
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