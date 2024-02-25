#pragma once

#include <types.h>
#include <gnu-efi/efi.h>

uint64 *get_page(uint64 pml4, uint64 virt_addr, bool huge_pages, EFI_SYSTEM_TABLE *system_table);
void map(uint64 pml4, uint64 virt_addr, uint64 phys_addr, bool huge_pages, EFI_SYSTEM_TABLE *system_table);
uint64 *next(uint64 *dir, uint64 entry, EFI_SYSTEM_TABLE *system_table);
