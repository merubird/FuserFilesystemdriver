/*
  Fuser : user-mode file system library for Windows

  Copyright (C) 2011 - 2013 Christian Auer christian.auer@gmx.ch
  Copyright (C) 2007 - 2011 Hiroki Asakawa http://dokan-dev.net/en

This program is free software; you can redistribute it and/or modify it under
the terms of the GNU Lesser General Public License as published by the Free
Software Foundation; either version 3 of the License, or (at your option) any
later version.

This program is distributed in the hope that it will be useful, but WITHOUT ANY
WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
FOR A PARTICULAR PURPOSE. See the GNU General Public License for more details.

You should have received a copy of the GNU Lesser General Public License along
with this program. If not, see <http://www.gnu.org/licenses/>.
*/



#include "fuser.h"

#ifdef ALLOC_PRAGMA
#pragma alloc_text (INIT, DriverEntry)
#pragma alloc_text (PAGE, FuserUnload)
#pragma alloc_text (PAGE, FuserDispatchShutdown)
#pragma alloc_text (PAGE, FuserDispatchPnp)
#endif



UNICODE_STRING				FcbFileNameNull;
ULONG 						g_Debug = FUSER_DEBUG_DEFAULT;

NPAGED_LOOKASIDE_LIST		FuserIrpEntryLookasideList;
FAST_IO_CHECK_IF_POSSIBLE 	FuserFastIoCheckIfPossible;
FAST_IO_ACQUIRE_FILE 		FuserAcquireForCreateSection;
FAST_IO_RELEASE_FILE 		FuserReleaseForCreateSection;


#if _WIN32_WINNT < 0x0501
	PFN_FSRTLTEARDOWNPERSTREAMCONTEXTS FuserFsRtlTeardownPerStreamContexts;
#endif

// TODO: revise





BOOLEAN
FuserFastIoCheckIfPossible (
    __in PFILE_OBJECT	FileObject,
    __in PLARGE_INTEGER	FileOffset,
    __in ULONG			Length,
    __in BOOLEAN		Wait,
    __in ULONG			LockKey,
    __in BOOLEAN		CheckForReadOperation,
    __out PIO_STATUS_BLOCK	IoStatus,
    __in PDEVICE_OBJECT		DeviceObject
    )
{
	FDbgPrint("FuserFastIoCheckIfPossible\n");
	return FALSE;
}



VOID
FuserAcquireForCreateSection(
	__in PFILE_OBJECT FileObject
	)
{
	PFSRTL_ADVANCED_FCB_HEADER	header;

	header = FileObject->FsContext;
	if (header && header->Resource) {
		ExAcquireResourceExclusiveLite(header->Resource, TRUE);
	}

	FDbgPrint("FuserAcquireForCreateSection\n");
}



NTSTATUS
FuserFilterCallbackAcquireForCreateSection(
	__in PFS_FILTER_CALLBACK_DATA CallbackData,
    __out PVOID *CompletionContext
	)
{
	PFSRTL_ADVANCED_FCB_HEADER	header;
	FDbgPrint("FuserFilterCallbackAcquireForCreateSection\n");

	header = CallbackData->FileObject->FsContext;

	if (header && header->Resource) {
		ExAcquireResourceExclusiveLite(header->Resource, TRUE);
	}

	if (CallbackData->Parameters.AcquireForSectionSynchronization.SyncType
		!= SyncTypeCreateSection) {
		return STATUS_FSFILTER_OP_COMPLETED_SUCCESSFULLY;
	} else {
		return STATUS_FILE_LOCKED_WITH_WRITERS;
	}
}


VOID
FuserReleaseForCreateSection(
   __in PFILE_OBJECT FileObject
	)
{
	PFSRTL_ADVANCED_FCB_HEADER	header;

	header = FileObject->FsContext;
	if (header && header->Resource) {
		ExReleaseResourceLite(header->Resource);
	}

	FDbgPrint("FuserReleaseForCreateSection\n");
}







