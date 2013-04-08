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

#include <mountdev.h>
#include <mountmgr.h>
#include <ntddvol.h>

#include "..\_general\version.h"



VOID
PrintUnknownDeviceIoctlCode(
	__in ULONG	IoctlCode
	)
{
	PCHAR baseCodeStr = "unknown";
	ULONG baseCode = DEVICE_TYPE_FROM_CTL_CODE(IoctlCode);
	ULONG functionCode = (IoctlCode & (~0xffffc003)) >> 2;

	FDbgPrint("   Unknown Code 0x%x\n", IoctlCode);

	switch (baseCode) {
	case IOCTL_STORAGE_BASE:
		baseCodeStr = "IOCTL_STORAGE_BASE";
		break;
	case IOCTL_DISK_BASE:
		baseCodeStr = "IOCTL_DISK_BASE";
		break;
	case IOCTL_VOLUME_BASE:
		baseCodeStr = "IOCTL_VOLUME_BASE";
		break;
	case MOUNTDEVCONTROLTYPE:
		baseCodeStr = "MOUNTDEVCONTROLTYPE";
		break;
	case MOUNTMGRCONTROLTYPE:
		baseCodeStr = "MOUNTMGRCONTROLTYPE";
		break;
	}
	FDbgPrint("   BaseCode: 0x%x(%s) FunctionCode 0x%x(%d)\n",
		baseCode, baseCodeStr, functionCode, functionCode);
}




