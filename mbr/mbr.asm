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

mov byte[gs:0x00],'L'
mov byte[gs:0x01],0xA4
mov byte[gs:0x02],'O'
mov byte[gs:0x03],0xA4
mov byte[gs:0x04],'A'
mov byte[gs:0x05],0xA4
mov byte[gs:0x06],'D'
mov byte[gs:0x07],0xA4
mov byte[gs:0x08],'I'
mov byte[gs:0x09],0xA4
mov byte[gs:0x0a],'N'
mov byte[gs:0x0b],0xA4
mov byte[gs:0x0c],'f'
mov byte[gs:0x0d],0xA4

mov eax,LOADER_START_SECTOR
mov bx,LOADER_BASE_ADDR
mov cx,4
call rd_disk_m_16

jmp LOADER_BASE_ADDR

rd_disk_m_16:
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
        ret

times 510-($-$$) db 0
dw 0xaa55