NTSTATUS
DriverEntry(
	__in PDRIVER_OBJECT  DriverObject,
	__in PUNICODE_STRING RegistryPath
	)
{
	PDEVICE_OBJECT		deviceObject;
	NTSTATUS			status;
	PFAST_IO_DISPATCH	fastIoDispatch;
	UNICODE_STRING		functionName;
	FS_FILTER_CALLBACKS filterCallbacks;
	PFUSER_GLOBAL		fuserGlobal = NULL;

	FDbgPrint("==> DriverEntry\n");

	status = FuserCreateGlobalDiskDevice(DriverObject, &fuserGlobal);

	if (status != STATUS_SUCCESS) {
		return status;
	}	
	
	
	// - Dispatch-Entry set:
	DriverObject->DriverUnload								= FuserUnload;
	DriverObject->MajorFunction[IRP_MJ_CREATE]				= FuserDispatchCreate;	
	DriverObject->MajorFunction[IRP_MJ_CLOSE]				= FuserDispatchClose;
	DriverObject->MajorFunction[IRP_MJ_CLEANUP] 			= FuserDispatchCleanup;
	DriverObject->MajorFunction[IRP_MJ_DEVICE_CONTROL]		= FuserDispatchDeviceControl;
	DriverObject->MajorFunction[IRP_MJ_FILE_SYSTEM_CONTROL] = FuserDispatchFileSystemControl;

	DriverObject->MajorFunction[IRP_MJ_DIRECTORY_CONTROL]   = FuserDispatchDirectoryControl;

	DriverObject->MajorFunction[IRP_MJ_QUERY_INFORMATION]   = FuserDispatchQueryInformation;
    DriverObject->MajorFunction[IRP_MJ_SET_INFORMATION]     = FuserDispatchSetInformation;

    DriverObject->MajorFunction[IRP_MJ_QUERY_VOLUME_INFORMATION]	= FuserDispatchQueryVolumeInformation;
    DriverObject->MajorFunction[IRP_MJ_SET_VOLUME_INFORMATION]		= FuserDispatchSetVolumeInformation;
	DriverObject->MajorFunction[IRP_MJ_READ]				= FuserDispatchRead;
	DriverObject->MajorFunction[IRP_MJ_WRITE]				= FuserDispatchWrite;
	DriverObject->MajorFunction[IRP_MJ_FLUSH_BUFFERS]		= FuserDispatchFlush;

	DriverObject->MajorFunction[IRP_MJ_SHUTDOWN]            = FuserDispatchShutdown;
	DriverObject->MajorFunction[IRP_MJ_PNP]					= FuserDispatchPnp;

	DriverObject->MajorFunction[IRP_MJ_LOCK_CONTROL]		= FuserDispatchLock;

	DriverObject->MajorFunction[IRP_MJ_QUERY_SECURITY]		= FuserDispatchQuerySecurity; // TODO: Check if these methods can be removed
	DriverObject->MajorFunction[IRP_MJ_SET_SECURITY]		= FuserDispatchSetSecurity;   // TODO: Check if these methods can be removed
	
	fastIoDispatch = ExAllocatePool(sizeof(FAST_IO_DISPATCH));	
	RtlZeroMemory(fastIoDispatch, sizeof(FAST_IO_DISPATCH));


	fastIoDispatch->SizeOfFastIoDispatch = sizeof(FAST_IO_DISPATCH);
    fastIoDispatch->FastIoCheckIfPossible = FuserFastIoCheckIfPossible;
    //fastIoDispatch->FastIoRead = FuserFastIoRead;
	fastIoDispatch->FastIoRead = FsRtlCopyRead;
	fastIoDispatch->FastIoWrite = FsRtlCopyWrite;
	fastIoDispatch->AcquireFileForNtCreateSection = FuserAcquireForCreateSection;
	fastIoDispatch->ReleaseFileForNtCreateSection = FuserReleaseForCreateSection;
    fastIoDispatch->MdlRead = FsRtlMdlReadDev;
    fastIoDispatch->MdlReadComplete = FsRtlMdlReadCompleteDev;
    fastIoDispatch->PrepareMdlWrite = FsRtlPrepareMdlWriteDev;
    fastIoDispatch->MdlWriteComplete = FsRtlMdlWriteCompleteDev;
	DriverObject->FastIoDispatch = fastIoDispatch;


	ExInitializeNPagedLookasideList(
		&FuserIrpEntryLookasideList, NULL, NULL, 0, sizeof(IRP_ENTRY), TAG, 0);

#if _WIN32_WINNT < 0x0501
    RtlInitUnicodeString(&functionName, L"FsRtlTeardownPerStreamContexts");
    FuserFsRtlTeardownPerStreamContexts = MmGetSystemRoutineAddress(&functionName);
#endif

    RtlZeroMemory(&filterCallbacks, sizeof(FS_FILTER_CALLBACKS));

	// only be used by filter driver?
	filterCallbacks.SizeOfFsFilterCallbacks = sizeof(FS_FILTER_CALLBACKS);
	filterCallbacks.PreAcquireForSectionSynchronization = FuserFilterCallbackAcquireForCreateSection;

	status = FsRtlRegisterFileSystemFilterCallbacks(DriverObject, &filterCallbacks);
	if (!NT_SUCCESS(status)) {
		IoDeleteDevice(fuserGlobal->DeviceObject);
		FDbgPrint("  FsRtlRegisterFileSystemFilterCallbacks returned 0x%x\n", status);
		return status;
	}


	FDbgPrint("<== DriverEntry\n");
	return( status );
}



