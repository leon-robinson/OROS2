#pragma once

#include <types.h>

typedef struct IDT_ENTRY {
	uint16 base_low;
	uint16 selector;
	uint8 reserved0;
	uint8 type_attr;
	uint16 base_mid;
	uint32 base_high;
	uint32 reserved1;
} PACKED IDT_ENTRY;

typedef struct IDT_DESCRIPTOR {
	uint16 limit;
	uint64 base;
} PACKED IDT_DESCRIPTOR;

extern IDT_DESCRIPTOR idtr;

void idt_init();
