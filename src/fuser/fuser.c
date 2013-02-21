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


#define _UNICODE
#include <windows.h>
// TODO: is later already included (fuseri.h)#include <winioctl.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <conio.h>
#include <tchar.h>
#include <process.h>
#include <locale.h>
#include "fileinfo.h"
#include "fuseri.h"
#include "list.h"

// FuserOptions->DebugMode is ON?
BOOL	g_DebugMode = TRUE;

// FuserOptions->UseStdErr is ON?
BOOL	g_UseStdErr = FALSE;


CRITICAL_SECTION	g_InstanceCriticalSection;
LIST_ENTRY			g_InstanceList;	


DWORD WINAPI
FuserLoop(
   PFUSER_INSTANCE FuserInstance
	)
{
	HANDLE	device;
	char	buffer[EVENT_CONTEXT_MAX_SIZE];
	ULONG	count = 0;
	BOOL	status;
	ULONG	returnedLength;
	DWORD	result = 0;

	RtlZeroMemory(buffer, sizeof(buffer));

	device = CreateFile(
				GetRawDeviceName(FuserInstance->DeviceName), // lpFileName
				GENERIC_READ | GENERIC_WRITE,       // dwDesiredAccess
				FILE_SHARE_READ | FILE_SHARE_WRITE, // dwShareMode
				NULL,                               // lpSecurityAttributes
				OPEN_EXISTING,                      // dwCreationDistribution
				0,                                  // dwFlagsAndAttributes
				NULL                                // hTemplateFile
			);

	if (device == INVALID_HANDLE_VALUE) {
		DbgPrint("Fuser Error: CreateFile failed %ws: %d\n",
			GetRawDeviceName(FuserInstance->DeviceName), GetLastError());
		result = -1;
		_endthreadex(result);
		return result;
	}

	while(1) {

		status = DeviceIoControl(
					device,				// Handle to device
					IOCTL_EVENT_WAIT,	// IO Control code
					NULL,				// Input Buffer to driver.
					0,					// Length of input buffer in bytes.
					buffer,             // Output Buffer from driver.
					sizeof(buffer),		// Length of output buffer in bytes.
					&returnedLength,	// Bytes placed in buffer.
					NULL                // synchronous call
					);

		if (!status) {
			DbgPrint("Ioctl failed with code %d\n", GetLastError());
			result = -1;
			break;
		}

		//printf("#%d got notification %d\n", (ULONG)Param, count++);

		if(returnedLength > 0) {
			PEVENT_CONTEXT context = (PEVENT_CONTEXT)buffer;
			if (context->MountId != FuserInstance->MountId) {
				DbgPrint("Fuser Error: Invalid MountId (expected:%d, acctual:%d)\n",
						FuserInstance->MountId, context->MountId);
				continue;
			}

			// TODO: secure all these methods with trycatch:
			switch (context->MajorFunction) {
			case IRP_MJ_CREATE:
				DispatchCreate(device, context, FuserInstance);
				break;
			case IRP_MJ_CLEANUP:
				DispatchCleanup(device, context, FuserInstance);
				break;
			case IRP_MJ_CLOSE:
				DispatchClose(device, context, FuserInstance);
				break;
			case IRP_MJ_DIRECTORY_CONTROL:
				DispatchDirectoryInformation(device, context, FuserInstance);
				break;
			case IRP_MJ_READ:
				DispatchRead(device, context, FuserInstance);
				break;
			case IRP_MJ_WRITE:
				DispatchWrite(device, context, FuserInstance);
				break;
			case IRP_MJ_QUERY_INFORMATION:
				DispatchQueryInformation(device, context, FuserInstance);
				break;
			case IRP_MJ_QUERY_VOLUME_INFORMATION:
				DispatchQueryVolumeInformation(device ,context, FuserInstance);
				break;
			case IRP_MJ_LOCK_CONTROL:
				DispatchLock(device, context, FuserInstance);
				break;
			case IRP_MJ_SET_INFORMATION:
				DispatchSetInformation(device, context, FuserInstance);
				break;
			case IRP_MJ_FLUSH_BUFFERS:
				DispatchFlush(device, context, FuserInstance);
				break;
			case IRP_MJ_QUERY_SECURITY:
				DispatchQuerySecurity(device, context, FuserInstance);
				break;
			case IRP_MJ_SET_SECURITY:
				DispatchSetSecurity(device, context, FuserInstance);
				break;
			case IRP_MJ_SHUTDOWN:
				// this cass is used before unmount not shutdown
				DispatchUnmount(device, context, FuserInstance);
				break;
			default:
				break;
			}

		} else {
			DbgPrint("ReturnedLength %d\n", returnedLength);
		}
	}

	CloseHandle(device);
	_endthreadex(result);
	return result;
}



