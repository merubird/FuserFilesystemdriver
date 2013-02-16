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

#ifndef _FUSERC_H_
#define _FUSERC_H_

#include "fuser.h"

#ifdef __cplusplus
extern "C" {
#endif



// TODO: Remove these codes
/*
#define FUSER_MOUNTER_SERVICE L"FuserMounter"
#define FUSER_DRIVER_SERVICE L"Fuser"
#define FUSER_CONTROL_CHECK		3
#define FUSER_CONTROL_FIND		4
#define FUSER_CONTROL_LIST		5
#define FUSER_CONTROL_OPTION_FORCE_UNMOUNT 1
#define FUSER_CONTROL_SUCCESS	1
*/



#define FUSER_MOUNT_POINT_SUPPORTED_VERSION 600 // TODO: Remove this
#define FUSER_SECURITY_SUPPORTED_VERSION	600 // TODO: Remove this


#define FUSER_GLOBAL_DEVICE_NAME	L"\\\\.\\Fuser" // TODO: adapt at the opportunity
#define FUSER_CONTROL_PIPE			L"\\\\.\\pipe\\FuserMounter"


#define FUSER_MAX_THREAD		15


#define FUSER_CONTROL_MOUNT		1
#define FUSER_CONTROL_UNMOUNT	2


#define FUSER_CONTROL_FAIL		0


#define FUSER_SERVICE_START		1
#define FUSER_SERVICE_STOP		2
#define FUSER_SERVICE_DELETE	3

#define FUSER_KEEPALIVE_TIME	3000 // in miliseconds       // TODO: should be moved to .NET class


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
	WCHAR	DeviceName[64];
	ULONG	Option;
	ULONG	Status;

} FUSER_CONTROL, *PFUSER_CONTROL;



BOOL FUSERAPI
FuserMountControl(PFUSER_CONTROL Control);


BOOL FUSERAPI
FuserServiceInstall(
	LPCWSTR	ServiceName,
	DWORD	ServiceType,
	LPCWSTR ServiceFullPath);


BOOL FUSERAPI
FuserServiceDelete(
	LPCWSTR	ServiceName);




BOOL FUSERAPI
FuserNetworkProviderInstall(); // TODO: remove method



BOOL FUSERAPI
FuserNetworkProviderUninstall();	// TODO: remove method



BOOL FUSERAPI
FuserSetDebugMode(ULONG Mode);


	

#ifdef __cplusplus
}
#endif



#endif