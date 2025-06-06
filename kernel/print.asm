%include "kernel/video.inc"

[bits 32]
SECTION .text

;   将栈中的一个字符写入到光标所在处
global put_char
put_char:
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
    pushad
;   加载显存段，方便写入, gs是频段选择子
    mov ax, SELECTOR_VIDEO
    mov gs, ax
;   获取光标位置
;   高8位
    mov dx, 0x03d4
    mov al, 0x0e
    out dx, al
    mov dx, 0x03d5
    in  al, dx
    mov ah, al
;   低8位
    mov dx, 0x03d4
    mov al, 0x0f
    out dx, al
    mov dx, 0x03d5
    in  al, dx
    mov bx, ax
;   获取栈中的字符
    mov ecx, [esp+36]
;   判断字符
;   CR 回车
    cmp cl, 0x0d
    jz .is_carriage_return
;   LF 换行
    cmp cl, 0x0a
    jz .is_line_feed
;   BS 退格
    cmp cl, 0x08
    jz .is_backspace
;   其他字符
    jmp .put_other
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

;处理退格
.is_backspace:
    dec bx
    shl bx, 1
    mov byte [gs:bx], 0x20
    inc bx
    shr bx, 1
    jmp .set_cursor
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

;处理其他字符
.put_other:
    shl bx, 1
    mov byte [gs:bx], cl
    inc bx
    mov byte [gs:bx], 0x07
    shr bx, 1
    inc bx
    cmp bx, 2000
    jl .set_cursor
    jmp .is_line_feed
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

.is_line_feed:
.is_carriage_return:
    xor dx, dx
    mov ax, bx
    mov si, 80

    div si

    sub bx, dx

.is_carriage_return_end:
    add bx, 80
    cmp bx, 2000
.is_line_feed_end:
    jl .set_cursor

.roll_screen:
    cld
    mov ecx, 960
    mov esi, 0xc00b80a0
    mov edi, 0xc00b8000
    rep movsd
    mov ebx, 3840
    mov ecx, 80

.cls:
    mov word [gs:ebx], 0x0720
    add ebx, 2
    loop .cls
    mov bx, 1920

.set_cursor:
    mov dx, 0x03d4
    mov al, 0x0e
    out dx, al
    mov dx, 0x03d5
    mov al, bh
    out dx, al

    mov dx, 0x03d4
    mov al, 0x0f
    out dx, al
    mov dx, 0x03d5
    mov al, bl
    out dx, al
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

.put_char_done:
    popad
    ret