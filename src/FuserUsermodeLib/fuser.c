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

// TODO: recheck or remove
// FuserOptions->DebugMode is ON?
BOOL	g_DebugMode = TRUE;

// TODO: recheck or remove
// FuserOptions->UseStdErr is ON?
BOOL	g_UseStdErr = FALSE;


CRITICAL_SECTION	g_InstanceCriticalSection;
LIST_ENTRY			g_InstanceList;	


DWORD WINAPI
FuserLoop(
   PFUSER_INSTANCE FuserInstance
	)
{ // TODO: revise, proper event processing
	HANDLE	device;
	char	buffer[EVENT_CONTEXT_MAX_SIZE];
	ULONG	count = 0;
	BOOL	status;
	ULONG	returnedLength;
	DWORD	result = 0;

	RtlZeroMemory(buffer, sizeof(buffer));

	device = CreateFile(
				FuserInstance->RawDevice, // lpFileName
				GENERIC_READ | GENERIC_WRITE,       // dwDesiredAccess
				FILE_SHARE_READ | FILE_SHARE_WRITE, // dwShareMode
				NULL,                               // lpSecurityAttributes
				OPEN_EXISTING,                      // dwCreationDistribution
				0,                                  // dwFlagsAndAttributes
				NULL                                // hTemplateFile
			);

	if (device == INVALID_HANDLE_VALUE) {
		DbgPrint("Fuser Error: CreateFile failed %ws: %d\n",
			FuserInstance->RawDevice, GetLastError());
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



// ask driver to release all pending IRP to prepare for Unmount.
BOOL
SendReleaseIRP(
	LPCWSTR	DeviceName)
{
	ULONG	returnedLength;
	WCHAR rawDeviceName[MAX_PATH];
	wcscpy_s(rawDeviceName, MAX_PATH, L"\\\\.");
	wcscat_s(rawDeviceName, MAX_PATH, DeviceName);
	
	DbgPrint("send release\n");
	if (!SendToDevice(
				rawDeviceName,
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
		DbgPrint("Fuser Error: Failed to open %ws with code %d\n", DeviceName, dwErrorCode);
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


BOOL CheckMountPoint(LPWSTR MountPoint) {
	ULONG	length = wcslen(MountPoint);
	
	//MountPoint always starts with "?:\"	
	if (length < 3)
		return FALSE;

	if (MountPoint[1] != L':' || MountPoint[2] != L'\\' )
		return FALSE;
	
	if (MountPoint[0] >= L'a' && MountPoint[0] <=L'z'){
		//Driverletter lowercase, convert to uppercase:
		MountPoint[0] -= 32;
	}
	
	if (!(MountPoint[0] >= L'A' && MountPoint[0] <=L'Z')){
		return FALSE;
	}
	
	if (length > 3) {
		HANDLE handle = CreateFile(MountPoint, GENERIC_WRITE, 0, NULL, OPEN_EXISTING, FILE_FLAG_OPEN_REPARSE_POINT | FILE_FLAG_BACKUP_SEMANTICS, NULL);
		if (handle == INVALID_HANDLE_VALUE) {
			FuserDbgPrintW(L"Fuser Error: bad mount point %s\n", MountPoint);
			return FALSE;
		}
		CloseHandle(handle);
		return TRUE;
	}
	return TRUE;
}


BOOL
FuserStart(PFUSER_INSTANCE Instance, ULONG MountFlags)
{
	EVENT_START			eventStart;
	EVENT_DRIVER_INFO	driverInfo;
	ULONG		returnedLength = 0;

	ZeroMemory(&eventStart, sizeof(EVENT_START));
	ZeroMemory(&driverInfo, sizeof(EVENT_DRIVER_INFO));

	eventStart.Version = GetBinaryVersion();

	eventStart.Flags |= FUSER_EVENT_KEEP_ALIVE_ON;
	if (MountFlags & FUSER_MOUNT_PARAMETER_FLAG_USEADS) {
		eventStart.Flags |= FUSER_EVENT_ALTERNATIVE_STREAM_ON;
	}	
	if (MountFlags & FUSER_MOUNT_PARAMETER_FLAG_TYPE_REMOVABLE) {
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
		if (driverInfo.Version != eventStart.Version) {
			FuserDbgPrint("Fuser Error: driver version mismatch, driver %X, dll %X\n",driverInfo.Version, eventStart.Version);
		} else {
			FuserDbgPrint("Fuser Error: driver start error\n");
		}
		return FALSE;
	} else if (driverInfo.Status == FUSER_MOUNTED) {
		Instance->MountId = driverInfo.MountId;
		Instance->DeviceNumber = driverInfo.DeviceNumber;
		
		// TODO: Remove completely as soon as DeviceName is never used again
		wcscpy_s(Instance->DeviceName,
				sizeof(Instance->DeviceName) / sizeof(WCHAR),
				driverInfo.DeviceName);


		wcscpy_s(Instance->RawDevice, sizeof(Instance->RawDevice) / sizeof(WCHAR), L"\\\\.");
		wcscat_s(Instance->RawDevice, sizeof(Instance->RawDevice) / sizeof(WCHAR), driverInfo.DeviceName);
		
				
		
		return TRUE;
	}

	return FALSE;
}


BOOL LoadEventHandler(PFUSER_MOUNT_PARAMETER MountParameter, PFUSER_EVENT_CALLBACKS FuserEvents){
	ULONG			i;	
	ULONG 			EventID;
	PFUSER_EVENT	EventHandler = (PFUSER_EVENT)malloc(sizeof(FUSER_EVENT));	
	
	if (MountParameter->EventLoader != NULL){	
		for (i = 0; i < 999999; ++i) {		
			ZeroMemory(EventHandler, sizeof(FUSER_EVENT));
			EventID = MountParameter->EventLoader(i,EventHandler);
			
			if (EventID == 0){
				break;
			} else {
				if (EventHandler->CallPointer == 0){
					free(EventHandler);
					return FALSE;
				} else {				
					switch (EventID) {
						case FUSER_EVENT_MOUNT:						FuserEvents->Mount = EventHandler->Mount;									break;
						case FUSER_EVENT_UNMOUNT:					FuserEvents->Unmount = EventHandler->Unmount; 								break;
						case FUSER_EVENT_GET_VOLUME_INFORMATION:	FuserEvents->GetVolumeInformation = EventHandler->GetVolumeInformation;		break;
						case FUSER_EVENT_GET_DISK_FREESPACE: 		FuserEvents->GetDiskFreeSpace = EventHandler->GetDiskFreeSpace;				break;
						case FUSER_EVENT_CREATE_FILE: 				FuserEvents->CreateFile = EventHandler->CreateFile;							break;
						case FUSER_EVENT_CREATE_DIRECTORY: 			FuserEvents->CreateDirectory = EventHandler->CreateDirectory;				break;
						case FUSER_EVENT_OPEN_DIRECTORY: 			FuserEvents->OpenDirectory = EventHandler->OpenDirectory;					break;
						case FUSER_EVENT_CLOSE_FILE: 				FuserEvents->CloseFile = EventHandler->CloseFile;							break;
						case FUSER_EVENT_CLEANUP: 					FuserEvents->Cleanup = EventHandler->Cleanup;								break;
						case FUSER_EVENT_READ_FILE: 				FuserEvents->ReadFile = EventHandler->ReadFile;								break;
						case FUSER_EVENT_WRITE_FILE: 				FuserEvents->WriteFile = EventHandler->WriteFile;							break;
						case FUSER_EVENT_FLUSH_FILEBUFFERS: 		FuserEvents->FlushFileBuffers = EventHandler->FlushFileBuffers;				break;
						case FUSER_EVENT_FIND_FILES: 				FuserEvents->FindFiles = EventHandler->FindFiles;							break;
						case FUSER_EVENT_FIND_FILES_WITH_PATTERN: 	FuserEvents->FindFilesWithPattern = EventHandler->FindFilesWithPattern;		break;
						case FUSER_EVENT_GET_FILE_INFORMATION:		FuserEvents->GetFileInformation = EventHandler->GetFileInformation;			break;
						case FUSER_EVENT_SET_FILE_ATTRIBUTES: 		FuserEvents->SetFileAttributes = EventHandler->SetFileAttributes;			break;
						case FUSER_EVENT_SET_FILE_TIME: 			FuserEvents->SetFileTime = EventHandler->SetFileTime;						break;
						case FUSER_EVENT_SET_End_OF_FILE: 			FuserEvents->SetEndOfFile = EventHandler->SetEndOfFile;						break;
						case FUSER_EVENT_SET_ALLOCATIONSIZE: 		FuserEvents->SetAllocationSize = EventHandler->SetAllocationSize;			break;
						case FUSER_EVENT_LOCK_FILE: 				FuserEvents->LockFile = EventHandler->LockFile;								break;
						case FUSER_EVENT_UNLOCK_FILE: 				FuserEvents->UnlockFile = EventHandler->UnlockFile;							break;
						case FUSER_EVENT_DELETE_FILE: 				FuserEvents->DeleteFile = EventHandler->DeleteFile;							break;
						case FUSER_EVENT_DELETE_DIRECTORY: 			FuserEvents->DeleteDirectory = EventHandler->DeleteDirectory;				break;
						case FUSER_EVENT_MOVE_FILE: 				FuserEvents->MoveFile = EventHandler->MoveFile;								break;
						case FUSER_EVENT_GET_FILESECURITY: 			FuserEvents->GetFileSecurity = EventHandler->GetFileSecurity;				break;
						case FUSER_EVENT_SET_FILESECURITY: 			FuserEvents->SetFileSecurity = EventHandler->SetFileSecurity;				break;

						default:	
							free(EventHandler);						
							return FALSE;
					}
				}
			}
		}
		free(EventHandler);
		return TRUE;
	}
	free(EventHandler);
	return FALSE;
}




int FUSERAPI FuserDeviceMount(PFUSER_MOUNT_PARAMETER MountParameter)
{	
	ULONG					threadNum = 0;
	ULONG					i;
	HANDLE					device;
	HANDLE					threadIds[FUSER_THREADS_MAX];	
	PFUSER_INSTANCE 		instance;			
	PFUSER_EVENT_CALLBACKS	FuserEvents;
	WCHAR 					NewMountPoint[MAX_PATH];
	
	if (MountParameter->StructVersion != 1){
		//StructVersion incompatible, in future version must here implemetation of convert the struct
		return FUSER_DEVICEMOUNT_VERSION_ERROR;
	}
	
	FuserEvents  = (PFUSER_EVENT_CALLBACKS)malloc(sizeof(FUSER_EVENT_CALLBACKS));
	ZeroMemory(FuserEvents, sizeof(FUSER_EVENT_CALLBACKS));
		
	if (!LoadEventHandler(MountParameter, FuserEvents)){
		free(FuserEvents);
		return FUSER_DEVICEMOUNT_EVENT_LOAD_ERROR; 
	}
	
	//  Events ready for use
	

	g_DebugMode = MountParameter->Flags & FUSER_MOUNT_PARAMETER_FLAG_DEBUG;
	g_UseStdErr = MountParameter->Flags & FUSER_MOUNT_PARAMETER_FLAG_STDERR;

	if (g_DebugMode) {
		DbgPrintW(L"Fuser: debug mode on\n");
	}

	if (g_UseStdErr) {
		g_DebugMode = TRUE;
		DbgPrintW(L"Fuser: debug mode on, output=stderr\n");		
	}

	if (MountParameter->ThreadsCount <= 0) 
		MountParameter->ThreadsCount = FUSER_THREADS_DEFAULT;

	if ( MountParameter->ThreadsCount > FUSER_THREADS_MAX) {		
		MountParameter->ThreadsCount = FUSER_THREADS_MAX;
	}
	
	
	wcscpy_s(NewMountPoint,  MAX_PATH / sizeof(WCHAR), MountParameter->MountPoint);
	if (!CheckMountPoint(NewMountPoint)){
		free(FuserEvents);
		return FUSER_DEVICEMOUNT_BAD_MOUNT_POINT_ERROR; 
	}

	// Parameters set
	
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
		FuserDbgPrintW(L"Fuser Error: CreatFile Failed %s: %d\n", FUSER_GLOBAL_DEVICE_NAME, GetLastError());
		free(FuserEvents);
		return FUSER_DEVICEMOUNT_DRIVER_INSTALL_ERROR;
	}
	
	// create an instance and fill the values:	
	instance = NewFuserInstance();	
	instance->FuserEvents = FuserEvents;	
	wcscpy_s(instance->MountPoint, sizeof(instance->MountPoint) / sizeof(WCHAR), NewMountPoint);
	

	if (!FuserStart(instance, MountParameter->Flags)) { //This method creates the device for the new drive
		free(FuserEvents);
		return FUSER_DEVICEMOUNT_DRIVER_START_ERROR;
	}
		
	if (!FuserMount(instance->MountPoint, instance->DeviceName)) {
		SendReleaseIRP(instance->DeviceName);
		FuserDbgPrint("Fuser Error: DefineDosDevice Failed\n");
		free(FuserEvents);
		return FUSER_DEVICEMOUNT_MOUNT_ERROR;
	}
				
	if (FuserEvents->Mount) {
		FuserEvents->Mount(instance->MountPoint, instance->DeviceName, instance->RawDevice);
	}
		
	DbgPrintW(L"mounted: %s -> %s\n", instance->MountPoint, instance->DeviceName);
	
	//Device created and mounted
	
	for (i = 0; i < MountParameter->ThreadsCount; ++i) {
		threadIds[threadNum++] = (HANDLE)_beginthreadex(
			NULL, // Security Atributes
			0, //stack size
			FuserLoop,
			(PVOID)instance, // param
			0, // create flag
			NULL
		);
	}
	
	// Device up and running...


	// wait for thread terminations
	WaitForMultipleObjects(threadNum, threadIds, TRUE, INFINITE);
	for (i = 0; i < threadNum; ++i) {
		CloseHandle(threadIds[i]);
	}

    CloseHandle(device);

	//Sleep(1000); TODO: If there is a problem, activate it again
		
	DeleteFuserInstance(instance);
	free(FuserEvents);
    return FUSER_DEVICEMOUNT_SUCCESS;
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

	if (FuserInstance->FuserEvents->Unmount) {
		// ignore return value
		FuserInstance->FuserEvents->Unmount(&fileInfo);
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
		//openInfo->FuserInstance = FuserInstance; TODO: Remove
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
					FuserDeviceUnmount(instance->MountPoint);
					free(instance);

				}
				LeaveCriticalSection(&g_InstanceCriticalSection);
				DeleteCriticalSection(&g_InstanceCriticalSection);
			}
			break;

	}
	return TRUE;	
}


