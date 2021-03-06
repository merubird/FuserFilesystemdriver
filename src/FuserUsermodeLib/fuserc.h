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

// TODO: change name of this file c=control = no longer exists

#ifndef _FUSERC_H_
#define _FUSERC_H_

#include "fuser.h"

#ifdef __cplusplus
extern "C" {
#endif


#define FUSER_MOUNT_POINT_SUPPORTED_VERSION 600 // TODO: Remove this
#define FUSER_SECURITY_SUPPORTED_VERSION	600 // TODO: Remove this



#define FUSER_GLOBAL_DEVICE_NAME    	L"\\\\.\\FuserSystem" //local ressource refere to FUSER_GLOBAL_SYMBOLIC_LINK_NAME
#define FUSER_AGENT_CONTROL_PIPE		L"\\\\.\\pipe\\FuserDeviceAgentCommand" //communication with FuserDeviceAgent


#define FUSER_THREADS_MAX		15		//maximum worker threads for fuser multithreading
#define FUSER_THREADS_DEFAULT	7		//default count threads for fuser multithreading



#define FUSER_CONTROL_MOUNT			1
#define FUSER_CONTROL_UNMOUNT		2
#define FUSER_CONTROL_HEARTBEAT		88 // TODO: Arrange values meaningful, clearly define what return values are and what commands are

#define FUSER_CONTROL_OPTION_FORCE_UNMOUNT 1


#define FUSER_CONTROL_FAIL		0
#define FUSER_CONTROL_SUCCESS	1 // used by DeviceAgent
#define FUSER_CONTROL_CHECK		3 // TODO: check what this is supposed to do and possibly remove it, used by DeviceAgent
#define FUSER_CONTROL_FIND		4 // TODO: check what this is supposed to do and possibly remove it, used by DeviceAgent
#define FUSER_CONTROL_LIST		5 // used by DeviceAgent




#define FUSER_KEEPALIVE_TIME	3000 // in miliseconds       // TODO: should be moved to .NET class

// TODO: Reflect for alternative (.net), maybe remove both variables:
// FuserOptions->DebugMode is ON?
extern	BOOL	g_DebugMode;

// FuserOptions->UseStdErr is ON?
extern	BOOL	g_UseStdErr;



static
VOID
FuserDbgPrint(LPCSTR format, ...)
{
	char buffer[512];
	va_list argp;
	va_start(argp, format);
    vsprintf_s(buffer, sizeof(buffer)/sizeof(char), format, argp);
    va_end(argp);
	if (g_UseStdErr)
		fprintf(stderr, buffer);
	else
		OutputDebugStringA(buffer);
}



static
VOID
FuserDbgPrintW(LPCWSTR format, ...)
{
	WCHAR buffer[512];
	va_list argp;
	va_start(argp, format);
    vswprintf_s(buffer, sizeof(buffer)/sizeof(WCHAR), format, argp);
    va_end(argp);
	if (g_UseStdErr)
		fwprintf(stderr, buffer);
	else
		OutputDebugStringW(buffer);
}


#define DbgPrint(format, ... ) \
	do {\
		if (g_DebugMode) {\
			FuserDbgPrint(format, __VA_ARGS__);\
		}\
	} while(0)

#define DbgPrintW(format, ... ) \
	do {\
		if (g_DebugMode) {\
			FuserDbgPrintW(format, __VA_ARGS__);\
		}\
	} while(0)
	
	
	
	


typedef struct _FUSER_CONTROL {
	ULONG	Type;
	WCHAR	MountPoint[MAX_PATH];	
	WCHAR	RawDeviceName[64];
	ULONG	Option;
	ULONG	Status;

} FUSER_CONTROL, *PFUSER_CONTROL;



BOOL FUSERAPI FuserAgentControl(PFUSER_CONTROL Control);


BOOL FUSERAPI
FuserServiceInstall(
	LPCWSTR	ServiceName,
	DWORD	ServiceType,
	LPCWSTR ServiceFullPath);


BOOL FUSERAPI
FuserServiceDelete(
	LPCWSTR	ServiceName);

	

#ifdef __cplusplus
}
#endif



#endif