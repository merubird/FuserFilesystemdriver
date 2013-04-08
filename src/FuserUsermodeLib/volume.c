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


#include "fuseri.h"
#include "fileinfo.h"

#define FUSER_DEFAULT_VOLUME_LABEL			L"FUSER"
#define FUSER_DEFAULT_SERIALNUMBER			0x19831116;



int FuserGetVolumeInformation(
	LPWSTR		VolumeNameBuffer,
	DWORD		VolumeNameSize,
	LPDWORD		VolumeSerialNumber,
	LPDWORD		MaximumComponentLength,
	LPDWORD		FileSystemFlags,
	LPWSTR		FileSystemNameBuffer,
	DWORD		FileSystemNameSize,
	PFUSER_FILE_INFO	FuserFileInfo)
{
	wcscpy_s(VolumeNameBuffer, VolumeNameSize / sizeof(WCHAR), L"FUSER");
	*VolumeSerialNumber = FUSER_DEFAULT_SERIALNUMBER;
	*MaximumComponentLength = 256;
	*FileSystemFlags = FILE_CASE_SENSITIVE_SEARCH | 
						FILE_CASE_PRESERVED_NAMES | 
						FILE_SUPPORTS_REMOTE_STORAGE |
						FILE_UNICODE_ON_DISK;

	wcscpy_s(FileSystemNameBuffer, FileSystemNameSize / sizeof(WCHAR), L"Fuser");

	return 0;
}




ULONG
FuserFsVolumeInformation(
	PEVENT_INFORMATION		EventInfo,
	PEVENT_CONTEXT			EventContext,
	PFUSER_FILE_INFO		FileInfo,
	PFUSER_EVENT_CALLBACKS	FuserEvents)
{
	WCHAR	volumeName[MAX_PATH];
	DWORD	volumeSerial;
	DWORD	maxComLength;
	DWORD	fsFlags;
	WCHAR	fsName[MAX_PATH];
	ULONG	remainingLength;
	ULONG	bytesToCopy;

	int		status = -1;

	PFILE_FS_VOLUME_INFORMATION volumeInfo = 
		(PFILE_FS_VOLUME_INFORMATION)EventInfo->Buffer;

	
	if (!FuserEvents->GetVolumeInformation) {
		//return STATUS_NOT_IMPLEMENTED;
		FuserEvents->GetVolumeInformation = FuserGetVolumeInformation;
	}

	remainingLength = EventContext->Volume.BufferLength;

	if (remainingLength < sizeof(FILE_FS_VOLUME_INFORMATION)) {
		return STATUS_BUFFER_OVERFLOW;
	}


	RtlZeroMemory(volumeName, sizeof(volumeName));
	RtlZeroMemory(fsName, sizeof(fsName));

	status = FuserEvents->GetVolumeInformation(
				volumeName,							// VolumeNameBuffer
				sizeof(volumeName) / sizeof(WCHAR), // VolumeNameSize
				&volumeSerial,						// VolumeSerialNumber
				&maxComLength,						// MaximumComponentLength
				&fsFlags,							// FileSystemFlags
				fsName,								// FileSystemNameBuffer
				sizeof(fsName)  / sizeof(WCHAR),	// FileSystemNameSize
				FileInfo);
	if (status < 0) {
		return STATUS_INVALID_PARAMETER;
	}


	volumeInfo->VolumeCreationTime.QuadPart = 0;
	volumeInfo->VolumeSerialNumber = volumeSerial;
	volumeInfo->SupportsObjects = FALSE;

	remainingLength -= FIELD_OFFSET(FILE_FS_VOLUME_INFORMATION, VolumeLabel[0]);
	
	bytesToCopy = wcslen(volumeName) * sizeof(WCHAR);
	if (remainingLength < bytesToCopy) {
		bytesToCopy = remainingLength;
	}

	volumeInfo->VolumeLabelLength = bytesToCopy;
	RtlCopyMemory(volumeInfo->VolumeLabel, volumeName, bytesToCopy);
	remainingLength -= bytesToCopy;

	EventInfo->BufferLength = EventContext->Volume.BufferLength - remainingLength;

	return STATUS_SUCCESS;
}


int FuserGetDiskFreeSpace(
	PULONGLONG			FreeBytesAvailable,
	PULONGLONG			TotalNumberOfBytes,
	PULONGLONG			TotalNumberOfFreeBytes,
	PFUSER_FILE_INFO	FuserFileInfo)
{
	*FreeBytesAvailable = 512*1024*1024;
	*TotalNumberOfBytes = 1024*1024*1024;
	*TotalNumberOfFreeBytes = 512*1024*1024;
	
	return 0;
}