VOID
FuserUnload(
	__in PDRIVER_OBJECT DriverObject
	)
/*++

Routine Description:

	This routine gets called to remove the driver from the system.

Arguments:

	DriverObject	- the system supplied driver object.

Return Value:

	NTSTATUS

--*/
{

	PDEVICE_OBJECT	deviceObject = DriverObject->DeviceObject;
	WCHAR			symbolicLinkBuf[] = FUSER_GLOBAL_SYMBOLIC_LINK_NAME;
	UNICODE_STRING	symbolicLinkName;

	PAGED_CODE();
	FDbgPrint("==> FuserUnload\n");

	if (GetIdentifierType(deviceObject->DeviceExtension) == DGL) {
		FDbgPrint("  Delete Global DeviceObject\n");
		RtlInitUnicodeString(&symbolicLinkName, symbolicLinkBuf);
		IoDeleteSymbolicLink(&symbolicLinkName);
		IoDeleteDevice(deviceObject);
	}

	ExDeleteNPagedLookasideList(&FuserIrpEntryLookasideList);

	FDbgPrint("<== FuserUnload\n");
	return;
}



// System shutdown or driver unloading
NTSTATUS
FuserDispatchShutdown(
	__in PDEVICE_OBJECT DeviceObject,
	__in PIRP Irp
   )
{
	PAGED_CODE();
	FDbgPrint("==> FuserShutdown\n");

	Irp->IoStatus.Status = STATUS_SUCCESS;
	Irp->IoStatus.Information = 0;
	IoCompleteRequest(Irp, IO_NO_INCREMENT);

	FDbgPrint("<== FuserShutdown\n");
	return STATUS_SUCCESS;
}




NTSTATUS
FuserDispatchPnp(
	__in PDEVICE_OBJECT DeviceObject,
	__in PIRP Irp
   )
{
	PIO_STACK_LOCATION	irpSp;
	NTSTATUS			status = STATUS_SUCCESS;

	PAGED_CODE();

	__try {
		FDbgPrint("==> FuserPnp\n");

		irpSp = IoGetCurrentIrpStackLocation(Irp);

		switch (irpSp->MinorFunction) {
		case IRP_MN_QUERY_REMOVE_DEVICE:
			FDbgPrint("  IRP_MN_QUERY_REMOVE_DEVICE\n");
			break;
		case IRP_MN_SURPRISE_REMOVAL:
			FDbgPrint("  IRP_MN_SURPRISE_REMOVAL\n");
			break;
		case IRP_MN_REMOVE_DEVICE:
			FDbgPrint("  IRP_MN_REMOVE_DEVICE\n");
			break;
		case IRP_MN_CANCEL_REMOVE_DEVICE:
			FDbgPrint("  IRP_MN_CANCEL_REMOVE_DEVICE\n");
			break;
		case IRP_MN_QUERY_DEVICE_RELATIONS:
			FDbgPrint("  IRP_MN_QUERY_DEVICE_RELATIONS\n");
			status = STATUS_INVALID_PARAMETER;
			break;
		default:
			FDbgPrint("   other minnor function %d\n", irpSp->MinorFunction);
			break;
			//IoSkipCurrentIrpStackLocation(Irp);
			//status = IoCallDriver(Vcb->TargetDeviceObject, Irp);
		}
	} __finally {
		Irp->IoStatus.Status = status;
		Irp->IoStatus.Information = 0;
		IoCompleteRequest(Irp, IO_NO_INCREMENT);

		FDbgPrint("<== FuserPnp\n");
	}

	return status;
}


