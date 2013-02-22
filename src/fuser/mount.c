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
#include "fuseri.h"

// TODO: removes this
#define FUSER_NP_SERVICE_KEY	L"System\\CurrentControlSet\\Services\\Fuser"
#define FUSER_NP_DEVICE_NAME	L"\\Device\\FuserRedirector"
#define FUSER_NP_NAME			L"FuserNP"
#define FUSER_NP_PATH			L"System32\\fusernp.dll"
#define FUSER_NP_ORDER_KEY		L"System\\CurrentControlSet\\Control\\NetworkProvider\\Order"




BOOL FUSERAPI
FuserNetworkProviderInstall() // TODO: completely disclaim it, no networksupport offer
{
	HKEY key;
	DWORD position;
	DWORD type;
	WCHAR buffer[1024];
	DWORD buffer_size = sizeof(buffer);
	ZeroMemory(&buffer, sizeof(buffer));

	RegCreateKeyEx(HKEY_LOCAL_MACHINE, FUSER_NP_SERVICE_KEY L"\\NetworkProvider", 0, NULL,
		REG_OPTION_NON_VOLATILE, KEY_ALL_ACCESS, NULL, &key, &position);

	RegSetValueEx(key, L"DeviceName", 0, REG_SZ,
		(BYTE*)FUSER_NP_DEVICE_NAME, (wcslen(FUSER_NP_DEVICE_NAME)+1) * sizeof(WCHAR));

	RegSetValueEx(key, L"Name", 0, REG_SZ,
		(BYTE*)FUSER_NP_NAME, (wcslen(FUSER_NP_NAME)+1) * sizeof(WCHAR));

	RegSetValueEx(key, L"ProviderPath", 0, REG_SZ,
		(BYTE*)FUSER_NP_PATH, (wcslen(FUSER_NP_PATH)+1) * sizeof(WCHAR));

    RegCloseKey(key);

	RegOpenKeyEx(HKEY_LOCAL_MACHINE, FUSER_NP_ORDER_KEY, 0, KEY_ALL_ACCESS, &key);

	RegQueryValueEx(key, L"ProviderOrder", 0, &type, (BYTE*)&buffer, &buffer_size);

	if (wcsstr(buffer, L",Fuser") == NULL) {
		wcscat_s(buffer, sizeof(buffer) / sizeof(WCHAR), L",Fuser");
		RegSetValueEx(key, L"ProviderOrder", 0, REG_SZ,
			(BYTE*)&buffer, (wcslen(buffer) + 1) * sizeof(WCHAR));
	}

    RegCloseKey(key);
	return TRUE;
}


BOOL FUSERAPI
FuserNetworkProviderUninstall()
{
	HKEY key;
	DWORD type;
	WCHAR buffer[1024];
	WCHAR buffer2[1024];

	DWORD buffer_size = sizeof(buffer);
	ZeroMemory(&buffer, sizeof(buffer));
	ZeroMemory(&buffer2, sizeof(buffer));

	RegOpenKeyEx(HKEY_LOCAL_MACHINE, FUSER_NP_SERVICE_KEY, 0, KEY_ALL_ACCESS, &key);
	RegDeleteKey(key, L"NetworkProvider");

    RegCloseKey(key);

	RegOpenKeyEx(HKEY_LOCAL_MACHINE, FUSER_NP_ORDER_KEY, 0, KEY_ALL_ACCESS, &key);

	RegQueryValueEx(key, L"ProviderOrder", 0, &type, (BYTE*)&buffer, &buffer_size);

	if (wcsstr(buffer, L",Fuser") != NULL) {
		WCHAR* fuser_pos = wcsstr(buffer, L",Fuser");
		wcsncpy_s(buffer2, sizeof(buffer2) / sizeof(WCHAR), buffer, fuser_pos - buffer);
		wcscat_s(buffer2, sizeof(buffer2) / sizeof(WCHAR), fuser_pos + wcslen(L",Fuser"));
		RegSetValueEx(key, L"ProviderOrder", 0, REG_SZ,
			(BYTE*)&buffer2, (wcslen(buffer2) + 1) * sizeof(WCHAR));
	}

    RegCloseKey(key);

	return TRUE;
}





