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

// TODO: Check whether this exe is needed at all, possibly consolidate with agent.

#include <windows.h>
#include <stdio.h>
#include "fuser.h"
#include "fuserc.h"


#define GetOption(argc, argv, index) \
	(((argc) > (index) && \
		wcslen((argv)[(index)]) == 2 && \
		(argv)[(index)][0] == L'/')? \
		towlower((argv)[(index)][1]) : L'\0')



int ShowMountList()
{
	FUSER_CONTROL control;
	ULONG index = 0;
	ZeroMemory(&control, sizeof(FUSER_CONTROL));

	control.Type = FUSER_CONTROL_LIST;
	control.Option = 0;
	control.Status = FUSER_CONTROL_SUCCESS;

	while(FuserMountControl(&control)) {
		if (control.Status == FUSER_CONTROL_SUCCESS) {
			fwprintf(stderr, L"[% 2d] MountPoint: %s\n     DeviceName: %s\n",
				control.Option, control.MountPoint, control.DeviceName);
			control.Option++;
		} else {
			return 0;
		}
	}
	return 0;
}		


int Unmount(LPCWSTR	MountPoint, BOOL ForceUnmount)
{

	int status = 0;
	FUSER_CONTROL control;
	ZeroMemory(&control, sizeof(FUSER_CONTROL));

	if (wcslen(MountPoint) == 1 && L'0' <= MountPoint[0] && MountPoint[0] <= L'9') {
		control.Type = FUSER_CONTROL_LIST;
		control.Option = MountPoint[0] - L'0';
		FuserMountControl(&control);

		if (control.Status == FUSER_CONTROL_SUCCESS) {
			status = FuserRemoveMountPoint(control.MountPoint);
		} else {
			fwprintf(stderr, L"Mount entry %d not found\n", control.Option);
			status = -1;
		}
	} else if (ForceUnmount) {
		control.Type = FUSER_CONTROL_UNMOUNT;
		control.Option = FUSER_CONTROL_OPTION_FORCE_UNMOUNT;
		wcscpy_s(control.MountPoint, sizeof(control.MountPoint) / sizeof(WCHAR), MountPoint);
		FuserMountControl(&control);

		if (control.Status == FUSER_CONTROL_SUCCESS) {
			fwprintf(stderr, L"Unmount success: %s", MountPoint);
			status = 0;
		} else {
			fwprintf(stderr, L"Unmount failed: %s", MountPoint);
			status = -1;
		}

	} else {
		status = FuserRemoveMountPoint(MountPoint);
	}

	fwprintf(stderr, L"Unmount status = %d\n", status);
	return status;
}



int ShowUsage()
{
	// TODO: Adjust filename
	// TODO: Completely revise
	fprintf(stderr,
		"fuser /u MountPoint (/f)\n" \
		"fuser /m\n" \
		"fuser /i [d|s|a]\n" \
		"fuser /r [d|s|a]\n" \
		"fuser /v\n" \
		"\n" \
		"Example:\n" \
		"  /u M:               : Unmount M: drive\n" \
		"  /u C:\\mount\\fuser   : Unmount mount point C:\\mount\\fuser\n" \
		"  /u 1                : Unmount mount point 1\n" \
		"  /u M: /f            : Force unmount M: drive\n" \
		"  /m                  : Print mount points list\n" \
		"  /i s                : Install DeviceAgent\n" \
		"  /r d                : Remove driver\n" \
		"  /r a                : Remove driver and DeviceAgent\n" \
		"  /v                  : Print Fuser version\n");
	return -1;
}



