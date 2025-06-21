%include "include/boot.inc"

SECTION MBR vstart=0x7c00
mov ax,cs
mov ds,ax
mov es,ax
mov ss,ax

mov ax, 0x0600
mov bx, 0x0700
mov cx, 0
mov dx, 0x184f
int 0x10

mov ax,0xb800
mov gs,ax
mov sp,0x7c00
mov eax,LOADER_START_SECTOR
mov bx,LOADER_BASE_ADDR 
mov cx,4
call rd_disk_m_16;加载loader到内存
find_loader_addr:
    cmp dword [bx],'load'
    jne .not_here
    add bx,4
    jmp bx
.not_here:
    inc bx
    jmp find_loader_addr

rd_disk_m_16:
    pushad
    ;保存寄存器
    mov esi,eax
    mov di,cx

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

        mov ax,di
        mov dx,256
        mul dx
        mov cx,ax

        mov dx,0x1f0
    .go_on_read:
        in ax,dx
        mov [bx],ax
        add bx,2
        loop .go_on_read
        popad
        ret

times 510-($-$$) db 0
dw 0xaa55