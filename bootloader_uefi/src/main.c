#include <types.h>
#include <gnu-efi/efi.h>
#include <acpi.h>
#include <paging.h>
#include <x86.h>

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

EFI_FILE_HANDLE GetVolume(EFI_HANDLE ImageHandle, EFI_SYSTEM_TABLE *SystemTable) {
	EFI_LOADED_IMAGE *loaded_image = NULL;
	EFI_GUID lip_guid = EFI_LOADED_IMAGE_PROTOCOL_GUID;
	EFI_FILE_IO_INTERFACE *io_volume;
	EFI_GUID fs_guid = EFI_SIMPLE_FILE_SYSTEM_PROTOCOL_GUID;
	EFI_FILE_HANDLE volume;
	EFI_STATUS status = EFI_SUCCESS;

	if (SystemTable->BootServices->HandleProtocol(ImageHandle, &lip_guid, (VOID **)&loaded_image) != EFI_SUCCESS)
		error(L"Failed to get loaded image protocol interface.", SystemTable);
	if (SystemTable->BootServices->HandleProtocol(loaded_image->DeviceHandle, &fs_guid, (VOID *)&io_volume) != EFI_SUCCESS)
		error(L"Failed to get the volume handle.", SystemTable);
	if (io_volume->OpenVolume(io_volume, &volume) != EFI_SUCCESS)
		error(L"Failed to open the volume.", SystemTable);

	return volume;
}

VOID *AllocatePool(UINTN Size, EFI_SYSTEM_TABLE *SystemTable) {
	EFI_STATUS status;
	VOID *p;

	status = SystemTable->BootServices->AllocatePool(EfiLoaderData, Size, &p);
	if (EFI_ERROR(status)) {
		//DEBUG((D_ERROR, "AllocatePool: out of pool  %x\n", Status));
		p = NULL;
	}
	return p;
}

BOOLEAN GrowBuffer(EFI_STATUS *Status, VOID **Buffer, UINTN BufferSize, EFI_SYSTEM_TABLE *SystemTable) {
	BOOLEAN         TryAgain;

	if (!*Buffer && BufferSize) {
		*Status = EFI_BUFFER_TOO_SMALL;
	}
		
	TryAgain = FALSE;
	if (*Status == EFI_BUFFER_TOO_SMALL) {
		if (*Buffer) {
			SystemTable->BootServices->FreePool (*Buffer);
		}

		*Buffer = AllocatePool(BufferSize, SystemTable);

		if (*Buffer) {
			TryAgain = TRUE;
		} else {    
			*Status = EFI_OUT_OF_RESOURCES;
		} 
	}

	if (!TryAgain && EFI_ERROR(*Status) && *Buffer) {
		SystemTable->BootServices->FreePool (*Buffer);
		*Buffer = NULL;
	}

	return TryAgain;
}

EFI_FILE_INFO *LibFileInfo(EFI_FILE_HANDLE FHand, EFI_SYSTEM_TABLE *SystemTable) {
	EFI_STATUS Status = EFI_SUCCESS;
	EFI_FILE_INFO *Buffer = NULL;
	static UINTN BufferSize = SIZE_OF_EFI_FILE_INFO + 200;
	static EFI_GUID GenericFileInfo = EFI_FILE_INFO_ID;

	while (GrowBuffer (&Status, (VOID **) &Buffer, BufferSize, SystemTable)) {
	Status = FHand->GetInfo(
			FHand,
			&GenericFileInfo,
			&BufferSize,
			Buffer
		);
	}

	return Buffer;
}

UINT64 FileSize(EFI_FILE_HANDLE FileHandle, EFI_SYSTEM_TABLE *SystemTable) {
	EFI_FILE_INFO *file_info = LibFileInfo(FileHandle, SystemTable);
	UINT64 ret = file_info->FileSize;
	SystemTable->BootServices->FreePool(file_info);
	return ret;
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

	EFI_FILE_HANDLE volume = GetVolume(ImageHandle, SystemTable);
	EFI_FILE_HANDLE file_handle;
	CHAR16 *file_name = L"core_start.bin";

	volume->Open(volume, &file_handle, file_name, EFI_FILE_MODE_READ, EFI_FILE_READ_ONLY | EFI_FILE_HIDDEN | EFI_FILE_SYSTEM);
	if (file_handle == NULL)
		error(L"Failed to open core_start.bin", SystemTable);

	EFI_GUID file_info_guid = EFI_FILE_INFO_ID;
	UINT64 read_size = FileSize(file_handle, SystemTable);
	UINT8 *read_buffer = (UINT8 *)0x60000; // Trampoline location.

	if (file_handle->Read(file_handle, &read_size, read_buffer) != EFI_SUCCESS)
		error(L"Failed to read from core_start.bin", SystemTable);

	if (file_handle->Close(file_handle) != EFI_SUCCESS)
		error(L"Failed to close core_start.bin", SystemTable);

	uint64 *pml4 = (uint64 *)0x51000;
	SystemTable->BootServices->SetMem(pml4, 0x1000, 0x00);

	uint64 i = 0;
	for (i = 0x1000; i < 1024ULL * 1024ULL * 1024ULL * 2UL; i += 0x1000)
		map((uint64)pml4, i, i, false, SystemTable);

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

	// --- POST EXIT BOOT SERVICES.
	set_cr3((uint64)pml4);

	for (; ; );
	return EFI_SUCCESS;
}