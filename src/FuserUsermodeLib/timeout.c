/*
  Dokan : user-mode file system library for Windows

  Copyright (C) 2011 - 2013 Christian Auer christian.auer@gmx.ch
  Copyright (C) 2010 Hiroki Asakawa info@dokan-dev.net

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



#include <process.h>
#include "fuseri.h"


//TODO: Empty and remove file, integrate this method into other file

BOOL FUSERAPI
FuserSendHeartbeat(
	LPCWSTR MountPoint,
	LPCWSTR DeviceName
) {
	FUSER_CONTROL control;
	BOOL	status = FALSE;

	ZeroMemory(&control, sizeof(FUSER_CONTROL));
	control.Type = FUSER_CONTROL_HEARTBEAT;

	wcscpy_s(control.MountPoint, sizeof(control.MountPoint) / sizeof(WCHAR), MountPoint);	
		
	// TODO: Remove and pass through parameter as RAW
	wcscpy_s(control.RawDeviceName, sizeof(control.RawDeviceName) / sizeof(WCHAR), L"\\\\.");
	wcscat_s(control.RawDeviceName, sizeof(control.RawDeviceName) / sizeof(WCHAR), DeviceName);

	status = FuserAgentControl(&control);
	
	return status;
}