#define PrintStatus(val, flag) if(val == flag) FDbgPrint("  status = " #flag "\n")

VOID
FuserPrintNTStatus(
	NTSTATUS	Status)
{
	PrintStatus(Status, STATUS_SUCCESS);
	PrintStatus(Status, STATUS_NO_MORE_FILES);
	PrintStatus(Status, STATUS_END_OF_FILE);
	PrintStatus(Status, STATUS_NO_SUCH_FILE);
	PrintStatus(Status, STATUS_NOT_IMPLEMENTED);
	PrintStatus(Status, STATUS_BUFFER_OVERFLOW);
	PrintStatus(Status, STATUS_FILE_IS_A_DIRECTORY);
	PrintStatus(Status, STATUS_SHARING_VIOLATION);
	PrintStatus(Status, STATUS_OBJECT_NAME_INVALID);
	PrintStatus(Status, STATUS_OBJECT_NAME_NOT_FOUND);
	PrintStatus(Status, STATUS_OBJECT_NAME_COLLISION);
	PrintStatus(Status, STATUS_OBJECT_PATH_INVALID);
	PrintStatus(Status, STATUS_OBJECT_PATH_NOT_FOUND);
	PrintStatus(Status, STATUS_OBJECT_PATH_SYNTAX_BAD);
	PrintStatus(Status, STATUS_ACCESS_DENIED);
	PrintStatus(Status, STATUS_ACCESS_VIOLATION);
	PrintStatus(Status, STATUS_INVALID_PARAMETER);
	PrintStatus(Status, STATUS_INVALID_USER_BUFFER);
	PrintStatus(Status, STATUS_INVALID_HANDLE);
}


VOID
PrintIdType(
	__in VOID* Id)
{
	if (Id == NULL) {
		FDbgPrint("    IdType = NULL\n");
		return;
	}
	switch (GetIdentifierType(Id)) {
	case DGL:
		FDbgPrint("    IdType = DGL\n");
		break;
	case DCB:
		FDbgPrint("   IdType = DCB\n");
		break;
	case VCB:
		FDbgPrint("   IdType = VCB\n");
		break;
	case FCB:
		FDbgPrint("   IdType = FCB\n");
		break;
	case CCB:
		FDbgPrint("   IdType = CCB\n");
		break;
	default:
		FDbgPrint("   IdType = Unknown\n");
		break;
	}
}




BOOLEAN
FuserNoOpAcquire(
    __in PVOID Fcb,
    __in BOOLEAN Wait
    )
{
    UNREFERENCED_PARAMETER( Fcb );
    UNREFERENCED_PARAMETER( Wait );

	FDbgPrint("==> FuserNoOpAcquire\n");

    ASSERT(IoGetTopLevelIrp() == NULL);

    IoSetTopLevelIrp((PIRP)FSRTL_CACHE_TOP_LEVEL_IRP);

	FDbgPrint("<== FuserNoOpAcquire\n");
    
	return TRUE;
}


VOID
FuserNoOpRelease(
    __in PVOID Fcb
    )
{
	FDbgPrint("==> FuserNoOpRelease\n");
    ASSERT(IoGetTopLevelIrp() == (PIRP)FSRTL_CACHE_TOP_LEVEL_IRP);

    IoSetTopLevelIrp( NULL );

    UNREFERENCED_PARAMETER( Fcb );
	
	FDbgPrint("<== FuserNoOpRelease\n");
    return;
}







