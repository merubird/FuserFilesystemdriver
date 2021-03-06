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

NTSTATUS
FuserDispatchQueryInformation(
	__in PDEVICE_OBJECT DeviceObject,
	__in PIRP Irp
   )
{
	NTSTATUS				status = STATUS_NOT_IMPLEMENTED;
	PIO_STACK_LOCATION		irpSp;
	PVOID					buffer;
	ULONG					remainingLength;
	PFILE_OBJECT			fileObject;
	FILE_INFORMATION_CLASS	fileInfo;
	PFuserCCB				ccb;
	PFuserFCB				fcb;
	PFuserVCB				vcb;
	ULONG					info = 0;
	ULONG					eventLength;
	PEVENT_CONTEXT			eventContext;


	PAGED_CODE();
	__try {

		FsRtlEnterFileSystem();

		FDbgPrint("==> FuserQueryInformation\n");

		irpSp			= IoGetCurrentIrpStackLocation(Irp);
		fileObject		= irpSp->FileObject;

		FDbgPrint("  FileInfoClass %d\n", irpSp->Parameters.QueryFile.FileInformationClass);
		FDbgPrint("  ProcessId %lu\n", IoGetRequestorProcessId(Irp));

		if (fileObject == NULL) {
			FDbgPrint("  fileObject == NULL\n");
			status = STATUS_INVALID_PARAMETER;
			__leave;
		}

		FuserPrintFileName(fileObject);

		
		//if (fileObject->FsContext2 == NULL &&
		//	fileObject->FileName.Length == 0) {
		//	// volume open?
		//	status = STATUS_SUCCESS;
		//	__leave;
		//}

		vcb = DeviceObject->DeviceExtension;
		if (GetIdentifierType(vcb) != VCB ||
			!FuserCheckCCB(vcb->Dcb, fileObject->FsContext2)) {
			status = STATUS_INVALID_PARAMETER;
			__leave;
		}

		ccb	= (PFuserCCB)fileObject->FsContext2;
		ASSERT(ccb != NULL);

		fcb = ccb->Fcb;
		ASSERT(fcb != NULL);
		switch (irpSp->Parameters.QueryFile.FileInformationClass) {
		case FileBasicInformation:
			FDbgPrint("  FileBasicInformation\n");
			break;  
		case FileInternalInformation:
			FDbgPrint("  FileInternalInformation\n");
			break;
		case FileEaInformation:
			FDbgPrint("  FileEaInformation\n");
			break;          
		case FileStandardInformation:
			FDbgPrint("  FileStandardInformation\n");
			break;
		case FileAllInformation:
			FDbgPrint("  FileAllInformation\n");
			break;
		case FileAlternateNameInformation:
			FDbgPrint("  FileAlternateNameInformation\n");
			break;
		case FileAttributeTagInformation:
			FDbgPrint("  FileAttributeTagInformation\n");
			break;
		case FileCompressionInformation:
			FDbgPrint("  FileCompressionInformation\n");
			break;

		case FileNameInformation:
			{
				PFILE_NAME_INFORMATION nameInfo;

				FDbgPrint("  FileNameInformation\n");
	
				if (irpSp->Parameters.QueryFile.Length < sizeof(FILE_NAME_INFORMATION)
					+ fcb->FileName.Length) {

					status = STATUS_INSUFFICIENT_RESOURCES;
				
				} else {
					
					nameInfo = (PFILE_NAME_INFORMATION)Irp->AssociatedIrp.SystemBuffer;
					ASSERT(nameInfo != NULL);

					nameInfo->FileNameLength = fcb->FileName.Length;
					RtlCopyMemory(nameInfo->FileName, fcb->FileName.Buffer, fcb->FileName.Length);
					info = FIELD_OFFSET(FILE_NAME_INFORMATION, FileName[0]) + fcb->FileName.Length;
					status = STATUS_SUCCESS;
				}
				__leave;
			}
			break;
		case FileNetworkOpenInformation:
			FDbgPrint("  FileNetworkOpenInformation\n");
			break;

		case FilePositionInformation:
			{
				PFILE_POSITION_INFORMATION posInfo;
			
				FDbgPrint("  FilePositionInformation\n");

				if (irpSp->Parameters.QueryFile.Length < sizeof(FILE_POSITION_INFORMATION)) {
					status = STATUS_INSUFFICIENT_RESOURCES;
			
				} else {
					posInfo = (PFILE_POSITION_INFORMATION)Irp->AssociatedIrp.SystemBuffer;
					ASSERT(posInfo != NULL);

					RtlZeroMemory(posInfo, sizeof(FILE_POSITION_INFORMATION));
				
					// set the current file offset
					posInfo->CurrentByteOffset = fileObject->CurrentByteOffset;
				
					info = sizeof(FILE_POSITION_INFORMATION);
					status = STATUS_SUCCESS;
				}
				__leave;
			}
			break;
		case FileStreamInformation:
			FDbgPrint("  FileStreamInformation\n");
			break;	
		default:
			FDbgPrint("  unknown type:%d\n", irpSp->Parameters.QueryFile.FileInformationClass);
			break;
		}

		// if it is not treadted in swich case

		// calculate the length of EVENT_CONTEXT
		// sum of it's size and file name length
		eventLength = sizeof(EVENT_CONTEXT) + fcb->FileName.Length;

		eventContext = AllocateEventContext(vcb->Dcb, Irp, eventLength, ccb);
				
		if (eventContext == NULL) {
			status = STATUS_INSUFFICIENT_RESOURCES;
			__leave;
		}
			
		eventContext->Context = ccb->UserContext;
		//FDbgPrint("   get Context %X\n", (ULONG)ccb->UserContext);

		eventContext->File.FileInformationClass =
			irpSp->Parameters.QueryFile.FileInformationClass;

		// bytes length which is able to be returned
		eventContext->File.BufferLength = irpSp->Parameters.QueryFile.Length;

		// copy file name to EventContext from FCB
		eventContext->File.FileNameLength = fcb->FileName.Length;
		RtlCopyMemory(eventContext->File.FileName,
						fcb->FileName.Buffer,
						fcb->FileName.Length);

		// register this IRP to pending IPR list
		status = FuserRegisterPendingIrp(DeviceObject, Irp, eventContext, 0);

	} __finally {

		if (status != STATUS_PENDING) {
			Irp->IoStatus.Status = status;
			Irp->IoStatus.Information = info;
			IoCompleteRequest(Irp, IO_NO_INCREMENT);
			FuserPrintNTStatus(status);
		}

		FDbgPrint("<== FuserQueryInformation\n");

		FsRtlExitFileSystem();

	}
	return status;
}