VOID
ReleaseFuserOpenInfo(
	PEVENT_INFORMATION	EventInformation,
	PFUSER_INSTANCE		FuserInstance)
{
	PFUSER_OPEN_INFO openInfo;
	EnterCriticalSection(&FuserInstance->CriticalSection);

	openInfo = (PFUSER_OPEN_INFO)EventInformation->Context;
	if (openInfo != NULL) {
		openInfo->OpenCount--;
		if (openInfo->OpenCount < 1) {
			if (openInfo->DirListHead != NULL) {
				ClearFindData(openInfo->DirListHead);
				free(openInfo->DirListHead);
				openInfo->DirListHead = NULL;
			}
			free(openInfo);
			EventInformation->Context = 0;
		}
	}
	LeaveCriticalSection(&FuserInstance->CriticalSection);
}


VOID
SendEventInformation(
	HANDLE				Handle,
	PEVENT_INFORMATION	EventInfo,
	ULONG				EventLength,
	PFUSER_INSTANCE		FuserInstance)
{
	BOOL	status;
	ULONG	returnedLength;

	//DbgPrint("###EventInfo->Context %X\n", EventInfo->Context);
	if (FuserInstance != NULL) {
		ReleaseFuserOpenInfo(EventInfo, FuserInstance);
	}

	// send event info to driver
	status = DeviceIoControl(
					Handle,				// Handle to device
					IOCTL_EVENT_INFO,	// IO Control code
					EventInfo,			// Input Buffer to driver.
					EventLength,		// Length of input buffer in bytes.
					NULL,				// Output Buffer from driver.
					0,					// Length of output buffer in bytes.
					&returnedLength,	// Bytes placed in buffer.
					NULL				// synchronous call
					);

	if (!status) {
		DWORD errorCode = GetLastError();
		DbgPrint("Fuser Error: Ioctl failed with code %d\n", errorCode );
	}
}




PFUSER_INSTANCE 
NewFuserInstance()
{
	PFUSER_INSTANCE instance = (PFUSER_INSTANCE)malloc(sizeof(FUSER_INSTANCE));
	ZeroMemory(instance, sizeof(FUSER_INSTANCE));

#if _MSC_VER < 1300
	InitializeCriticalSection(&instance->CriticalSection);
#else
	InitializeCriticalSectionAndSpinCount(
		&instance->CriticalSection, 0x80000400);
#endif

	InitializeListHead(&instance->ListEntry);

	EnterCriticalSection(&g_InstanceCriticalSection);
	InsertTailList(&g_InstanceList, &instance->ListEntry);
	LeaveCriticalSection(&g_InstanceCriticalSection);

	return instance;
}


VOID 
DeleteFuserInstance(PFUSER_INSTANCE Instance)
{
	DeleteCriticalSection(&Instance->CriticalSection);

	EnterCriticalSection(&g_InstanceCriticalSection);
	RemoveEntryList(&Instance->ListEntry);
	LeaveCriticalSection(&g_InstanceCriticalSection);

	free(Instance);
}





LPCWSTR
GetRawDeviceName(LPCWSTR	DeviceName)
{
	static WCHAR rawDeviceName[MAX_PATH];
	wcscpy_s(rawDeviceName, MAX_PATH, L"\\\\.");
	wcscat_s(rawDeviceName, MAX_PATH, DeviceName);
	return rawDeviceName;
}



