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
#pragma alloc_text (PAGE, FuserDispatchCreate)
#endif


NTSTATUS
FuserFreeCCB(
  __in PFuserCCB ccb
  )
{
	PFuserFCB fcb;

	ASSERT(ccb != NULL);
	
	fcb = ccb->Fcb;

	KeEnterCriticalRegion();
	ExAcquireResourceExclusiveLite(&fcb->Resource, TRUE);

	RemoveEntryList(&ccb->NextCCB);

	ExReleaseResourceLite(&fcb->Resource);
	KeLeaveCriticalRegion();

	ExDeleteResourceLite(&ccb->Resource);

	if (ccb->SearchPattern) {
		ExFreePool(ccb->SearchPattern);
	}

	ExFreePool(ccb);
	InterlockedIncrement(&fcb->Vcb->CcbFreed);

	return STATUS_SUCCESS;
}


// We must NOT call without VCB lock
PFuserFCB
FuserAllocateFCB(
	__in PFuserVCB Vcb
	)
{
	PFuserFCB fcb = ExAllocatePool(sizeof(FuserFCB));
	if (fcb == NULL) {
		return NULL;
	}

	ASSERT(fcb != NULL);
	ASSERT(Vcb != NULL);

	RtlZeroMemory(fcb, sizeof(FuserFCB));

	fcb->Identifier.Type = FCB;
	fcb->Identifier.Size = sizeof(FuserFCB);

	fcb->Vcb = Vcb;

	ExInitializeResourceLite(&fcb->MainResource);
	ExInitializeResourceLite(&fcb->PagingIoResource);

	ExInitializeFastMutex(&fcb->AdvancedFCBHeaderMutex);

	
#if _WIN32_WINNT >= 0x0501 
	FsRtlSetupAdvancedHeader(&fcb->AdvancedFCBHeader, &fcb->AdvancedFCBHeaderMutex);
#else
	if (FuserFsRtlTeardownPerStreamContexts) {
		FsRtlSetupAdvancedHeader(&fcb->AdvancedFCBHeader, &fcb->AdvancedFCBHeaderMutex);
	}	
#endif

	fcb->AdvancedFCBHeader.ValidDataLength.LowPart = 0xffffffff;
	fcb->AdvancedFCBHeader.ValidDataLength.HighPart = 0x7fffffff;

	fcb->AdvancedFCBHeader.Resource = &fcb->MainResource;
	fcb->AdvancedFCBHeader.PagingIoResource = &fcb->PagingIoResource;

	fcb->AdvancedFCBHeader.AllocationSize.QuadPart = 4096;
	fcb->AdvancedFCBHeader.FileSize.QuadPart = 4096;

	fcb->AdvancedFCBHeader.IsFastIoPossible = FastIoIsNotPossible;

	ExInitializeResourceLite(&fcb->Resource);

	InitializeListHead(&fcb->NextCCB);
	InsertTailList(&Vcb->NextFCB, &fcb->NextFCB); // Critical region, change list

	InterlockedIncrement(&Vcb->FcbAllocated);

	return fcb;
}


NTSTATUS
FuserFreeFCB(
  __in PFuserFCB Fcb
  )
{
	PFuserVCB vcb;

	ASSERT(Fcb != NULL);

	vcb = Fcb->Vcb;

	KeEnterCriticalRegion();
	ExAcquireResourceExclusiveLite(&vcb->Resource, TRUE);
	ExAcquireResourceExclusiveLite(&Fcb->Resource, TRUE);

	Fcb->FileCount--;

	if (Fcb->FileCount == 0) {

		RemoveEntryList(&Fcb->NextFCB);

		FDbgPrint("  Free FCB:%X\n", Fcb);
		ExFreePool(Fcb->FileName.Buffer);

#if _WIN32_WINNT >= 0x0501
		FsRtlTeardownPerStreamContexts(&Fcb->AdvancedFCBHeader);
#else
		if (FuserFsRtlTeardownPerStreamContexts) {
			FuserFsRtlTeardownPerStreamContexts(&Fcb->AdvancedFCBHeader);
		}
#endif
		ExReleaseResourceLite(&Fcb->Resource);

		ExDeleteResourceLite(&Fcb->Resource);
		ExDeleteResourceLite(&Fcb->MainResource);
		ExDeleteResourceLite(&Fcb->PagingIoResource);

		InterlockedIncrement(&vcb->FcbFreed);
		ExFreePool(Fcb);

	} else {
		ExReleaseResourceLite(&Fcb->Resource);
	}

	ExReleaseResourceLite(&vcb->Resource);
	KeLeaveCriticalRegion();

	return STATUS_SUCCESS;
}