VOID
FuserCompleteQueryInformation(
	__in PIRP_ENTRY		IrpEntry,
	__in PEVENT_INFORMATION EventInfo
	)
{
	PIRP				irp;
	PIO_STACK_LOCATION	irpSp;
	NTSTATUS			status   = STATUS_SUCCESS;
	ULONG				info	 = 0;
	ULONG				bufferLen= 0;
	PVOID				buffer	 = NULL;
	PFuserCCB			ccb;

	//FsRtlEnterFileSystem();

	FDbgPrint("==> FuserCompleteQueryInformation\n");

	irp = IrpEntry->Irp;
	irpSp = IrpEntry->IrpSp;

	ccb = IrpEntry->FileObject->FsContext2;

	ASSERT(ccb != NULL);

	ccb->UserContext = EventInfo->Context;
	//FDbgPrint("   set Context %X\n", (ULONG)ccb->UserContext);

	// where we shold copy FileInfo to
	buffer = irp->AssociatedIrp.SystemBuffer;

	// available buffer size
	bufferLen = irpSp->Parameters.QueryFile.Length;

	// buffer is not specifed or short of size
	if (bufferLen == 0 || buffer == NULL || bufferLen < EventInfo->BufferLength) {
		info   = 0;
		status = STATUS_INSUFFICIENT_RESOURCES;

	} else {

		//
		// we write FileInfo from user mode
		//
		ASSERT(buffer != NULL);
		
		RtlZeroMemory(buffer, bufferLen);
		RtlCopyMemory(buffer, EventInfo->Buffer, EventInfo->BufferLength);

		// written bytes
		info = EventInfo->BufferLength;
		status = EventInfo->Status;

		if (NT_SUCCESS(status)
			&& irpSp->Parameters.QueryFile.FileInformationClass == FileAllInformation) {

			PFILE_ALL_INFORMATION allInfo = (PFILE_ALL_INFORMATION)buffer;
			allInfo->PositionInformation.CurrentByteOffset = IrpEntry->FileObject->CurrentByteOffset;
		}
	}


	irp->IoStatus.Status = status;
	irp->IoStatus.Information = info;
	IoCompleteRequest(irp, IO_NO_INCREMENT);

	FuserPrintNTStatus(status);
	FDbgPrint("<== FuserCompleteQueryInformation\n");

	//FsRtlExitFileSystem();

}