VOID
FuserNotifyReportChange0( // TODO: Change name of this method
	__in PFuserFCB			Fcb,
	__in PUNICODE_STRING	FileName,
	__in ULONG				FilterMatch,
	__in ULONG				Action)
{
	USHORT	nameOffset;

	FDbgPrint("==> FuserNotifyReportChange %wZ\n", FileName);

	ASSERT(Fcb != NULL);
	ASSERT(FileName != NULL);

	// search the last "\"
	nameOffset = (USHORT)(FileName->Length/sizeof(WCHAR)-1);
	for(; FileName->Buffer[nameOffset] != L'\\'; --nameOffset)
		;
	nameOffset++; // the next is the begining of filename

	nameOffset *= sizeof(WCHAR); // Offset is in bytes

	FsRtlNotifyFullReportChange(
		Fcb->Vcb->NotifySync,
		&Fcb->Vcb->DirNotifyList,
		(PSTRING)FileName,
		nameOffset,
		NULL, // StreamName
		NULL, // NormalizedParentName
		FilterMatch,
		Action,
		NULL); // TargetContext

	FDbgPrint("<== FuserNotifyReportChange\n");
}




VOID
FuserNotifyReportChange(
	__in PFuserFCB	Fcb,
	__in ULONG		FilterMatch,
	__in ULONG		Action)
{
	ASSERT(Fcb != NULL);
	FuserNotifyReportChange0(Fcb, &Fcb->FileName, FilterMatch, Action);
}



BOOLEAN
FuserCheckCCB(
	__in PFuserDCB	Dcb,
	__in PFuserCCB	Ccb)
{
	ASSERT(Dcb != NULL);
	if (GetIdentifierType(Dcb) != DCB) {
		PrintIdType(Dcb);
		return FALSE;
	}

	if (Ccb == NULL) {
		PrintIdType(Dcb);
		FDbgPrint("   ccb is NULL\n");
		return FALSE;
	}

	if (Ccb->MountId != Dcb->MountId) {
		FDbgPrint("   MountId is different\n");
		return FALSE;
	}

	if (!Dcb->Mounted) {
		FDbgPrint("  Not mounted\n");
		return FALSE;
	}

	return TRUE;
}




NTSTATUS
FuserAllocateMdl(
	__in PIRP	Irp,
	__in ULONG	Length
	)
{
	if (Irp->MdlAddress == NULL) {
		Irp->MdlAddress = IoAllocateMdl(Irp->UserBuffer, Length, FALSE, FALSE, Irp);

		if (Irp->MdlAddress == NULL) {
			FDbgPrint("    IoAllocateMdl returned NULL\n");
			return STATUS_INSUFFICIENT_RESOURCES;
		}
		__try {
			MmProbeAndLockPages(Irp->MdlAddress, Irp->RequestorMode, IoWriteAccess);

		} __except (EXCEPTION_EXECUTE_HANDLER) {
			FDbgPrint("    MmProveAndLockPages error\n");
			IoFreeMdl(Irp->MdlAddress);
			Irp->MdlAddress = NULL;
			return STATUS_INSUFFICIENT_RESOURCES;
		}
	}
	return STATUS_SUCCESS;
}





VOID
FuserFreeMdl(
	__in PIRP	Irp
	)
{
	if (Irp->MdlAddress != NULL) {
		MmUnlockPages(Irp->MdlAddress);
		IoFreeMdl(Irp->MdlAddress);
		Irp->MdlAddress = NULL;
	}
}


/* TODO: unused:
BOOLEAN
FuserFastIoRead (
    __in PFILE_OBJECT	FileObject,
    __in PLARGE_INTEGER	FileOffset,
    __in ULONG			Length,
    __in BOOLEAN		Wait,
    __in ULONG			LockKey,
    __in PVOID			Buffer,
    __out PIO_STATUS_BLOCK	IoStatus,
    __in PDEVICE_OBJECT		DeviceObject
    )
{
	FDbgPrint("FuserFastIoRead\n");
	return FALSE;
}
*/

