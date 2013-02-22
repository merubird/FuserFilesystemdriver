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

VOID
FuserUnmount( // TODO: does not belong in the timeout-file in my opinion
	__in PFuserDCB Dcb
	)
{
	ULONG					eventLength;
	PEVENT_CONTEXT			eventContext;
	PDRIVER_EVENT_CONTEXT	driverEventContext;	
	PKEVENT					completedEvent;
	LARGE_INTEGER			timeout;
	PFuserVCB				vcb = Dcb->Vcb;
	ULONG					deviceNamePos;

	eventLength = sizeof(EVENT_CONTEXT);
	eventContext = AllocateEventContextRaw(eventLength);

	if (eventContext == NULL) {
		;//STATUS_INSUFFICIENT_RESOURCES;
		FuserEventRelease(vcb->DeviceObject);
		return;
	}

	driverEventContext =
		CONTAINING_RECORD(eventContext, DRIVER_EVENT_CONTEXT, EventContext);
	completedEvent = ExAllocatePool(sizeof(KEVENT));
	if (completedEvent) {
		KeInitializeEvent(completedEvent, NotificationEvent, FALSE);
		driverEventContext->Completed = completedEvent;
	}

	deviceNamePos = Dcb->SymbolicLinkName->Length / sizeof(WCHAR) - 1;
	for (; Dcb->SymbolicLinkName->Buffer[deviceNamePos] != L'\\'; --deviceNamePos)
		;
	RtlStringCchCopyW(eventContext->Unmount.DeviceName,
			sizeof(eventContext->Unmount.DeviceName) / sizeof(WCHAR),
			&(Dcb->SymbolicLinkName->Buffer[deviceNamePos]));

	FDbgPrint("  Send Unmount to Service : %ws\n", eventContext->Unmount.DeviceName);

	FuserEventNotification(&Dcb->Global->NotifyService, eventContext);
	if (completedEvent) {
		timeout.QuadPart = -1 * 10 * 1000 * 10; // 10 sec
		KeWaitForSingleObject(completedEvent, Executive, KernelMode, FALSE, &timeout);
	}

	FuserEventRelease(vcb->DeviceObject);

	if (completedEvent) {
		ExFreePool(completedEvent);
	}
}




NTSTATUS
ReleaseTimeoutPendingIrp(
   __in PFuserDCB	Dcb
   )
{
	KIRQL				oldIrql;
    PLIST_ENTRY			thisEntry, nextEntry, listHead;
	PIRP_ENTRY			irpEntry;
	LARGE_INTEGER		tickCount;
	LIST_ENTRY			completeList;
	PIRP				irp;

	FDbgPrint("==> ReleaseTimeoutPendingIRP\n");
	InitializeListHead(&completeList);

	ASSERT(KeGetCurrentIrql() <= DISPATCH_LEVEL);
	KeAcquireSpinLock(&Dcb->PendingIrp.ListLock, &oldIrql);

	// when IRP queue is empty, there is nothing to do
	if (IsListEmpty(&Dcb->PendingIrp.ListHead)) {
		KeReleaseSpinLock(&Dcb->PendingIrp.ListLock, oldIrql);
		FDbgPrint("  IrpQueue is Empty\n");
		return STATUS_SUCCESS;
	}

	KeQueryTickCount(&tickCount);

	// search timeout IRP through pending IRP list
	listHead = &Dcb->PendingIrp.ListHead;
    for (thisEntry = listHead->Flink;
		thisEntry != listHead;
		thisEntry = nextEntry) {

        nextEntry = thisEntry->Flink;

        irpEntry = CONTAINING_RECORD(thisEntry, IRP_ENTRY, ListEntry);

		// this IRP is NOT timeout yet
		if (tickCount.QuadPart < irpEntry->TickCount.QuadPart) {
			break;
		}

		RemoveEntryList(thisEntry);

		FDbgPrint(" timeout Irp #%X\n", irpEntry->SerialNumber);

		irp = irpEntry->Irp;

		if (irp == NULL) {
			// this IRP has already been canceled
			ASSERT(irpEntry->CancelRoutineFreeMemory == FALSE);
			FuserFreeIrpEntry(irpEntry);
			continue;
		}

		// this IRP is not canceled yet
		if (IoSetCancelRoutine(irp, NULL) == NULL) {
			// Cancel routine will run as soon as we release the lock
			InitializeListHead(&irpEntry->ListEntry);
			irpEntry->CancelRoutineFreeMemory = TRUE;
			continue;
		}
		// IrpEntry is saved here for CancelRoutine
		// Clear it to prevent to be completed by CancelRoutine twice
		irp->Tail.Overlay.DriverContext[DRIVER_CONTEXT_IRP_ENTRY] = NULL;
		InsertTailList(&completeList, &irpEntry->ListEntry);
	}
	if (IsListEmpty(&Dcb->PendingIrp.ListHead)) {
		KeClearEvent(&Dcb->PendingIrp.NotEmpty);
	}
	KeReleaseSpinLock(&Dcb->PendingIrp.ListLock, oldIrql);
	
	while (!IsListEmpty(&completeList)) {
		listHead = RemoveHeadList(&completeList);
		irpEntry = CONTAINING_RECORD(listHead, IRP_ENTRY, ListEntry);
		irp = irpEntry->Irp;
		irp->IoStatus.Information = 0;
		irp->IoStatus.Status = STATUS_INSUFFICIENT_RESOURCES;
		FuserFreeIrpEntry(irpEntry);
		IoCompleteRequest(irp, IO_NO_INCREMENT);
	}

	FDbgPrint("<== ReleaseTimeoutPendingIRP\n");
	
	return STATUS_SUCCESS;
}

