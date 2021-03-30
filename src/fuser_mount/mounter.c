/*

Copyright (C) 2011 - 2013 Christian Auer christian.auer@gmx.ch
Copyright (C) 2007, 2008 Hiroki Asakawa asakaw@gmail.com

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in
all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
THE SOFTWARE.
*/



#include <windows.h>
#include <stdio.h>
#include <stdlib.h>
#include <sddl.h>
#include "mount.h"
#include "public.h"

static HANDLE                g_EventControl = NULL;
static SERVICE_STATUS        g_ServiceStatus;
static SERVICE_STATUS_HANDLE g_StatusHandle = NULL;

static CRITICAL_SECTION	g_CriticalSection;
static LIST_ENTRY		g_MountList;


BOOL g_DebugMode = TRUE;
BOOL g_UseStdErr = FALSE;


static DWORD WINAPI HandlerEx(DWORD dwControl, DWORD dwEventType, LPVOID lpEventData, LPVOID lpContext)
{
	switch (dwControl) {
	case SERVICE_CONTROL_STOP:

		g_ServiceStatus.dwWaitHint     = 50000;
		g_ServiceStatus.dwCheckPoint   = 0;
		g_ServiceStatus.dwCurrentState = SERVICE_STOP_PENDING;
		SetServiceStatus(g_StatusHandle, &g_ServiceStatus);

		SetEvent(g_EventControl);

		break;
	
	case SERVICE_CONTROL_INTERROGATE:
		SetServiceStatus(g_StatusHandle, &g_ServiceStatus);
		break;

	default:
		break;
	}

	return NO_ERROR;
}




static VOID BuildSecurityAttributes(PSECURITY_ATTRIBUTES SecurityAttributes)
{
	LPTSTR sd = L"D:P(A;;GA;;;SY)(A;;GRGWGX;;;BA)(A;;GRGW;;;WD)(A;;GR;;;RC)";

	ZeroMemory(SecurityAttributes, sizeof(SECURITY_ATTRIBUTES));
	
	ConvertStringSecurityDescriptorToSecurityDescriptor(
		sd,
		SDDL_REVISION_1,
		&SecurityAttributes->lpSecurityDescriptor,
		NULL);

	SecurityAttributes->nLength = sizeof(SECURITY_ATTRIBUTES);
    SecurityAttributes->bInheritHandle = TRUE;
}



PMOUNT_ENTRY
InsertMountEntry(PFUSER_CONTROL FuserControl)
{
	PMOUNT_ENTRY	mountEntry;
	mountEntry = malloc(sizeof(MOUNT_ENTRY));
	if (mountEntry == NULL) {
		DbgPrintW(L"InsertMountEntry malloc failed\n");
		return NULL;
	}
	ZeroMemory(mountEntry, sizeof(MOUNT_ENTRY));
	CopyMemory(&mountEntry->MountControl, FuserControl, sizeof(FUSER_CONTROL));
	InitializeListHead(&mountEntry->ListEntry);

	EnterCriticalSection(&g_CriticalSection);
	InsertTailList(&g_MountList, &mountEntry->ListEntry);
	LeaveCriticalSection(&g_CriticalSection);

	return mountEntry;
}


PMOUNT_ENTRY
FindMountEntry(PFUSER_CONTROL	FuserControl)
{
	PLIST_ENTRY		listEntry;
	PMOUNT_ENTRY	mountEntry;
	BOOL			useMountPoint = wcslen(FuserControl->MountPoint) > 0;
	BOOL			found = FALSE;

	if (!useMountPoint && wcslen(FuserControl->DeviceName) == 0) {
		return NULL;
	}

	EnterCriticalSection(&g_CriticalSection);

    for (listEntry = g_MountList.Flink; listEntry != &g_MountList; listEntry = listEntry->Flink) {
		mountEntry = CONTAINING_RECORD(listEntry, MOUNT_ENTRY, ListEntry);
		if (useMountPoint) {
			if (wcscmp(FuserControl->MountPoint, mountEntry->MountControl.MountPoint) == 0) {
				found = TRUE;
				break;
			}
		} else {
			if (wcscmp(FuserControl->DeviceName, mountEntry->MountControl.DeviceName) == 0) {
				found = TRUE;
				break;
			}
		}
	}

	LeaveCriticalSection(&g_CriticalSection);

	if (found) {
		DbgPrintW(L"FindMountEntry %s -> %s\n",
			mountEntry->MountControl.MountPoint, mountEntry->MountControl.DeviceName);
		return mountEntry;
	} else {
		return NULL;
	}
}