// ask driver to release all pending IRP to prepare for Unmount.
BOOL
SendReleaseIRP(
	LPCWSTR	DeviceName)
{
	ULONG	returnedLength;
	DbgPrint("send release\n");
	if (!SendToDevice(
				GetRawDeviceName(DeviceName),
				IOCTL_EVENT_RELEASE,
				NULL,
				0,
				NULL,
				0,
				&returnedLength) ) {
		
		DbgPrint("Failed to unmount device:%ws\n", DeviceName);
		return FALSE;
	}
	return TRUE;
}





BOOL
SendToDevice(
	LPCWSTR	DeviceName,
	DWORD	IoControlCode,
	PVOID	InputBuffer,
	ULONG	InputLength,
	PVOID	OutputBuffer,
	ULONG	OutputLength,
	PULONG	ReturnedLength)
{

	HANDLE	device;
	BOOL	status;
	ULONG	returnedLength;

	device = CreateFile(
				DeviceName,							// lpFileName
				GENERIC_READ | GENERIC_WRITE,       // dwDesiredAccess
                FILE_SHARE_READ | FILE_SHARE_WRITE, // dwShareMode
                NULL,                               // lpSecurityAttributes
                OPEN_EXISTING,                      // dwCreationDistribution
                0,                                  // dwFlagsAndAttributes
                NULL                                // hTemplateFile
			);

    if (device == INVALID_HANDLE_VALUE) {
		DWORD dwErrorCode = GetLastError();
		DbgPrint("Fuser Error: Failed to open %ws with code %d\n",
			DeviceName, dwErrorCode);
        return FALSE;
    }

	status = DeviceIoControl(
				device,                 // Handle to device
				IoControlCode,			// IO Control code
				InputBuffer,		    // Input Buffer to driver.
				InputLength,			// Length of input buffer in bytes.
				OutputBuffer,           // Output Buffer from driver.
				OutputLength,			// Length of output buffer in bytes.
				ReturnedLength,		    // Bytes placed in buffer.
				NULL                    // synchronous call
			);

	CloseHandle(device);

	if (!status) {
		DbgPrint("FuserError: Ioctl failed with code %d\n", GetLastError());
		return FALSE;
	}

	return TRUE;
}


BOOL
IsValidDriveLetter(WCHAR DriveLetter)
{
	return (L'd' <= DriveLetter && DriveLetter <= L'z') ||
		(L'D' <= DriveLetter && DriveLetter <= L'Z');
}



int
CheckMountPoint(LPCWSTR	MountPoint)
{
	ULONG	length = wcslen(MountPoint);

	if ((length == 1) ||
		(length == 2 && MountPoint[1] == L':') ||
		(length == 3 && MountPoint[1] == L':' && MountPoint[2] == L'\\')) {
		WCHAR driveLetter = MountPoint[0];
		
		if (IsValidDriveLetter(driveLetter)) {
			return FUSER_SUCCESS;
		} else {
			FuserDbgPrintW(L"Fuser Error: bad drive letter %s\n", MountPoint);
			return FUSER_DRIVE_LETTER_ERROR;
		}
	} else if (length > 3) {
		HANDLE handle = CreateFile(
						MountPoint, GENERIC_WRITE, 0, NULL, OPEN_EXISTING,
						FILE_FLAG_OPEN_REPARSE_POINT | FILE_FLAG_BACKUP_SEMANTICS, NULL);
		if (handle == INVALID_HANDLE_VALUE) {
			FuserDbgPrintW(L"Fuser Error: bad mount point %s\n", MountPoint);
			return FUSER_MOUNT_POINT_ERROR;
		}
		CloseHandle(handle);
		return FUSER_SUCCESS;
	}
	return FUSER_MOUNT_POINT_ERROR;
}


