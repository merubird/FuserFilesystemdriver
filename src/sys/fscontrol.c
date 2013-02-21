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
FuserUserFsRequest(
	__in PDEVICE_OBJECT	DeviceObject,
	__in PIRP			Irp
	)
{
	NTSTATUS			status = STATUS_NOT_IMPLEMENTED;
	PIO_STACK_LOCATION	irpSp;

	irpSp = IoGetCurrentIrpStackLocation(Irp);

	switch(irpSp->Parameters.FileSystemControl.FsControlCode) {
	case FSCTL_REQUEST_OPLOCK_LEVEL_1:
		FDbgPrint("    FSCTL_REQUEST_OPLOCK_LEVEL_1\n");
		status = STATUS_SUCCESS;
		break;

	case FSCTL_REQUEST_OPLOCK_LEVEL_2:
		FDbgPrint("    FSCTL_REQUEST_OPLOCK_LEVEL_2\n");
		status = STATUS_SUCCESS;
		break;

	case FSCTL_REQUEST_BATCH_OPLOCK:
		FDbgPrint("    FSCTL_REQUEST_BATCH_OPLOCK\n");
		status = STATUS_SUCCESS;
		break;

	case FSCTL_OPLOCK_BREAK_ACKNOWLEDGE:
		FDbgPrint("    FSCTL_OPLOCK_BREAK_ACKNOWLEDGE\n");
		status = STATUS_SUCCESS;
		break;

	case FSCTL_OPBATCH_ACK_CLOSE_PENDING:
		FDbgPrint("    FSCTL_OPBATCH_ACK_CLOSE_PENDING\n");
		status = STATUS_SUCCESS;
		break;

	case FSCTL_OPLOCK_BREAK_NOTIFY:
		FDbgPrint("    FSCTL_OPLOCK_BREAK_NOTIFY\n");
		status = STATUS_SUCCESS;
		break;

	case FSCTL_LOCK_VOLUME:
		FDbgPrint("    FSCTL_LOCK_VOLUME\n");
		status = STATUS_SUCCESS;
		break;

	case FSCTL_UNLOCK_VOLUME:
		FDbgPrint("    FSCTL_UNLOCK_VOLUME\n");
		status = STATUS_SUCCESS;
		break;

	case FSCTL_DISMOUNT_VOLUME:
		FDbgPrint("    FSCTL_DISMOUNT_VOLUME\n");
		break;

	case FSCTL_IS_VOLUME_MOUNTED:
		FDbgPrint("    FSCTL_IS_VOLUME_MOUNTED\n");
		status = STATUS_SUCCESS;
		break;

	case FSCTL_IS_PATHNAME_VALID:
		FDbgPrint("    FSCTL_IS_PATHNAME_VALID\n");
		break;

	case FSCTL_MARK_VOLUME_DIRTY:
		FDbgPrint("    FSCTL_MARK_VOLUME_DIRTY\n");
		break;

	case FSCTL_QUERY_RETRIEVAL_POINTERS:
		FDbgPrint("    FSCTL_QUERY_RETRIEVAL_POINTERS\n");
		break;

	case FSCTL_GET_COMPRESSION:
		FDbgPrint("    FSCTL_GET_COMPRESSION\n");
		break;

	case FSCTL_SET_COMPRESSION:
		FDbgPrint("    FSCTL_SET_COMPRESSION\n");
		break;

	case FSCTL_MARK_AS_SYSTEM_HIVE:
		FDbgPrint("    FSCTL_MARK_AS_SYSTEM_HIVE\n");
		break;

	case FSCTL_OPLOCK_BREAK_ACK_NO_2:
		FDbgPrint("    FSCTL_OPLOCK_BREAK_ACK_NO_2\n");
		break;

	case FSCTL_INVALIDATE_VOLUMES:
		FDbgPrint("    FSCTL_INVALIDATE_VOLUMES\n");
		break;

	case FSCTL_QUERY_FAT_BPB:
		FDbgPrint("    FSCTL_QUERY_FAT_BPB\n");
		break;

	case FSCTL_REQUEST_FILTER_OPLOCK:
		FDbgPrint("    FSCTL_REQUEST_FILTER_OPLOCK\n");
		break;

	case FSCTL_FILESYSTEM_GET_STATISTICS:
		FDbgPrint("    FSCTL_FILESYSTEM_GET_STATISTICS\n");
		break;

	case FSCTL_GET_NTFS_VOLUME_DATA:
		FDbgPrint("    FSCTL_GET_NTFS_VOLUME_DATA\n");
		break;

	case FSCTL_GET_NTFS_FILE_RECORD:
		FDbgPrint("    FSCTL_GET_NTFS_FILE_RECORD\n");
		break;

	case FSCTL_GET_VOLUME_BITMAP:
		FDbgPrint("    FSCTL_GET_VOLUME_BITMAP\n");
		break;

	case FSCTL_GET_RETRIEVAL_POINTERS:
		FDbgPrint("    FSCTL_GET_RETRIEVAL_POINTERS\n");
		break;

	case FSCTL_MOVE_FILE:
		FDbgPrint("    FSCTL_MOVE_FILE\n");
		break;

	case FSCTL_IS_VOLUME_DIRTY:
		FDbgPrint("    FSCTL_IS_VOLUME_DIRTY\n");
		break;

	case FSCTL_ALLOW_EXTENDED_DASD_IO:
		FDbgPrint("    FSCTL_ALLOW_EXTENDED_DASD_IO\n");
		break;

	case FSCTL_FIND_FILES_BY_SID:
		FDbgPrint("    FSCTL_FIND_FILES_BY_SID\n");
		break;

	case FSCTL_SET_OBJECT_ID:
		FDbgPrint("    FSCTL_SET_OBJECT_ID\n");
		break;

	case FSCTL_GET_OBJECT_ID:
		FDbgPrint("    FSCTL_GET_OBJECT_ID\n");
		break;

	case FSCTL_DELETE_OBJECT_ID:
		FDbgPrint("    FSCTL_DELETE_OBJECT_ID\n");
		break;

	case FSCTL_SET_REPARSE_POINT:
		FDbgPrint("    FSCTL_SET_REPARSE_POINT\n");
		break;

	case FSCTL_GET_REPARSE_POINT:
		FDbgPrint("    FSCTL_GET_REPARSE_POINT\n");
		status = STATUS_NOT_A_REPARSE_POINT;
		break;

	case FSCTL_DELETE_REPARSE_POINT:
		FDbgPrint("    FSCTL_DELETE_REPARSE_POINT\n");
		break;

	case FSCTL_ENUM_USN_DATA:
		FDbgPrint("    FSCTL_ENUM_USN_DATA\n");
		break;

	case FSCTL_SECURITY_ID_CHECK:
		FDbgPrint("    FSCTL_SECURITY_ID_CHECK\n");
		break;

	case FSCTL_READ_USN_JOURNAL:
		FDbgPrint("    FSCTL_READ_USN_JOURNAL\n");
		break;

	case FSCTL_SET_OBJECT_ID_EXTENDED:
		FDbgPrint("    FSCTL_SET_OBJECT_ID_EXTENDED\n");
		break;

	case FSCTL_CREATE_OR_GET_OBJECT_ID:
		FDbgPrint("    FSCTL_CREATE_OR_GET_OBJECT_ID\n");
		break;

	case FSCTL_SET_SPARSE:
		FDbgPrint("    FSCTL_SET_SPARSE\n");
		break;

	case FSCTL_SET_ZERO_DATA:
		FDbgPrint("    FSCTL_SET_ZERO_DATA\n");
		break;

	case FSCTL_QUERY_ALLOCATED_RANGES:
		FDbgPrint("    FSCTL_QUERY_ALLOCATED_RANGES\n");
		break;

	case FSCTL_SET_ENCRYPTION:
		FDbgPrint("    FSCTL_SET_ENCRYPTION\n");
		break;

	case FSCTL_ENCRYPTION_FSCTL_IO:
		FDbgPrint("    FSCTL_ENCRYPTION_FSCTL_IO\n");
		break;

	case FSCTL_WRITE_RAW_ENCRYPTED:
		FDbgPrint("    FSCTL_WRITE_RAW_ENCRYPTED\n");
		break;

	case FSCTL_READ_RAW_ENCRYPTED:
		FDbgPrint("    FSCTL_READ_RAW_ENCRYPTED\n");
		break;

	case FSCTL_CREATE_USN_JOURNAL:
		FDbgPrint("    FSCTL_CREATE_USN_JOURNAL\n");
		break;

	case FSCTL_READ_FILE_USN_DATA:
		FDbgPrint("    FSCTL_READ_FILE_USN_DATA\n");
		break;

	case FSCTL_WRITE_USN_CLOSE_RECORD:
		FDbgPrint("    FSCTL_WRITE_USN_CLOSE_RECORD\n");
		break;

	case FSCTL_EXTEND_VOLUME:
		FDbgPrint("    FSCTL_EXTEND_VOLUME\n");
		break;

	case FSCTL_QUERY_USN_JOURNAL:
		FDbgPrint("    FSCTL_QUERY_USN_JOURNAL\n");
		break;

	case FSCTL_DELETE_USN_JOURNAL:
		FDbgPrint("    FSCTL_DELETE_USN_JOURNAL\n");
		break;

	case FSCTL_MARK_HANDLE:
		FDbgPrint("    FSCTL_MARK_HANDLE\n");
		break;

	case FSCTL_SIS_COPYFILE:
		FDbgPrint("    FSCTL_SIS_COPYFILE\n");
		break;

	case FSCTL_SIS_LINK_FILES:
		FDbgPrint("    FSCTL_SIS_LINK_FILES\n");
		break;

	case FSCTL_RECALL_FILE:
		FDbgPrint("    FSCTL_RECALL_FILE\n");
		break;

	default:
		FDbgPrint("    Unknown FSCTL %d\n",
			(irpSp->Parameters.FileSystemControl.FsControlCode >> 2) & 0xFFF);
		status = STATUS_INVALID_DEVICE_REQUEST;
	}
	return status;
}