NTSTATUS
DiskDeviceControl(
	__in PDEVICE_OBJECT DeviceObject,
	__in PIRP Irp
	)
{
	PIO_STACK_LOCATION	irpSp;
	PFuserDCB			dcb;
	NTSTATUS			status = STATUS_NOT_IMPLEMENTED;
	ULONG				outputLength = 0;

	FDbgPrint("   => FuserDiskDeviceControl\n");
	irpSp = IoGetCurrentIrpStackLocation(Irp);
	dcb = DeviceObject->DeviceExtension;
	outputLength = irpSp->Parameters.DeviceIoControl.OutputBufferLength;
	switch (irpSp->Parameters.DeviceIoControl.IoControlCode) {
	case IOCTL_DISK_GET_DRIVE_GEOMETRY:
		{
			PDISK_GEOMETRY	diskGeometry;
			ULONG		    length;

			FDbgPrint("  IOCTL_DISK_GET_DRIVE_GEOMETRY\n");
			if (outputLength < sizeof(DISK_GEOMETRY)) {
				status = STATUS_BUFFER_TOO_SMALL;
				Irp->IoStatus.Information = 0;
				break;
			}

			diskGeometry = (PDISK_GEOMETRY)Irp->AssociatedIrp.SystemBuffer;
			ASSERT(diskGeometry != NULL);

			length = 1024*1024*1024;
			diskGeometry->Cylinders.QuadPart = length / FUSER_SECTOR_SIZE / 32 / 2;
			diskGeometry->MediaType = FixedMedia;
			diskGeometry->TracksPerCylinder = 2;
			diskGeometry->SectorsPerTrack = 32;
			diskGeometry->BytesPerSector = FUSER_SECTOR_SIZE;

			status = STATUS_SUCCESS;
			Irp->IoStatus.Information = sizeof(DISK_GEOMETRY);
		}
		break;
	case IOCTL_DISK_GET_LENGTH_INFO:
		{
			PGET_LENGTH_INFORMATION getLengthInfo;

			FDbgPrint("  IOCTL_DISK_GET_LENGTH_INFO\n");
            
			if (outputLength < sizeof(GET_LENGTH_INFORMATION)) {
				status = STATUS_BUFFER_TOO_SMALL;
				Irp->IoStatus.Information = 0;
				break;
			}

			getLengthInfo = (PGET_LENGTH_INFORMATION) Irp->AssociatedIrp.SystemBuffer;
			ASSERT(getLengthInfo != NULL);

			getLengthInfo->Length.QuadPart = 1024*1024*500;
			status = STATUS_SUCCESS;
			Irp->IoStatus.Information = sizeof(GET_LENGTH_INFORMATION);
		}
		break;

	case IOCTL_DISK_GET_PARTITION_INFO:
		FDbgPrint("  IOCTL_DISK_GET_PARTITION_INFO\n");
		break;

	case IOCTL_DISK_GET_PARTITION_INFO_EX:
		FDbgPrint("  IOCTL_DISK_GET_PARTITION_INFO_EX\n");
		break;

	case IOCTL_DISK_IS_WRITABLE:
		FDbgPrint("  IOCTL_DISK_IS_WRITABLE\n");
		status = STATUS_SUCCESS;
		break;

	case IOCTL_DISK_MEDIA_REMOVAL:
		FDbgPrint("  IOCTL_DISK_MEDIA_REMOVAL\n");
		status = STATUS_SUCCESS;
		break;

	case IOCTL_STORAGE_MEDIA_REMOVAL:
		FDbgPrint("  IOCTL_STORAGE_MEDIA_REMOVAL\n");
		status = STATUS_SUCCESS;
		break;

	case IOCTL_DISK_SET_PARTITION_INFO:
		FDbgPrint("  IOCTL_DISK_SET_PARTITION_INFO\n");
		break;

	case IOCTL_DISK_VERIFY:
		FDbgPrint("  IOCTL_DISK_VERIFY\n");
		break;

	case IOCTL_STORAGE_GET_HOTPLUG_INFO:
		{
			PSTORAGE_HOTPLUG_INFO hotplugInfo;
			FDbgPrint("  IOCTL_STORAGE_GET_HOTPLUG_INFO\n");
			if (outputLength < sizeof(STORAGE_HOTPLUG_INFO)) {
				status = STATUS_BUFFER_TOO_SMALL;
				Irp->IoStatus.Information = 0;
				break;
			}
			hotplugInfo = Irp->AssociatedIrp.SystemBuffer;
			hotplugInfo->Size =  sizeof(STORAGE_HOTPLUG_INFO);
			hotplugInfo->MediaRemovable = 1;
			hotplugInfo->MediaHotplug = 1;
			hotplugInfo->DeviceHotplug = 1;
			hotplugInfo->WriteCacheEnableOverride = 0;
			status = STATUS_SUCCESS;
			Irp->IoStatus.Information = sizeof(STORAGE_HOTPLUG_INFO);
		}
		break;

	case IOCTL_VOLUME_GET_GPT_ATTRIBUTES:
		{
			FDbgPrint("   IOCTL_VOLUME_GET_GPT_ATTRIBUTES\n");
			status = STATUS_SUCCESS;
		}
		break;
	case IOCTL_DISK_CHECK_VERIFY:
		FDbgPrint("  IOCTL_DISK_CHECK_VERIFY\n");
		status = STATUS_SUCCESS;
		break;

	case IOCTL_STORAGE_CHECK_VERIFY:
		FDbgPrint("  IOCTL_STORAGE_CHECK_VERIFY\n");
		status = STATUS_SUCCESS;
		break;

	case IOCTL_STORAGE_CHECK_VERIFY2:
		FDbgPrint("  IOCTL_STORAGE_CHECK_VERIFY2\n");
		status = STATUS_SUCCESS;
		break;

	case IOCTL_MOUNTDEV_QUERY_DEVICE_NAME:
		{
			PMOUNTDEV_NAME	mountdevName;
			ULONG			bufferLength = irpSp->Parameters.DeviceIoControl.OutputBufferLength;
			PUNICODE_STRING	deviceName =  dcb->DiskDeviceName;
			
			FDbgPrint("   IOCTL_MOUNTDEV_QUERY_DEVICE_NAME\n");

			if (bufferLength < sizeof(MOUNTDEV_NAME)) {
				status = STATUS_BUFFER_TOO_SMALL;
				Irp->IoStatus.Information = sizeof(MOUNTDEV_NAME);
				break;
			}

			if (!dcb->Mounted) {
				status = STATUS_INVALID_PARAMETER;
				break;
			}

			mountdevName = (PMOUNTDEV_NAME)Irp->AssociatedIrp.SystemBuffer;
			ASSERT(mountdevName != NULL);
			// NOTE: When Windows API GetVolumeNameForVolumeMountPoint is called, this IO control is called.
			//   Even if status = STATUS_SUCCESS, GetVolumeNameForVolumeMountPoint returns error.
			//   Something is wrong..
			
			mountdevName->NameLength = deviceName->Length;

			if (sizeof(USHORT) + mountdevName->NameLength < bufferLength) {
				RtlCopyMemory((PCHAR)mountdevName->Name,
								deviceName->Buffer,
								mountdevName->NameLength);
				Irp->IoStatus.Information = sizeof(USHORT) + mountdevName->NameLength;
				status = STATUS_SUCCESS;
				FDbgPrint("  DeviceName %wZ\n", deviceName);
			} else {
				Irp->IoStatus.Information = sizeof(MOUNTDEV_NAME);
				status = STATUS_BUFFER_OVERFLOW;
			}
		}
		break;

	case IOCTL_MOUNTDEV_QUERY_UNIQUE_ID:	
		{
			PMOUNTDEV_UNIQUE_ID uniqueId;
			ULONG				bufferLength = irpSp->Parameters.DeviceIoControl.OutputBufferLength;

			FDbgPrint("   IOCTL_MOUNTDEV_QUERY_UNIQUE_ID\n");
			if (bufferLength < sizeof(MOUNTDEV_UNIQUE_ID)) {
				status = STATUS_BUFFER_TOO_SMALL;
				Irp->IoStatus.Information = sizeof(MOUNTDEV_UNIQUE_ID);
				break;
			}

			uniqueId = (PMOUNTDEV_UNIQUE_ID)Irp->AssociatedIrp.SystemBuffer;
			ASSERT(uniqueId != NULL);

			uniqueId->UniqueIdLength = dcb->SymbolicLinkName->Length;

			if (sizeof(USHORT) + uniqueId->UniqueIdLength < bufferLength) {
				RtlCopyMemory((PCHAR)uniqueId->UniqueId,  
								dcb->SymbolicLinkName->Buffer,
								uniqueId->UniqueIdLength);
				Irp->IoStatus.Information = FIELD_OFFSET(MOUNTDEV_UNIQUE_ID, UniqueId[0]) +
											uniqueId->UniqueIdLength;
				status = STATUS_SUCCESS;
				FDbgPrint("  UniqueName %ws\n", uniqueId->UniqueId);
				break;
			} else {
				Irp->IoStatus.Information = sizeof(MOUNTDEV_UNIQUE_ID);
				status = STATUS_BUFFER_OVERFLOW;
			}
		}
		break;

	case IOCTL_MOUNTDEV_QUERY_SUGGESTED_LINK_NAME:
		FDbgPrint("   IOCTL_MOUNTDEV_QUERY_SUGGESTED_LINK_NAME\n");
		break;
	case IOCTL_MOUNTDEV_LINK_CREATED:
		{
			PMOUNTDEV_NAME	mountdevName = Irp->AssociatedIrp.SystemBuffer;
			FDbgPrint("   IOCTL_MOUNTDEV_LINK_CREATED\n");
			FDbgPrint("     Name: %ws\n", mountdevName->Name); 
			status = STATUS_SUCCESS;
		}
		break;
	case IOCTL_MOUNTDEV_LINK_DELETED:
		FDbgPrint("   IOCTL_MOUNTDEV_LINK_DELETED\n");
		status = STATUS_SUCCESS;
		break;
	//case IOCTL_MOUNTDEV_UNIQUE_ID_CHANGE_NOTIFY:
	//	FDbgPrint("   IOCTL_MOUNTDEV_UNIQUE_ID_CHANGE_NOTIFY\n");
	//	break;
	case IOCTL_MOUNTDEV_QUERY_STABLE_GUID:
		FDbgPrint("   IOCTL_MOUNTDEV_QUERY_STABLE_GUID\n");
		break;
	case IOCTL_VOLUME_ONLINE:
		FDbgPrint("   IOCTL_VOLUME_ONLINE\n");
		status = STATUS_SUCCESS;
		break;
	case IOCTL_VOLUME_OFFLINE:
		FDbgPrint("   IOCTL_VOLUME_OFFLINE\n");
		status = STATUS_SUCCESS;
		break;
	case IOCTL_VOLUME_READ_PLEX:
		FDbgPrint("   IOCTL_VOLUME_READ_PLEX\n");
		break;
	case IOCTL_VOLUME_PHYSICAL_TO_LOGICAL:
		FDbgPrint("   IOCTL_VOLUME_PHYSICAL_TO_LOGICAL\n");
		break;
	case IOCTL_VOLUME_IS_CLUSTERED:
		FDbgPrint("   IOCTL_VOLUME_IS_CLUSTERED\n");
		break;

	case IOCTL_VOLUME_GET_VOLUME_DISK_EXTENTS:
		{
			PVOLUME_DISK_EXTENTS	volume;
			ULONG	bufferLength = irpSp->Parameters.DeviceIoControl.OutputBufferLength;

			FDbgPrint("   IOCTL_VOLUME_GET_VOLUME_DISK_EXTENTS\n");
			if (bufferLength < sizeof(VOLUME_DISK_EXTENTS)) {
				status =  STATUS_INVALID_PARAMETER;
				Irp->IoStatus.Information = 0;
				break;
			}
			volume = Irp->AssociatedIrp.SystemBuffer;
			RtlZeroMemory(volume, sizeof(VOLUME_DISK_EXTENTS));
			volume->NumberOfDiskExtents = 1;
			Irp->IoStatus.Information = sizeof(VOLUME_DISK_EXTENTS);
			status = STATUS_SUCCESS;
		}
		break;

	case IOCTL_STORAGE_EJECT_MEDIA:
		{
			FDbgPrint("   IOCTL_STORAGE_EJECT_MEDIA\n");
			FuserUnmount(dcb);				
			status = STATUS_SUCCESS;
		}
		break;
	case IOCTL_REDIR_QUERY_PATH:
		{
			FDbgPrint("  IOCTL_REDIR_QUERY_PATH\n");
		}
		break;


	default:
		PrintUnknownDeviceIoctlCode(irpSp->Parameters.DeviceIoControl.IoControlCode);
		status = STATUS_NOT_IMPLEMENTED;
		break;
	}
	FDbgPrint("   <= FuserDiskDeviceControl\n");
	return status;
}




