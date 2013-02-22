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

#ifndef _FUSERI_H_
#define _FUSERI_H_

#include <windows.h>
#include <stdio.h>
#include <stdlib.h>
#include "devioctl.h"
#include "public.h"
#include "fuser.h"
#include "fuserc.h"
#include "list.h"

#ifdef __cplusplus
extern "C" {
#endif




typedef struct _FUSER_INSTANCE {
	// to ensure that unmount dispatch is called at once
	CRITICAL_SECTION	CriticalSection;

	// store CurrentDeviceName
	// (when there are many mounts, each mount use 
	// other DeviceName)
	WCHAR	DeviceName[64];
	WCHAR	MountPoint[MAX_PATH];

	ULONG	DeviceNumber;
	ULONG	MountId;

	PFUSER_OPTIONS		FuserOptions;
	PFUSER_OPERATIONS	FuserOperations;
	LIST_ENTRY	ListEntry;

} FUSER_INSTANCE, *PFUSER_INSTANCE;


typedef struct _FUSER_OPEN_INFO {
	BOOL			IsDirectory;
	ULONG			OpenCount;
	PEVENT_CONTEXT	EventContext;
	PFUSER_INSTANCE	FuserInstance;
	ULONG64			UserContext;
	ULONG			EventId;
	PLIST_ENTRY		DirListHead;
} FUSER_OPEN_INFO, *PFUSER_OPEN_INFO;



BOOL
SendReleaseIRP(
	LPCWSTR DeviceName);


BOOL
SendToDevice(
	LPCWSTR	DeviceName,
	DWORD	IoControlCode,
	PVOID	InputBuffer,
	ULONG	InputLength,
	PVOID	OutputBuffer,
	ULONG	OutputLength,
	PULONG	ReturnedLength);

	


LPCWSTR
GetRawDeviceName(LPCWSTR	DeviceName);	



BOOL
FuserMount(
	LPCWSTR	MountPoint,
	LPCWSTR	DeviceName,
	BOOL    UseHeartbeatControl);


DWORD WINAPI
FuserKeepAlive(
	PVOID	Param);	
	



VOID
DispatchUnmount(
  	HANDLE				Handle,
	PEVENT_CONTEXT		EventContext,
	PFUSER_INSTANCE		FuserInstance);


VOID
DispatchCreate(
	HANDLE				Handle,
	PEVENT_CONTEXT		EventContext,
	PFUSER_INSTANCE		FuserInstance);	
	


VOID
CheckFileName(
	LPWSTR	FileName);
	


VOID
SendEventInformation(
	HANDLE				Handle,
	PEVENT_INFORMATION	EventInfo,
	ULONG				EventLength,
	PFUSER_INSTANCE		FuserInstance);



VOID
ClearFindData(
  PLIST_ENTRY	ListHead);



  
VOID
DispatchQueryInformation(
 	HANDLE				Handle,
	PEVENT_CONTEXT		EventContext,
	PFUSER_INSTANCE		FuserInstance);


PEVENT_INFORMATION
DispatchCommon(
	PEVENT_CONTEXT		EventContext,
	ULONG				SizeOfEventInfo,
	PFUSER_INSTANCE		FuserInstance,
	PFUSER_FILE_INFO	FuserFileInfo,
	PFUSER_OPEN_INFO*	FuserOpenInfo);



VOID
DispatchQueryVolumeInformation(
 	HANDLE				Handle,
	PEVENT_CONTEXT		EventContext,
	PFUSER_INSTANCE		FuserInstance);


VOID
DispatchCleanup(
  	HANDLE				Handle,
	PEVENT_CONTEXT		EventContext,
	PFUSER_INSTANCE		FuserInstance);

	


VOID
DispatchClose(
  	HANDLE				Handle,
	PEVENT_CONTEXT		EventContext,
	PFUSER_INSTANCE		FuserInstance);


VOID
ReleaseFuserOpenInfo(
	PEVENT_INFORMATION	EventInfomation,
	PFUSER_INSTANCE		FuserInstance);


VOID
DispatchDirectoryInformation(
	HANDLE				Handle,
	PEVENT_CONTEXT		EventContext,
	PFUSER_INSTANCE		FuserInstance);


VOID
DispatchRead(
 	HANDLE				Handle,
	PEVENT_CONTEXT		EventContext,
	PFUSER_INSTANCE		FuserInstance);


VOID
DispatchWrite(
 	HANDLE				Handle,
	PEVENT_CONTEXT		EventContext,
	PFUSER_INSTANCE		FuserInstance);	


VOID
DispatchLock(
	HANDLE				Handle,
	PEVENT_CONTEXT		EventContext,
	PFUSER_INSTANCE		FuserInstance);


VOID
DispatchSetInformation(
 	HANDLE				Handle,
	PEVENT_CONTEXT		EventContext,
	PFUSER_INSTANCE		FuserInstance);
	


ULONG
GetNTStatus(DWORD ErrorCode);	



VOID
DispatchFlush(
  	HANDLE				Handle,
	PEVENT_CONTEXT		EventContext,
	PFUSER_INSTANCE		FuserInstance);



VOID
DispatchQuerySecurity(
	HANDLE				Handle,
	PEVENT_CONTEXT		EventContext,
	PFUSER_INSTANCE		FuserInstance);	
	
VOID
DispatchSetSecurity(
	HANDLE			Handle,
	PEVENT_CONTEXT	EventContext,
	PFUSER_INSTANCE	FuserInstance);
	

#ifdef __cplusplus
}
#endif	
	

#endif




/* TODO: obsolete and unused, can be removed
BOOL
FuserStart(
	PFUSER_INSTANCE	Instance);
DWORD __stdcall
FuserLoop(
	PVOID Param);
BOOLEAN
InstallDriver(
	SC_HANDLE  SchSCManager,
	LPCWSTR    DriverName,
	LPCWSTR    ServiceExe);
BOOLEAN
RemoveDriver(
    SC_HANDLE  SchSCManager,
    LPCWSTR    DriverName);
BOOLEAN
StartDriver(
    SC_HANDLE  SchSCManager,
    LPCWSTR    DriverName);
BOOLEAN
StopDriver(
    SC_HANDLE  SchSCManager,
    LPCWSTR    DriverName);
BOOLEAN
ManageDriver(
	LPCWSTR  DriverName,
    LPCWSTR  ServiceName,
    USHORT   Function);
PFUSER_OPEN_INFO
GetFuserOpenInfo(
	PEVENT_CONTEXT		EventInfomation,
	PFUSER_INSTANCE		FuserInstance);
*/

