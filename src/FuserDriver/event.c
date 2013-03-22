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
RegisterPendingIrpMain(
    __in PDEVICE_OBJECT DeviceObject,
    __in PIRP			Irp,
	__in ULONG			SerialNumber,
	__in PIRP_LIST		IrpList,
	__in ULONG			Flags,
	__in ULONG			CheckMount
    )
{
 	PIRP_ENTRY			irpEntry;
    PIO_STACK_LOCATION	irpSp;
    KIRQL				oldIrql;



	//FDbgPrint("==> FuserRegisterPendingIrpMain\n");

	if (GetIdentifierType(DeviceObject->DeviceExtension) == VCB) {
		PFuserVCB vcb = DeviceObject->DeviceExtension;
		if (CheckMount && !vcb->Dcb->Mounted) {
			FDbgPrint(" device is not mounted\n");
			return STATUS_INSUFFICIENT_RESOURCES;
		}
	}
 
    irpSp = IoGetCurrentIrpStackLocation(Irp);
 
    // Allocate a record and save all the event context.
    irpEntry = FuserAllocateIrpEntry();

    if (NULL == irpEntry) {
		FDbgPrint("  can't allocate IRP_ENTRY\n");
        return  STATUS_INSUFFICIENT_RESOURCES;
    }

	RtlZeroMemory(irpEntry, sizeof(IRP_ENTRY));

    InitializeListHead(&irpEntry->ListEntry);

	irpEntry->SerialNumber		= SerialNumber;
    irpEntry->FileObject		= irpSp->FileObject;
    irpEntry->Irp				= Irp;
	irpEntry->IrpSp				= irpSp;
	irpEntry->IrpList			= IrpList;
	irpEntry->Flags				= Flags;

	FuserUpdateTimeout(&irpEntry->TickCount, FUSER_IRP_PENDING_TIMEOUT);

	//FDbgPrint("  Lock IrpList.ListLock\n");
	ASSERT(KeGetCurrentIrql() <= DISPATCH_LEVEL);
    KeAcquireSpinLock(&IrpList->ListLock, &oldIrql);

    IoSetCancelRoutine(Irp, FuserIrpCancelRoutine);

    if (Irp->Cancel) {
        if (IoSetCancelRoutine(Irp, NULL) != NULL) {
			//FDbgPrint("  Release IrpList.ListLock %d\n", __LINE__);
            KeReleaseSpinLock(&IrpList->ListLock, oldIrql);

            FuserFreeIrpEntry(irpEntry);

            return STATUS_CANCELLED;
        }
	}

    IoMarkIrpPending(Irp);

    InsertTailList(&IrpList->ListHead, &irpEntry->ListEntry);

    irpEntry->CancelRoutineFreeMemory = FALSE;

	// save the pointer in order to be accessed by cancel routine
	Irp->Tail.Overlay.DriverContext[DRIVER_CONTEXT_IRP_ENTRY] =  irpEntry;


	KeSetEvent(&IrpList->NotEmpty, IO_NO_INCREMENT, FALSE);

	//FDbgPrint("  Release IrpList.ListLock\n");
    KeReleaseSpinLock(&IrpList->ListLock, oldIrql);

	//FDbgPrint("<== FuserRegisterPendingIrpMain\n");
    return STATUS_PENDING;;
}





NTSTATUS
FuserRegisterPendingIrp(
    __in PDEVICE_OBJECT DeviceObject,
    __in PIRP			Irp,
	__in PEVENT_CONTEXT	EventContext,
	__in ULONG			Flags
    )
{
	PFuserVCB vcb = DeviceObject->DeviceExtension;
	NTSTATUS status;

	if (GetIdentifierType(vcb) != VCB) {
		DbgPrint("  Type != VCB\n");
		return STATUS_INVALID_PARAMETER;
	}

	status = RegisterPendingIrpMain(
		DeviceObject,
		Irp,
		EventContext->SerialNumber,
		&vcb->Dcb->PendingIrp,
		Flags,
		TRUE);

	if (status == STATUS_PENDING) {
		FuserEventNotification(&vcb->Dcb->NotifyEvent, EventContext);
	} else {
		FuserFreeEventContext(EventContext);
	}
	return status;
}