VOID
RemoveMountEntry(PMOUNT_ENTRY MountEntry)
{
	EnterCriticalSection(&g_CriticalSection);
	RemoveEntryList(&MountEntry->ListEntry);
	LeaveCriticalSection(&g_CriticalSection);

	free(MountEntry);
}


VOID
FuserControlFind(PFUSER_CONTROL Control)
{
	PLIST_ENTRY		listEntry;
	PMOUNT_ENTRY	mountEntry;

	mountEntry = FindMountEntry(Control);
	if (mountEntry == NULL) {
		Control->Status = FUSER_CONTROL_FAIL;
	} else {
		wcscpy_s(Control->DeviceName, sizeof(Control->DeviceName) / sizeof(WCHAR),
				mountEntry->MountControl.DeviceName);
		wcscpy_s(Control->MountPoint, sizeof(Control->MountPoint) / sizeof(WCHAR),
				mountEntry->MountControl.MountPoint);
		Control->Status = FUSER_CONTROL_SUCCESS;
	}
}




VOID
FuserControlList(PFUSER_CONTROL Control)
{
	PLIST_ENTRY		listEntry;
	PMOUNT_ENTRY	mountEntry;
	ULONG			index = 0;

	EnterCriticalSection(&g_CriticalSection);
	Control->Status = FUSER_CONTROL_FAIL;

	for (listEntry = g_MountList.Flink;
		listEntry != &g_MountList;
		listEntry = listEntry->Flink) {
		mountEntry = CONTAINING_RECORD(listEntry, MOUNT_ENTRY, ListEntry);
		if (Control->Option == index++) {
			wcscpy_s(Control->DeviceName, sizeof(Control->DeviceName) / sizeof(WCHAR),
					mountEntry->MountControl.DeviceName);
			wcscpy_s(Control->MountPoint, sizeof(Control->MountPoint) / sizeof(WCHAR),
					mountEntry->MountControl.MountPoint);
			Control->Status = FUSER_CONTROL_SUCCESS;
			break;
		}
	}
	LeaveCriticalSection(&g_CriticalSection);
}



static VOID FuserControl(PFUSER_CONTROL Control)
{
	PMOUNT_ENTRY	mountEntry;
	ULONG	index = 0;
	DWORD written = 0;
	Control->Status = FUSER_CONTROL_FAIL;

	switch (Control->Type)
	{
	case FUSER_CONTROL_MOUNT:

		DbgPrintW(L"FuserControl Mount\n");

		
		if (FuserControlMount(Control->MountPoint, Control->DeviceName )) {			
			Control->Status = FUSER_CONTROL_SUCCESS;
			mountEntry = InsertMountEntry(Control);
			if (Control->Option == 1) {
				// enable heartbeat-control:
				HeartbeatStart(mountEntry);
			}			
		} else {
			Control->Status = FUSER_CONTROL_FAIL;
		}
		break;

	case FUSER_CONTROL_UNMOUNT:

		DbgPrintW(L"FuserControl Unmount\n");

		mountEntry = FindMountEntry(Control);

		if (mountEntry == NULL) {
			if (Control->Option == FUSER_CONTROL_OPTION_FORCE_UNMOUNT &&
				FuserControlUnmount(Control->MountPoint)) {
				Control->Status = FUSER_CONTROL_SUCCESS;
				break;
			}
			Control->Status = FUSER_CONTROL_FAIL;
			break;	
		}

		if (FuserControlUnmount(mountEntry->MountControl.MountPoint)) {
			Control->Status = FUSER_CONTROL_SUCCESS;
			if (wcslen(Control->DeviceName) == 0) {
				wcscpy_s(Control->DeviceName, sizeof(Control->DeviceName) / sizeof(WCHAR),
						mountEntry->MountControl.DeviceName);
			}
			HeartbeatStop(mountEntry);
			RemoveMountEntry(mountEntry);
		} else {
			mountEntry->MountControl.Status = FUSER_CONTROL_FAIL;
			Control->Status = FUSER_CONTROL_FAIL;
		}

		break;
	case FUSER_CONTROL_CHECK:
		{
			DbgPrint("FuserControl Check\n");
			Control->Status = 0;
		}
		break;
	case FUSER_CONTROL_FIND:
		{
			DbgPrintW(L"FuserControl Find\n");
			FuserControlFind(Control);
		}
		break;
	case FUSER_CONTROL_LIST:
		{
			DbgPrintW(L"FuserControl List\n");
			FuserControlList(Control);
		}
		break;
	
	case FUSER_CONTROL_HEARTBEAT:
		{			
			mountEntry = FindMountEntry(Control);
			if (mountEntry == NULL) {
				Control->Status = FUSER_CONTROL_FAIL;
				break;	
			}
			HeartbeatSetAliveSignal(mountEntry);
			Control->Status = FUSER_CONTROL_SUCCESS;
		}
		break;

	default:
		DbgPrintW(L"FuserControl UnknownType %u\n", Control->Type);
	}
	return;
}



