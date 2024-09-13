#include <efi.h>
#include <efilib.h>

EFI_STATUS
EFIAPI
efi_main(EFI_HANDLE imageHandle, EFI_SYSTEM_TABLE *systemTable)
{
    InitializeLib(imageHandle, systemTable);

    Print(L"Hello, World!");

    // Wait for a key press
    EFI_INPUT_KEY Key;
    uefi_call_wrapper(systemTable->ConIn->Reset, 2, systemTable->ConIn, FALSE);
    while (uefi_call_wrapper(systemTable->ConIn->ReadKeyStroke, 2, systemTable->ConIn, &Key) == EFI_NOT_READY);
    return EFI_SUCCESS;
}