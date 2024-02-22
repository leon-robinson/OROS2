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

INTN CompareMem(CONST VOID *MemOne, CONST VOID *MemTwo, UINTN Len) {
    UINT8 *ByteOne = (UINT8 *)MemOne;
    UINT8 *ByteTwo = (UINT8 *)MemTwo;
    UINTN i;

    for (i = 0; i < Len; i++) {
        if (ByteOne[i] != ByteTwo[i]) {
            return ByteOne[i] > ByteTwo[i] ? 1 : -1;
        }
    }

    return 0;
}

EFI_STATUS efi_main(EFI_HANDLE ImageHandle, EFI_SYSTEM_TABLE *SystemTable) {
	SystemTable->ConOut->ClearScreen(SystemTable->ConOut);

	EFI_CONFIGURATION_TABLE *config_table = SystemTable->ConfigurationTable;
	UINTN config_entries = SystemTable->NumberOfTableEntries;
	VOID *rsdp_addr = NULL;
	EFI_GUID gEfiAcpiTableGuid = ACPI_20_TABLE_GUID;

	for (UINTN i = 0; i < config_entries; i++) {
		if (CompareGuid(&config_table[i].VendorGuid, &gEfiAcpiTableGuid) == 0) {
			void *acpi_table = config_table[i].VendorTable;
			if (CompareMem(acpi_table, "RSD PTR ", 8) == 0) {
				rsdp_addr = acpi_table;
			}
		}
	}

	if (rsdp_addr == NULL)
		error(L"Couldn't find ACPI RSDP address.", SystemTable);

	ACPI_RSDP *rsdp = (ACPI_RSDP *)rsdp_addr;

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

	*(unsigned char *)0x150000 = 0x69;

	for (; ; );
	return EFI_SUCCESS;
}