BOOL FUSERAPI
FuserRemoveMountPoint(
	LPCWSTR MountPoint)
{

	FUSER_CONTROL control;
	BOOL result;
	ZeroMemory(&control, sizeof(FUSER_CONTROL));
	control.Type = FUSER_CONTROL_UNMOUNT;
	wcscpy_s(control.MountPoint, sizeof(control.MountPoint) / sizeof(WCHAR), MountPoint);

	DbgPrintW(L"FuserRemoveMountPoint %ws\n", MountPoint);
	result = FuserMountControl(&control);
	if (result) {
		DbgPrint("FuserControl recieved DeviceName:%ws\n", control.DeviceName);
		SendReleaseIRP(control.DeviceName);
	} else {
		DbgPrint("FuserRemoveMountPoint failed\n");
	}
	return result;
}




BOOL FUSERAPI
FuserMountControl(PFUSER_CONTROL Control)
{
	HANDLE pipe;
	DWORD writtenBytes;
	DWORD readBytes;
	DWORD pipeMode;
	DWORD error;
	for (;;) {
		pipe = CreateFile(FUSER_CONTROL_PIPE,  GENERIC_READ|GENERIC_WRITE,
						0, NULL, OPEN_EXISTING, 0, NULL);
		if (pipe != INVALID_HANDLE_VALUE) {
			break;
		}
		error = GetLastError();
		if (error == ERROR_PIPE_BUSY) {
			if (!WaitNamedPipe(FUSER_CONTROL_PIPE, NMPWAIT_USE_DEFAULT_WAIT)) {
				DbgPrint("FuserMounter service : ERROR_PIPE_BUSY\n");
				return FALSE;
			}
			continue;
		} else if (error == ERROR_ACCESS_DENIED) {
			DbgPrint("failed to connect FuserMounter service: access denied\n");
			return FALSE;
		} else {
			DbgPrint("failed to connect FuserMounter service: %d\n", GetLastError());
			return FALSE;
		}
	}

	pipeMode = PIPE_READMODE_MESSAGE|PIPE_WAIT;

	if(!SetNamedPipeHandleState(pipe, &pipeMode, NULL, NULL)) {
		DbgPrint("failed to set named pipe state: %d\n", GetLastError());
		CloseHandle(pipe);
		return FALSE;
	}


	if(!TransactNamedPipe(pipe, Control, sizeof(FUSER_CONTROL),
		Control, sizeof(FUSER_CONTROL), &readBytes, NULL)) {
		DbgPrint("failed to transact named pipe: %d\n", GetLastError());
	}

	CloseHandle(pipe);
	
	if(Control->Status != FUSER_CONTROL_FAIL) {
		return TRUE;
	} else {
		return FALSE;
	}
}



static BOOL
FuserServiceControl(
	LPCWSTR	ServiceName,
	ULONG	Type)
{
	SC_HANDLE controlHandle;
	SC_HANDLE serviceHandle;
	SERVICE_STATUS ss;
	BOOL result = TRUE;

	controlHandle = OpenSCManager(NULL, NULL, SC_MANAGER_CONNECT);

	if (controlHandle == NULL) {
		FuserDbgPrint("failed to open SCM: %d\n", GetLastError());
		return FALSE;
	}

	serviceHandle = OpenService(controlHandle, ServiceName,
		SERVICE_START | SERVICE_STOP | SERVICE_QUERY_STATUS | DELETE);

	if (serviceHandle == NULL) {
		FuserDbgPrintW(L"failed to open Service (%s): %d\n", ServiceName, GetLastError());
		CloseServiceHandle(controlHandle);
		return FALSE;
	}
	
	QueryServiceStatus(serviceHandle, &ss);

	if (Type == FUSER_SERVICE_DELETE) {
		if (DeleteService(serviceHandle)) {
			FuserDbgPrintW(L"Service (%s) deleted\n", ServiceName);
			result = TRUE;
		} else {
			FuserDbgPrintW(L"failed to delete service (%s): %d\n", ServiceName, GetLastError());
			result = FALSE;
		}

	} else if (ss.dwCurrentState == SERVICE_STOPPED && Type == FUSER_SERVICE_START) {
		if (StartService(serviceHandle, 0, NULL)) {
			FuserDbgPrintW(L"Service (%s) started\n", ServiceName);
			result = TRUE;
		} else {
			FuserDbgPrintW(L"failed to start service (%s): %d\n", ServiceName, GetLastError());
			result = FALSE;
		}
	} else if (ss.dwCurrentState == SERVICE_RUNNING && Type == FUSER_SERVICE_STOP) {
		// TODO: when stopping the service, the active drives should be disconnected
		if (ControlService(serviceHandle, SERVICE_CONTROL_STOP, &ss)) {
			FuserDbgPrintW(L"Service (%s) stopped\n", ServiceName);
			result = TRUE;
		} else {
			FuserDbgPrintW(L"failed to stop service (%s): %d\n", ServiceName, GetLastError());
			result = FALSE;
		}
	}

	CloseServiceHandle(serviceHandle);
	CloseServiceHandle(controlHandle);

	Sleep(100);
	return result;
}





