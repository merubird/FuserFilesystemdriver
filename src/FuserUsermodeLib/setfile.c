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




#include <windows.h>
#include <stdio.h>
#include <stdlib.h>
#include "fuseri.h"
#include "fileinfo.h"



int
FuserSetAllocationInformation(
	 PEVENT_CONTEXT			EventContext,
	 PFUSER_FILE_INFO		FileInfo,
	 PFUSER_EVENT_CALLBACKS FuserEvents)
{
	PFILE_ALLOCATION_INFORMATION allocInfo =
		(PFILE_ALLOCATION_INFORMATION)((PCHAR)EventContext + EventContext->SetFile.BufferOffset);

	// A file's allocation size and end-of-file position are independent of each other,
	// with the following exception: The end-of-file position must always be less than
	// or equal to the allocation size. If the allocation size is set to a value that
	// is less than the end-of-file position, the end-of-file position is automatically
	// adjusted to match the allocation size.

	if (FuserEvents->SetAllocationSize) {
		return FuserEvents->SetAllocationSize(
			EventContext->SetFile.FileName,
			allocInfo->AllocationSize.QuadPart,
			FileInfo);
	}
	// How can we check the current end-of-file position?
	if (allocInfo->AllocationSize.QuadPart == 0) {
		return FuserEvents->SetEndOfFile(
			EventContext->SetFile.FileName,
			allocInfo->AllocationSize.QuadPart,
			FileInfo);
	} else {
		DbgPrint("  SetAllocationInformation %I64d, can't handle this parameter.\n",
				allocInfo->AllocationSize.QuadPart);
	}

	return 0;
}


int
FuserSetEndOfFileInformation(
	 PEVENT_CONTEXT			EventContext,
	 PFUSER_FILE_INFO		FileInfo,
	 PFUSER_EVENT_CALLBACKS FuserEvents)
{
	PFILE_END_OF_FILE_INFORMATION endInfo =
		(PFILE_END_OF_FILE_INFORMATION)((PCHAR)EventContext + EventContext->SetFile.BufferOffset);

	if (!FuserEvents->SetEndOfFile)
		return -1;

	return FuserEvents->SetEndOfFile(
		EventContext->SetFile.FileName,
		endInfo->EndOfFile.QuadPart,
		FileInfo);
}


int
FuserSetBasicInformation(
	 PEVENT_CONTEXT			EventContext,
	 PFUSER_FILE_INFO		FileInfo,
	 PFUSER_EVENT_CALLBACKS FuserEvents)
{
	FILETIME creation, lastAccess, lastWrite;
	int status = -1;

	PFILE_BASIC_INFORMATION basicInfo =
		(PFILE_BASIC_INFORMATION)((PCHAR)EventContext + EventContext->SetFile.BufferOffset);

	if (!FuserEvents->SetFileAttributes)
		return -1;
	
	if (!FuserEvents->SetFileTime)
		return -1;

	status = FuserEvents->SetFileAttributes(
		EventContext->SetFile.FileName,
		basicInfo->FileAttributes,
		FileInfo);

	if (status < 0)
		return status;

	creation.dwLowDateTime = basicInfo->CreationTime.LowPart;
	creation.dwHighDateTime = basicInfo->CreationTime.HighPart;
	lastAccess.dwLowDateTime = basicInfo->LastAccessTime.LowPart;
	lastAccess.dwHighDateTime = basicInfo->LastAccessTime.HighPart;
	lastWrite.dwLowDateTime = basicInfo->LastWriteTime.LowPart;
	lastWrite.dwHighDateTime = basicInfo->LastWriteTime.HighPart;


	return FuserEvents->SetFileTime(
		EventContext->SetFile.FileName,
		&creation,
		&lastAccess,
		&lastWrite,
		FileInfo);
}



int
FuserSetDispositionInformation(
	 PEVENT_CONTEXT			EventContext,
	 PFUSER_FILE_INFO		FileInfo,
	 PFUSER_EVENT_CALLBACKS FuserEvents)
{
	PFILE_DISPOSITION_INFORMATION dispositionInfo =
		(PFILE_DISPOSITION_INFORMATION)((PCHAR)EventContext + EventContext->SetFile.BufferOffset);

	if (!FuserEvents->DeleteFile || !FuserEvents->DeleteDirectory)
		return -1;

	if (!dispositionInfo->DeleteFile) {
		return 0;
	}

	if (FileInfo->IsDirectory) {
		return FuserEvents->DeleteDirectory(
			EventContext->SetFile.FileName,
			FileInfo);
	} else {
		return FuserEvents->DeleteFile(
			EventContext->SetFile.FileName,
			FileInfo);
	}
}





int
FuserSetLinkInformation(
	PEVENT_CONTEXT			EventContext,
	PFUSER_FILE_INFO		FileInfo,
	PFUSER_EVENT_CALLBACKS	FuserEvents)
{// TODO: check method what does it do
	PFUSER_LINK_INFORMATION linkInfo =
		(PFUSER_LINK_INFORMATION)((PCHAR)EventContext + EventContext->SetFile.BufferOffset);
	return -1;
}





