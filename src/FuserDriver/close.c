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
FuserDispatchClose(
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
	PIO_STACK_LOCATION	irpSp;
	NTSTATUS			status = STATUS_INVALID_PARAMETER;
	PFILE_OBJECT		fileObject;
	PFuserCCB			ccb;
	PEVENT_CONTEXT		eventContext;
	ULONG				eventLength;
	PFuserFCB			fcb;

	PAGED_CODE();
	__try {
		FsRtlEnterFileSystem();

		FDbgPrint("==> FuserClose\n");
	
		irpSp = IoGetCurrentIrpStackLocation(Irp);
		fileObject = irpSp->FileObject;

		if (fileObject == NULL) {
			FDbgPrint("  fileObject is NULL\n");
			status = STATUS_SUCCESS;
			__leave;
		}

		FDbgPrint("  ProcessId %lu\n", IoGetRequestorProcessId(Irp));
		FuserPrintFileName(fileObject);

		vcb = DeviceObject->DeviceExtension;

		if (GetIdentifierType(vcb) != VCB ||
			!FuserCheckCCB(vcb->Dcb, fileObject->FsContext2)) {

			if (fileObject->FsContext2) {
				ccb = fileObject->FsContext2;
				ASSERT(ccb != NULL);

				fcb = ccb->Fcb;
				ASSERT(fcb != NULL);

				FDbgPrint("   Free CCB:%X\n", ccb);
				FuserFreeCCB(ccb);

				FuserFreeFCB(fcb);
			}

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
			//status = STATUS_INSUFFICIENT_RESOURCES;
			FDbgPrint("   eventContext == NULL\n");
			FDbgPrint("   Free CCB:%X\n", ccb);
			FuserFreeCCB(ccb);
			FuserFreeFCB(fcb);
			status = STATUS_SUCCESS;
			__leave;
		}

		eventContext->Context = ccb->UserContext;
		FDbgPrint("   UserContext:%X\n", (ULONG)ccb->UserContext);

		// copy the file name to be closed
		eventContext->Close.FileNameLength = fcb->FileName.Length;
		RtlCopyMemory(eventContext->Close.FileName, fcb->FileName.Buffer, fcb->FileName.Length);

		FDbgPrint("   Free CCB:%X\n", ccb);
		FuserFreeCCB(ccb);

		FuserFreeFCB(fcb);

		// Close can not be pending status
		// don't register this IRP
		//status = FuserRegisterPendingIrp(DeviceObject, Irp, eventContext->SerialNumber, 0);

		// inform it to user-mode
		FuserEventNotification(&vcb->Dcb->NotifyEvent, eventContext);

		status = STATUS_SUCCESS;

	} __finally {

		if (status != STATUS_PENDING) {
			Irp->IoStatus.Status = status;
			Irp->IoStatus.Information = 0;
			IoCompleteRequest(Irp, IO_NO_INCREMENT);
		}

		FDbgPrint("<== FuserClose\n");

		FsRtlExitFileSystem();
	}

	return status;
}

