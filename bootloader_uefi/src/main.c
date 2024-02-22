#include <gnu-efi/efi.h>

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
#endif

__attribute__((no_return)) void error(CHAR16 *str, EFI_SYSTEM_TABLE *SystemTable) {
	SystemTable->ConOut->OutputString(SystemTable->ConOut, L"ERROR: ");
	SystemTable->ConOut->OutputString(SystemTable->ConOut, str);
	for (; ; );
}

EFI_STATUS efi_main(EFI_HANDLE ImageHandle, EFI_SYSTEM_TABLE *SystemTable) {
	SystemTable->ConOut->ClearScreen(SystemTable->ConOut);

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
			&memoryMapDescriptorVersion) != EFI_SUCCESS) {
		error(L"Final GetMemoryMap failed.", SystemTable);
	}

	// Printing before ExitBootServices makes it fail?
	if (SystemTable->BootServices->ExitBootServices(ImageHandle, memoryMapKey) != EFI_SUCCESS)
		error(L"ExitBootServices failed.", SystemTable);

	*(unsigned char *)0x150000 = 0x69;

	for (; ; );
	return EFI_SUCCESS;
}