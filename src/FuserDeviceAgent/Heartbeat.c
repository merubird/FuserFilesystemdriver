/*

Copyright (C) 2011 - 2013 Christian Auer christian.auer@gmx.ch

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
#include <winioctl.h>
#include <stdio.h>
#include <stdlib.h>
#include <process.h>
#include "FuserDeviceAgent.h"

#include "fuser.h"
#include "public.h"


//based on values of FuserDeviceAgent.h
#define HEARTBEAT_TIMEOUT_COUNT HEARTBEAT_TIMEOUT / HEARTBEAT_INTERVAL
#define HEARTBEAT_SENDKEEPALIVE_COUNT HEARTBEAT_SENDKEEPALIVE / HEARTBEAT_INTERVAL


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





// ask driver to release all pending IRP to prepare for Unmount.
VOID
SendReleaseIRP(
	LPCWSTR	DeviceName)
{
	ULONG	returnedLength;	
	WCHAR rawDeviceName[MAX_PATH];
	
	wcscpy_s(rawDeviceName, MAX_PATH, L"\\\\.");
	wcscat_s(rawDeviceName, MAX_PATH, DeviceName);
	
	SendToDevice(
				rawDeviceName,
				IOCTL_EVENT_RELEASE,
				NULL,
				0,
				NULL,
				0,
				&returnedLength);		
}

BOOL KeepAliveSendSignal(HANDLE device){
	ULONG	returnedLength;
	
	return DeviceIoControl(
			device,                 // Handle to device
			IOCTL_KEEPALIVE,			// IO Control code
			NULL,		    // Input Buffer to driver.
			0,			// Length of input buffer in bytes.
			NULL,           // Output Buffer from driver.
			0,			// Length of output buffer in bytes.
			&returnedLength,		    // Bytes placed in buffer.
			NULL                    // synchronous call
		);	
}

HANDLE KeepAliveOpen(LPCWSTR DeviceName) {
	HANDLE	device;	
	WCHAR rawDeviceName[MAX_PATH];
	
	wcscpy_s(rawDeviceName, MAX_PATH, L"\\\\.");
	wcscat_s(rawDeviceName, MAX_PATH, DeviceName);
	
	device = CreateFile(
				rawDeviceName,
				GENERIC_READ | GENERIC_WRITE,       // dwDesiredAccess
                FILE_SHARE_READ | FILE_SHARE_WRITE, // dwShareMode
                NULL,                               // lpSecurityAttributes
                OPEN_EXISTING,                      // dwCreationDistribution
                0,                                  // dwFlagsAndAttributes
                NULL                                // hTemplateFile
			);
	return device;
}


DWORD WINAPI
HeartbeatCheck (PMOUNT_ENTRY mount)
{
	// Check whether synchronisation is important here.
	BOOL unmount = FALSE;
	int timeout = 0;
	int keepalive = HEARTBEAT_SENDKEEPALIVE_COUNT;
	HANDLE device;	
	
	device = KeepAliveOpen(mount->MountControl.DeviceName);
	if (device == INVALID_HANDLE_VALUE){
		//Device not found, unmount:
		mount->HeartbeatAbort=TRUE;  unmount = TRUE;
	}

	while(!mount->HeartbeatAbort){
		Sleep(HEARTBEAT_INTERVAL);
		timeout++;
		keepalive++;
	
		if (mount->HeartbeatSignal){
			mount->HeartbeatSignal=FALSE;
			timeout = 0; //Timeout Reset		
		}
		
		if (timeout >= HEARTBEAT_TIMEOUT_COUNT){
			unmount = TRUE;
			mount->HeartbeatAbort = TRUE;
		}
		
		if (keepalive >= HEARTBEAT_SENDKEEPALIVE_COUNT){
			if (!KeepAliveSendSignal(device)){
				unmount = TRUE;
				mount->HeartbeatAbort = TRUE;
			}
		}
	}
	CloseHandle(device);
	
	if (unmount){
		mount->HeartbeatActive = FALSE;

		if (FuserControlUnmount(mount->MountControl.MountPoint)) {
			RemoveMountEntry(mount);
			SendReleaseIRP(mount->MountControl.DeviceName);
		}
	}
	
	return 0;
}


VOID
HeartbeatStart(PMOUNT_ENTRY mount)
{	
	HANDLE	threadId = 0;
	
	if (mount == NULL){
		return;
	}	
	if (mount->HeartbeatActive){
		return;
	}
	
	mount->HeartbeatSignal = FALSE;
	mount->HeartbeatAbort = FALSE;

	threadId = (HANDLE)_beginthreadex(
			NULL, // Security Atributes
			0, //stack size
			HeartbeatCheck,
			mount, // param
			0, // create flag
			NULL);
			
	mount->HeartbeatThread = threadId;
	mount->HeartbeatActive = TRUE;
}


VOID
HeartbeatStop(PMOUNT_ENTRY mount){
	if (mount == NULL){
		return;
	}	
	if (!mount->HeartbeatActive){
		return;
	}
		
	mount->HeartbeatAbort = TRUE;
	mount->HeartbeatActive = FALSE;
	WaitForSingleObject(mount->HeartbeatThread, INFINITE);	
	// TODO: CloseHandle and copy _endthreadex in multithreading
}


VOID
HeartbeatSetAliveSignal(PMOUNT_ENTRY mount)
{	
	if (mount == NULL){
		return;
	}	
	if (!mount->HeartbeatActive){
		return;
	}
	
	mount->HeartbeatSignal = TRUE;		
}