VOID
FuserIrpCancelRoutine(
    __in PDEVICE_OBJECT   DeviceObject,
    __in PIRP             Irp
    )
{
    KIRQL               oldIrql;
    PIRP_ENTRY			irpEntry;
	ULONG				serialNumber;
	PIO_STACK_LOCATION	irpSp;

    FDbgPrint("==> FuserIrpCancelRoutine\n");

    // Release the cancel spinlock
    IoReleaseCancelSpinLock(Irp->CancelIrql);

    irpEntry = Irp->Tail.Overlay.DriverContext[DRIVER_CONTEXT_IRP_ENTRY];
	if (irpEntry != NULL) {
		PKSPIN_LOCK	lock = &irpEntry->IrpList->ListLock;
		// Acquire the queue spinlock
		ASSERT(KeGetCurrentIrql() <= DISPATCH_LEVEL);
		KeAcquireSpinLock(lock, &oldIrql);

		irpSp = IoGetCurrentIrpStackLocation(Irp);
		ASSERT(irpSp != NULL);

		serialNumber = irpEntry->SerialNumber;

		RemoveEntryList(&irpEntry->ListEntry);

		// If Write is canceld before completion and buffer that saves writing
		// content is not freed, free it here
		if (irpSp->MajorFunction == IRP_MJ_WRITE) {
			PVOID eventContext = Irp->Tail.Overlay.DriverContext[DRIVER_CONTEXT_EVENT];
			if (eventContext != NULL) {
				FuserFreeEventContext(eventContext);
			}
			Irp->Tail.Overlay.DriverContext[DRIVER_CONTEXT_EVENT] = NULL;
		}

		if (IsListEmpty(&irpEntry->IrpList->ListHead)) {
			//FDbgPrint("    list is empty ClearEvent\n");
			KeClearEvent(&irpEntry->IrpList->NotEmpty);
		}

		irpEntry->Irp = NULL;
		if (irpEntry->CancelRoutineFreeMemory == FALSE) {
			InitializeListHead(&irpEntry->ListEntry);
		} else {
			FuserFreeIrpEntry(irpEntry);
			irpEntry = NULL;
		}

		Irp->Tail.Overlay.DriverContext[DRIVER_CONTEXT_IRP_ENTRY] = NULL; 

		KeReleaseSpinLock(lock, oldIrql);
	}

	FDbgPrint("   canceled IRP #%X\n", serialNumber);
    Irp->IoStatus.Status = STATUS_CANCELLED;
    Irp->IoStatus.Information = 0;
    IoCompleteRequest(Irp, IO_NO_INCREMENT);

	FDbgPrint("<== FuserIrpCancelRoutine\n");
    return;

}


NTSTATUS
FuserRegisterPendingIrpForEvent(
    __in PDEVICE_OBJECT DeviceObject,
    __in PIRP			Irp
    )
{
	PFuserVCB vcb = DeviceObject->DeviceExtension;

	if (GetIdentifierType(vcb) != VCB) {
		FDbgPrint("  Type != VCB\n");
		return STATUS_INVALID_PARAMETER;
	}

	//FDbgPrint("FuserRegisterPendingIrpForEvent\n");

	return RegisterPendingIrpMain(
		DeviceObject,
		Irp,
		0, // SerialNumber
		&vcb->Dcb->PendingEvent,
		0, // Flags
		TRUE);
}


NTSTATUS
FuserRegisterPendingIrpForService(
	__in PDEVICE_OBJECT	DeviceObject,
	__in PIRP			Irp
	)
{
	PFUSER_GLOBAL fuserGlobal;
	FDbgPrint("FuserRegisterPendingIrpForService\n");

	fuserGlobal = DeviceObject->DeviceExtension;
	if (GetIdentifierType(fuserGlobal) != DGL) {
		return STATUS_INVALID_PARAMETER;
	}

	return RegisterPendingIrpMain(
		DeviceObject,
		Irp,
		0, // SerialNumber
		&fuserGlobal->PendingService,
		0, // Flags
		FALSE);
}