int __cdecl
wmain(int argc, PWCHAR argv[])
{
	ULONG	i;
	WCHAR	fileName[MAX_PATH];
	WCHAR	driverFullPath[MAX_PATH];
	WCHAR	deviceAgentFullPath[MAX_PATH];
	WCHAR	type;

	//setlocale(LC_ALL, "");

	GetModuleFileName(NULL, fileName, MAX_PATH);

	// search the last "\"
	for(i = wcslen(fileName) -1; i > 0 && fileName[i] != L'\\'; --i)
		;
	fileName[i] = L'\0';

	ZeroMemory(deviceAgentFullPath, sizeof(deviceAgentFullPath)); 
	ZeroMemory(driverFullPath, sizeof(driverFullPath));
	wcscpy_s(deviceAgentFullPath, MAX_PATH, fileName);  
	deviceAgentFullPath[i] = L'\\';
	wcscat_s(deviceAgentFullPath, MAX_PATH, L"FuserDeviceAgent.exe");  

	GetSystemDirectory(driverFullPath, MAX_PATH);
	wcscat_s(driverFullPath, MAX_PATH, L"\\drivers\\fuser.sys"); // TODO: When change the filename, change this too.

	fwprintf(stderr, L"driver path %s\n", driverFullPath);
	fwprintf(stderr, L"Fuser DeviceAgent Path %s\n", deviceAgentFullPath);


	if (GetOption(argc, argv, 1) == L'v') {		
		fprintf(stderr, "Fuser version : %d\n", FuserVersion());		
		fprintf(stderr, "Fuser driver version : 0x%X\n", FuserDriverVersion());		
		return 0;

	} else if (GetOption(argc, argv, 1) == L'm') {
		return ShowMountList();
	} else if (GetOption(argc, argv, 1) == L'u' && argc == 3) {
		return Unmount(argv[2], FALSE);
	} else if (GetOption(argc, argv, 1) == L'u' &&
				GetOption(argc, argv, 3) == L'f' && argc == 4) {
		return Unmount(argv[2], TRUE);

	} else if (argc < 3 || wcslen(argv[1]) != 2 || argv[1][0] != L'/' ) {
		return ShowUsage();
	}

	type = towlower(argv[2][0]);

	switch(towlower(argv[1][1])) {
	case L'i':
		if (type ==  L'd') {
			if (FuserServiceInstall(FUSER_DRIVER_SERVICE,
									SERVICE_FILE_SYSTEM_DRIVER,
									driverFullPath))
				fprintf(stderr, "driver install ok\n");
			else
				fprintf(stderr, "driver install failed\n");

		} else if (type == L's') {
			if (FuserServiceInstall(FUSER_AGENT_SERVICE,
									SERVICE_WIN32_OWN_PROCESS,
									deviceAgentFullPath))
				fprintf(stderr, "DeviceAgent install ok\n");
			else
				fprintf(stderr, "DeviceAgent install failed\n");
		
		} else if (type == L'a') {
			if (FuserServiceInstall(FUSER_DRIVER_SERVICE,
									SERVICE_FILE_SYSTEM_DRIVER,
									driverFullPath))
				fprintf(stderr, "driver install ok\n");
			else
				fprintf(stderr, "driver install failed\n");

			if (FuserServiceInstall(FUSER_AGENT_SERVICE,
									SERVICE_WIN32_OWN_PROCESS,
									deviceAgentFullPath))
				fprintf(stderr, "DeviceAgent install ok\n");
			else
				fprintf(stderr, "DeviceAgent install failed\n");
		} else if (type == L'n') {
			if (FuserNetworkProviderInstall())
				fprintf(stderr, "network provider install ok\n");
			else
				fprintf(stderr, "network provider install failed\n");
		}
		break;

	case L'r':
		if (type == L'd') {
			if (FuserServiceDelete(FUSER_DRIVER_SERVICE))
				fprintf(stderr, "driver remove ok\n");
			else
				fprintf(stderr, "driver remvoe failed\n");
		
		} else if (type == L's') {
			if (FuserServiceDelete(FUSER_AGENT_SERVICE))
				fprintf(stderr, "DeviceAgent remove ok\n");
			else
				fprintf(stderr, "DeviceAgent remvoe failed\n");	
		
		} else if (type == L'a') {
			if (FuserServiceDelete(FUSER_AGENT_SERVICE))
				fprintf(stderr, "DeviceAgent remove ok\n");
			else
				fprintf(stderr, "DeviceAgent remvoe failed\n");

			if (FuserServiceDelete(FUSER_DRIVER_SERVICE))
				fprintf(stderr, "driver remove ok\n");
			else
				fprintf(stderr, "driver remvoe failed\n");
		} else if (type == L'n') {
			if (FuserNetworkProviderUninstall())
				fprintf(stderr, "network provider remove ok\n");
			else
				fprintf(stderr, "network provider remove failed\n");
		}
		break;
	case L'd':
		if (L'0' <= type && type <= L'9') {
			ULONG mode = type - L'0';
			if (FuserSetDebugMode(mode)) {
				fprintf(stderr, "set debug mode ok\n");
			} else {
				fprintf(stderr, "set debug mode failed\n");
			}
		}
		break;
	default:
		fprintf(stderr, "unknown option\n");
	}

	return 0;
}

/* TODO: Unused code, can be removed
//#include <stdlib.h>
//#include <locale.h>
*/