BOOL
FuserStart(PFUSER_INSTANCE Instance)
{
	EVENT_START			eventStart;
	EVENT_DRIVER_INFO	driverInfo;
	ULONG		returnedLength = 0;

	ZeroMemory(&eventStart, sizeof(EVENT_START));
	ZeroMemory(&driverInfo, sizeof(EVENT_DRIVER_INFO));

	eventStart.UserVersion = FUSER_DRIVER_VERSION;
	if (Instance->FuserOptions->Options & FUSER_OPTION_ALT_STREAM) {
		eventStart.Flags |= FUSER_EVENT_ALTERNATIVE_STREAM_ON;
	}
	if (Instance->FuserOptions->Options & FUSER_OPTION_KEEP_ALIVE) {
		eventStart.Flags |= FUSER_EVENT_KEEP_ALIVE_ON;
	}
	if (Instance->FuserOptions->Options & FUSER_OPTION_NETWORK) {
		eventStart.DeviceType = FUSER_NETWORK_FILE_SYSTEM;
	}
	if (Instance->FuserOptions->Options & FUSER_OPTION_REMOVABLE) {
		eventStart.Flags |= FUSER_EVENT_REMOVABLE;
	}

	SendToDevice(
		FUSER_GLOBAL_DEVICE_NAME,
		IOCTL_EVENT_START,
		&eventStart,
		sizeof(EVENT_START),
		&driverInfo,
		sizeof(EVENT_DRIVER_INFO),
		&returnedLength);

	if (driverInfo.Status == FUSER_START_FAILED) {
		if (driverInfo.DriverVersion != eventStart.UserVersion) {
			FuserDbgPrint(
				"Fuser Error: driver version mismatch, driver %X, dll %X\n",
				driverInfo.DriverVersion, eventStart.UserVersion);
		} else {
			FuserDbgPrint("Fuser Error: driver start error\n");
		}
		return FALSE;
	} else if (driverInfo.Status == FUSER_MOUNTED) {
		Instance->MountId = driverInfo.MountId;
		Instance->DeviceNumber = driverInfo.DeviceNumber;
		wcscpy_s(Instance->DeviceName,
				sizeof(Instance->DeviceName) / sizeof(WCHAR),
				driverInfo.DeviceName);
		return TRUE;
	}

	return FALSE;
}