// When user-mode file system application returns EventInformation,
// search corresponding pending IRP and complete it
NTSTATUS
FuserCompleteIrp(
    __in PDEVICE_OBJECT DeviceObject,
    __in PIRP Irp
	)
{
	KIRQL				oldIrql;
    PLIST_ENTRY			thisEntry, nextEntry, listHead;
	PIRP_ENTRY			irpEntry;
	PFuserVCB			vcb;
	PEVENT_INFORMATION	eventInfo;

	eventInfo		= (PEVENT_INFORMATION)Irp->AssociatedIrp.SystemBuffer;
	ASSERT(eventInfo != NULL);
	
	//FDbgPrint("==> FuserCompleteIrp [EventInfo #%X]\n", eventInfo->SerialNumber);

	vcb = DeviceObject->DeviceExtension;
	if (GetIdentifierType(vcb) != VCB) {
		return STATUS_INVALID_PARAMETER;
	}

	//FDbgPrint("      Lock IrpList.ListLock\n");
	ASSERT(KeGetCurrentIrql() <= DISPATCH_LEVEL);
	KeAcquireSpinLock(&vcb->Dcb->PendingIrp.ListLock, &oldIrql);

	// search corresponding IRP through pending IRP list
	listHead = &vcb->Dcb->PendingIrp.ListHead;

    for (thisEntry = listHead->Flink; thisEntry != listHead; thisEntry = nextEntry) {

		PIRP				irp;
		PIO_STACK_LOCATION	irpSp;

        nextEntry = thisEntry->Flink;

        irpEntry = CONTAINING_RECORD(thisEntry, IRP_ENTRY, ListEntry);

		// check whether this is corresponding IRP

        //FDbgPrint("SerialNumber irpEntry %X eventInfo %X\n", irpEntry->SerialNumber, eventInfo->SerialNumber);

		// this irpEntry must be freed in this if statement
		if (irpEntry->SerialNumber != eventInfo->SerialNumber)  {
			continue;
		}

		RemoveEntryList(thisEntry);

		irp = irpEntry->Irp;
	
		if (irp == NULL) {
			// this IRP is already canceled
			ASSERT(irpEntry->CancelRoutineFreeMemory == FALSE);
			FuserFreeIrpEntry(irpEntry);
			irpEntry = NULL;
			break;
		}

		if (IoSetCancelRoutine(irp, NULL) == NULL) {
			// Cancel routine will run as soon as we release the lock
			InitializeListHead(&irpEntry->ListEntry);
			irpEntry->CancelRoutineFreeMemory = TRUE;
			break;
		}

		// IRP is not canceled yet
		irpSp = irpEntry->IrpSp;	
		
		ASSERT(irpSp != NULL);
					
		// IrpEntry is saved here for CancelRoutine
		// Clear it to prevent to be completed by CancelRoutine twice
		irp->Tail.Overlay.DriverContext[DRIVER_CONTEXT_IRP_ENTRY] = NULL;
		KeReleaseSpinLock(&vcb->Dcb->PendingIrp.ListLock, oldIrql);

		switch (irpSp->MajorFunction) {
		case IRP_MJ_DIRECTORY_CONTROL:
			FuserCompleteDirectoryControl(irpEntry, eventInfo);
			break;
		case IRP_MJ_READ:
			FuserCompleteRead(irpEntry, eventInfo);
			break;
		case IRP_MJ_WRITE:
			FuserCompleteWrite(irpEntry, eventInfo);
			break;
		case IRP_MJ_QUERY_INFORMATION:
			FuserCompleteQueryInformation(irpEntry, eventInfo);
			break;
		case IRP_MJ_QUERY_VOLUME_INFORMATION:
			FuserCompleteQueryVolumeInformation(irpEntry, eventInfo);
			break;
		case IRP_MJ_CREATE:
			FuserCompleteCreate(irpEntry, eventInfo);
			break;
		case IRP_MJ_CLEANUP:
			FuserCompleteCleanup(irpEntry, eventInfo);
			break;
		case IRP_MJ_LOCK_CONTROL:
			FuserCompleteLock(irpEntry, eventInfo);
			break;
		case IRP_MJ_SET_INFORMATION:
			FuserCompleteSetInformation(irpEntry, eventInfo);
			break;
		case IRP_MJ_FLUSH_BUFFERS:
			FuserCompleteFlush(irpEntry, eventInfo);
			break;
		case IRP_MJ_QUERY_SECURITY:
			FuserCompleteQuerySecurity(irpEntry, eventInfo);
			break;
		case IRP_MJ_SET_SECURITY:
			FuserCompleteSetSecurity(irpEntry, eventInfo);
			break;

		default:
			FDbgPrint("Unknown IRP %d\n", irpSp->MajorFunction);
			// TODO: in this case, should complete this IRP
			break;
		}		

		FuserFreeIrpEntry(irpEntry);
		irpEntry = NULL;

		return STATUS_SUCCESS;
	}

	KeReleaseSpinLock(&vcb->Dcb->PendingIrp.ListLock, oldIrql);

    //FDbgPrint("<== AACompleteIrp [EventInfo #%X]\n", eventInfo->SerialNumber);

	// TODO: should return error	
    return STATUS_SUCCESS;
}




