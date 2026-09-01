[bits 32]
section .text
global switch_to
; void switch_to(struct task_struct *cur, struct task_struct *next);
; [esp+4]=cur, [esp+8]=next  （cdecl，进入后还有返回地址）
switch_to:
    push esi
    push edi
    push ebx
    push ebp

    mov eax, [esp + 20]       ; cur
    mov [eax], esp            ; cur->self_kstack = esp

    mov eax, [esp + 24]       ; next
    mov esp, [eax]            ; esp = next->self_kstack

    pop ebp
    pop ebx
    pop edi
    pop esi
    ret