ULONG GetBinaryVersion(){
	FUSER_VERSION_SINGLE version;
	
	version.FullValue.Major = VER_MAJOR;
	version.FullValue.Minor = VER_MINOR;
	version.FullValue.Revision = VER_REVISION;
	
	return version.SingleValue;
}


NTSTATUS
GlobalDeviceControl(
	__in PDEVICE_OBJECT DeviceObject,
	__in PIRP Irp
	)
{
	PIO_STACK_LOCATION	irpSp;
	NTSTATUS			status = STATUS_NOT_IMPLEMENTED;

	FDbgPrint("   => FuserGlobalDeviceControl\n");
	irpSp = IoGetCurrentIrpStackLocation(Irp);

	switch (irpSp->Parameters.DeviceIoControl.IoControlCode) {
	case IOCTL_EVENT_START:
		//Methode FuserStart on FuserUsermodeLibrary
		FDbgPrint("  IOCTL_EVENT_START\n");
		status = FuserEventStart(DeviceObject, Irp);
		break;

	case IOCTL_SERVICE_WAIT:
		status = FuserRegisterPendingIrpForService(DeviceObject, Irp);
		break;

	
	case IOCTL_GET_VERSION:
		if (irpSp->Parameters.DeviceIoControl.OutputBufferLength >= sizeof(ULONG)) {			
			*(ULONG*)Irp->AssociatedIrp.SystemBuffer = GetBinaryVersion();
			Irp->IoStatus.Information = sizeof(ULONG);
			status = STATUS_SUCCESS;
			break;
		}

	default:
		PrintUnknownDeviceIoctlCode(irpSp->Parameters.DeviceIoControl.IoControlCode);
		status = STATUS_INVALID_PARAMETER;
		break;
	}

	FDbgPrint("   <= FuserGlobalDeviceControl\n");
	return status;
}