ULONG
FuserFsSizeInformation(
	PEVENT_INFORMATION		EventInfo,
	PEVENT_CONTEXT			EventContext,
	PFUSER_FILE_INFO		FileInfo,
	PFUSER_EVENT_CALLBACKS	FuserEvents)
{
	ULONGLONG	freeBytesAvailable = 0;
	ULONGLONG	totalBytes = 0;
	ULONGLONG	freeBytes = 0;
	
	int			status = -1;

	PFILE_FS_SIZE_INFORMATION sizeInfo = (PFILE_FS_SIZE_INFORMATION)EventInfo->Buffer;

	
	if (!FuserEvents->GetDiskFreeSpace) {
		//return STATUS_NOT_IMPLEMENTED;
		FuserEvents->GetDiskFreeSpace = FuserGetDiskFreeSpace;
	}

	if (EventContext->Volume.BufferLength < sizeof(FILE_FS_SIZE_INFORMATION) ) {
		return STATUS_BUFFER_OVERFLOW;
	}

	status = FuserEvents->GetDiskFreeSpace(
		&freeBytesAvailable, // FreeBytesAvailable
		&totalBytes, // TotalNumberOfBytes
		&freeBytes, // TotalNumberOfFreeBytes
		FileInfo);

	if (status < 0) {
		return STATUS_INVALID_PARAMETER;
	}

	sizeInfo->TotalAllocationUnits.QuadPart		= totalBytes / FUSER_ALLOCATION_UNIT_SIZE;
	sizeInfo->AvailableAllocationUnits.QuadPart	= freeBytesAvailable / FUSER_ALLOCATION_UNIT_SIZE;
	sizeInfo->SectorsPerAllocationUnit			= FUSER_ALLOCATION_UNIT_SIZE / FUSER_SECTOR_SIZE;
	sizeInfo->BytesPerSector					= FUSER_SECTOR_SIZE;

	EventInfo->BufferLength = sizeof(FILE_FS_SIZE_INFORMATION);

	return STATUS_SUCCESS;
}




ULONG
FuserFsAttributeInformation(
	PEVENT_INFORMATION		EventInfo,
	PEVENT_CONTEXT			EventContext,
	PFUSER_FILE_INFO		FileInfo,
	PFUSER_EVENT_CALLBACKS	FuserEvents)
{
	WCHAR	volumeName[MAX_PATH];
	DWORD	volumeSerial;
	DWORD	maxComLength;
	DWORD	fsFlags;
	WCHAR	fsName[MAX_PATH];
	ULONG	remainingLength;
	ULONG	bytesToCopy;

	int		status = -1;

	PFILE_FS_ATTRIBUTE_INFORMATION attrInfo = 
		(PFILE_FS_ATTRIBUTE_INFORMATION)EventInfo->Buffer;
	
	if (!FuserEvents->GetVolumeInformation) {
		FuserEvents->GetVolumeInformation = FuserGetVolumeInformation;
		//return STATUS_NOT_IMPLEMENTED;
	}

	remainingLength = EventContext->Volume.BufferLength;

	if (remainingLength < sizeof(FILE_FS_ATTRIBUTE_INFORMATION)) {
		return STATUS_BUFFER_OVERFLOW;
	}


	RtlZeroMemory(volumeName, sizeof(volumeName));
	RtlZeroMemory(fsName, sizeof(fsName));

	status = FuserEvents->GetVolumeInformation(
				volumeName,			// VolumeNameBuffer
				sizeof(volumeName),	// VolumeNameSize
				&volumeSerial,		// VolumeSerialNumber
				&maxComLength,		// MaximumComponentLength
				&fsFlags,			// FileSystemFlags
				fsName,				// FileSystemNameBuffer
				sizeof(fsName),		// FileSystemNameSize
				FileInfo);

	if (status < 0) {
		return STATUS_INVALID_PARAMETER;
	}


	attrInfo->FileSystemAttributes = fsFlags;
	attrInfo->MaximumComponentNameLength = maxComLength;

	remainingLength -= FIELD_OFFSET(FILE_FS_ATTRIBUTE_INFORMATION, FileSystemName[0]);
	
	bytesToCopy = wcslen(fsName) * sizeof(WCHAR);
	if (remainingLength < bytesToCopy) {
		bytesToCopy = remainingLength;
	}

	attrInfo->FileSystemNameLength = bytesToCopy;
	RtlCopyMemory(attrInfo->FileSystemName, fsName, bytesToCopy);
	remainingLength -= bytesToCopy;

	EventInfo->BufferLength = EventContext->Volume.BufferLength - remainingLength;
	
	return STATUS_SUCCESS;
}