int FUSERAPI
FuserMain(PFUSER_OPTIONS FuserOptions, PFUSER_OPERATIONS FuserOperations)
{
	ULONG	threadNum = 0;
	ULONG	i;
	BOOL	status;
	int		error;
	HANDLE	device;
	HANDLE	threadIds[FUSER_MAX_THREAD];
	ULONG   returnedLength;
	char	buffer[1024];
	BOOL	useMountPoint = FALSE;
	PFUSER_INSTANCE instance;

	g_DebugMode = FuserOptions->Options & FUSER_OPTION_DEBUG;
	g_UseStdErr = FuserOptions->Options & FUSER_OPTION_STDERR;

	if (g_DebugMode) {
		DbgPrintW(L"Fuser: debug mode on\n");
	}

	if (g_UseStdErr) {
		DbgPrintW(L"Fuser: use stderr\n");
		g_DebugMode = TRUE;
	}


	if (FuserOptions->ThreadCount == 0) {
		FuserOptions->ThreadCount = 5;

	} else if (FUSER_MAX_THREAD-1 < FuserOptions->ThreadCount) {
		// FUSER_MAX_THREAD includes FuserKeepAlive thread, so 
		// available thread is FUSER_MAX_THREAD -1
		FuserDbgPrintW(L"Fuser Error: too many thread count %d\n",
			FuserOptions->ThreadCount);
		FuserOptions->ThreadCount = FUSER_MAX_THREAD-1;
	}

	// TODO: solve this workaround:
	if (FUSER_MOUNT_POINT_SUPPORTED_VERSION <= FuserOptions->Version &&
		FuserOptions->MountPoint) {
		error = CheckMountPoint(FuserOptions->MountPoint);
		if (error != FUSER_SUCCESS) {
			return error;
		}
		useMountPoint = TRUE;
	} else if (!IsValidDriveLetter((WCHAR)FuserOptions->Version)) {
		// Older versions use the first 2 bytes of FuserOptions struct as DriveLetter.
		FuserDbgPrintW(L"Fuser Error: bad drive letter %wc\n", (WCHAR)FuserOptions->Version);
		return FUSER_DRIVE_LETTER_ERROR;
	}


	device = CreateFile(
					FUSER_GLOBAL_DEVICE_NAME,			// lpFileName
					GENERIC_READ|GENERIC_WRITE,			// dwDesiredAccess
					FILE_SHARE_READ|FILE_SHARE_WRITE,	// dwShareMode
					NULL,								// lpSecurityAttributes
					OPEN_EXISTING,						// dwCreationDistribution
					0,									// dwFlagsAndAttributes
					NULL								// hTemplateFile
                    );

	if (device == INVALID_HANDLE_VALUE){
		FuserDbgPrintW(L"Fuser Error: CreatFile Failed %s: %d\n", 
			FUSER_GLOBAL_DEVICE_NAME, GetLastError());
		return FUSER_DRIVER_INSTALL_ERROR;
	}

	DbgPrint("device opened\n");

	instance = NewFuserInstance();

	instance->FuserOptions = FuserOptions;
	instance->FuserOperations = FuserOperations;
	
	// TODO: remove this workaround and solve it correctly
	if (useMountPoint) {
		wcscpy_s(instance->MountPoint, sizeof(instance->MountPoint) / sizeof(WCHAR),
				FuserOptions->MountPoint);
	} else {
		// Older versions use the first 2 bytes of FuserOptions struct as DriveLetter.
		instance->MountPoint[0] = (WCHAR)FuserOptions->Version;
		instance->MountPoint[1] = L':';
		instance->MountPoint[2] = L'\\';
	}

	if (!FuserStart(instance)) {
		return FUSER_START_ERROR;
	}

	if (!FuserMount(instance->MountPoint, instance->DeviceName)) {
		SendReleaseIRP(instance->DeviceName);
		FuserDbgPrint("Fuser Error: DefineDosDevice Failed\n");
		return FUSER_MOUNT_ERROR;
	}

	DbgPrintW(L"mounted: %s -> %s\n", instance->MountPoint, instance->DeviceName);

	if (FuserOptions->Options & FUSER_OPTION_KEEP_ALIVE) {
		threadIds[threadNum++] = (HANDLE)_beginthreadex(
			NULL, // Security Atributes
			0, //stack size
			FuserKeepAlive,
			instance, // param
			0, // create flag
			NULL);
	}

	for (i = 0; i < FuserOptions->ThreadCount; ++i) {
		threadIds[threadNum++] = (HANDLE)_beginthreadex(
			NULL, // Security Atributes
			0, //stack size
			FuserLoop,
			(PVOID)instance, // param
			0, // create flag
			NULL);
	}


	// wait for thread terminations
	WaitForMultipleObjects(threadNum, threadIds, TRUE, INFINITE);

	for (i = 0; i < threadNum; ++i) {
		CloseHandle(threadIds[i]);
	}

    CloseHandle(device);

	Sleep(1000);
	
	DbgPrint("\nunload\n");
	DeleteFuserInstance(instance);
    return FUSER_SUCCESS;
}






BOOL
FuserSetDebugMode(
	ULONG	Mode)
{
	ULONG returnedLength;
	return SendToDevice(
		FUSER_GLOBAL_DEVICE_NAME,
		IOCTL_SET_DEBUG_MODE,
		&Mode,
		sizeof(ULONG),
		NULL,
		0,
		&returnedLength);
}



VOID
DispatchUnmount(
	HANDLE				Handle,
	PEVENT_CONTEXT		EventContext,
	PFUSER_INSTANCE		FuserInstance)
{
	FUSER_FILE_INFO			fileInfo;
	static int count = 0;

	// Unmount is called only once
	EnterCriticalSection(&FuserInstance->CriticalSection); 
	
	if (count > 0) {
		LeaveCriticalSection(&FuserInstance->CriticalSection);
		return;
	}
	count++;

	RtlZeroMemory(&fileInfo, sizeof(FUSER_FILE_INFO));

	fileInfo.ProcessId = EventContext->ProcessId;

	if (FuserInstance->FuserOperations->Unmount) {
		// ignore return value
		FuserInstance->FuserOperations->Unmount(&fileInfo);
	}

	LeaveCriticalSection(&FuserInstance->CriticalSection);

	// do not notice enything to the driver
	return;
}


VOID
CheckFileName(
	LPWSTR	FileName)
{
	// if the begining of file name is "\\",
	// replace it with "\"
	if (FileName[0] == L'\\' && FileName[1] == L'\\') {
		int i;
		for (i = 0; FileName[i+1] != L'\0'; ++i) {
			FileName[i] = FileName[i+1];
		}
		FileName[i] = L'\0';
	}
}




