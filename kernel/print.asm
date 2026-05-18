%include "boot.inc"

[bits 32]
SECTION .text

global put_char
put_char:
    pushad
    xor eax,eax
    mov ebx,eax
    mov ecx,eax
    mov ecx, [esp + 36]
    
    mov eax, SELECTOR_VIDEO
    mov gs, ax
    mov dx, 0x03D4
    mov al, 0x0E
    out dx, al
    
    mov dx, 0x03D5
    in al, dx
    mov ah, al

    mov dx, 0x03D4
    mov al, 0x0F
    out dx, al
    mov dx, 0x03D5
    in al, dx
    mov bx, ax

    cmp cl, 0x08
    je .backspace
    
    cmp cl, 0x09
    je .tab
    
    cmp cl, 0x0D
    je .carriage_return
    
    cmp cl, 0x0A
    je .line_feed

    mov ax, bx
    shl ax, 1

    mov byte [gs:eax], cl
    mov byte [gs:eax + 1], 0x07

    inc bx
    jmp .check_roll_screen
    
    ; ========================================
    ; 退格键处理
    ; ========================================
.backspace:
    cmp bx, 0                       ; 已经在行首？
    je .done                        ; 如果是，不做任何操作
    
    dec bx                          ; 光标后退1
    mov eax, ebx
    shl eax, 1                      ; 计算显存偏移
    
    mov word [gs:eax], 0x0720       ; 清空字符 (空格+属性)
    jmp .set_cursor                 ; 直接设置光标
    
    ; ========================================
    ; 制表符处理 (对齐到8的倍数)
    ; ========================================
.tab:
    mov eax, ebx
    mov edx, 8
    div edx                         ; eax = 商, edx = 余数
    
    cmp edx, 0
    je .tab_aligned                 ; 已经在边界上
    
    mov ecx, 8
    sub ecx, edx                    ; ecx = 需要添加的空格数
    add ebx, ecx                    ; 光标前进到边界
    
    ; 在当前位置到边界之间填充空格
    sub ecx, edx                    ; 重新计算
    .tab_fill:
        dec ecx
        jl .check_roll_screen
        
        ; 计算要填充的位置
        push ecx
        mov eax, ebx
        sub eax, ecx
        dec eax                     ; 调整索引
        shl eax, 1
        mov word [gs:eax], 0x0720
        pop ecx
        jmp .tab_fill
    
    .tab_aligned:
        jmp .check_roll_screen
    
    ; ========================================
    ; 回车处理 (光标移到行首)
    ; ========================================
.carriage_return:
    mov eax, ebx
    xor edx, edx
    mov ecx, 80
    div ecx                         ; eax = 行号, edx = 列号
    sub bx, dx                      ; bx = 当前行起始位置
    jmp .done
    
    ; ========================================
    ; 换行处理 (光标移到下一行行首)
    ; ========================================
.line_feed:
    mov eax, ebx
    xor edx, edx
    mov ecx, 80
    div ecx                         ; eax = 行号, edx = 列号
    sub bx, dx                      ; bx = 当前行起始位置
    add bx, 80                      ; bx = 下一行起始位置
    
    ; ========================================
    ; 检查是否需要滚屏
    ; ========================================
.check_roll_screen:
    cmp bx, 2000                    ; 2000 = 25行 × 80列
    jl .set_cursor                  ; 未超出，直接设置光标
    
    ; ========================================
    ; 滚屏操作 (向上滚动一行)
    ; ========================================
.roll_screen:
    ; 复制第2-25行到第1-24行
    ; 源地址: 第2行起始 (偏移 160 字节)
    ; 目标地址: 第1行起始 (偏移 0 字节)
    ; 复制内容: 24行 × 80列 = 1920 个字符 = 3840 字节
    
    cld
    mov ecx, 1920                   ; 复制 1920 个字符
    mov esi, 160                    ; 源偏移 (第2行)
    mov edi, 0                      ; 目标偏移 (第1行)
    
.roll_copy:
    mov ax, [gs:esi]                ; 读取字符+属性
    mov [gs:edi], ax                ; 写入到上一行
    add esi, 2
    add edi, 2
    loop .roll_copy
    
    ; 清空最后一行 (第25行)
    mov ecx, 80                     ; 80个字符
    mov edi, 3840                   ; 第25行起始偏移 (24×160)
    
.clear_last_line:
    mov word [gs:edi], 0x0720       ; 写入空格+属性
    add edi, 2
    loop .clear_last_line
    
    ; 光标设置到最后一行行首
    mov bx, 1920                    ; 第25行起始位置 (24×80)
    
    ; ========================================
    ; 设置光标位置
    ; ========================================
