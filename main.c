#include <efi.h>
#include <efilib.h>


EFI_DRIVER_BINDING_PROTOCOL gDriverBinding = {
    Supported,
    Start,
    Stop,
    0x10,
    NULL,
    NULL
};

EFI_STATUS
EFIAPI
Supported (
    IN EFI_DRIVER_BINDING_PROTOCOL *This,
    IN EFI_HANDLE                  ControllerHandle,
    IN EFI_DEVICE_PATH_PROTOCOL    *RemainingDevicePath
    )
{
    // Check if the driver supports the device
    return EFI_SUCCESS;
}

EFI_STATUS
EFIAPI
Start (
    IN EFI_DRIVER_BINDING_PROTOCOL *This,
    IN EFI_HANDLE                  ControllerHandle,
    IN EFI_DEVICE_PATH_PROTOCOL    *RemainingDevicePath
    )
{
    // Start the driver
    return EFI_SUCCESS;
}

EFI_STATUS
EFIAPI
Stop (
    IN EFI_DRIVER_BINDING_PROTOCOL *This,
    IN EFI_HANDLE                  ControllerHandle,
    IN UINTN                       NumberOfChildren,
    IN EFI_HANDLE                  *ChildHandleBuffer
    )
{
    // Stop the driver
    return EFI_SUCCESS;
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

EFI_STATUS EFIAPI efi_main(EFI_HANDLE ImageHandle, EFI_SYSTEM_TABLE *SystemTable)
{
    EFI_SIMPLE_FILE_SYSTEM_PROTOCOL *FileSystem;
    EFI_STATUS Status;
    UINTN HandleCount;
    EFI_HANDLE *HandleBuffer;

    // Initialize the library
    InitializeLib(ImageHandle, SystemTable);

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
    EFI_FILE_PROTOCOL *File;
    Status = uefi_call_wrapper(Root->Open, 5, Root, &File, L"\\hello.txt", EFI_FILE_MODE_CREATE | EFI_FILE_MODE_READ | EFI_FILE_MODE_WRITE, 0);
    if (EFI_ERROR(Status)) {
        // Handle error
        Print(L"Error opening the file! %d\n", Status);
        return Status;
    }

    // write
    CHAR8 *message = "Hello, World!\n";
    const UINTN messageSize = AsciiStrLen(message) + 1; // one extra for the null terminator
    Print(L"Message: %a\n", message);
    Print(L"Message size: %d bytes.\n", messageSize);
    Status = uefi_call_wrapper(File->Write, 3, File, &messageSize, message);
    Print(L"Wrote %d bytes.\n", messageSize);
    if (EFI_ERROR(Status)) {
        Print(L"Error writing to the file!\n");
        return Status;
    }

    // write again
    CHAR8* message2 = "Goodbye, World!\n";
    const UINTN messageSize2 = AsciiStrLen(message2) + 1;
    Print(L"Message2: %a\n", message2);
    Print(L"Message size: %d bytes.\n", messageSize2);
    Print(L"Starting from %d bytes.\n", messageSize);
    Status = uefi_call_wrapper(File->Write, 3, File, &messageSize2, message2);
    Print(L"Wrote %d bytes.\n", messageSize2);
    if (EFI_ERROR(Status)) {
        Print(L"Error writing to the file!\n");
    }

    // close file
    Status = uefi_call_wrapper(File->Flush, 1, File);
    if (EFI_ERROR(Status)) {
        Print(L"Error flushing the file!\n");
    }
    uefi_call_wrapper(File->Close, 1, File);


    return EFI_SUCCESS;
}


    