PFUSER_OPEN_INFO
GetFuserOpenInfo(
	PEVENT_CONTEXT		EventContext,
	PFUSER_INSTANCE		FuserInstance)
{
	PFUSER_OPEN_INFO openInfo;
	EnterCriticalSection(&FuserInstance->CriticalSection);

	openInfo = (PFUSER_OPEN_INFO)EventContext->Context;
	if (openInfo != NULL) {
		openInfo->OpenCount++;
		openInfo->EventContext = EventContext;
		openInfo->FuserInstance = FuserInstance;
	}
	LeaveCriticalSection(&FuserInstance->CriticalSection);
	return openInfo;
}




PEVENT_INFORMATION
DispatchCommon(
	PEVENT_CONTEXT		EventContext,
	ULONG				SizeOfEventInfo,
	PFUSER_INSTANCE		FuserInstance,
	PFUSER_FILE_INFO	FuserFileInfo,
	PFUSER_OPEN_INFO*	FuserOpenInfo)
{
	PEVENT_INFORMATION	eventInfo = (PEVENT_INFORMATION)malloc(SizeOfEventInfo);

	RtlZeroMemory(eventInfo, SizeOfEventInfo);
	RtlZeroMemory(FuserFileInfo, sizeof(FUSER_FILE_INFO));

	eventInfo->BufferLength = 0;
	eventInfo->SerialNumber = EventContext->SerialNumber;

	FuserFileInfo->ProcessId	= EventContext->ProcessId;
	FuserFileInfo->FuserOptions = FuserInstance->FuserOptions;
	if (EventContext->FileFlags & FUSER_DELETE_ON_CLOSE) {
		FuserFileInfo->DeleteOnClose = 1;
	}
	if (EventContext->FileFlags & FUSER_PAGING_IO) {
		FuserFileInfo->PagingIo = 1;
	}
	if (EventContext->FileFlags & FUSER_WRITE_TO_END_OF_FILE) {
		FuserFileInfo->WriteToEndOfFile = 1;
	}
	if (EventContext->FileFlags & FUSER_SYNCHRONOUS_IO) {
		FuserFileInfo->SynchronousIo = 1;
	}
	if (EventContext->FileFlags & FUSER_NOCACHE) {
		FuserFileInfo->Nocache = 1;
	}


	*FuserOpenInfo = GetFuserOpenInfo(EventContext, FuserInstance);

	if (*FuserOpenInfo == NULL) {
		DbgPrint("error openInfo is NULL\n");
		return eventInfo;
	}

	FuserFileInfo->Context		= (ULONG64)(*FuserOpenInfo)->UserContext;
	FuserFileInfo->IsDirectory	= (UCHAR)(*FuserOpenInfo)->IsDirectory;
	FuserFileInfo->FuserContext = (ULONG64)(*FuserOpenInfo);

	eventInfo->Context = (ULONG64)(*FuserOpenInfo);	

	return eventInfo;
}








BOOL WINAPI DllMain(
	HINSTANCE	Instance,
	DWORD		Reason,
	LPVOID		Reserved)
{
	switch(Reason) {
		case DLL_PROCESS_ATTACH:
			{				
#if _MSC_VER < 1300
				InitializeCriticalSection(&g_InstanceCriticalSection);
#else
				InitializeCriticalSectionAndSpinCount(
					&g_InstanceCriticalSection, 0x80000400);
#endif
				InitializeListHead(&g_InstanceList);
			}
			break;			
		case DLL_PROCESS_DETACH:
			{							
				EnterCriticalSection(&g_InstanceCriticalSection);
				while(!IsListEmpty(&g_InstanceList)) {
					PLIST_ENTRY entry = RemoveHeadList(&g_InstanceList);
					PFUSER_INSTANCE instance =
						CONTAINING_RECORD(entry, FUSER_INSTANCE, ListEntry);
					FuserRemoveMountPoint(instance->MountPoint);
					free(instance);

				}
				LeaveCriticalSection(&g_InstanceCriticalSection);
				DeleteCriticalSection(&g_InstanceCriticalSection);
			}
			break;

	}
	return TRUE;	
}


