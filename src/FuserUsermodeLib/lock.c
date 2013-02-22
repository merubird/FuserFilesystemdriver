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
DispatchLock(
	HANDLE				Handle,
	PEVENT_CONTEXT		EventContext,
	PFUSER_INSTANCE		FuserInstance)
{
	FUSER_FILE_INFO		fileInfo;
	PEVENT_INFORMATION	eventInfo;
	ULONG				sizeOfEventInfo = sizeof(EVENT_INFORMATION);
	PFUSER_OPEN_INFO	openInfo;
	int status;

	CheckFileName(EventContext->Lock.FileName);

	eventInfo = DispatchCommon(
		EventContext, sizeOfEventInfo, FuserInstance, &fileInfo, &openInfo);

	DbgPrint("###Lock %04d\n", openInfo != NULL ? openInfo->EventId : -1);

	eventInfo->Status = STATUS_NOT_IMPLEMENTED;

	switch (EventContext->MinorFunction) {
	case IRP_MN_LOCK:
		if (FuserInstance->FuserOperations->LockFile) {

			status = FuserInstance->FuserOperations->LockFile(
						EventContext->Lock.FileName,
						EventContext->Lock.ByteOffset.QuadPart,
						EventContext->Lock.Length.QuadPart,
						//EventContext->Lock.Key,
						&fileInfo);
			// TODO: support statuscodes
			eventInfo->Status = status < 0 ?
				STATUS_LOCK_NOT_GRANTED : STATUS_SUCCESS;
		}
		break;
	case IRP_MN_UNLOCK_ALL:
		break;
	case IRP_MN_UNLOCK_ALL_BY_KEY:
		break;
	case IRP_MN_UNLOCK_SINGLE:
		if (FuserInstance->FuserOperations->UnlockFile) {
		
			status = FuserInstance->FuserOperations->UnlockFile(
						EventContext->Lock.FileName,
						EventContext->Lock.ByteOffset.QuadPart,
						EventContext->Lock.Length.QuadPart,
						//EventContext->Lock.Key,
						&fileInfo);

			// TODO: support statuscodes
			eventInfo->Status = STATUS_SUCCESS; // at any time return success ?
		}
		break;
	default:
		DbgPrint("unkown lock function %d\n", EventContext->MinorFunction);
	}

	openInfo->UserContext = fileInfo.Context;

	SendEventInformation(Handle, eventInfo, sizeOfEventInfo, FuserInstance);

	free(eventInfo);
	return;
}
