%include "include/boot.inc"

SECTION MBR vstart=0x7c00
    cli
    xor ax, ax
    mov ds, ax
    mov es, ax
    mov ss, ax
    mov sp, 0x7c00
    sti

    mov [BOOT_DRIVE], dl
    mov ah, 0x41
    mov bx, 0x55aa
    mov dl, [BOOT_DRIVE]
    int 0x13
    jc disk_error_halt
    cmp bx, 0xaa55
    jne disk_error_halt

    mov bx, LOADER_OFS
    mov ax, LOADER_SEG
    mov es, ax 
    
    mov eax, LOADER_START_SECTOR
    mov cx, 4
    call rd_disk_m_16

    mov bx,  0x0
    mov ax, KERNEL_BIN_BASE_ADDR>>4
    mov es, ax
    
    mov eax, KERNEL_START_SECTOR  
    

    mov bp, 8                       
.load_kernel_loop:
    mov cx, 16
    call rd_disk_m_16
    
    add eax, 16

    mov dx, es
    add dx, 0x200
    mov es, dx
    
    dec bp
    jnz .load_kernel_loop

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

rd_disk_m_16:
    pushad

    mov word [dap_sector_count], cx
    mov word [dap_buffer_offset], bx
    mov word [dap_buffer_segment], es
    mov dword [dap_lba_low], eax

    xor ecx, ecx
    mov dword [dap_lba_high], ecx

    mov ah, 0x42
    mov dl, [BOOT_DRIVE]
    mov si, dap
    int 0x13
    
    jc disk_error_halt
    popad
    ret

disk_error_halt:
    mov al, ah
    call print_hex
    hlt
    jmp $

print_hex:
    push ax
    mov al, ah
    shr al, 4
    call print_nibble
    pop ax
    and al, 0x0F
    call print_nibble
    ret

print_nibble:
    add al, '0'
    cmp al, '9'
    jle .print
    add al, 7
.print:
    mov ah, 0x0e
    int 0x10
    ret

BOOT_DRIVE: db 0

align 4
dap:
    dap_size           db 0x10
    dap_reserved       db 0
    dap_sector_count   dw 0
    dap_buffer_offset  dw 0
    dap_buffer_segment dw 0
    dap_lba_low        dd 0
    dap_lba_high       dd 0

times 510 - ($ - $$) db 0
dw 0xaa55