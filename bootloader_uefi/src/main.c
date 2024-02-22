#include <types.h>
#include <gnu-efi/efi.h>
#include <acpi.h>

#define DEBUG
#ifdef DEBUG
VOID* memset(VOID* ptr, INT32 value, UINTN num) {
    UINT8* bytes = (UINT8*)ptr;
    for (UINTN i = 0; i < num; i++) {
        bytes[i] = (UINT8)value;
    }
    return ptr;
}

void ReverseString(CHAR16 *str, UINTN length) {
	UINTN start = 0;
	UINTN end = length - 1;
	while (start < end) {
		CHAR16 temp = str[start];
		str[start] = str[end];
		str[end] = temp;
		start++;
		end--;
	}
}

void PrintUINTN(UINTN value, EFI_SYSTEM_TABLE *SystemTable) {
	CHAR16 str[20] = {0};
	UINTN i = 0;
	if (value == 0) {
		str[i++] = L'0';
	} else {
		while (value != 0) {
			UINTN remainder = value % 10;
			str[i++] = L'0' + remainder;
			value = value / 10;
		}
	}

	str[i] = L'\0';

	ReverseString(str, i);

	str[i] = L'\0';

	SystemTable->ConOut->OutputString(SystemTable->ConOut, str);
}

void PrintUINTNHex(UINTN value, EFI_SYSTEM_TABLE *SystemTable) {
    CHAR16 str[20] = {0};
    UINTN i = 0;

    if (value == 0) {
        str[i++] = L'0';
    } else {
        while (value != 0) {
            UINTN remainder = value % 16;
            if (remainder < 10) {
                str[i++] = L'0' + remainder;
            } else {
                str[i++] = L'A' + remainder - 10;
            }
            value = value / 16;
        }
    }

    str[i] = L'\0';

    ReverseString(str, i);

    SystemTable->ConOut->OutputString(SystemTable->ConOut, str);
}

void PrintMemoryMapStarts(EFI_MEMORY_DESCRIPTOR *memoryMap, UINTN memoryMapSize, UINTN memoryMapDescriptorSize,
		EFI_SYSTEM_TABLE *SystemTable) {
	UINT8 *ptr = (UINT8 *)memoryMap;
	UINT64 end = (UINTN)ptr + memoryMapSize;
	for (; (UINTN) ptr < end; ptr += memoryMapDescriptorSize) {
		EFI_MEMORY_DESCRIPTOR *desc = (EFI_MEMORY_DESCRIPTOR *)ptr;
		PrintUINTNHex((UINTN) desc->PhysicalStart, SystemTable);
		SystemTable->ConOut->OutputString(SystemTable->ConOut, L"  ");
	}
}
#endif

NO_RETURN void error(CHAR16 *str, EFI_SYSTEM_TABLE *SystemTable) {
	SystemTable->ConOut->OutputString(SystemTable->ConOut, L"ERROR: ");
	SystemTable->ConOut->OutputString(SystemTable->ConOut, str);
	for (; ; );
}

INTN CompareGuid(EFI_GUID *Guid1, EFI_GUID *Guid2) {
	INT32 *g1, *g2, r;

	g1 = (INT32 *)Guid1;
	g2 = (INT32 *)Guid2;

	r = g1[0] - g2[0];
	r |= g1[1] - g2[1];
	r |= g1[2] - g2[2];
	r |= g1[3] - g2[3];

	return r;
}

INT16 SLP_TYPa = 0;
INT16 SLP_TYPb = 0;
INT16 SLP_EN = 0;
BOOLEAN slp_set = false;

ACPI_FADT *FADT = NULL;
ACPI_MADT *MADT = NULL;

UINT8 cpu_core_apic_ids[512];
UINT8 cpu_core_apic_id_count = 0;

void ParseFADT(ACPI_HEADER *hdr) {
	FADT = (ACPI_FADT *)hdr;

	if (*(UINT32 *)(UINTN)FADT->dsdt == DSDT_SIGNATURE) {
		UINT8 *s5_addr = (UINT8 *)(UINTN)FADT->dsdt + sizeof(ACPI_HEADER);
		INT32 dsdt_length = *((INT32 *)(UINTN)FADT->dsdt + 1) - sizeof(ACPI_HEADER);

		while (0 < dsdt_length--) {
			if (*(UINT32 *)s5_addr == S5_SIGNATURE)
				break;
			s5_addr++;
		}

		if (dsdt_length > 0) {
			if ((*(s5_addr - 1) == 0x08 || (*(s5_addr - 2) == 0x08 && *(s5_addr - 1) == '\\')) && *(s5_addr + 4) == 0x12) {
				s5_addr += 5;
				s5_addr += ((*s5_addr & 0xC0) >> 6) + 2;
				if (*s5_addr == 0x0A)
					s5_addr++;
				SLP_TYPa = (INT16)(*(s5_addr) << 10);
				s5_addr++;
				if (*s5_addr == 0x0A)
					s5_addr++;
				SLP_TYPb = (INT16)(*(s5_addr) << 10);
				SLP_EN = 1 << 13;

				slp_set = true;

				return;
			}
		}
	}
}

