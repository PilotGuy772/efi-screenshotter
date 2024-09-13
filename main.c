#include <efi.h>
#include <efilib.h>

EFI_STATUS
EFIAPI
efi_main(EFI_HANDLE imageHandle, EFI_SYSTEM_TABLE *systemTable)
{
    InitializeLib(imageHandle, systemTable);

    Print(L"Hello, World!");

    // Wait for a key press
    EFI_INPUT_KEY key;
    uefi_call_wrapper(systemTable->ConIn->Reset, 2, systemTable->ConIn, FALSE);

    
    while (1) {
        EFI_STATUS status = uefi_call_wrapper(systemTable->ConIn->ReadKeyStroke, 2, systemTable->ConIn, &key);
        if (status == EFI_SUCCESS) {
            Print(L"Key pressed: ScanCode=%d, UnicodeChar=%c\n", key.ScanCode, key.UnicodeChar);
        }
    }

    return EFI_SUCCESS;
}