BOOL FUSERAPI
FuserServiceInstall(
	LPCWSTR	ServiceName,
	DWORD	ServiceType,
	LPCWSTR ServiceFullPath)
{
	SC_HANDLE	controlHandle;
	SC_HANDLE	serviceHandle;

	controlHandle = OpenSCManager(NULL, NULL, SC_MANAGER_CREATE_SERVICE);
	if (controlHandle == NULL) {
		FuserDbgPrint("failed to open SCM");
		return FALSE;
	}

	serviceHandle = CreateService(controlHandle, ServiceName, ServiceName, 0,
		ServiceType, SERVICE_AUTO_START, SERVICE_ERROR_IGNORE,
		ServiceFullPath, NULL, NULL, NULL, NULL, NULL);

	if (serviceHandle == NULL) {
		if (GetLastError() == ERROR_SERVICE_EXISTS) {
			FuserDbgPrintW(L"Service (%s) is already installed\n", ServiceName);
		} else {
			FuserDbgPrintW(L"failted to install service (%s): %d\n", ServiceName, GetLastError());
		}
		CloseServiceHandle(controlHandle);
		return FALSE;
	}

	CloseServiceHandle(serviceHandle);
	CloseServiceHandle(controlHandle);

	FuserDbgPrintW(L"Service (%s) installed\n", ServiceName);

	if (FuserServiceControl(ServiceName, FUSER_SERVICE_START)) {
		FuserDbgPrintW(L"Service (%s) started\n", ServiceName);
		return TRUE;
	} else {
		FuserDbgPrintW(L"Service (%s) start failed\n", ServiceName);
		return FALSE;
	}
}


static BOOL
FuserServiceCheck(
	LPCWSTR	ServiceName)
{
	SC_HANDLE controlHandle;
	SC_HANDLE serviceHandle;
	SERVICE_STATUS ss;

	controlHandle = OpenSCManager(NULL, NULL, SC_MANAGER_CONNECT);

	if (controlHandle == NULL) {
		DbgPrint("failed to open SCM: %d\n", GetLastError());
		return FALSE;
	}

	serviceHandle = OpenService(controlHandle, ServiceName,
		SERVICE_START | SERVICE_STOP | SERVICE_QUERY_STATUS);

	if (serviceHandle == NULL) {
		CloseServiceHandle(controlHandle);
		return FALSE;
	}
	
	CloseServiceHandle(serviceHandle);
	CloseServiceHandle(controlHandle);

	return TRUE;
}




BOOL FUSERAPI
FuserServiceDelete(
	LPCWSTR	ServiceName)
{
	if (FuserServiceCheck(ServiceName)) {
		FuserServiceControl(ServiceName, FUSER_SERVICE_STOP);
		if (FuserServiceControl(ServiceName, FUSER_SERVICE_DELETE)) {
			return TRUE;
		} else {
			return FALSE;
		}
	}
	return TRUE;
}



BOOL
FuserMount(
	LPCWSTR	MountPoint,
	LPCWSTR	DeviceName,
	BOOL    UseHeartbeatControl)
{
	FUSER_CONTROL control;

	ZeroMemory(&control, sizeof(FUSER_CONTROL));
	control.Type = FUSER_CONTROL_MOUNT;
	control.Option = 0;
	if (UseHeartbeatControl){
		control.Option = 1;
	}

	wcscpy_s(control.MountPoint, sizeof(control.MountPoint) / sizeof(WCHAR), MountPoint);
	wcscpy_s(control.DeviceName, sizeof(control.DeviceName) / sizeof(WCHAR), DeviceName);

	return  FuserMountControl(&control);
}


BOOL FUSERAPI
FuserUnmount(
	WCHAR	DriveLetter)
{
	WCHAR mountPoint[] = L"M:\\";
	mountPoint[0] = DriveLetter;
	return FuserRemoveMountPoint(mountPoint);
}