NTSTATUS
FuserDispatchFileSystemControl(
	__in PDEVICE_OBJECT DeviceObject,
	__in PIRP Irp
	)
{
	NTSTATUS			status = STATUS_INVALID_PARAMETER;
	PIO_STACK_LOCATION	irpSp;
	PFuserVCB			vcb;

	PAGED_CODE();

	__try {
		FsRtlEnterFileSystem();

		FDbgPrint("==> FuserFileSystemControl\n");
		FDbgPrint("  ProcessId %lu\n", IoGetRequestorProcessId(Irp));

		vcb = DeviceObject->DeviceExtension;
		if (GetIdentifierType(vcb) != VCB) {
			status = STATUS_INVALID_PARAMETER;
			__leave;
		}

		irpSp = IoGetCurrentIrpStackLocation(Irp);

		switch(irpSp->MinorFunction) {
		case IRP_MN_KERNEL_CALL:
			FDbgPrint("	 IRP_MN_KERNEL_CALL\n");
			break;

		case IRP_MN_LOAD_FILE_SYSTEM:
			FDbgPrint("	 IRP_MN_LOAD_FILE_SYSTEM\n");
			break;
		case IRP_MN_MOUNT_VOLUME:
			{
				PVPB vpb;
				FDbgPrint("	 IRP_MN_MOUNT_VOLUME\n");
				if (irpSp->Parameters.MountVolume.DeviceObject != vcb->Dcb->DeviceObject) {
					FDbgPrint("   Not FuserDiskDevice\n");
					status = STATUS_INVALID_PARAMETER;
				}
				vpb = irpSp->Parameters.MountVolume.Vpb;
				vpb->DeviceObject = vcb->DeviceObject;
				vpb->RealDevice = vcb->DeviceObject;
				vpb->Flags |= VPB_MOUNTED;

				vpb->VolumeLabelLength = wcslen(FUSER_DEFAULT_VOLUME_LABEL) * sizeof(WCHAR);
				RtlStringCchCopyW(vpb->VolumeLabel, sizeof(vpb->VolumeLabel) / sizeof(WCHAR), FUSER_DEFAULT_VOLUME_LABEL);
				vpb->SerialNumber = FUSER_DEFAULT_SERIALNUMBER;
				status = STATUS_SUCCESS;
			}
			break;
		case IRP_MN_USER_FS_REQUEST:
			FDbgPrint("	 IRP_MN_USER_FS_REQUEST\n");
			status = FuserUserFsRequest(DeviceObject, Irp);
			break;

		case IRP_MN_VERIFY_VOLUME:
			FDbgPrint("	 IRP_MN_VERIFY_VOLUME\n");
			break;

		default:
			FDbgPrint("  unknown %d\n", irpSp->MinorFunction);
			status = STATUS_INVALID_PARAMETER;
			break;

		}
	} __finally {
		
		Irp->IoStatus.Status = status;
		Irp->IoStatus.Information = 0;
		IoCompleteRequest(Irp, IO_NO_INCREMENT);

		FuserPrintNTStatus(status);
		FDbgPrint("<== FuserFileSystemControl\n");

		FsRtlExitFileSystem();
	}
	return status;
}