NTSTATUS
FuserDispatchSetInformation(
	__in PDEVICE_OBJECT DeviceObject,
	__in PIRP Irp
   )
{
	NTSTATUS			status = STATUS_NOT_IMPLEMENTED;
	PIO_STACK_LOCATION  irpSp;
	PVOID				buffer;
	ULONG				remainingLength;
	PFILE_OBJECT		fileObject;
	FILE_INFORMATION_CLASS fileInfo;
	PFuserCCB			ccb;
	PFuserFCB			fcb;
	PFuserVCB			vcb;
	ULONG				eventLength;
	PFILE_OBJECT		targetFileObject;
	PEVENT_CONTEXT		eventContext;

	PAGED_CODE();
	__try {
		FsRtlEnterFileSystem();

		FDbgPrint("==> FuserSetInformationn\n");
		irpSp			= IoGetCurrentIrpStackLocation(Irp);
		fileObject		= irpSp->FileObject;
		
		if (fileObject == NULL) {
			FDbgPrint("  fileObject == NULL\n");
			status = STATUS_INVALID_PARAMETER;
			__leave;
		}
		
		vcb = DeviceObject->DeviceExtension;
		if (GetIdentifierType(vcb) != VCB ||
			!FuserCheckCCB(vcb->Dcb, fileObject->FsContext2)) {
			status = STATUS_INVALID_PARAMETER;
			__leave;
		}
		
		ccb	= (PFuserCCB)fileObject->FsContext2;
		ASSERT(ccb != NULL);

		fcb = ccb->Fcb;
		ASSERT(fcb != NULL);

		FDbgPrint("  ProcessId %lu\n", IoGetRequestorProcessId(Irp));
		FuserPrintFileName(fileObject);

		buffer = Irp->AssociatedIrp.SystemBuffer;
		switch (irpSp->Parameters.SetFile.FileInformationClass) {
		case FileAllocationInformation:
			FDbgPrint("  FileAllocationInformation %lld\n",
						((PFILE_ALLOCATION_INFORMATION)buffer)->AllocationSize.QuadPart);
			break;
		case FileBasicInformation:
			FDbgPrint("  FileBasicInformation\n");
			break;
		case FileDispositionInformation:
			FDbgPrint("  FileDispositionInformation\n");
			break;
		case FileEndOfFileInformation:
			FDbgPrint("  FileEndOfFileInformation %lld\n",
						((PFILE_END_OF_FILE_INFORMATION)buffer)->EndOfFile.QuadPart);
			break;
		case FileLinkInformation:
			FDbgPrint("  FileLinkInformation\n");
			break;

		case FilePositionInformation:
			{
				PFILE_POSITION_INFORMATION posInfo;
				
				posInfo = (PFILE_POSITION_INFORMATION)Irp->AssociatedIrp.SystemBuffer;
				ASSERT(posInfo != NULL);

				FDbgPrint("  FilePositionInformation %lld\n",
								posInfo->CurrentByteOffset.QuadPart);
				fileObject->CurrentByteOffset = posInfo->CurrentByteOffset;

				status = STATUS_SUCCESS;

				__leave;
			}
			break;

		case FileRenameInformation:
			FDbgPrint("  FileRenameInformation\n");
			break;
		case FileValidDataLengthInformation:
			FDbgPrint("  FileValidDataLengthInformation\n");
			break;
		default:
			FDbgPrint("  unknown type:%d\n", irpSp->Parameters.SetFile.FileInformationClass);
			break;
		}

		//
		// when this IRP is not handled in swich case
		//

		// calcurate the size of EVENT_CONTEXT
		// it is sum of file name length and size of FileInformation
		eventLength = sizeof(EVENT_CONTEXT) + fcb->FileName.Length + 
						irpSp->Parameters.SetFile.Length;

		targetFileObject = irpSp->Parameters.SetFile.FileObject;

		if (targetFileObject) {
			FDbgPrint("  FileObject Specified %wZ\n", &(targetFileObject->FileName));
			eventLength += targetFileObject->FileName.Length;
		}

		eventContext = AllocateEventContext(vcb->Dcb, Irp, eventLength, ccb);
	
		if (eventContext == NULL) {
			status = STATUS_INSUFFICIENT_RESOURCES;
			__leave;
		}

		eventContext->Context = ccb->UserContext;

		eventContext->SetFile.FileInformationClass =
			irpSp->Parameters.SetFile.FileInformationClass;

		// the size of FileInformation
		eventContext->SetFile.BufferLength = irpSp->Parameters.SetFile.Length;

		// the offset from begining of structure to fill FileInfo
		eventContext->SetFile.BufferOffset = FIELD_OFFSET(EVENT_CONTEXT, SetFile.FileName[0]) +
												fcb->FileName.Length + sizeof(WCHAR); // the last null char
	
		// copy FileInformation
		RtlCopyMemory((PCHAR)eventContext + eventContext->SetFile.BufferOffset,
						Irp->AssociatedIrp.SystemBuffer,
						irpSp->Parameters.SetFile.Length);

		if (irpSp->Parameters.SetFile.FileInformationClass == FileRenameInformation ||
			irpSp->Parameters.SetFile.FileInformationClass == FileLinkInformation) {

			// We need to hanle FileRenameInformation separetly because the structure of FILE_RENAME_INFORMATION
			// has HANDLE type field, which size is different in 32 bit and 64 bit environment.
			// This cases problems when driver is 64 bit and user mode library is 32 bit.
			PFILE_RENAME_INFORMATION renameInfo = (PFILE_RENAME_INFORMATION)Irp->AssociatedIrp.SystemBuffer;			
			PFUSER_RENAME_INFORMATION renameContext = 
				(PFUSER_RENAME_INFORMATION)((PCHAR)eventContext + eventContext->SetFile.BufferOffset);

			// This code assumes FILE_RENAME_INFORMATION and FILE_LINK_INFORMATION have
			// the same typse and fields.
			ASSERT(sizeof(FILE_RENAME_INFORMATION) == sizeof(FILE_LINK_INFORMATION));

			renameContext->ReplaceIfExists = renameInfo->ReplaceIfExists;
			renameContext->FileNameLength = renameInfo->FileNameLength;
			RtlCopyMemory(renameContext->FileName, renameInfo->FileName, renameInfo->FileNameLength);

			if (targetFileObject != NULL) {
				// if Parameters.SetFile.FileObject is specified, replace FILE_RENAME_INFO's file name by
				// FileObject's file name. The buffer size is already adjusted.
				FDbgPrint("  renameContext->FileNameLength %d\n", renameContext->FileNameLength);
				FDbgPrint("  renameContext->FileName %ws\n", renameContext->FileName);
				RtlZeroMemory(renameContext->FileName, renameContext->FileNameLength);
				RtlCopyMemory(renameContext->FileName, targetFileObject->FileName.Buffer, targetFileObject->FileName.Length);
				renameContext->FileNameLength = targetFileObject->FileName.Length;
			}
		}

		// copy the file name
		eventContext->SetFile.FileNameLength = fcb->FileName.Length;
		RtlCopyMemory(eventContext->SetFile.FileName,
						fcb->FileName.Buffer,
						fcb->FileName.Length);

		// register this IRP to waiting IRP list and make it pending status
		status = FuserRegisterPendingIrp(DeviceObject, Irp, eventContext, 0);

	} __finally {

		if (status != STATUS_PENDING) {
			Irp->IoStatus.Status = status;
			Irp->IoStatus.Information = 0;
			IoCompleteRequest(Irp, IO_NO_INCREMENT);
			FuserPrintNTStatus(status);
		}


		FDbgPrint("<== FuserSetInformation\n");

		FsRtlExitFileSystem();
	}
	return status;
}