int
FuserSetRenameInformation(
PEVENT_CONTEXT				EventContext,
	 PFUSER_FILE_INFO		FileInfo,
	 PFUSER_EVENT_CALLBACKS	FuserEvents)
{
	PFUSER_RENAME_INFORMATION renameInfo =
		(PFUSER_RENAME_INFORMATION)((PCHAR)EventContext + EventContext->SetFile.BufferOffset);

	WCHAR newName[MAX_PATH];
	ZeroMemory(newName, sizeof(newName));

	if (renameInfo->FileName[0] != L'\\') {
		ULONG pos;
		for (pos = EventContext->SetFile.FileNameLength/sizeof(WCHAR);
			pos != 0; --pos) {
			if (EventContext->SetFile.FileName[pos] == '\\')
				break;
		}
		RtlCopyMemory(newName, EventContext->SetFile.FileName, (pos+1)*sizeof(WCHAR));
		RtlCopyMemory((PCHAR)newName + (pos+1)*sizeof(WCHAR), renameInfo->FileName, renameInfo->FileNameLength);
	} else {
		RtlCopyMemory(newName, renameInfo->FileName, renameInfo->FileNameLength);
	}

	if (!FuserEvents->MoveFile)
		return -1;

	return FuserEvents->MoveFile(
		EventContext->SetFile.FileName,
		newName,
		renameInfo->ReplaceIfExists,
		FileInfo);
}



int
FuserSetValidDataLengthInformation(
	PEVENT_CONTEXT			EventContext,
	PFUSER_FILE_INFO		FileInfo,
	PFUSER_EVENT_CALLBACKS	FuserEvents)
{
	PFILE_VALID_DATA_LENGTH_INFORMATION validInfo =
		(PFILE_VALID_DATA_LENGTH_INFORMATION)((PCHAR)EventContext + EventContext->SetFile.BufferOffset);

	if (!FuserEvents->SetEndOfFile)
		return -1;

	return FuserEvents->SetEndOfFile(
		EventContext->SetFile.FileName,
		validInfo->ValidDataLength.QuadPart,
		FileInfo);
}






VOID
DispatchSetInformation(
 	HANDLE				Handle,
	PEVENT_CONTEXT		EventContext,
	PFUSER_INSTANCE		FuserInstance)
{
	PEVENT_INFORMATION		eventInfo;
	PFUSER_OPEN_INFO		openInfo;
	FUSER_FILE_INFO			fileInfo;
	int						status;
	ULONG					sizeOfEventInfo = sizeof(EVENT_INFORMATION);

	if (EventContext->SetFile.FileInformationClass == FileRenameInformation) {
		PFUSER_RENAME_INFORMATION renameInfo =
		(PFUSER_RENAME_INFORMATION)((PCHAR)EventContext + EventContext->SetFile.BufferOffset);
		sizeOfEventInfo += renameInfo->FileNameLength;
	}

	CheckFileName(EventContext->SetFile.FileName);

	eventInfo = DispatchCommon(
		EventContext, sizeOfEventInfo, FuserInstance, &fileInfo, &openInfo);

	switch (EventContext->SetFile.FileInformationClass) {
	case FileAllocationInformation:
		status = FuserSetAllocationInformation(
				EventContext, &fileInfo, FuserInstance->FuserEvents);
		break;

	case FileBasicInformation:
		status = FuserSetBasicInformation(
				EventContext, &fileInfo, FuserInstance->FuserEvents);
		break;

	case FileDispositionInformation:
		status = FuserSetDispositionInformation(
				EventContext, &fileInfo, FuserInstance->FuserEvents);
		break;

	case FileEndOfFileInformation:
		status = FuserSetEndOfFileInformation(
				EventContext, &fileInfo, FuserInstance->FuserEvents);
		break;

	case FileLinkInformation:
		status = FuserSetLinkInformation(
				EventContext, &fileInfo, FuserInstance->FuserEvents);
		break;

	case FilePositionInformation:
		// this case is dealed with by driver
		status = -1;
		break;


	case FileRenameInformation:
		status = FuserSetRenameInformation(
				EventContext, &fileInfo, FuserInstance->FuserEvents);
		break;

	case FileValidDataLengthInformation:
		status = FuserSetValidDataLengthInformation(
				EventContext, &fileInfo, FuserInstance->FuserEvents);
		break;

	}

	openInfo->UserContext = fileInfo.Context;

	eventInfo->BufferLength = 0;

	// TODO: support errorcodes
	if (EventContext->SetFile.FileInformationClass == FileDispositionInformation) {
		if (status == 0) {
			PFILE_DISPOSITION_INFORMATION dispositionInfo =
				(PFILE_DISPOSITION_INFORMATION)((PCHAR)EventContext + EventContext->SetFile.BufferOffset);
			eventInfo->Delete.DeleteOnClose = dispositionInfo->DeleteFile ? TRUE : FALSE;
			DbgPrint("  dispositionInfo->DeleteFile = %d\n", dispositionInfo->DeleteFile);
			eventInfo->Status = STATUS_SUCCESS;
		} else if (status == -ERROR_DIR_NOT_EMPTY) {
			DbgPrint("  DispositionInfo status = STATUS_DIRECTORY_NOT_EMPTY\n");
			eventInfo->Status = STATUS_DIRECTORY_NOT_EMPTY;
		} else if (status < 0) {
			DbgPrint("  DispositionInfo status = STATUS_CANNOT_DELETE\n");
			eventInfo->Status = STATUS_CANNOT_DELETE;
		}

	} else {
		if (status < 0) {
			int error = status * -1;
			eventInfo->Status = GetNTStatus(error);
		
		} else {
			eventInfo->Status = STATUS_SUCCESS;

			// notice new file name to driver
			if (EventContext->SetFile.FileInformationClass == FileRenameInformation) {
				PFUSER_RENAME_INFORMATION renameInfo =
					(PFUSER_RENAME_INFORMATION)((PCHAR)EventContext + EventContext->SetFile.BufferOffset);
				eventInfo->BufferLength = renameInfo->FileNameLength;
				CopyMemory(eventInfo->Buffer, renameInfo->FileName, renameInfo->FileNameLength);
			}
		}
	}

	//DbgPrint("SetInfomation status = %d\n\n", status);

	SendEventInformation(Handle, eventInfo, sizeOfEventInfo, FuserInstance);
	free(eventInfo);

	return;
}

