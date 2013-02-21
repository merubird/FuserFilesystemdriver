/*
  Dokan : user-mode file system library for Windows

  Copyright (C) 2011 - 2013 Christian Auer christian.auer@gmx.ch
  Copyright (C) 2010 Hiroki Asakawa info@dokan-dev.net

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

// TODO: remove support for Filesecurity


#include "fuser.h"

NTSTATUS
FuserDispatchQuerySecurity(
	__in PDEVICE_OBJECT DeviceObject,
	__in PIRP Irp
	)
{
	PIO_STACK_LOCATION	irpSp;
	NTSTATUS			status = STATUS_NOT_IMPLEMENTED;
	PFILE_OBJECT		fileObject;
	ULONG				info = 0;
	ULONG				bufferLength;
	SECURITY_DESCRIPTOR dummySecurityDesc;
	ULONG				descLength;
	PSECURITY_DESCRIPTOR securityDesc;
	PSECURITY_INFORMATION securityInfo;
	PFuserFCB			fcb;
	PFuserDCB			dcb;
	PFuserVCB			vcb;
	PFuserCCB			ccb;
	ULONG				eventLength;
	PEVENT_CONTEXT		eventContext;
	ULONG				flags = 0;

	__try {
		FsRtlEnterFileSystem();

		FDbgPrint("==> FuserQuerySecurity\n");

		irpSp = IoGetCurrentIrpStackLocation(Irp);
		fileObject = irpSp->FileObject;

		if (fileObject == NULL) {
			FDbgPrint("  fileObject == NULL\n");
			status = STATUS_INVALID_PARAMETER;
			__leave;
		}

		vcb = DeviceObject->DeviceExtension;
		if (GetIdentifierType(vcb) != VCB) {
			FDbgPrint("    DeviceExtension != VCB\n");
			status = STATUS_INVALID_PARAMETER;
			__leave;
		}
		dcb = vcb->Dcb;

		FDbgPrint("  ProcessId %lu\n", IoGetRequestorProcessId(Irp));
		FuserPrintFileName(fileObject);

		ccb = fileObject->FsContext2;
		if (ccb == NULL) {
			FDbgPrint("    ccb == NULL\n");
			status = STATUS_INVALID_PARAMETER;
			__leave;
		}
		fcb = ccb->Fcb;

		bufferLength = irpSp->Parameters.QuerySecurity.Length;
		securityInfo = &irpSp->Parameters.QuerySecurity.SecurityInformation;

		if (*securityInfo & OWNER_SECURITY_INFORMATION) {
			FDbgPrint("    OWNER_SECURITY_INFORMATION\n");
		}
		if (*securityInfo & GROUP_SECURITY_INFORMATION) {
			FDbgPrint("    GROUP_SECURITY_INFORMATION\n");
		}
		if (*securityInfo & DACL_SECURITY_INFORMATION) {
			FDbgPrint("    DACL_SECURITY_INFORMATION\n");
		}
		if (*securityInfo & SACL_SECURITY_INFORMATION) {
			FDbgPrint("    SACL_SECURITY_INFORMATION\n");
		}
		if (*securityInfo & LABEL_SECURITY_INFORMATION) {
			FDbgPrint("    LABEL_SECURITY_INFORMATION\n");
		}

		eventLength = sizeof(EVENT_CONTEXT) + fcb->FileName.Length;
		eventContext = AllocateEventContext(dcb, Irp, eventLength, ccb);

		if (eventContext == NULL) {
			status = STATUS_INSUFFICIENT_RESOURCES;
			__leave;
		}

		if (Irp->UserBuffer != NULL && bufferLength > 0) {
			// make a MDL for UserBuffer that can be used later on another thread context	
			if (Irp->MdlAddress == NULL) {
				status = FuserAllocateMdl(Irp, bufferLength);
				if (!NT_SUCCESS(status)) {
					ExFreePool(eventContext);
					__leave;
				}
				flags = FUSER_MDL_ALLOCATED;
			}
		}

		eventContext->Context = ccb->UserContext;
		eventContext->Security.SecurityInformation = *securityInfo;
		eventContext->Security.BufferLength = bufferLength;
	
		eventContext->Security.FileNameLength = fcb->FileName.Length;
		RtlCopyMemory(eventContext->Security.FileName,
				fcb->FileName.Buffer, fcb->FileName.Length);

		status = FuserRegisterPendingIrp(DeviceObject, Irp, eventContext, flags);

	} __finally {

		if (status != STATUS_PENDING) {
			Irp->IoStatus.Status = status;
			Irp->IoStatus.Information = info;
			IoCompleteRequest(Irp, IO_NO_INCREMENT);
			FuserPrintNTStatus(status);
		}

		FDbgPrint("<== FuserQuerySecurity\n");
		FsRtlExitFileSystem();
	}

	return status;
}



VOID
FuserCompleteQuerySecurity(
	__in PIRP_ENTRY		IrpEntry,
	__in PEVENT_INFORMATION EventInfo
	)
{
	PIRP		irp;
	PIO_STACK_LOCATION	irpSp;
	NTSTATUS	status;
	PVOID		buffer = NULL;
	ULONG		bufferLength;
	ULONG		info = 0;
	PFILE_OBJECT	fileObject;
	PFuserCCB		ccb;

	FDbgPrint("==> FuserCompleteQuerySecurity\n");
// TODO: trycatch missing here

	irp   = IrpEntry->Irp;
	irpSp = IrpEntry->IrpSp;	

	if (irp->MdlAddress) {
		buffer = MmGetSystemAddressForMdlSafe(irp->MdlAddress, NormalPagePriority);
	}

	bufferLength = irpSp->Parameters.QuerySecurity.Length;

	if (EventInfo->Status == STATUS_SUCCESS &&
		EventInfo->BufferLength <= bufferLength &&
		buffer != NULL) {
		RtlCopyMemory(buffer, EventInfo->Buffer, EventInfo->BufferLength);
		info = EventInfo->BufferLength;
		status = STATUS_SUCCESS;

	} else if (EventInfo->Status == STATUS_BUFFER_OVERFLOW ||
			(EventInfo->Status == STATUS_SUCCESS && bufferLength < EventInfo->BufferLength)) {
		info = EventInfo->BufferLength;
		status = STATUS_BUFFER_OVERFLOW;

	} else {
		info = 0;
		status = EventInfo->Status;
	}

	if (IrpEntry->Flags & FUSER_MDL_ALLOCATED) {
		FuserFreeMdl(irp);
		IrpEntry->Flags &= ~FUSER_MDL_ALLOCATED;
	}

	fileObject = IrpEntry->FileObject;
	ASSERT(fileObject != NULL);

	ccb = fileObject->FsContext2;
	if (ccb != NULL) {
		ccb->UserContext = EventInfo->Context;
	} else {
		FDbgPrint("  ccb == NULL\n");
	}

	irp->IoStatus.Status = status;
	irp->IoStatus.Information = info;
	IoCompleteRequest(irp, IO_NO_INCREMENT);

	FuserPrintNTStatus(status);

	FDbgPrint("<== FuserCompleteQuerySecurity\n");
}



NTSTATUS
FuserDispatchSetSecurity(
	__in PDEVICE_OBJECT DeviceObject,
	__in PIRP Irp
	)
{
	PIO_STACK_LOCATION	irpSp;
	PFuserVCB			vcb;
	PFuserDCB			dcb;
	PFuserCCB			ccb;
	PFuserFCB			fcb;
	NTSTATUS			status = STATUS_NOT_IMPLEMENTED;
	PFILE_OBJECT		fileObject;
	ULONG				info = 0;
	PSECURITY_INFORMATION	securityInfo;
	PSECURITY_DESCRIPTOR	securityDescriptor;
	PSECURITY_DESCRIPTOR	selfRelativesScurityDescriptor = NULL;
	ULONG				securityDescLength;
	ULONG				eventLength;
	PEVENT_CONTEXT		eventContext;

	__try {
		FsRtlEnterFileSystem();

		FDbgPrint("==> FuserSetSecurity\n");

		irpSp = IoGetCurrentIrpStackLocation(Irp);
		fileObject = irpSp->FileObject;

		if (fileObject == NULL) {
			FDbgPrint("  fileObject == NULL\n");
			status = STATUS_INVALID_PARAMETER;
			__leave;
		}

		vcb = DeviceObject->DeviceExtension;
		if (GetIdentifierType(vcb) != VCB) {
			FDbgPrint("    DeviceExtension != VCB\n");
			status = STATUS_INVALID_PARAMETER;
			__leave;
		}
		dcb = vcb->Dcb;

		FDbgPrint("  ProcessId %lu\n", IoGetRequestorProcessId(Irp));
		FuserPrintFileName(fileObject);

		ccb = fileObject->FsContext2;
		if (ccb == NULL) {
			FDbgPrint("    ccb == NULL\n");
			status = STATUS_INVALID_PARAMETER;
			__leave;
		}
		fcb = ccb->Fcb;

		securityInfo = &irpSp->Parameters.SetSecurity.SecurityInformation;

		if (*securityInfo & OWNER_SECURITY_INFORMATION) {
			FDbgPrint("    OWNER_SECURITY_INFORMATION\n");
		}
		if (*securityInfo & GROUP_SECURITY_INFORMATION) {
			FDbgPrint("    GROUP_SECURITY_INFORMATION\n");
		}
		if (*securityInfo & DACL_SECURITY_INFORMATION) {
			FDbgPrint("    DACL_SECURITY_INFORMATION\n");
		}
		if (*securityInfo & SACL_SECURITY_INFORMATION) {
			FDbgPrint("    SACL_SECURITY_INFORMATION\n");
		}
		if (*securityInfo & LABEL_SECURITY_INFORMATION) {
			FDbgPrint("    LABEL_SECURITY_INFORMATION\n");
		}

		securityDescriptor = irpSp->Parameters.SetSecurity.SecurityDescriptor;

		// Assumes the parameter is self relative SD.
		securityDescLength = RtlLengthSecurityDescriptor(securityDescriptor);

		eventLength = sizeof(EVENT_CONTEXT) + securityDescLength + fcb->FileName.Length;

		if (EVENT_CONTEXT_MAX_SIZE < eventLength) {
			// TODO: Handle this case like DispatchWrite.
			FDbgPrint("    SecurityDescriptor is too big: %d (limit %d)\n",
					eventLength, EVENT_CONTEXT_MAX_SIZE);
			status = STATUS_INSUFFICIENT_RESOURCES;
			__leave;
		}

		eventContext = AllocateEventContext(vcb->Dcb, Irp, eventLength, ccb);

		if (eventContext == NULL) {
			status = STATUS_INSUFFICIENT_RESOURCES;
			__leave;
		}
		eventContext->Context = ccb->UserContext;
		eventContext->SetSecurity.SecurityInformation = *securityInfo;
		eventContext->SetSecurity.BufferLength = securityDescLength;
		eventContext->SetSecurity.BufferOffset = FIELD_OFFSET(EVENT_CONTEXT, SetSecurity.FileName[0]) +
													fcb->FileName.Length + sizeof(WCHAR);
		RtlCopyMemory((PCHAR)eventContext + eventContext->SetSecurity.BufferOffset,
				securityDescriptor, securityDescLength);


		eventContext->SetSecurity.FileNameLength = fcb->FileName.Length;
		RtlCopyMemory(eventContext->SetSecurity.FileName, fcb->FileName.Buffer, fcb->FileName.Length);

		status = FuserRegisterPendingIrp(DeviceObject, Irp, eventContext, 0);

	} __finally {

		if (status != STATUS_PENDING) {
			Irp->IoStatus.Status = status;
			Irp->IoStatus.Information = info;
			IoCompleteRequest(Irp, IO_NO_INCREMENT);
			FuserPrintNTStatus(status);
		}

		FDbgPrint("<== FuserSetSecurity\n");
		FsRtlExitFileSystem();
	}

	return status;
}





VOID
FuserCompleteSetSecurity(
	__in PIRP_ENTRY		IrpEntry,
	__in PEVENT_INFORMATION EventInfo
	)
{
	PIRP				irp;
	PIO_STACK_LOCATION	irpSp;
	PFILE_OBJECT		fileObject;
	PFuserCCB			ccb;
	NTSTATUS			status;

	FDbgPrint("==> FuserCompleteSetSecurity\n");

	irp   = IrpEntry->Irp;
	irpSp = IrpEntry->IrpSp;	

	fileObject = IrpEntry->FileObject;
	ASSERT(fileObject != NULL);

	ccb = fileObject->FsContext2;
	if (ccb != NULL) {
		ccb->UserContext = EventInfo->Context;
	} else {
		FDbgPrint("  ccb == NULL\n");
	}

	status = EventInfo->Status;

	irp->IoStatus.Status = status;
	irp->IoStatus.Information = 0;
	IoCompleteRequest(irp, IO_NO_INCREMENT);

	FuserPrintNTStatus(status);

	FDbgPrint("<== FuserCompleteSetSecurity\n");
}