PFuserFCB
FuserGetFCB(
	__in PFuserVCB	Vcb,
	__in PWCHAR		FileName,
	__in ULONG		FileNameLength)
{
	PLIST_ENTRY		thisEntry, nextEntry, listHead;
	PFuserFCB		fcb = NULL;
	ULONG			pos;

	KeEnterCriticalRegion();
	ExAcquireResourceExclusiveLite(&Vcb->Resource, TRUE);

	// search the FCB which is already allocated
	// (being used now)
	listHead = &Vcb->NextFCB;
    for (thisEntry = listHead->Flink;
			thisEntry != listHead;
			thisEntry = nextEntry) {

		nextEntry = thisEntry->Flink;

        fcb = CONTAINING_RECORD(thisEntry, FuserFCB, NextFCB);

		if (fcb->FileName.Length == FileNameLength) {
			// FileNameLength in bytes
			for (pos = 0; pos < FileNameLength/sizeof(WCHAR); ++pos) {
				if (fcb->FileName.Buffer[pos] != FileName[pos])
					break;
			}
			// we have the FCB which is already allocated and used
			if (pos == FileNameLength/sizeof(WCHAR))
				break;
		}

		fcb = NULL;
	}

	// we don't have FCB
	if (fcb == NULL) {
		FDbgPrint("  Allocate FCB\n");

		fcb = FuserAllocateFCB(Vcb);
		// no memory?
		if (fcb == NULL) {
			ExFreePool(FileName);
			ExReleaseResourceLite(&Vcb->Resource);
			KeLeaveCriticalRegion();
			return NULL;
		}

		ASSERT(fcb != NULL);

		fcb->FileName.Buffer = FileName;
		fcb->FileName.Length = (USHORT)FileNameLength;
		fcb->FileName.MaximumLength = (USHORT)FileNameLength;

	// we already have FCB
	} else {
		// FileName (argument) is never used and must be freed
		ExFreePool(FileName);
	}

	ExReleaseResourceLite(&Vcb->Resource);
	KeLeaveCriticalRegion();

	InterlockedIncrement(&fcb->FileCount);	
	return fcb;
}


// Take care of Lock itself
PFuserCCB
FuserAllocateCCB(
	__in PFuserDCB	Dcb,
	__in PFuserFCB	Fcb
	)
{
	PFuserCCB ccb = ExAllocatePool(sizeof(FuserCCB));

	if (ccb == NULL)
		return NULL;

	ASSERT(ccb != NULL);
	ASSERT(Fcb != NULL);

	RtlZeroMemory(ccb, sizeof(FuserCCB));

	ccb->Identifier.Type = CCB;
	ccb->Identifier.Size = sizeof(FuserCCB);

	ccb->Fcb = Fcb;

	ExInitializeResourceLite(&ccb->Resource);

	InitializeListHead(&ccb->NextCCB);

	KeEnterCriticalRegion();
	ExAcquireResourceExclusiveLite(&Fcb->Resource, TRUE);

	InsertTailList(&Fcb->NextCCB, &ccb->NextCCB);

	ExReleaseResourceLite(&Fcb->Resource);
	KeLeaveCriticalRegion();

	ccb->MountId = Dcb->MountId;

	InterlockedIncrement(&Fcb->Vcb->CcbAllocated);
	return ccb;
}





LONG
FuserUnicodeStringChar(
	__in PUNICODE_STRING UnicodeString,
	__in WCHAR	Char)
{
	ULONG i = 0;
	for (; i < UnicodeString->Length/sizeof(WCHAR); ++i) {
		if (UnicodeString->Buffer[i] == Char) {
			return i;
		}
	}
	return -1;
}


VOID
SetFileObjectForVCB(
	__in PFILE_OBJECT	FileObject,
	__in PFuserVCB		Vcb)
{
	FileObject->SectionObjectPointer = &Vcb->SectionObjectPointers;
	FileObject->FsContext = &Vcb->VolumeFileHeader;
}


