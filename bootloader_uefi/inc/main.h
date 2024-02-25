#pragma once

#include <gnu-efi/efi.h>
#include <types.h>

NO_RETURN void error(CHAR16 *str, EFI_SYSTEM_TABLE *SystemTable);