VOID
UnmountAll()
{
	PLIST_ENTRY		listEntry;
	PMOUNT_ENTRY	mountEntry;
	PMOUNT_ENTRY	umEntry;
	ULONG			index = 0;
	FUSER_CONTROL 	control;

	while(1){		
		umEntry = NULL;
		
		
		EnterCriticalSection(&g_CriticalSection);
		for (listEntry = g_MountList.Flink;
			listEntry != &g_MountList;
			listEntry = listEntry->Flink) {
			mountEntry = CONTAINING_RECORD(listEntry, MOUNT_ENTRY, ListEntry);
			if (mountEntry != NULL) {
				umEntry = mountEntry;
				break;
			}
		}
		LeaveCriticalSection(&g_CriticalSection);
		
		
		if (umEntry == NULL){
			break;
		} else {
			ZeroMemory(&control, sizeof(FUSER_CONTROL));
			control.Type = FUSER_CONTROL_UNMOUNT;
						
			wcscpy_s(control.DeviceName, sizeof(control.DeviceName) / sizeof(WCHAR), umEntry->MountControl.DeviceName);
			wcscpy_s(control.MountPoint, sizeof(control.MountPoint) / sizeof(WCHAR), umEntry->MountControl.MountPoint);
		
			FuserControl(&control);
			if (control.Status == FUSER_CONTROL_SUCCESS){
				SendReleaseIRP(control.DeviceName);
			} else {
				break;
			}
		}
		
		
	}
	

}





