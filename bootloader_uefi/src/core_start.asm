BASE_ADDR EQU 0x50000
CORE_MAIN EQU (BASE_ADDR + 0x00)
STACKS EQU (BASE_ADDR + 0x08)
SHARED_GDT EQU (BASE_ADDR + 0x16)
SHARED_IDT EQU (BASE_ADDR + 0x24)
SHARED_PAGE_TABLE EQU (BASE_ADDR + 0x1000)
TRAMPOLINE EQU (BASE_ADDR + 0x10000)

BITS 16
ORG TRAMPOLINE

entry:
	cli

	lidt [idt]

	cld
	xor ax, ax
	mov ds, ax
	mov es, ax
	mov ss, ax
	mov eax, cr4
	or eax, 0x20
	mov cr4, eax

	mov ecx, SHARED_PAGE_TABLE
	mov cr3, ecx

	mov ecx, 0xC0000080
	rdmsr
	or eax, 1 << 8
	or eax, 1 << 11
	wrmsr

	mov eax, cr0
	or eax, 0x80000001
	mov cr0, eax

	lgdt [cs:gdt.ptr]
	jmp dword 0x08:entry64

	cli
	hlt

BITS 64

entry64:
	mov ax, 0x10
	mov ds, ax
	mov es, ax
	mov fs, ax
	mov gs, ax
	mov ss, ax

	mov rbx, STACKS
	mov rsp, [rbx]
	mov rbp, rsp

	mov rax, SHARED_GDT
	mov rax, [rax]
	lgdt [rax]

	mov rax, SHARED_IDT
	mov rax, [rax]
	lidt [rax]

	sti

	mov rbx, CORE_MAIN
	mov rbx, [rbx]
	call rbx

	cli
	hlt
	jmp $

gdt:
	dq 0x0000000000000000
	dq 0x00209A0000000000
	dq 0x0000920000000000
ALIGN 4
	dw 0
.ptr:
	dw $-gdt-1
	dd gdt

ALIGN 4
idt:
	dw 0
	dd 0

times 512-($-$$) db 0