void ParseMADT(ACPI_HEADER *hdr) {
	MADT = (ACPI_MADT *)hdr;

	UINT8 *p = (UINT8 *)(MADT + 1);
	UINT8 *end = (UINT8 *)MADT + MADT->header.length;
	while (p < end) {
		APIC_HEADER *header = (APIC_HEADER *)p;
		APIC_TYPE type = header->type;

		switch (type) {
			case APIC_TYPE_LOCAL_APIC:
				APIC_LOCAL_APIC *lapic = (APIC_LOCAL_APIC *)p;
				// Not enabled and online capable.
				if ((lapic->flags & 1) ^ ((lapic->flags >> 1) & 1))
					cpu_core_apic_ids[cpu_core_apic_id_count++] = lapic->apic_id;
				break;
			case APIC_TYPE_IO_APIC:
				break;
			case APIC_TYPE_INTERRUPT_OVERRIDE:
				break;
		}

		p += header->length;
	}
}

void ParseHPET(ACPI_HEADER *hdr) {

}

void ParseMCFG(ACPI_HEADER *hdr) {

}

void ParseDT(ACPI_HEADER *hdr) {
	UINT32 signature = *(UINT32 *)hdr->signature;

	switch (signature) {
		case FADT_SIGNATURE:
			ParseFADT(hdr);
			break;
		case MADT_SIGNATURE:
			ParseMADT(hdr);
			break;
		case HPET_SIGNATURE:
			ParseHPET(hdr);
			break;
		case MCFG_SIGNATURE:
			ParseMCFG(hdr);
			break;
	}
}

EFI_STATUS efi_main(EFI_HANDLE ImageHandle, EFI_SYSTEM_TABLE *SystemTable) {
	SystemTable->ConOut->ClearScreen(SystemTable->ConOut);

	EFI_CONFIGURATION_TABLE *config_table = SystemTable->ConfigurationTable;
	UINTN config_entries = SystemTable->NumberOfTableEntries;
	VOID *rsdp_addr = NULL;
	EFI_GUID gEfiAcpiTableGuid = ACPI_20_TABLE_GUID;

	for (UINTN i = 0; i < config_entries; i++) {
		if (CompareGuid(&config_table[i].VendorGuid, &gEfiAcpiTableGuid) == 0) {
			UINTN *acpi_table = config_table[i].VendorTable;
			if (*acpi_table == RSD_PTR_SIGNATURE) {
				rsdp_addr = acpi_table;
			}
		}
	}

	if (rsdp_addr == NULL)
		error(L"Couldn't find ACPI RSDP address.", SystemTable);

	ACPI_RSDP *rsdp = (ACPI_RSDP *)rsdp_addr;
	ACPI_HEADER *rsdt = (ACPI_HEADER *)(UINTN)rsdp->rsdt_address;

	if (rsdt != NULL && *(UINT32 *)rsdt == RSDT_SIGNATURE) {
		UINT32 *p = (UINT32 *)(rsdt + 1);
		UINT32 *end = (UINT32 *)((UINT8 *)rsdt + rsdt->length);

		while (p < end) {
			UINT32 address = *p++;
			ParseDT((ACPI_HEADER *)(UINTN)address);
		}
	}

	if (FADT == NULL)
		error(L"Couldn't find ACPI FADT address.", SystemTable);
	if (!slp_set)
		error(L"Did not set SLP values.", SystemTable);

	PrintUINTNHex(SLP_EN, SystemTable);
	SystemTable->ConOut->OutputString(SystemTable->ConOut, L"   ");
	PrintUINTNHex(SLP_TYPa, SystemTable);
	SystemTable->ConOut->OutputString(SystemTable->ConOut, L"   ");
	PrintUINTNHex(SLP_TYPb, SystemTable);
	SystemTable->ConOut->OutputString(SystemTable->ConOut, L"\n\r");
	for (int i = 0; i < cpu_core_apic_id_count; i++) {
		SystemTable->ConOut->OutputString(SystemTable->ConOut, L"Found CPU core: ");
		PrintUINTN(cpu_core_apic_ids[i], SystemTable);
		SystemTable->ConOut->OutputString(SystemTable->ConOut, L"\n\r");
	}

	UINTN memoryMapSize = 0;
	EFI_MEMORY_DESCRIPTOR *memoryMap = NULL;
	UINTN memoryMapKey = 0;
	UINTN memoryMapDescriptorSize = 0;
	UINT32 memoryMapDescriptorVersion = 0;

 	// Fetch memoryMapSize first.
	if (SystemTable->BootServices->GetMemoryMap(&memoryMapSize, memoryMap, &memoryMapKey, &memoryMapDescriptorSize,
			&memoryMapDescriptorVersion) != EFI_BUFFER_TOO_SMALL) {
		error(L"Initial GetMemoryMap gave unexpected result.", SystemTable);
	}

	memoryMapSize += memoryMapDescriptorSize; // Maybe += memoryMapDescriptorSize * 2?

	if (SystemTable->BootServices->AllocatePool(EfiLoaderData, memoryMapSize, (VOID **)&memoryMap) != EFI_SUCCESS 
			|| memoryMap == NULL) {
		error(L"Memory map pool allocation failed.", SystemTable);
	}

	if (SystemTable->BootServices->GetMemoryMap(&memoryMapSize, memoryMap, &memoryMapKey, &memoryMapDescriptorSize,
			&memoryMapDescriptorVersion) != EFI_SUCCESS || memoryMap == NULL) {
		error(L"Final GetMemoryMap failed.", SystemTable);
	}

	// Printing before ExitBootServices makes it fail?
	if (SystemTable->BootServices->ExitBootServices(ImageHandle, memoryMapKey) != EFI_SUCCESS)
		error(L"ExitBootServices failed.", SystemTable);

	for (; ; );
	return EFI_SUCCESS;
}