VOID
FuserCheckKeepAlive(
	__in PFuserDCB	Dcb
	)
{
	LARGE_INTEGER		tickCount;
	ULONG				eventLength;
	PEVENT_CONTEXT		eventContext;
	ULONG				mounted;
	PFuserVCB			vcb = Dcb->Vcb;

	//FDbgPrint("==> FuserCheckKeepAlive\n");

	KeEnterCriticalRegion();
	KeQueryTickCount(&tickCount);
	ExAcquireResourceSharedLite(&Dcb->Resource, TRUE);

	if (Dcb->TickCount.QuadPart < tickCount.QuadPart) {

		mounted = Dcb->Mounted;

		ExReleaseResourceLite(&Dcb->Resource);

		FDbgPrint("  Timeout, force to umount\n");

		if (!mounted) {
			// not mounted
			KeLeaveCriticalRegion();
			return;
		}
		FuserUnmount(Dcb);

	} else {
		ExReleaseResourceLite(&Dcb->Resource);
	}	
	KeLeaveCriticalRegion();
	//FDbgPrint("<== FuserCheckKeepAlive\n");
}




KSTART_ROUTINE FuserTimeoutThread;
VOID
FuserTimeoutThread(
	PFuserDCB	Dcb)
/*++

Routine Description:

	checks wheter pending IRP is timeout or not each FUSER_CHECK_INTERVAL

--*/
{
	NTSTATUS		status;
	KTIMER			timer;
	PVOID			pollevents[2];
	LARGE_INTEGER	timeout = {0};
	FDbgPrint("==> FuserTimeoutThread\n");

	KeInitializeTimerEx(&timer, SynchronizationTimer);
	
	pollevents[0] = (PVOID)&Dcb->KillEvent;
	pollevents[1] = (PVOID)&timer;

	KeSetTimerEx(&timer, timeout, FUSER_CHECK_INTERVAL, NULL);
	while (TRUE) {
		status = KeWaitForMultipleObjects(2, pollevents, WaitAny,
			Executive, KernelMode, FALSE, NULL, NULL);
		
		if (!NT_SUCCESS(status) || status ==  STATUS_WAIT_0) {
			FDbgPrint("  FuserTimeoutThread catched KillEvent\n");
			// KillEvent or something error is occured
			break;
		}

		ReleaseTimeoutPendingIrp(Dcb);
		if (Dcb->UseKeepAlive)
			FuserCheckKeepAlive(Dcb);
	}

	KeCancelTimer(&timer);

	FDbgPrint("<== FuserTimeoutThread\n");

	PsTerminateSystemThread(STATUS_SUCCESS);
}



