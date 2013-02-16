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
FuserDispatchQueryVolumeInformation(
	__in PDEVICE_OBJECT DeviceObject,
	__in PIRP Irp
   )
{
	NTSTATUS			status = STATUS_INVALID_PARAMETER;
	PIO_STACK_LOCATION  irpSp;
	PVOID				buffer;
	PFILE_OBJECT		fileObject;
	PFuserVCB			vcb;
	PFuserDCB			dcb;
	PFuserCCB			ccb;
	ULONG               info = 0;

	PAGED_CODE();
	__try {
		FsRtlEnterFileSystem();

		FDbgPrint("==> FuserQueryVolumeInformation\n");
		FDbgPrint("  ProcessId %lu\n", IoGetRequestorProcessId(Irp));

		vcb = DeviceObject->DeviceExtension;
		if (GetIdentifierType(vcb) != VCB) {
			return STATUS_INVALID_PARAMETER;
		}
		dcb = vcb->Dcb;

		irpSp			= IoGetCurrentIrpStackLocation(Irp);
		buffer			= Irp->AssociatedIrp.SystemBuffer;

		fileObject		= irpSp->FileObject;

		if (fileObject == NULL) {
			FDbgPrint("  fileObject == NULL\n");
			status = STATUS_INVALID_PARAMETER;
			__leave;
		}


		FDbgPrint("  FileName: %wZ\n", &fileObject->FileName);

		ccb = fileObject->FsContext2;

		//	ASSERT(ccb != NULL);
		switch(irpSp->Parameters.QueryVolume.FsInformationClass) {
		case FileFsVolumeInformation:
			FDbgPrint("  FileFsVolumeInformation\n");
			break;

		case FileFsLabelInformation:
			FDbgPrint("  FileFsLabelInformation\n");
			break;
	        
		case FileFsSizeInformation:
			FDbgPrint("  FileFsSizeInformation\n");
			break;
		case FileFsDeviceInformation:
			{

				PFILE_FS_DEVICE_INFORMATION device;
				FDbgPrint("  FileFsDeviceInformation\n");
				device = (PFILE_FS_DEVICE_INFORMATION)Irp->AssociatedIrp.SystemBuffer;
				if (irpSp->Parameters.QueryVolume.Length < sizeof(FILE_FS_DEVICE_INFORMATION)) {
					status = STATUS_BUFFER_TOO_SMALL;
					info = sizeof(FILE_FS_DEVICE_INFORMATION);
					__leave;
				}
				device->DeviceType = dcb->DeviceType;
				device->Characteristics = dcb->DeviceCharacteristics;
				status = STATUS_SUCCESS;
				info = sizeof(FILE_FS_DEVICE_INFORMATION);
				__leave;
			}
			break;
		case FileFsAttributeInformation:
			FDbgPrint("  FileFsAttributeInformation\n");
			break;
	    
		case FileFsControlInformation:
			FDbgPrint("  FileFsControlInformation\n");
			break;
	    
		case FileFsFullSizeInformation:
			FDbgPrint("  FileFsFullSizeInformation\n");
			break;
		case FileFsObjectIdInformation:
			FDbgPrint("  FileFsObjectIdInformation\n");
			break;
	    
		case FileFsMaximumInformation:
			FDbgPrint("  FileFsMaximumInformation\n");
			break;
		default:
			break;
		}

		if (irpSp->Parameters.QueryVolume.FsInformationClass == FileFsVolumeInformation
			|| irpSp->Parameters.QueryVolume.FsInformationClass == FileFsSizeInformation
			|| irpSp->Parameters.QueryVolume.FsInformationClass == FileFsAttributeInformation
			|| irpSp->Parameters.QueryVolume.FsInformationClass == FileFsFullSizeInformation) {


			ULONG			eventLength = sizeof(EVENT_CONTEXT);
			PEVENT_CONTEXT	eventContext;

			if (ccb && !FuserCheckCCB(vcb->Dcb, fileObject->FsContext2)) {
				status = STATUS_INVALID_PARAMETER;
				__leave;
			}

			// this memory must be freed in this {}
			eventContext = AllocateEventContext(vcb->Dcb, Irp, eventLength, NULL);

			if (eventContext == NULL) {
				status = STATUS_INSUFFICIENT_RESOURCES;
				__leave;
			}
		
			if (ccb) {
				eventContext->Context = ccb->UserContext;
				eventContext->FileFlags = ccb->Flags;
				//FDbgPrint("   get Context %X\n", (ULONG)ccb->UserContext);
			}

			eventContext->Volume.FsInformationClass =
				irpSp->Parameters.QueryVolume.FsInformationClass;

			// the length which can be returned to user-mode
			eventContext->Volume.BufferLength = irpSp->Parameters.QueryVolume.Length;


			status = FuserRegisterPendingIrp(DeviceObject, Irp, eventContext, 0);
		}

	} __finally {

		if (status != STATUS_PENDING) {
			Irp->IoStatus.Status = status;
			Irp->IoStatus.Information = info;
			IoCompleteRequest(Irp, IO_NO_INCREMENT);
			FuserPrintNTStatus(status);
		}

		FDbgPrint("<== FuserQueryVolumeInformation\n");

		FsRtlExitFileSystem();
	}
	return status;
}




VOID
FuserCompleteQueryVolumeInformation(
	__in PIRP_ENTRY			IrpEntry,
	__in PEVENT_INFORMATION	EventInfo
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

	FDbgPrint("==> FuserCompleteQueryVolumeInformation\n");

	irp = IrpEntry->Irp;
	irpSp = IrpEntry->IrpSp;

	ccb = IrpEntry->FileObject->FsContext2;

	//ASSERT(ccb != NULL);

	// does not save Context!!
	// ccb->UserContext = EventInfo->Context;

	// buffer which is used to copy VolumeInfo
	buffer = irp->AssociatedIrp.SystemBuffer;

	// available buffer size to inform
	bufferLen = irpSp->Parameters.QueryVolume.Length;

	// if buffer is invalid or short of length
	if (bufferLen == 0 || buffer == NULL || bufferLen < EventInfo->BufferLength) {

		info   = 0;
		status = STATUS_INSUFFICIENT_RESOURCES;

	} else {

		// copy the information from user-mode to specified buffer
		ASSERT(buffer != NULL);
		
		RtlZeroMemory(buffer, bufferLen);
		RtlCopyMemory(buffer, EventInfo->Buffer, EventInfo->BufferLength);

		// the written length
		info = EventInfo->BufferLength;

		status = EventInfo->Status;
	}


	irp->IoStatus.Status = status;
	irp->IoStatus.Information = info;
	IoCompleteRequest(irp, IO_NO_INCREMENT);

	FuserPrintNTStatus(status);
	FDbgPrint("<== FuserCompleteQueryVolumeInformation\n");

	//FsRtlExitFileSystem();
}



NTSTATUS
FuserDispatchSetVolumeInformation(
	__in PDEVICE_OBJECT DeviceObject,
	__in PIRP Irp
   )
{
// TODO: Add support for changing the volume label
	NTSTATUS status = STATUS_INVALID_PARAMETER;

	PAGED_CODE();

	//FsRtlEnterFileSystem();

	FDbgPrint("==> FuserSetVolumeInformation\n");

	Irp->IoStatus.Status = status;
	Irp->IoStatus.Information = 0;
	IoCompleteRequest(Irp, IO_NO_INCREMENT);

	FDbgPrint("<== FuserSetVolumeInformation");

	//FsRtlExitFileSystem();

	return status;
}