VOID
FuserCompleteSetInformation(
	__in PIRP_ENTRY		IrpEntry,
	__in PEVENT_INFORMATION EventInfo
	)
{
	PIRP				irp;
	PIO_STACK_LOCATION	irpSp;
	NTSTATUS			status;
	ULONG				info	 = 0;
	PFuserCCB			ccb;
	PFuserFCB			fcb;
	UNICODE_STRING		oldFileName;

	FILE_INFORMATION_CLASS infoClass;
	__try {

		FDbgPrint("==> FuserCompleteSetInformation\n");

		irp = IrpEntry->Irp;
		irpSp = IrpEntry->IrpSp;

		ccb = IrpEntry->FileObject->FsContext2;
		ASSERT(ccb != NULL);

		ExAcquireResourceExclusiveLite(&ccb->Resource, TRUE);

		fcb = ccb->Fcb;
		ASSERT(fcb != NULL);

		ccb->UserContext = EventInfo->Context;

		status = EventInfo->Status;
		info = EventInfo->BufferLength;

		infoClass = irpSp->Parameters.SetFile.FileInformationClass;

		RtlZeroMemory(&oldFileName, sizeof(UNICODE_STRING));

		if (NT_SUCCESS(status)) {
			
			if (infoClass == FileDispositionInformation) {

				if (EventInfo->Delete.DeleteOnClose) {

					if (!MmFlushImageSection(
						&fcb->SectionObjectPointers,
						MmFlushForDelete)) {
						FDbgPrint("  Cannot delete user mapped image\n");
						status = STATUS_CANNOT_DELETE;
					} else {
						ccb->Flags |= FUSER_DELETE_ON_CLOSE;
						fcb->Flags |= FUSER_DELETE_ON_CLOSE;
						FDbgPrint("   FileObject->DeletePending = TRUE\n");
						IrpEntry->FileObject->DeletePending = TRUE;
					}

				} else {
					ccb->Flags &= ~FUSER_DELETE_ON_CLOSE;
					fcb->Flags &= ~FUSER_DELETE_ON_CLOSE;
					FDbgPrint("   FileObject->DeletePending = FALSE\n");
					IrpEntry->FileObject->DeletePending = FALSE;
				}
			}

			// if rename is executed, reassign the file name
			if(infoClass == FileRenameInformation) {
				PVOID buffer = NULL;

				ExAcquireResourceExclusiveLite(&fcb->Resource, TRUE);

				// this is used to inform rename in the bellow switch case
				oldFileName.Buffer = fcb->FileName.Buffer;
				oldFileName.Length = (USHORT)fcb->FileName.Length;
				oldFileName.MaximumLength = (USHORT)fcb->FileName.Length;

				// copy new file name
				buffer = ExAllocatePool(EventInfo->BufferLength+sizeof(WCHAR));

				if (buffer == NULL) {
					status = STATUS_INSUFFICIENT_RESOURCES;
					ExReleaseResourceLite(&fcb->Resource);
					ExReleaseResourceLite(&ccb->Resource);
					__leave;
				}

				fcb->FileName.Buffer = buffer;

				ASSERT(fcb->FileName.Buffer != NULL);

				RtlZeroMemory(fcb->FileName.Buffer, EventInfo->BufferLength+sizeof(WCHAR));
				RtlCopyMemory(fcb->FileName.Buffer, EventInfo->Buffer, EventInfo->BufferLength);

				fcb->FileName.Length = (USHORT)EventInfo->BufferLength;
				fcb->FileName.MaximumLength = (USHORT)EventInfo->BufferLength;

				ExReleaseResourceLite(&fcb->Resource);
			}
		}

		ExReleaseResourceLite(&ccb->Resource);

		if (NT_SUCCESS(status)) {
			switch (irpSp->Parameters.SetFile.FileInformationClass) {
			case FileAllocationInformation:
				FuserNotifyReportChange(fcb, FILE_NOTIFY_CHANGE_SIZE, FILE_ACTION_MODIFIED);
				break;
			case FileBasicInformation:
				FuserNotifyReportChange(fcb,
					FILE_NOTIFY_CHANGE_ATTRIBUTES |
					FILE_NOTIFY_CHANGE_LAST_WRITE |
					FILE_NOTIFY_CHANGE_LAST_ACCESS |
					FILE_NOTIFY_CHANGE_CREATION,
					FILE_ACTION_MODIFIED);
				break;
			case FileDispositionInformation:
				if (IrpEntry->FileObject->DeletePending) {
					if (fcb->Flags & FUSER_FILE_DIRECTORY) {
						FuserNotifyReportChange(fcb, FILE_NOTIFY_CHANGE_DIR_NAME, FILE_ACTION_REMOVED);
					} else {
						FuserNotifyReportChange(fcb, FILE_NOTIFY_CHANGE_FILE_NAME, FILE_ACTION_REMOVED);
					}
				}
				break;
			case FileEndOfFileInformation:
				FuserNotifyReportChange(fcb, FILE_NOTIFY_CHANGE_SIZE, FILE_ACTION_MODIFIED);
				break;
			case FileLinkInformation:
				// TODO: should check whether this is a directory
				// TODO: should notify new link name
				//FuserNotifyReportChange(vcb, ccb, FILE_NOTIFY_CHANGE_FILE_NAME, FILE_ACTION_ADDED);
				break;
			case FilePositionInformation:
				// this is never used
				break;
			case FileRenameInformation:
				{
					FuserNotifyReportChange0(fcb, &oldFileName,
						FILE_NOTIFY_CHANGE_FILE_NAME, FILE_ACTION_RENAMED_OLD_NAME);
					
					// free old file name
					ExFreePool(oldFileName.Buffer);

					FuserNotifyReportChange(fcb, FILE_NOTIFY_CHANGE_FILE_NAME, FILE_ACTION_RENAMED_NEW_NAME);
				}
				break;
			case FileValidDataLengthInformation:
				FuserNotifyReportChange(fcb, FILE_NOTIFY_CHANGE_SIZE, FILE_ACTION_MODIFIED);
				break;
			default:
				FDbgPrint("  unknown type:%d\n", irpSp->Parameters.SetFile.FileInformationClass);
				break;
			}
		}

	} __finally {

		irp->IoStatus.Status = status;
		irp->IoStatus.Information = info;
		IoCompleteRequest(irp, IO_NO_INCREMENT);

		FuserPrintNTStatus(status);

		FDbgPrint("<== FuserCompleteSetInformation\n");
	}

}

