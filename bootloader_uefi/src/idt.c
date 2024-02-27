#include <idt.h>

static IDT_ENTRY idt[256] = {};

static void idt_set(IDT_ENTRY *idt, int i, uintptr base, uint16 selector, uint8 type_attr) {
	idt[i].base_low = (base & 0xFFFF);
	idt[i].base_mid = (base >> 16) & 0xFFFF;
	idt[i].base_high = (base >> 32) & 0xFFFFFFFF;
	idt[i].selector = selector;
	idt[i].type_attr = type_attr;
	idt[i].reserved0 = 0;
	idt[i].reserved1 = 0;
}

void idt_init() {
	
}