NTSTATUS
FuserDispatchCreate(
	__in PDEVICE_OBJECT DeviceObject,
	__in PIRP Irp
	)

/*++

Routine Description:

	This device control dispatcher handles create & close IRPs.

Arguments:

	DeviceObject - Context for the activity.
	Irp 		 - The device control argument block.

Return Value:

	NTSTATUS

--*/
{

	PFuserVCB			vcb;
	PFuserDCB			dcb;

	PIO_STACK_LOCATION	irpSp;
	NTSTATUS			status = STATUS_INVALID_PARAMETER;
	PFILE_OBJECT		fileObject;
	ULONG				info = 0;
	PEPROCESS			process;
	PUNICODE_STRING		processImageName;
	PEVENT_CONTEXT		eventContext;
	PFILE_OBJECT		relatedFileObject;
	ULONG				fileNameLength = 0;
	ULONG				eventLength;
	PFuserFCB			fcb;
	PFuserCCB			ccb;
	PWCHAR				fileName;
	BOOLEAN				needBackSlashAfterRelatedFile = FALSE;
	HANDLE				accessTokenHandle;
	PAGED_CODE();
	
	__try {
		FsRtlEnterFileSystem();

		FDbgPrint("==> FuserCreate\n");

		irpSp = IoGetCurrentIrpStackLocation(Irp);
		fileObject = irpSp->FileObject;
		relatedFileObject = fileObject->RelatedFileObject;

		if (fileObject == NULL) {
			FDbgPrint("  fileObject == NULL\n");
			status = STATUS_INVALID_PARAMETER;
			__leave;
		}

		FDbgPrint("  ProcessId %lu\n", IoGetRequestorProcessId(Irp));
		FDbgPrint("  FileName:%wZ\n", &fileObject->FileName);

		vcb = DeviceObject->DeviceExtension;
		PrintIdType(vcb);
		if (GetIdentifierType(vcb) != VCB) {
			status = STATUS_SUCCESS;
			__leave;
		}
		dcb = vcb->Dcb;

		FDbgPrint("  IrpSp->Flags = %d\n", irpSp->Flags);
		if (irpSp->Flags & SL_CASE_SENSITIVE) {
			FDbgPrint("  IrpSp->Flags SL_CASE_SENSITIVE\n");
		}
		if (irpSp->Flags & SL_FORCE_ACCESS_CHECK) {
			FDbgPrint("  IrpSp->Flags SL_FORCE_ACCESS_CHECK\n");
		}
		if (irpSp->Flags & SL_OPEN_PAGING_FILE) {
			FDbgPrint("  IrpSp->Flags SL_OPEN_PAGING_FILE\n");
		}
		if (irpSp->Flags & SL_OPEN_TARGET_DIRECTORY) {
			FDbgPrint("  IrpSp->Flags SL_OPEN_TARGET_DIRECTORY\n");
		}

	   if ((fileObject->FileName.Length > sizeof(WCHAR)) &&
			(fileObject->FileName.Buffer[1] == L'\\') &&
			(fileObject->FileName.Buffer[0] == L'\\')) {

			fileObject->FileName.Length -= sizeof(WCHAR);

			RtlMoveMemory(&fileObject->FileName.Buffer[0],
						&fileObject->FileName.Buffer[1],
						fileObject->FileName.Length);
	   }

		if (relatedFileObject != NULL) {
			fileObject->Vpb = relatedFileObject->Vpb;
		} else {
			fileObject->Vpb = dcb->DeviceObject->Vpb;
		}

		if ((relatedFileObject == NULL || relatedFileObject->FileName.Length == 0) &&
			fileObject->FileName.Length == 0) {

			FDbgPrint("   request for FS device\n");

			if (irpSp->Parameters.Create.Options & FILE_DIRECTORY_FILE) {
				status = STATUS_NOT_A_DIRECTORY;
			} else {
				SetFileObjectForVCB(fileObject, vcb);
				info = FILE_OPENED;
				status = STATUS_SUCCESS;
			}
			__leave;
		}

	   if (fileObject->FileName.Length > sizeof(WCHAR) &&
		   fileObject->FileName.Buffer[fileObject->FileName.Length/sizeof(WCHAR)-1] == L'\\') {
			fileObject->FileName.Length -= sizeof(WCHAR);
	   }

   		fileNameLength = fileObject->FileName.Length;
		if (relatedFileObject) {
			fileNameLength += relatedFileObject->FileName.Length;

			if (fileObject->FileName.Length > 0 &&
				fileObject->FileName.Buffer[0] == '\\') {
				FDbgPrint("  when RelatedFileObject is specified, the file name should be relative path\n");
				status = STATUS_OBJECT_NAME_INVALID;
				__leave;
			}
			if (relatedFileObject->FileName.Length > 0 &&
				relatedFileObject->FileName.Buffer[relatedFileObject->FileName.Length/sizeof(WCHAR)-1] != '\\') {
				needBackSlashAfterRelatedFile = TRUE;
				fileNameLength += sizeof(WCHAR);
			}
		}

		// don't open file like stream
		if (!dcb->UseAltStream &&
			FuserUnicodeStringChar(&fileObject->FileName, L':') != -1) {
			FDbgPrint("    alternate stream\n");
			status = STATUS_INVALID_PARAMETER;
			info = 0;
			__leave;
		}
			
		// this memory is freed by FuserGetFCB if needed
		// "+ sizeof(WCHAR)" is for the last NULL character
		fileName = ExAllocatePool(fileNameLength + sizeof(WCHAR));
		if (fileName == NULL) {
			status = STATUS_INSUFFICIENT_RESOURCES;
			__leave;
		}

		RtlZeroMemory(fileName, fileNameLength + sizeof(WCHAR));

		if (relatedFileObject != NULL) {
			FDbgPrint("  RelatedFileName:%wZ\n", &relatedFileObject->FileName);

			// copy the file name of related file object
			RtlCopyMemory(fileName,
							relatedFileObject->FileName.Buffer,
							relatedFileObject->FileName.Length);

			if (needBackSlashAfterRelatedFile) {
				((PWCHAR)fileName)[relatedFileObject->FileName.Length/sizeof(WCHAR)] = '\\';
			}
			// copy the file name of fileObject
			RtlCopyMemory((PCHAR)fileName +
							relatedFileObject->FileName.Length +
							(needBackSlashAfterRelatedFile? sizeof(WCHAR) : 0),
							fileObject->FileName.Buffer,
							fileObject->FileName.Length);

		} else {
			// if related file object is not specifed, copy the file name of file object
			RtlCopyMemory(fileName,
							fileObject->FileName.Buffer,
							fileObject->FileName.Length);
		}


		fcb = FuserGetFCB(vcb, fileName, fileNameLength);
		if (fcb == NULL) {
			status = STATUS_INSUFFICIENT_RESOURCES;
			__leave;
		}

		if (irpSp->Flags & SL_OPEN_PAGING_FILE) {
			fcb->AdvancedFCBHeader.Flags2 |= FSRTL_FLAG2_IS_PAGING_FILE;
			fcb->AdvancedFCBHeader.Flags2 &= ~FSRTL_FLAG2_SUPPORTS_FILTER_CONTEXTS;
		}

		ccb = FuserAllocateCCB(dcb, fcb);
		if (ccb == NULL) {
			FuserFreeFCB(fcb); // FileName is freed here
			status = STATUS_INSUFFICIENT_RESOURCES;
			__leave;
		}

		fileObject->FsContext = &fcb->AdvancedFCBHeader;
		fileObject->FsContext2 = ccb;
		fileObject->PrivateCacheMap = NULL;
		fileObject->SectionObjectPointer = &fcb->SectionObjectPointers;
		//fileObject->Flags |= FILE_NO_INTERMEDIATE_BUFFERING;

		eventLength = sizeof(EVENT_CONTEXT) + fcb->FileName.Length;
		eventContext = AllocateEventContext(vcb->Dcb, Irp, eventLength, ccb);

		if (eventContext == NULL) {
			status = STATUS_INSUFFICIENT_RESOURCES;
			__leave;
		}

		eventContext->Context = 0;
		eventContext->FileFlags |= fcb->Flags;

		// copy the file name
		eventContext->Create.FileNameLength = fcb->FileName.Length;
		RtlCopyMemory(eventContext->Create.FileName, fcb->FileName.Buffer, fcb->FileName.Length);

		eventContext->Create.FileAttributes = irpSp->Parameters.Create.FileAttributes;
		eventContext->Create.CreateOptions  = irpSp->Parameters.Create.Options;
		eventContext->Create.DesiredAccess  = irpSp->Parameters.Create.SecurityContext->DesiredAccess;
		eventContext->Create.ShareAccess    = irpSp->Parameters.Create.ShareAccess;

		// register this IRP to waiting IPR list
		status = FuserRegisterPendingIrp(DeviceObject, Irp, eventContext, 0);

	} __finally {

		if (status != STATUS_PENDING) {
			Irp->IoStatus.Status = status;
			Irp->IoStatus.Information = info;
			IoCompleteRequest(Irp, IO_NO_INCREMENT);
			FuserPrintNTStatus(status);
		}

		FDbgPrint("<== FuserCreate\n");
		FsRtlExitFileSystem();
	}

	return status;
}



