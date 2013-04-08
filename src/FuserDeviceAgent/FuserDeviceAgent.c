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
#include <shellapi.h>
#include "FuserDeviceAgent.h"
#include "public.h"
#include "..\_general\version.h"

#define MAX_BUFFER_SIZE 2048

#define SERVICECONTROL_START    1
#define SERVICECONTROL_STOP     2
#define SERVICECONTROL_DELETE   3


// TODO: Include a function so that the driver can be unloaded -> update without reboot

VOID DisplayMessage(LPCWSTR message)
{
    int msgboxID = MessageBox(
        NULL,
        message,
        (LPCWSTR)L"Fuser Device Agent - Info",
        MB_OK
    );

   return;
}
VOID DisplayWarning(LPCWSTR message)
{
    int msgboxID = MessageBox(
        NULL,
        message,
        (LPCWSTR)L"Fuser Device Agent - Warning",
        MB_OK + MB_ICONWARNING
    );

   return;
}

VOID ShowMountList()
{	
	WCHAR  buffer[MAX_BUFFER_SIZE];
	WCHAR  partBuff[MAX_BUFFER_SIZE];

	FUSER_CONTROL control;
	ULONG index = 0;
	ZeroMemory(&control, sizeof(FUSER_CONTROL));
	ZeroMemory(buffer, sizeof(buffer));	

	control.Type = FUSER_CONTROL_LIST;
	control.Option = 0;
	control.Status = FUSER_CONTROL_SUCCESS;

	wcscpy_s(buffer, MAX_BUFFER_SIZE, L"Mountpoint list:\n\n");
	
	while(FuserAgentControl(&control)) {
		if (control.Status == FUSER_CONTROL_SUCCESS) {
			ZeroMemory(partBuff, sizeof(partBuff));
			_snwprintf(partBuff,100, L"[%d] MountPoint: %s  \t> Device: %s\n",
				control.Option, control.MountPoint, control.DeviceName);
						
			wcscat_s(buffer, MAX_BUFFER_SIZE, partBuff);				
			control.Option++;
		} else {
			break;
		}
	}
	
	if (control.Option == 0){
		wcscpy_s(buffer, MAX_BUFFER_SIZE, L"No Devices are mounted!\n");
	}
	
		
	DisplayMessage(buffer);
	return;	
}

int ServiceControl(LPCWSTR ServiceName, ULONG Type) {
	SC_HANDLE controlHandle;
	SC_HANDLE serviceHandle;
	SERVICE_STATUS ss;
	int ret = 0; // may only assign x0 numbers
	

	controlHandle = OpenSCManager(NULL, NULL, SC_MANAGER_CONNECT);

	if (controlHandle == NULL) {		
		return 10;
	}

	serviceHandle = OpenService(controlHandle, ServiceName, SERVICE_START | SERVICE_STOP | SERVICE_QUERY_STATUS | DELETE);

	if (serviceHandle == NULL) {		
		CloseServiceHandle(controlHandle);
		return 20;
	}
	
	QueryServiceStatus(serviceHandle, &ss);
	
	if (Type == SERVICECONTROL_DELETE) {
		if (DeleteService(serviceHandle)) {
			ret = 0;
		} else {
			ret = 30;
		}
	} else if (ss.dwCurrentState == SERVICE_RUNNING && Type == SERVICECONTROL_STOP) {		
		if (ControlService(serviceHandle, SERVICE_CONTROL_STOP, &ss)) {			
			ret = 0;
		} else {			
			ret = 40;
		}
	} else if (ss.dwCurrentState == SERVICE_STOPPED && Type == SERVICECONTROL_START) {
		if (StartService(serviceHandle, 0, NULL)) {
			ret = 0;
		} else {
			ret = 50;
		}
	}
	
	CloseServiceHandle(serviceHandle);
	CloseServiceHandle(controlHandle);

	Sleep(100);
	return ret;
}

int ServiceInstall(LPCWSTR ServiceName, DWORD ServiceType, LPCWSTR ServicePath){
	SC_HANDLE	controlHandle;
	SC_HANDLE	serviceHandle;
	SERVICE_DESCRIPTION sd;
	int tmp;

	controlHandle = OpenSCManager(NULL, NULL, SC_MANAGER_CREATE_SERVICE);
	if (controlHandle == NULL) {
		return 1;
	}

	serviceHandle = CreateService(controlHandle, ServiceName, ServiceName, 0,
		ServiceType, SERVICE_AUTO_START, SERVICE_ERROR_IGNORE,
		ServicePath, NULL, NULL, NULL, NULL, NULL);

	if (serviceHandle == NULL) {		
		tmp = GetLastError();
		CloseServiceHandle(controlHandle);
		
		if (tmp == ERROR_SERVICE_EXISTS) {
			return 2;
		} else {
			return 3;
		}
	}	
	
	CloseServiceHandle(serviceHandle);
	CloseServiceHandle(controlHandle);
	
	return ServiceControl(ServiceName, SERVICECONTROL_START);
}