static VOID WINAPI ServiceMain(DWORD dwArgc, LPTSTR *lpszArgv)
{
	DWORD			eventNo;
	HANDLE			pipe, device;
	HANDLE			eventConnect, eventUnmount;
	HANDLE			eventArray[3];
	FUSER_CONTROL	control, unmount;
	OVERLAPPED		ov, driver;
	ULONG			returnedBytes;
	EVENT_CONTEXT	eventContext;
	SECURITY_ATTRIBUTES sa;

#if _MSC_VER < 1300
	InitializeCriticalSection(&g_CriticalSection);
#else
	InitializeCriticalSectionAndSpinCount(&g_CriticalSection, 0x80000400);
#endif
	InitializeListHead(&g_MountList);

	g_StatusHandle = RegisterServiceCtrlHandlerEx(L"FuserMounter", HandlerEx, NULL); // TODO: Change name

	// extend completion time
	g_ServiceStatus.dwServiceType				= SERVICE_WIN32_OWN_PROCESS;
	g_ServiceStatus.dwWin32ExitCode				= NO_ERROR;
	g_ServiceStatus.dwControlsAccepted			= SERVICE_ACCEPT_STOP;
	g_ServiceStatus.dwServiceSpecificExitCode	= 0;
	g_ServiceStatus.dwWaitHint					= 30000;
	g_ServiceStatus.dwCheckPoint				= 1;
	g_ServiceStatus.dwCurrentState				= SERVICE_START_PENDING;
	SetServiceStatus(g_StatusHandle, &g_ServiceStatus);

	BuildSecurityAttributes(&sa);

	pipe = CreateNamedPipe(FUSER_CONTROL_PIPE,
		PIPE_ACCESS_DUPLEX | FILE_FLAG_OVERLAPPED, 
		PIPE_TYPE_MESSAGE | PIPE_READMODE_MESSAGE | PIPE_WAIT,
		1, sizeof(control), sizeof(control), 1000, &sa);

	if (pipe == INVALID_HANDLE_VALUE) {
		// TODO: should do something
		DbgPrintW(L"FuserMounter: failed to create named pipe: %d\n", GetLastError());
	}

	device = CreateFile(
				FUSER_GLOBAL_DEVICE_NAME,			// lpFileName
				GENERIC_READ | GENERIC_WRITE,       // dwDesiredAccess
				FILE_SHARE_READ | FILE_SHARE_WRITE, // dwShareMode
				NULL,                               // lpSecurityAttributes
				OPEN_EXISTING,                      // dwCreationDistribution
				FILE_FLAG_OVERLAPPED,               // dwFlagsAndAttributes
				NULL                                // hTemplateFile
			);
	
	if (device == INVALID_HANDLE_VALUE) {
		// TODO: should do something
		DbgPrintW(L"FuserMounter: failed to open device: %d\n", GetLastError());
	}

	eventConnect = CreateEvent(NULL, FALSE, FALSE, NULL);
	eventUnmount = CreateEvent(NULL, FALSE, FALSE, NULL);
	g_EventControl = CreateEvent(NULL, TRUE, FALSE, NULL);

	g_ServiceStatus.dwWaitHint     = 0;
	g_ServiceStatus.dwCheckPoint   = 0;
	g_ServiceStatus.dwCurrentState = SERVICE_RUNNING;
	SetServiceStatus(g_StatusHandle, &g_ServiceStatus);

	for (;;) {
		ZeroMemory(&ov, sizeof(OVERLAPPED));
		ZeroMemory(&driver, sizeof(OVERLAPPED));
		ZeroMemory(&eventContext, sizeof(EVENT_CONTEXT));

		ov.hEvent = eventConnect;
		driver.hEvent = eventUnmount;
		ConnectNamedPipe(pipe, &ov);
		if (!DeviceIoControl(device, IOCTL_SERVICE_WAIT, NULL, 0,
			&eventContext, sizeof(EVENT_CONTEXT), NULL, &driver)) {
			DWORD error = GetLastError();
			if (error != 997) {
				DbgPrintW(L"FuserMounter: DeviceIoControl error: %d\n", error);
			}
		}

		eventArray[0] = eventConnect;
		eventArray[1] = eventUnmount;
		eventArray[2] = g_EventControl;

		eventNo = WaitForMultipleObjects(3, eventArray, FALSE, INFINITE) - WAIT_OBJECT_0;

		DbgPrintW(L"FuserMouner: get an event\n");
		if (eventNo == 0) {

			DWORD result = 0;

			ZeroMemory(&control, sizeof(control));
			if (ReadFile(pipe, &control, sizeof(control), &result, NULL)) {
				FuserControl(&control);
				WriteFile(pipe, &control, sizeof(control), &result, NULL);
			}
			FlushFileBuffers(pipe);
			DisconnectNamedPipe(pipe);
		} else if (eventNo == 1) {

			if (GetOverlappedResult(device, &driver, &returnedBytes, FALSE)) {
				if (returnedBytes == sizeof(EVENT_CONTEXT)) {
					DbgPrintW(L"FuserMounter: Unmount\n");

					ZeroMemory(&unmount, sizeof(FUSER_CONTROL));
					unmount.Type = FUSER_CONTROL_UNMOUNT;
					wcscpy_s(unmount.DeviceName, sizeof(unmount.DeviceName) / sizeof(WCHAR),
							eventContext.Unmount.DeviceName);
					FuserControl(&unmount);
				} else {
					DbgPrintW(L"FuserMounter: Unmount error\n", control.Type);
				}
			}

		} else if (eventNo == 2) {		
			UnmountAll(); // Removes all mounts before terminating
		
			DbgPrintW(L"FuserMounter: stop mounter service\n");
			g_ServiceStatus.dwWaitHint     = 0;
			g_ServiceStatus.dwCheckPoint   = 0;
			g_ServiceStatus.dwCurrentState = SERVICE_STOPPED;			
			SetServiceStatus(g_StatusHandle, &g_ServiceStatus);

			break;
		}
		else
			break;
	}


	CloseHandle(pipe);
	CloseHandle(eventConnect);
	CloseHandle(g_EventControl);
	CloseHandle(device);
	CloseHandle(eventUnmount);

	DeleteCriticalSection(&g_CriticalSection);
	return;
}




int WINAPI WinMain(HINSTANCE hinst, HINSTANCE hinstPrev, LPSTR lpszCmdLine, int nCmdShow)
{
	SERVICE_TABLE_ENTRY serviceTable[] = {
		{L"FuserMounter", ServiceMain}, {NULL, NULL}
	};

	StartServiceCtrlDispatcher(serviceTable);
	return 0;
}

/* TODO: Obsolete and can be removed
static HANDLE	g_EventLog = NULL;
*/
