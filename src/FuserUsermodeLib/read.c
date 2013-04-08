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


VOID
DispatchRead(
	HANDLE				Handle,
	PEVENT_CONTEXT		EventContext,
	PFUSER_INSTANCE		FuserInstance)
{
	PEVENT_INFORMATION		eventInfo;
	PFUSER_OPEN_INFO		openInfo;
	ULONG					readLength = 0;
	int						status;
	FUSER_FILE_INFO			fileInfo;
	ULONG					sizeOfEventInfo;
	
	sizeOfEventInfo = sizeof(EVENT_INFORMATION) - 8 + EventContext->Read.BufferLength;

	CheckFileName(EventContext->Read.FileName);

	eventInfo = DispatchCommon(
		EventContext, sizeOfEventInfo, FuserInstance, &fileInfo, &openInfo);

	if (FuserInstance->FuserEvents->ReadFile) {
		status = FuserInstance->FuserEvents->ReadFile(
						EventContext->Read.FileName,
						eventInfo->Buffer,
						EventContext->Read.BufferLength,
						&readLength,
						EventContext->Read.ByteOffset.QuadPart,
						&fileInfo);
	} else {
		status = -1;
	}

	openInfo->UserContext = fileInfo.Context;
	eventInfo->BufferLength = 0;

	if (status < 0) {
		// TODO: support numerous other errors, including filelock errors
		eventInfo->Status = STATUS_INVALID_PARAMETER;
	} else if(readLength == 0) {
		eventInfo->Status = STATUS_END_OF_FILE;
	} else {
		eventInfo->Status = STATUS_SUCCESS;
		eventInfo->BufferLength = readLength;
		eventInfo->Read.CurrentByteOffset.QuadPart =
			EventContext->Read.ByteOffset.QuadPart + readLength;
	}

	SendEventInformation(Handle, eventInfo, sizeOfEventInfo, FuserInstance);
	free(eventInfo);
	return;
}