ULONG
FuserFsFullSizeInformation(
	PEVENT_INFORMATION		EventInfo,
	PEVENT_CONTEXT			EventContext,
	PFUSER_FILE_INFO		FileInfo,
	PFUSER_EVENT_CALLBACKS	FuserEvents)
{
	ULONGLONG	freeBytesAvailable = 0;
	ULONGLONG	totalBytes = 0;
	ULONGLONG	freeBytes = 0;

	int			status = -1;

	PFILE_FS_FULL_SIZE_INFORMATION sizeInfo = (PFILE_FS_FULL_SIZE_INFORMATION)EventInfo->Buffer;

	
	if (!FuserEvents->GetDiskFreeSpace) {
		FuserEvents->GetDiskFreeSpace = FuserGetDiskFreeSpace;
		//return STATUS_NOT_IMPLEMENTED;
	}

	if (EventContext->Volume.BufferLength < sizeof(FILE_FS_FULL_SIZE_INFORMATION) ) {
		return STATUS_BUFFER_OVERFLOW;
	}

	status = FuserEvents->GetDiskFreeSpace(
		&freeBytesAvailable, // FreeBytesAvailable
		&totalBytes, // TotalNumberOfBytes
		&freeBytes, // TotalNumberOfFreeBytes
		FileInfo);

	if (status < 0) {
		return STATUS_INVALID_PARAMETER;
	}

	sizeInfo->TotalAllocationUnits.QuadPart		= totalBytes / FUSER_ALLOCATION_UNIT_SIZE;
	sizeInfo->ActualAvailableAllocationUnits.QuadPart = freeBytes / FUSER_ALLOCATION_UNIT_SIZE;
	sizeInfo->CallerAvailableAllocationUnits.QuadPart = freeBytesAvailable / FUSER_ALLOCATION_UNIT_SIZE;
	sizeInfo->SectorsPerAllocationUnit			= FUSER_ALLOCATION_UNIT_SIZE / FUSER_SECTOR_SIZE;
	sizeInfo->BytesPerSector					= FUSER_SECTOR_SIZE;

	EventInfo->BufferLength = sizeof(FILE_FS_FULL_SIZE_INFORMATION);

	return STATUS_SUCCESS;
}






VOID
DispatchQueryVolumeInformation(
	HANDLE				Handle,
	PEVENT_CONTEXT		EventContext,
	PFUSER_INSTANCE		FuserInstance)
{
	PEVENT_INFORMATION		eventInfo;
	FUSER_FILE_INFO			fileInfo;
	PFUSER_OPEN_INFO		openInfo;
	int						status = -1;
	ULONG					sizeOfEventInfo = sizeof(EVENT_INFORMATION)
								- 8 + EventContext->Volume.BufferLength;

	eventInfo = (PEVENT_INFORMATION)malloc(sizeOfEventInfo);

	RtlZeroMemory(eventInfo, sizeOfEventInfo);
	RtlZeroMemory(&fileInfo, sizeof(FUSER_FILE_INFO));

	// There is no Context because file is not opened
	// so DispatchCommon is not used here
	openInfo = (PFUSER_OPEN_INFO)EventContext->Context; // TODO: Convert call to methods without context

	eventInfo->BufferLength = 0;
	eventInfo->SerialNumber = EventContext->SerialNumber;

	fileInfo.ProcessId = EventContext->ProcessId;
	
	eventInfo->Status = STATUS_NOT_IMPLEMENTED;
	eventInfo->BufferLength = 0;


	switch (EventContext->Volume.FsInformationClass) {
	case FileFsVolumeInformation:
		eventInfo->Status = FuserFsVolumeInformation(
								eventInfo, EventContext, &fileInfo, FuserInstance->FuserEvents);
		break;

	case FileFsSizeInformation:
		eventInfo->Status = FuserFsSizeInformation(
								eventInfo, EventContext, &fileInfo, FuserInstance->FuserEvents);
		break;
	case FileFsAttributeInformation:
		eventInfo->Status = FuserFsAttributeInformation(
								eventInfo, EventContext, &fileInfo, FuserInstance->FuserEvents);
		break;

	case FileFsFullSizeInformation:
		eventInfo->Status = FuserFsFullSizeInformation(
								eventInfo, EventContext, &fileInfo, FuserInstance->FuserEvents);
		break;

	default:
		DbgPrint("error unknown volume info %d\n", EventContext->Volume.FsInformationClass);
	}

	SendEventInformation(Handle, eventInfo, sizeOfEventInfo, NULL);
	free(eventInfo);
	return;
}