BOOL ChangeServiceDescription(LPCWSTR ServiceName, LPWSTR ServiceDescription)
{
    SC_HANDLE controlHandle;
    SC_HANDLE serviceHandle;
    SERVICE_DESCRIPTION sd;
	BOOL ret = FALSE;
    
    // Get a handle to the SCM database. 

    controlHandle = OpenSCManager( 
        NULL,                    // local computer
        NULL,                    // ServicesActive database 
        SC_MANAGER_ALL_ACCESS);  // full access rights 
 
    if (NULL == controlHandle) 
    {        
        return FALSE;
    }

    // Get a handle to the service.

    serviceHandle = OpenService( 
        controlHandle,            // SCM database 
        ServiceName,               // name of service 
        SERVICE_CHANGE_CONFIG);  // need change config access 
 
    if (serviceHandle == NULL)
    {         
        CloseServiceHandle(controlHandle);
        return FALSE;
    }    

    // Change the service description.
    sd.lpDescription = ServiceDescription;

    if( ChangeServiceConfig2(
        serviceHandle,                 // handle to service
        SERVICE_CONFIG_DESCRIPTION, // change: description
        &sd) )                      // new description
    {
        ret = TRUE;
    }
    

    CloseServiceHandle(serviceHandle); 
    CloseServiceHandle(controlHandle);
	return ret;
}



BOOL ServiceCheck(LPCWSTR ServiceName){
	SC_HANDLE controlHandle;
	SC_HANDLE serviceHandle;
	SERVICE_STATUS ss;

	controlHandle = OpenSCManager(NULL, NULL, SC_MANAGER_CONNECT);

	if (controlHandle == NULL) {		
		return FALSE;
	}

	serviceHandle = OpenService(controlHandle, ServiceName, SERVICE_START | SERVICE_STOP | SERVICE_QUERY_STATUS);

	if (serviceHandle == NULL) {
		CloseServiceHandle(controlHandle);
		return FALSE;
	}
	
	CloseServiceHandle(serviceHandle);
	CloseServiceHandle(controlHandle);

	return TRUE;
}

int ServiceUninstall(LPCWSTR ServiceName) {
	if (ServiceCheck(ServiceName)) {
		ServiceControl(ServiceName, SERVICECONTROL_STOP);
		return ServiceControl(ServiceName, SERVICECONTROL_DELETE);
	}
	return 0;	
}

int DriverInstallation(BOOL showMessage){
	WCHAR  driverPath[MAX_PATH];
	WCHAR  deviceAgentPath[MAX_PATH];
	WCHAR  buffer[MAX_BUFFER_SIZE];	
	int ret = 0;
		
	ZeroMemory(driverPath, sizeof(driverPath));
	ZeroMemory(deviceAgentPath, sizeof(deviceAgentPath)); 
	ZeroMemory(buffer, sizeof(buffer));
	
	GetModuleFileName(NULL, deviceAgentPath, MAX_PATH);
	wcscat_s(deviceAgentPath, MAX_PATH, L" /s");  
	

	GetSystemDirectory(driverPath, MAX_PATH);
	wcscat_s(driverPath, MAX_PATH, L"\\drivers\\fuser.sys");

	if (showMessage){
		_snwprintf(buffer,MAX_BUFFER_SIZE,	L"Installation start with the follow parameters:\n" \
											L"Driver-Path: \t%s\n" \
											L"DeviceAgent-Path: \t%s\n"
											, driverPath, deviceAgentPath);
		DisplayMessage(buffer);
	}
	
	ret = ServiceInstall(SERVICE_NAME_DRIVER, SERVICE_FILE_SYSTEM_DRIVER, driverPath);
	if (ret != 0){
		if (showMessage){
			ZeroMemory(buffer, sizeof(buffer));
			_snwprintf(buffer,MAX_BUFFER_SIZE,	L"Driver installation failed! Errorcode: %d" , ret);
			DisplayWarning(buffer);
		}
		return 1000 + ret;
	}
	
	
	ret = ServiceInstall(SERVICE_NAME_AGENT, SERVICE_WIN32_OWN_PROCESS, deviceAgentPath);
	if (ret != 0){		
		if (showMessage){
			ZeroMemory(buffer, sizeof(buffer));
			_snwprintf(buffer,MAX_BUFFER_SIZE,	L"DeviceAgent installation failed! Errorcode: %d" , ret);
			DisplayWarning(buffer);
		}
		return 2000 + ret;
	}
	ChangeServiceDescription(SERVICE_NAME_AGENT, SERVICEDESCRIPTION);
	
	
	return 0;
}


