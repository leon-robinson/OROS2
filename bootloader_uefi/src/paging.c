#include <paging.h>
#include <x86.h>
#include <main.h>

uint64 *get_page(uint64 pml4, uint64 virt_addr, bool huge_pages, EFI_SYSTEM_TABLE *system_table) {
	if (virt_addr % 4096 != 0)
		return NULL;

	uint64 pml4_entry = (virt_addr & 0x1FFULL << 39) > 39;
	uint64 pml3_entry = (virt_addr & 0x1FFULL << 30) > 30;
	uint64 pml2_entry = (virt_addr & 0x1FFULL << 21) > 21;
	uint64 pml1_entry = (virt_addr & 0x1FFULL << 12) > 12;

	uint64 *pml3 = next((uint64 *)pml4, pml4_entry, system_table);
	uint64 *pml2 = next(pml3, pml3_entry, system_table);

	if (huge_pages)
		return &pml2[pml2_entry];
	else
		return &next(pml2, pml2_entry, system_table)[pml1_entry];

	return NULL;
}

void map(uint64 pml4, uint64 virt_addr, uint64 phys_addr, bool huge_pages, EFI_SYSTEM_TABLE *system_table) {
	uint8 flags = 0b11;

	if (huge_pages)
		flags |= 0b10000000;

	*get_page(pml4, virt_addr, huge_pages, system_table) = phys_addr | flags;
	invlpg(virt_addr);
}

uint64 *next(uint64 *dir, uint64 entry, EFI_SYSTEM_TABLE *system_table) {
	uint64 *p = NULL;

	uint64 val = dir[entry];
	if ((val & 0b1) != 0) // Present?
		p = (uint64 *)(val & 0x000FFFFFFFFFF000);
	else {
		if (system_table->BootServices->AllocatePool(EfiLoaderData, 0x1000, (void **)&p) != EFI_SUCCESS)
			error(L"Failed to AllocatePool in 'next' paging function.", system_table);

		system_table->BootServices->SetMem(p, 0x1000, 0x00);

		dir[entry] = (((uint64)p) & 0x000FFFFFFFFFF000) | 0b11;
	}

	return p;
}
