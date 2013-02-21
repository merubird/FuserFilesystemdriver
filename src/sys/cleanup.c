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
FuserDispatchCleanup(
	__in PDEVICE_OBJECT DeviceObject,
	__in PIRP Irp
	)

/*++

Routine Description:

	This device control dispatcher handles Cleanup IRP.

Arguments:

	DeviceObject - Context for the activity.
	Irp 		 - The device control argument block.

Return Value:

	NTSTATUS

--*/
{
	PFuserVCB			vcb;
	PIO_STACK_LOCATION	irpSp;
	NTSTATUS			status = STATUS_INVALID_PARAMETER;
	PFILE_OBJECT		fileObject;
	PFuserCCB			ccb = NULL;
	PFuserFCB			fcb = NULL;
	PEVENT_CONTEXT		eventContext;
	ULONG				eventLength;

	PAGED_CODE();

	__try {

		FsRtlEnterFileSystem();

		FDbgPrint("==> FuserCleanup\n");
	
		irpSp = IoGetCurrentIrpStackLocation(Irp);
		fileObject = irpSp->FileObject;

		FDbgPrint("  ProcessId %lu\n", IoGetRequestorProcessId(Irp));
		FuserPrintFileName(fileObject);

		// Cleanup must be success in any case
		if (fileObject == NULL) {
			FDbgPrint("  fileObject == NULL\n");
			status = STATUS_SUCCESS;
			__leave;
		}

		vcb = DeviceObject->DeviceExtension;
		if (GetIdentifierType(vcb) != VCB ||
			!FuserCheckCCB(vcb->Dcb, fileObject->FsContext2)) {
			status = STATUS_SUCCESS;
			__leave;
		}

		ccb = fileObject->FsContext2;
		ASSERT(ccb != NULL);

		fcb = ccb->Fcb;
		ASSERT(fcb != NULL);

		eventLength = sizeof(EVENT_CONTEXT) + fcb->FileName.Length;
		eventContext = AllocateEventContext(vcb->Dcb, Irp, eventLength, ccb);

		if (eventContext == NULL) {
			status = STATUS_INSUFFICIENT_RESOURCES;
			__leave;
		}

		if (fileObject->SectionObjectPointer != NULL &&
			fileObject->SectionObjectPointer->DataSectionObject != NULL) {
			CcFlushCache(&fcb->SectionObjectPointers, NULL, 0, NULL);
			CcPurgeCacheSection(&fcb->SectionObjectPointers, NULL, 0, FALSE);
			CcUninitializeCacheMap(fileObject, NULL, NULL);
		}
		fileObject->Flags |= FO_CLEANUP_COMPLETE;

		eventContext->Context = ccb->UserContext;
		//FDbgPrint("   get Context %X\n", (ULONG)ccb->UserContext);

		// copy the filename to EventContext from ccb
		eventContext->Cleanup.FileNameLength = fcb->FileName.Length;
		RtlCopyMemory(eventContext->Cleanup.FileName, fcb->FileName.Buffer, fcb->FileName.Length);

		// register this IRP to pending IRP list
		status = FuserRegisterPendingIrp(DeviceObject, Irp, eventContext, 0);

	} __finally {

		if (status != STATUS_PENDING) {
			Irp->IoStatus.Status = status;
			Irp->IoStatus.Information = 0;
			IoCompleteRequest(Irp, IO_NO_INCREMENT);
			FuserPrintNTStatus(status);
		}

		FDbgPrint("<== FuserCleanup\n");
	
		FsRtlExitFileSystem();
	}

	return status;
}




VOID
FuserCompleteCleanup(
	 __in PIRP_ENTRY			IrpEntry,
	 __in PEVENT_INFORMATION	EventInfo
	 )
{
	PIRP				irp;
	PIO_STACK_LOCATION	irpSp;
	NTSTATUS			status   = STATUS_SUCCESS;
	ULONG				info	 = 0;
	PFuserCCB			ccb;
	PFuserFCB			fcb;
	PFuserVCB			vcb;
	PFILE_OBJECT		fileObject;

	FDbgPrint("==> FuserCompleteCleanup\n");

	irp   = IrpEntry->Irp;
	irpSp = IrpEntry->IrpSp;

	fileObject = IrpEntry->FileObject;
	ASSERT(fileObject != NULL);

	ccb	= fileObject->FsContext2;
	ASSERT(ccb != NULL);

	ccb->UserContext = EventInfo->Context;
	//FDbgPrint("   set Context %X\n", (ULONG)ccb->UserContext);

	fcb = ccb->Fcb;
	ASSERT(fcb != NULL);

	vcb = fcb->Vcb;

	status = EventInfo->Status;

	if (fcb->Flags & FUSER_FILE_DIRECTORY) {
		FsRtlNotifyCleanup(vcb->NotifySync, &vcb->DirNotifyList, ccb);
	}

	irp->IoStatus.Status = status;
	irp->IoStatus.Information = 0;
	IoCompleteRequest(irp, IO_NO_INCREMENT);

	FDbgPrint("<== FuserCompleteCleanup\n");
}

