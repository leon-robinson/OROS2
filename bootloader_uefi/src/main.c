#include <gnu-efi/efi.h>

EFI_STATUS efi_main(EFI_HANDLE ImageHandle, EFI_SYSTEM_TABLE *SystemTable) {
	SystemTable->ConOut->OutputString(SystemTable->ConOut, L"Hello World!");
	for (; ; );
	return EFI_SUCCESS;
}