VOID
FuserCompleteCreate(
	 __in PIRP_ENTRY			IrpEntry,
	 __in PEVENT_INFORMATION	EventInfo
	 )
{
	PIRP				irp;
	PIO_STACK_LOCATION	irpSp;
	NTSTATUS			status;
	ULONG				info;
	PFuserCCB			ccb;
	PFuserFCB			fcb;

	irp   = IrpEntry->Irp;
	irpSp = IrpEntry->IrpSp;	

	FDbgPrint("==> FuserCompleteCreate\n");

	ccb	= IrpEntry->FileObject->FsContext2;
	ASSERT(ccb != NULL);
	
	fcb = ccb->Fcb;
	ASSERT(fcb != NULL);

	FDbgPrint("  FileName:%wZ\n", &fcb->FileName);

	ccb->UserContext = EventInfo->Context;
	//FDbgPrint("   set Context %X\n", (ULONG)ccb->UserContext);

	status = EventInfo->Status;

	info = EventInfo->Create.Information;
	switch (info) {
	case FILE_OPENED:
		FDbgPrint("  FILE_OPENED\n");
		break;
	case FILE_CREATED:
		FDbgPrint("  FILE_CREATED\n");
		break;
	case FILE_OVERWRITTEN:
		FDbgPrint("  FILE_OVERWRITTEN\n");
		break;
	case FILE_DOES_NOT_EXIST:
		FDbgPrint("  FILE_DOES_NOT_EXIST\n");
		break;
	case FILE_EXISTS:
		FDbgPrint("  FILE_EXISTS\n");
		break;
	default:
		FDbgPrint("  info = %d\n", info);
		break;
	}

	ExAcquireResourceExclusiveLite(&fcb->Resource, TRUE);
	if (NT_SUCCESS(status) &&
		(irpSp->Parameters.Create.Options & FILE_DIRECTORY_FILE ||
		EventInfo->Create.Flags & FUSER_FILE_DIRECTORY)) {
		if (irpSp->Parameters.Create.Options & FILE_DIRECTORY_FILE) {
			FDbgPrint("  FILE_DIRECTORY_FILE %p\n", fcb);
		} else {
			FDbgPrint("  FUSER_FILE_DIRECTORY %p\n", fcb);
		}
		fcb->Flags |= FUSER_FILE_DIRECTORY;
	}
	ExReleaseResourceLite(&fcb->Resource);

	ExAcquireResourceExclusiveLite(&ccb->Resource, TRUE);
	if (NT_SUCCESS(status)) {
		ccb->Flags |= FUSER_FILE_OPENED;
	}
	ExReleaseResourceLite(&ccb->Resource);
	if (NT_SUCCESS(status)) {
		if (info == FILE_CREATED) {
			if (fcb->Flags & FUSER_FILE_DIRECTORY) {
				FuserNotifyReportChange(fcb, FILE_NOTIFY_CHANGE_DIR_NAME, FILE_ACTION_ADDED); // notify everywhere when changes are made to the file system.
			} else {
				FuserNotifyReportChange(fcb, FILE_NOTIFY_CHANGE_FILE_NAME, FILE_ACTION_ADDED);
			}
		}
	} else {
		FDbgPrint("   IRP_MJ_CREATE failed. Free CCB:%X\n", ccb);
		FuserFreeCCB(ccb);
		FuserFreeFCB(fcb);
	}

	irp->IoStatus.Status = status;
	irp->IoStatus.Information = info;
	IoCompleteRequest(irp, IO_NO_INCREMENT);

	FuserPrintNTStatus(status);
	FDbgPrint("<== FuserCompleteCreate\n");
}

