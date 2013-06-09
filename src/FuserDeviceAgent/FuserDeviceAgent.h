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

#ifndef _MOUNT_H_
#define _MOUNT_H_

#include "fuserc.h"
#include "devioctl.h"
#include "list.h"


#ifdef __cplusplus
extern "C" {
#endif




// ---------------------------------------------------------------------
//                 S E R V I C E
//

#define SERVICEDESCRIPTION              L"Controls and manage the virtual fuser devices."
#define SERVICE_NAME_DRIVER             L"FuserDeviceDriver"
#define SERVICE_NAME_AGENT              L"FuserDeviceAgent"

// ---------------------------------------------------------------------





// ---------------------------------------------------------------------
//                 H E A R T B E A T
//

#define HEARTBEAT_INTERVAL              500  		// (miliseconds) Interval to work the Heartbeat-procedure
#define HEARTBEAT_TIMEOUT     			1000 * 10	// (miliseconds) Timeout for autmatic device-unmount if no heartbeat signal found.  should be multiple of HEARTBEAT_INTERVAL
#define HEARTBEAT_SENDKEEPALIVE			1000 * 3    // (miliseconds) Interval for automatic send KeepAlive to the driver. Should be a multiple of HEARTBEAT_INTERVAL
// ---------------------------------------------------------------------







typedef struct _MOUNT_ENTRY {
	LIST_ENTRY		ListEntry;
	FUSER_CONTROL	MountControl;
	
	//Heartbeat:
	BOOL			HeartbeatActive;
	HANDLE			HeartbeatThread;	
	BOOL			HeartbeatSignal;
	BOOL			HeartbeatAbort;
	
	
} MOUNT_ENTRY, *PMOUNT_ENTRY;

BOOL FuserControlMount(LPCWSTR MountPoint, LPCWSTR RawDeivceName);

BOOL FuserControlUnmount(LPCWSTR MountPoint);

VOID HeartbeatStart(PMOUNT_ENTRY mount);

VOID HeartbeatStop(PMOUNT_ENTRY mount);

VOID HeartbeatSetAliveSignal(PMOUNT_ENTRY mount);

VOID SendReleaseIRPraw(LPCWSTR rawDeviceName);

VOID RemoveMountEntry(PMOUNT_ENTRY MountEntry);

VOID StartServiceMode();

#ifdef __cplusplus
}
#endif

#endif
