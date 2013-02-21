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
FuserDispatchLock(
	__in PDEVICE_OBJECT DeviceObject,
	__in PIRP Irp
	)
{
	PIO_STACK_LOCATION	irpSp;
	NTSTATUS			status = STATUS_INVALID_PARAMETER;
	PFILE_OBJECT		fileObject;
	PFuserCCB			ccb;
	PFuserFCB			fcb;
	PFuserVCB			vcb;
	PEVENT_CONTEXT		eventContext;
	ULONG				eventLength;

	PAGED_CODE();

	__try {
		FsRtlEnterFileSystem();

		FDbgPrint("==> FuserLock\n");
	
		irpSp = IoGetCurrentIrpStackLocation(Irp);
		fileObject = irpSp->FileObject;

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

		FDbgPrint("  ProcessId %lu\n", IoGetRequestorProcessId(Irp));
		FuserPrintFileName(fileObject);

		switch(irpSp->MinorFunction) {
		case IRP_MN_LOCK:
			FDbgPrint("  IRP_MN_LOCK\n");
			break;
		case IRP_MN_UNLOCK_ALL:
			FDbgPrint("  IRP_MN_UNLOCK_ALL\n");
			break;
		case IRP_MN_UNLOCK_ALL_BY_KEY:
			FDbgPrint("  IRP_MN_UNLOCK_ALL_BY_KEY\n");
			break;
		case IRP_MN_UNLOCK_SINGLE:
			FDbgPrint("  IRP_MN_UNLOCK_SINGLE\n");
			break;
		default:
			FDbgPrint("  unknown function : %d\n", irpSp->MinorFunction);
			break;
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

		eventContext->Context = ccb->UserContext;
		FDbgPrint("   get Context %X\n", (ULONG)ccb->UserContext);

		// copy file name to be locked
		eventContext->Lock.FileNameLength = fcb->FileName.Length;
		RtlCopyMemory(eventContext->Lock.FileName, fcb->FileName.Buffer, fcb->FileName.Length);

		// parameters of Lock
		eventContext->Lock.ByteOffset = irpSp->Parameters.LockControl.ByteOffset;
		if (irpSp->Parameters.LockControl.Length != NULL) {
			eventContext->Lock.Length.QuadPart = irpSp->Parameters.LockControl.Length->QuadPart;
		} else {
			FDbgPrint("  LockControl.Length = NULL\n");
		}
		eventContext->Lock.Key = irpSp->Parameters.LockControl.Key;

		// register this IRP to waiting IRP list and make it pending status
		status = FuserRegisterPendingIrp(DeviceObject, Irp, eventContext, 0);

	} __finally {

		if (status != STATUS_PENDING) {
			//
			// complete the Irp
			//
			Irp->IoStatus.Status = status;
			Irp->IoStatus.Information = 0;
			IoCompleteRequest(Irp, IO_NO_INCREMENT);
			FuserPrintNTStatus(status);
		}

		FDbgPrint("<== FuserLock\n");
		FsRtlExitFileSystem();
	}

	return status;
}




VOID
FuserCompleteLock(
	__in PIRP_ENTRY			IrpEntry,
	__in PEVENT_INFORMATION	EventInfo
	)
{
	PIRP				irp;
	PIO_STACK_LOCATION	irpSp;
	PFuserCCB			ccb;
	PFILE_OBJECT		fileObject;
	NTSTATUS			status;

	irp   = IrpEntry->Irp;
	irpSp = IrpEntry->IrpSp;	

	//FsRtlEnterFileSystem();

	FDbgPrint("==> FuserCompleteLock\n");

	fileObject = irpSp->FileObject;
	ccb = fileObject->FsContext2;
	ASSERT(ccb != NULL);

	ccb->UserContext = EventInfo->Context;
	// FDbgPrint("   set Context %X\n", (ULONG)ccb->UserContext);

	status = EventInfo->Status;
	irp->IoStatus.Status = status;
	irp->IoStatus.Information = 0;
	IoCompleteRequest(irp, IO_NO_INCREMENT);

	FuserPrintNTStatus(status);

	FDbgPrint("<== FuserCompleteLock\n");

	//FsRtlExitFileSystem();
}