.set_cursor:
    ; 高8位
    mov dx, 0x03D4
    mov al, 0x0E
    out dx, al
    mov dx, 0x03D5
    mov al, bh                      ; bh = 光标高8位
    out dx, al
    
    ; 低8位
    mov dx, 0x03D4
    mov al, 0x0F
    out dx, al
    mov dx, 0x03D5
    mov al, bl                      ; bl = 光标低8位
    out dx, al
    
.done:
    popad                           ; 恢复所有寄存器
    ret

; ============================================
; 辅助函数: 清屏
; ============================================
global clear_screen
clear_screen:
    pushad
    
    mov ax, SELECTOR_VIDEO
    mov gs, ax
    
    mov ecx, 2000                   ; 2000个字符
    mov ebx, 0
    
.clear_loop:
    mov word [gs:ebx], 0x0720       ; 空格 + 属性
    add ebx, 2
    loop .clear_loop
    
    ; 光标归零
    xor bx, bx
    jmp .set_cursor_from_clear
    
.set_cursor_from_clear:
    mov dx, 0x03D4
    mov al, 0x0E
    out dx, al
    mov dx, 0x03D5
    mov al, 0
    out dx, al
    
    mov dx, 0x03D4
    mov al, 0x0F
    out dx, al
    mov dx, 0x03D5
    mov al, 0
    out dx, al
    
    popad
    ret

; ============================================
; 辅助函数: 设置光标位置
; 输入: bx = 光标位置 (0-1999)
; ============================================
global set_cursor
set_cursor:
    pushad
    
    mov ax, SELECTOR_VIDEO
    mov gs, ax
    
    mov dx, 0x03D4
    mov al, 0x0E
    out dx, al
    mov dx, 0x03D5
    mov al, bh
    out dx, al
    
    mov dx, 0x03D4
    mov al, 0x0F
    out dx, al
    mov dx, 0x03D5
    mov al, bl
    out dx, al
    
    popad
    ret

; ============================================
; 辅助函数: 获取光标位置
; 输出: bx = 光标位置 (0-1999)
; ============================================
global get_cursor
get_cursor:
    push eax
    push edx
    
    mov dx, 0x03D4
    mov al, 0x0E
    out dx, al
    mov dx, 0x03D5
    in al, dx
    mov ah, al
    
    mov dx, 0x03D4
    mov al, 0x0F
    out dx, al
    mov dx, 0x03D5
    in al, dx
    mov bx, ax
    
    pop edx
    pop eax
    ret

; ============================================
; 辅助函数: 打印字符串
; 输入: esi = 字符串地址 (以0结尾)
; ============================================
global print_string
print_string:
    pushad
    
.print_loop:
    lodsb                           ; al = 当前字符
    test al, al                     ; 检查是否为0
    jz .print_done
    
    push eax                        ; 压入字符参数
    call put_char
    add esp, 4                      ; 清理栈参数
    
    jmp .print_loop
    
.print_done:
    popad
    ret

; ============================================
; 辅助函数: 打印十六进制数
; 输入: eax = 要打印的数字
; ============================================
global print_hex
print_hex:
    pushad
    
    mov ecx, 8                      ; 打印8个十六进制数字
    mov ebx, eax                    ; 保存原值
    
.print_hex_loop:
    rol ebx, 4                      ; 循环左移4位
    mov al, bl
    and al, 0x0F                    ; 取低4位
    
    add al, '0'
    cmp al, '9'
    jle .print_hex_digit
    
    add al, 7                       ; 'A' - '9' - 1 = 7
    
.print_hex_digit:
    push eax
    call put_char
    add esp, 4
    
    loop .print_hex_loop
    
    popad
    ret

; ============================================
; 辅助函数: 打印十进制数
; 输入: eax = 要打印的数字
; ============================================
global print_dec
print_dec:
    pushad
    
    mov ecx, 0                      ; 计数器
    mov ebx, 10                     ; 除数
    
    test eax, eax
    jnz .print_dec_positive
    
    ; 处理0的情况
    push '0'
    call put_char
    add esp, 4
    jmp .print_dec_done
    
.print_dec_positive:
    ; 如果是负数
    test eax, 0x80000000
    jz .print_dec_convert
    
    push eax
    push '-'
    call put_char
    add esp, 4
    pop eax
    neg eax                         ; 转为正数
    
.print_dec_convert:
    xor edx, edx
    div ebx                         ; eax = 商, edx = 余数
    push edx                        ; 保存余数
    inc ecx
    test eax, eax
    jnz .print_dec_convert
    
.print_dec_output:
    pop edx
    add dl, '0'
    push edx
    call put_char
    add esp, 4
    loop .print_dec_output
    
.print_dec_done:
    popad
    ret