/*++

Routine Description:

	This device control dispatcher handles IOCTLs.

Arguments:

	DeviceObject - Context for the activity.
	Irp 		 - The device control argument block.

Return Value:

	NTSTATUS

--*/
NTSTATUS
FuserDispatchDeviceControl(
	__in PDEVICE_OBJECT DeviceObject,
	__in PIRP Irp
	)
{
	PFuserVCB			vcb;
	PFuserDCB			dcb;
	PIO_STACK_LOCATION	irpSp;
	NTSTATUS			status = STATUS_NOT_IMPLEMENTED;
	ULONG				controlCode = 0;




	__try {
		FsRtlEnterFileSystem();

		Irp->IoStatus.Information = 0;

		irpSp = IoGetCurrentIrpStackLocation(Irp);

		controlCode = irpSp->Parameters.DeviceIoControl.IoControlCode;
	
		if (controlCode != IOCTL_EVENT_WAIT &&
			controlCode != IOCTL_EVENT_INFO &&
			controlCode != IOCTL_KEEPALIVE) {

			FDbgPrint("==> FuserDispatchIoControl\n");
			FDbgPrint("  ProcessId %lu\n", IoGetRequestorProcessId(Irp));
		}

		vcb = DeviceObject->DeviceExtension;

		if (GetIdentifierType(vcb) == DGL) {
			status = GlobalDeviceControl(DeviceObject, Irp);
			__leave;
		} else if (GetIdentifierType(vcb) == DCB) {
			status = DiskDeviceControl(DeviceObject, Irp);
			__leave;
		} else if (GetIdentifierType(vcb) != VCB) {
			status = STATUS_INVALID_PARAMETER;
			__leave;
		}
		dcb = vcb->Dcb;
		switch (irpSp->Parameters.DeviceIoControl.IoControlCode) {

		case IOCTL_EVENT_WAIT:
			//FDbgPrint("  IOCTL_EVENT_WAIT\n");
			status = FuserRegisterPendingIrpForEvent(DeviceObject, Irp);
			break;

		case IOCTL_EVENT_INFO:
			//FDbgPrint("  IOCTL_EVENT_INFO\n");
			status = FuserCompleteIrp(DeviceObject, Irp); // Called when UserMode method returns Event.
			break;

		case IOCTL_EVENT_RELEASE:
			FDbgPrint("  IOCTL_EVENT_RELEASE\n");
			status = FuserEventRelease(DeviceObject);
			break;

		case IOCTL_EVENT_WRITE:
			FDbgPrint("  IOCTL_EVENT_WRITE\n");
			status = FuserEventWrite(DeviceObject, Irp);
			break;

		case IOCTL_KEEPALIVE:		
			if (dcb->Mounted) {
				ExAcquireResourceExclusiveLite(&dcb->Resource, TRUE);
				FuserUpdateTimeout(&dcb->TickCount, FUSER_KEEPALIVE_TIMEOUT);
				ExReleaseResourceLite(&dcb->Resource);
				status = STATUS_SUCCESS;
			} else {
				FDbgPrint(" device is not mounted\n");
				status = STATUS_INSUFFICIENT_RESOURCES;
			}
			break;

		default:
			{
				PrintUnknownDeviceIoctlCode(irpSp->Parameters.DeviceIoControl.IoControlCode);
				status = STATUS_NOT_IMPLEMENTED;
			}
			break;
		} // switch IoControlCode
	
	} __finally {

		if (status != STATUS_PENDING) {
			//
			// complete the Irp
			//
			Irp->IoStatus.Status = status;
			IoCompleteRequest(Irp, IO_NO_INCREMENT);
		}

		if (controlCode != IOCTL_EVENT_WAIT &&
			controlCode != IOCTL_EVENT_INFO &&
			controlCode != IOCTL_KEEPALIVE) {

			FuserPrintNTStatus(status);
			FDbgPrint("<== FuserDispatchIoControl\n");
		}

		FsRtlExitFileSystem();
	}
	return status;
}