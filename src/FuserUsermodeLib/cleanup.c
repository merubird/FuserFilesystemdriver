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
DispatchCleanup(
	HANDLE				Handle,
	PEVENT_CONTEXT		EventContext,
	PFUSER_INSTANCE		FuserInstance)
{
	PEVENT_INFORMATION		eventInfo;
	FUSER_FILE_INFO			fileInfo;	
	PFUSER_OPEN_INFO		openInfo;
	ULONG					sizeOfEventInfo = sizeof(EVENT_INFORMATION);

	CheckFileName(EventContext->Cleanup.FileName);

	eventInfo = DispatchCommon(
		EventContext, sizeOfEventInfo, FuserInstance, &fileInfo, &openInfo);
	
	eventInfo->Status = STATUS_SUCCESS; // return success at any case

	DbgPrint("###Cleanup %04d\n", openInfo != NULL ? openInfo->EventId : -1);

	if (FuserInstance->FuserEvents->Cleanup) {
		// ignore return value
		// TODO: Adapt the structure of the cleanup so that no return value can be transferred
		FuserInstance->FuserEvents->Cleanup(
			EventContext->Cleanup.FileName,
			&fileInfo);
	}

	openInfo->UserContext = fileInfo.Context;

	SendEventInformation(Handle, eventInfo, sizeOfEventInfo, FuserInstance);

	free(eventInfo);
	return;
}