// start event dispatching
NTSTATUS
FuserEventStart(
    __in PDEVICE_OBJECT DeviceObject,
    __in PIRP Irp
   )
{ // TODO: Revise:
	ULONG				outBufferLen;
	ULONG				inBufferLen;
	PVOID				buffer;
	PIO_STACK_LOCATION	irpSp;
	EVENT_START			eventStart;
	PEVENT_DRIVER_INFO	driverInfo;
	PFUSER_GLOBAL		fuserGlobal;
	PFuserDCB			dcb;
	NTSTATUS			status;
	DEVICE_TYPE			deviceType;
	ULONG				deviceCharacteristics;
	WCHAR				baseGuidString[64];
	GUID				baseGuid = FUSER_BASE_GUID;
	UNICODE_STRING		unicodeGuid;
	ULONG				deviceNamePos;


	FDbgPrint("==> FuserEventStart\n");

	fuserGlobal = DeviceObject->DeviceExtension;
	if (GetIdentifierType(fuserGlobal) != DGL) { // TODO: DGL must be replaced with FGL or something else clever
		return STATUS_INVALID_PARAMETER;
	}

	irpSp = IoGetCurrentIrpStackLocation(Irp);

	outBufferLen = irpSp->Parameters.DeviceIoControl.OutputBufferLength;
	inBufferLen = irpSp->Parameters.DeviceIoControl.InputBufferLength;

	if (outBufferLen != sizeof(EVENT_DRIVER_INFO) ||
		inBufferLen != sizeof(EVENT_START)) {
		return STATUS_INSUFFICIENT_RESOURCES;
	}

	RtlCopyMemory(&eventStart, Irp->AssociatedIrp.SystemBuffer, sizeof(EVENT_START));
	driverInfo = Irp->AssociatedIrp.SystemBuffer;

	if (eventStart.Version != GetBinaryVersion() ) {
		driverInfo->Version = GetBinaryVersion();
		driverInfo->Status = FUSER_START_FAILED;
		Irp->IoStatus.Status = STATUS_SUCCESS;
		Irp->IoStatus.Information = sizeof(EVENT_DRIVER_INFO);
		return STATUS_SUCCESS;
	}

	deviceCharacteristics = FILE_DEVICE_IS_MOUNTED;

	// TODO: cleanup the different name prefixes for parameters
	/* TODO: remove network file system support
	switch (eventStart.DeviceType) {
	case FUSER_DISK_FILE_SYSTEM:
		deviceType = FILE_DEVICE_DISK_FILE_SYSTEM;
		break;
	case FUSER_NETWORK_FILE_SYSTEM:
		deviceType = FILE_DEVICE_NETWORK_FILE_SYSTEM;
		deviceCharacteristics |= FILE_REMOTE_DEVICE;
		break;
	default:
		FDbgPrint("  Unknown device type: %d\n", eventStart.DeviceType);
		deviceType = FILE_DEVICE_DISK_FILE_SYSTEM;
	}
	*/
	deviceType = FILE_DEVICE_DISK_FILE_SYSTEM;
	

	if (eventStart.Flags & FUSER_EVENT_REMOVABLE) {
		FDbgPrint("  DeviceCharacteristics |= FILE_REMOVABLE_MEDIA\n");
		deviceCharacteristics |= FILE_REMOVABLE_MEDIA;
	}
	
	baseGuid.Data1 ^= fuserGlobal->MountId;

	status = RtlStringFromGUID(&baseGuid, &unicodeGuid);
	if (!NT_SUCCESS(status)) {
		return status;
	}
	RtlZeroMemory(baseGuidString, sizeof(baseGuidString));
	RtlStringCchCopyW(baseGuidString, sizeof(baseGuidString) / sizeof(WCHAR), unicodeGuid.Buffer);
	RtlFreeUnicodeString(&unicodeGuid);

	InterlockedIncrement(&fuserGlobal->MountId); // TODO: Only limited number of mounts possible, this problem should be solved.
	

	KeEnterCriticalRegion();
	ExAcquireResourceExclusiveLite(&fuserGlobal->Resource, TRUE);

	status = FuserCreateDiskDevice(
				DeviceObject->DriverObject,
				fuserGlobal->MountId,
				baseGuidString,
				fuserGlobal,
				deviceType,
				deviceCharacteristics,
				&dcb);

	if (!NT_SUCCESS(status)) {
		ExReleaseResourceLite(&fuserGlobal->Resource);
		KeLeaveCriticalRegion();
		return status;
	}

	FDbgPrint("  MountId:%d\n", dcb->MountId);
	driverInfo->DeviceNumber = fuserGlobal->MountId;
	driverInfo->MountId = fuserGlobal->MountId;
	driverInfo->Status = FUSER_MOUNTED;
	driverInfo->Version = GetBinaryVersion();

	// SymbolicName is \\DosDevices\\Global\\FuserDevice{b9892757-6a70-4e51-9eaf-9ef1e093b0e8}
	// Finds the last '\' and copy into DeviceName.
	// DeviceName is \FuserDevice{b9892757-6a70-4e51-9eaf-9ef1e093b0e8}
	deviceNamePos = dcb->SymbolicLinkName->Length / sizeof(WCHAR) - 1;
	for (; dcb->SymbolicLinkName->Buffer[deviceNamePos] != L'\\'; --deviceNamePos)
		;
	RtlStringCchCopyW(driverInfo->DeviceName,
			sizeof(driverInfo->DeviceName) / sizeof(WCHAR),
			&(dcb->SymbolicLinkName->Buffer[deviceNamePos]));

	FDbgPrint("  DeviceName:%ws\n", driverInfo->DeviceName);
	FuserUpdateTimeout(&dcb->TickCount, FUSER_KEEPALIVE_TIMEOUT);

	dcb->UseAltStream = 0;
	if (eventStart.Flags & FUSER_EVENT_ALTERNATIVE_STREAM_ON) {
		FDbgPrint("  ALT_STREAM_ON\n");
		dcb->UseAltStream = 1;
	}
	dcb->UseKeepAlive = 0;
	if (eventStart.Flags & FUSER_EVENT_KEEP_ALIVE_ON) {
		FDbgPrint("  KEEP_ALIVE_ON\n");
		dcb->UseKeepAlive = 1;
	}
	dcb->Mounted = 1;
	

	FuserStartEventNotificationThread(dcb);
	FuserStartCheckThread(dcb);

	ExReleaseResourceLite(&fuserGlobal->Resource);
	KeLeaveCriticalRegion();
	Irp->IoStatus.Status = STATUS_SUCCESS;
	Irp->IoStatus.Information = sizeof(EVENT_DRIVER_INFO);

	FDbgPrint("<== FuserEventStart\n");

	return Irp->IoStatus.Status;
}



