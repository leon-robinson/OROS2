#pragma once

#include <types.h>

ALWAYS_INLINE inline void set_cr3(uint64 pml4) {
	__asm__ volatile ("mov %0, %%cr3" : : "r" (pml4) : "memory");
}

ALWAYS_INLINE inline void invlpg(uint64 virt_addr) {
	__asm__ volatile ("invlpg (%0)" : : "r" (virt_addr) : "memory");
}