int DriverUninstallation(BOOL showMessage){
	WCHAR  buffer[MAX_BUFFER_SIZE];	
	int ret = 0;
	
	//ZeroMemory(buffer, sizeof(buffer));

	ret = ServiceUninstall(SERVICE_NAME_AGENT);
	if (ret != 0){
		if (showMessage){
			ZeroMemory(buffer, sizeof(buffer));
			_snwprintf(buffer,MAX_BUFFER_SIZE,	L"DeviceAgent uninstallation failed! Errorcode: %d" , ret);
			DisplayWarning(buffer);
		}
		return 2000 + ret;
	}

	ret = ServiceUninstall(SERVICE_NAME_DRIVER); 
	if (ret != 0){
		if (showMessage){
			ZeroMemory(buffer, sizeof(buffer));
			_snwprintf(buffer,MAX_BUFFER_SIZE,	L"Driver uninstallation failed! Errorcode: %d" , ret);
			DisplayWarning(buffer);
		}
		return 1000 + ret;
	}

	return ret;
}


ULONG GetBinaryVersion(){
	FUSER_VERSION_SINGLE version;
	
	version.FullValue.Major = VER_MAJOR;
	version.FullValue.Minor = VER_MINOR;
	version.FullValue.Revision = VER_REVISION;
	
	return version.SingleValue;
}


VOID ShowVersion(){
	WCHAR  buffer[MAX_BUFFER_SIZE];	
	FUSER_VERSION_SINGLE ver;
	ZeroMemory(buffer, sizeof(buffer));
	
	ver.SingleValue = FuserVersion();
	if (ver.SingleValue == 0){		
		DisplayWarning(L"Fuser Filesystemdriver does not work correct! Perhaps different driver parts installed or driver is not running.");
	} else {
		if (ver.SingleValue != GetBinaryVersion()){
			DisplayWarning(L"Version of DeviceAgent is different to Filesystemdriver!");
		} else {	
			_snwprintf(buffer,MAX_BUFFER_SIZE,	L"Fuser version: %d.%d.%d\n"  , ver.FullValue.Major, ver.FullValue.Minor, ver.FullValue.Revision);
			DisplayMessage(buffer);
		}
	}
}

int ExecuteCommand(LPWSTR *ParamList, int CountParam){
	if( ParamList != NULL && CountParam > 1) {
		if (CountParam >= 2){
		
			if (_wcsicmp(ParamList[1], L"/s") == 0){			
				//service-mode
				StartServiceMode();
				return 0;
			}
				
			if (_wcsicmp(ParamList[1], L"/v") == 0){			
				//Show Version-Info
				ShowVersion();				
				return 0;
			}

			if (_wcsicmp(ParamList[1], L"/l") == 0){
				//List all mounted devices
				ShowMountList();				
				return 0;
			}
			
			if (_wcsicmp(ParamList[1], L"/i") == 0){
				//install the driver:
				return DriverInstallation(FALSE);
			}
			
			if (_wcsicmp(ParamList[1], L"/r") == 0){
				//remove the driver:
				return DriverUninstallation(FALSE);
			}
		}
		
		if (CountParam >= 3){
			if (_wcsicmp(ParamList[1], L"/u") == 0){
				if (FuserDeviceUnmount(ParamList[2])){
					return 0;
				} else {
					return 1;
				}
			}
		}
	}
	
	DisplayMessage(  		
		L"FuserDeviceAgent.exe /u MountPoint \n" \
		L"FuserDeviceAgent.exe /l\n" \
		L"FuserDeviceAgent.exe /i\n" \
		L"FuserDeviceAgent.exe /r\n" \
		L"FuserDeviceAgent.exe /v\n" \
		L"\n" \
		L"Example:\n" \
		L"  /u X:             \t> Unmount mount X:\n" \
		L"  /u C:\\fuser      \t> Unmount mount C:\\fuser\n" \
		L"  /l                \t> Show list of active mountpoints\n" \
		L"  /i                \t> Install Systemdriver and DeviceAgent\n" \
		L"  /r                \t> Remove Systemdriver and DeviceAgent\n" \
		L"  /v                \t> Show Fuser Systemdriver Version\n"
	);		
	return 2; //Not supported Parameter
}


int WINAPI WinMain(HINSTANCE hinst, HINSTANCE hinstPrev, LPSTR lpszCmdLine, int nCmdShow)
{  
	LPWSTR *szArglist;
	int nArgs;	
	int ret = 1; //Unknown error
	
	szArglist = CommandLineToArgvW(GetCommandLineW(), &nArgs);   
	if( szArglist != NULL )
	{
		ret = ExecuteCommand(szArglist, nArgs);
		LocalFree(szArglist); // Free memory allocated for CommandLineToArgvW arguments.
	}   
	
	return ret;	
}