NTSTATUS
FuserStartCheckThread(
	__in PFuserDCB	Dcb)
/*++

Routine Description:

	execute FuserTimeoutThread

--*/
{
	NTSTATUS status;
	HANDLE	thread;

	FDbgPrint("==> FuserStartCheckThread\n");

	status = PsCreateSystemThread(&thread, THREAD_ALL_ACCESS,
		NULL, NULL, NULL, (PKSTART_ROUTINE)FuserTimeoutThread, Dcb);

	if (!NT_SUCCESS(status)) {
		return status;
	}

	ObReferenceObjectByHandle(thread, THREAD_ALL_ACCESS, NULL,
		KernelMode, (PVOID*)&Dcb->TimeoutThread, NULL);

	ZwClose(thread);

	FDbgPrint("<== FuserStartCheckThread\n");

	return STATUS_SUCCESS;
}






NTSTATUS
FuserResetPendingIrpTimeout(
   __in PDEVICE_OBJECT	DeviceObject,
   __in PIRP			Irp
   )
{
	KIRQL				oldIrql;
    PLIST_ENTRY			thisEntry, nextEntry, listHead;
	PIRP_ENTRY			irpEntry;
	PFuserVCB			vcb;
	PEVENT_INFORMATION	eventInfo;
	ULONG				timeout; // in milisecond


	FDbgPrint("==> ResetPendingIrpTimeout\n");

	eventInfo		= (PEVENT_INFORMATION)Irp->AssociatedIrp.SystemBuffer;
	ASSERT(eventInfo != NULL);

	timeout = eventInfo->ResetTimeout.Timeout;
	if (FUSER_IRP_PENDING_TIMEOUT_RESET_MAX < timeout) {
		timeout = FUSER_IRP_PENDING_TIMEOUT_RESET_MAX;
	}
	
	vcb = DeviceObject->DeviceExtension;
	if (GetIdentifierType(vcb) != VCB) {
		return STATUS_INVALID_PARAMETER;
	}
	ASSERT(KeGetCurrentIrql() <= DISPATCH_LEVEL);
	KeAcquireSpinLock(&vcb->Dcb->PendingIrp.ListLock, &oldIrql);

	// search corresponding IRP through pending IRP list
	listHead = &vcb->Dcb->PendingIrp.ListHead;
    for (thisEntry = listHead->Flink; thisEntry != listHead; thisEntry = nextEntry) {

		PIRP				irp;
		PIO_STACK_LOCATION	irpSp;

        nextEntry = thisEntry->Flink;

        irpEntry = CONTAINING_RECORD(thisEntry, IRP_ENTRY, ListEntry);

		if (irpEntry->SerialNumber != eventInfo->SerialNumber)  {
			continue;
		}

		FuserUpdateTimeout(&irpEntry->TickCount, timeout);
		break;
	}
	KeReleaseSpinLock(&vcb->Dcb->PendingIrp.ListLock, oldIrql);
	FDbgPrint("<== ResetPendingIrpTimeout\n");
		
	return STATUS_SUCCESS;
}



VOID
FuserUpdateTimeout(
	__out PLARGE_INTEGER TickCount,
	__in ULONG	Timeout
	)
{
	KeQueryTickCount(TickCount);	
	TickCount->QuadPart += Timeout * 1000 * 10 / KeQueryTimeIncrement();
}




VOID
FuserStopCheckThread(
	__in PFuserDCB	Dcb)
/*++

Routine Description:

	exits FuserTimeoutThread

--*/
{
	FDbgPrint("==> FuserStopCheckThread\n");
	
	KeSetEvent(&Dcb->KillEvent, 0, FALSE);

	if (Dcb->TimeoutThread) {
		KeWaitForSingleObject(Dcb->TimeoutThread, Executive,
			KernelMode, FALSE, NULL);
		ObDereferenceObject(Dcb->TimeoutThread);
		Dcb->TimeoutThread = NULL;
	}
	
	FDbgPrint("<== FuserStopCheckThread\n");
}



/* TODO: unused:
NTSTATUS
FuserInformServiceAboutUnmount(
   __in PDEVICE_OBJECT	DeviceObject,
   __in PIRP			Irp)
{

	return STATUS_SUCCESS;
}
*/