// user assinged bigger buffer that is enough to return WriteEventContext
NTSTATUS
FuserEventWrite(
    __in PDEVICE_OBJECT DeviceObject,
    __in PIRP Irp
	)
{
	KIRQL				oldIrql;
    PLIST_ENTRY			thisEntry, nextEntry, listHead;
	PIRP_ENTRY			irpEntry;
    PFuserVCB			vcb;
	PEVENT_INFORMATION	eventInfo;
	PIRP				writeIrp;

	eventInfo		= (PEVENT_INFORMATION)Irp->AssociatedIrp.SystemBuffer; // TODO: Build in security whether buffer is large enough (like: IOCTL_DISK_GET_DRIVE_GEOMETRY)
	ASSERT(eventInfo != NULL);
	
	FDbgPrint("==> FuserEventWrite [EventInfo #%X]\n", eventInfo->SerialNumber);

	vcb = DeviceObject->DeviceExtension;

	if (GetIdentifierType(vcb) != VCB) {
		return STATUS_INVALID_PARAMETER;
	}

	ASSERT(KeGetCurrentIrql() <= DISPATCH_LEVEL);
	KeAcquireSpinLock(&vcb->Dcb->PendingIrp.ListLock, &oldIrql);

	// search corresponding write IRP through pending IRP list
	listHead = &vcb->Dcb->PendingIrp.ListHead;

    for (thisEntry = listHead->Flink; thisEntry != listHead; thisEntry = nextEntry) {
		PIO_STACK_LOCATION writeIrpSp, eventIrpSp;
		PEVENT_CONTEXT	eventContext;
		ULONG			info = 0;
		NTSTATUS		status;

		nextEntry = thisEntry->Flink;
        irpEntry = CONTAINING_RECORD(thisEntry, IRP_ENTRY, ListEntry);

		// check whehter this is corresponding IRP

        //FDbgPrint("SerialNumber irpEntry %X eventInfo %X\n", irpEntry->SerialNumber, eventInfo->SerialNumber);

		if (irpEntry->SerialNumber != eventInfo->SerialNumber)  {
			continue;
		}

		// do NOT free irpEntry here
		writeIrp = irpEntry->Irp;
		if (writeIrp == NULL) {
			// this IRP has already been canceled
			ASSERT(irpEntry->CancelRoutineFreeMemory == FALSE);
			FuserFreeIrpEntry(irpEntry);
			continue;
		}

		if (IoSetCancelRoutine(writeIrp, FuserIrpCancelRoutine) == NULL) {
		//if (IoSetCancelRoutine(writeIrp, NULL) != NULL) {
			// Cancel routine will run as soon as we release the lock
			InitializeListHead(&irpEntry->ListEntry);
			irpEntry->CancelRoutineFreeMemory = TRUE;
			continue;
		}

		writeIrpSp = irpEntry->IrpSp;
		eventIrpSp = IoGetCurrentIrpStackLocation(Irp);
			
		ASSERT(writeIrpSp != NULL);
		ASSERT(eventIrpSp != NULL);

		eventContext = (PEVENT_CONTEXT)writeIrp->Tail.Overlay.DriverContext[DRIVER_CONTEXT_EVENT];
		ASSERT(eventContext != NULL);

		// short of buffer length
		if (eventIrpSp->Parameters.DeviceIoControl.OutputBufferLength
			< eventContext->Length) {		
			FDbgPrint("  EventWrite: STATUS_INSUFFICIENT_RESOURCE\n");
			status =  STATUS_INSUFFICIENT_RESOURCES;
		} else {
			PVOID buffer;
			//FDbgPrint("  EventWrite CopyMemory\n");
			//FDbgPrint("  EventLength %d, BufLength %d\n", eventContext->Length,
			//			eventIrpSp->Parameters.DeviceIoControl.OutputBufferLength);
			if (Irp->MdlAddress)
				buffer = MmGetSystemAddressForMdlSafe(Irp->MdlAddress, NormalPagePriority);
			else
				buffer = Irp->AssociatedIrp.SystemBuffer;
					
			ASSERT(buffer != NULL);
			RtlCopyMemory(buffer, eventContext, eventContext->Length);
						
			info = eventContext->Length;
			status = STATUS_SUCCESS;
		}

		FuserFreeEventContext(eventContext);
		writeIrp->Tail.Overlay.DriverContext[DRIVER_CONTEXT_EVENT] = 0;

		KeReleaseSpinLock(&vcb->Dcb->PendingIrp.ListLock, oldIrql);

		Irp->IoStatus.Status = status;
		Irp->IoStatus.Information = info;

		// this IRP will be completed by caller function
		return Irp->IoStatus.Status;
	}

	KeReleaseSpinLock(&vcb->Dcb->PendingIrp.ListLock, oldIrql);

   return STATUS_SUCCESS;
}



