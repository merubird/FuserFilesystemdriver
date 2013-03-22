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

#ifndef _FUSER_H_
#define _FUSER_H_

#include <ntifs.h>
#include <ntdddisk.h>
#include <ntstrsafe.h>

#include "public.h"



// ************************************************
// *  Definitionen:                               *
// ************************************************

#define FUSER_DEBUG_DEFAULT 0

extern ULONG g_Debug; // TODO: find out where is used

#define FUSER_DEFAULT_VOLUME_LABEL			L"Fuser Filesystem Driver"
#define FUSER_DEFAULT_SERIALNUMBER			0x19831116;

#define FUSER_MDL_ALLOCATED					0x1 // TODO: change name


// global device unique for systemdriver
#define FUSER_GLOBAL_DEVICE_NAME			L"\\Device\\FuserSystem"
#define FUSER_GLOBAL_SYMBOLIC_LINK_NAME		L"\\DosDevices\\Global\\FuserSystem" // access: \\.\FuserSystem

#define FUSER_FS_DEVICE_NAME				L"\\Device\\FuserFS"                   // FS Device :   \Device\Fuser{b9892757-6a70-4e51-9eaf-9ef1e093b0e8}   TODO: check if still used
#define FUSER_DISK_DEVICE_NAME				L"\\Device\\FuserDevice"               // DeviceName:   \Device\Volume{b9892757-6a70-4e51-9eaf-9ef1e093b0e8}  TODO: check if still used
#define FUSER_SYMBOLIC_LINK_NAME    		L"\\DosDevices\\Global\\FuserDevice"   // SymbolicName: \DosDevices\Global\Volume{b9892757-6a70-4e51-9eaf-9ef1e093b0e8}  TODO: check if still used

#define FUSER_NET_DEVICE_NAME		  	    L"\\Device\\FuserRedirector"   // TODO remove networkprovider
#define FUSER_NET_SYMBOLIC_LINK_NAME        L"\\DosDevices\\Global\\FuserRedirector"   // TODO remove networkprovider
							
#define FUSER_BASE_GUID						{0xb9892757, 0x6a70, 0x4e51, {0x9e, 0xaf, 0x9e, 0xf1, 0xe0, 0x93, 0xb0, 0xe8}} // {b9892757-6a70-4e51-9eaf-9ef1e093b0e8}

	


#define DRIVER_CONTEXT_EVENT		2
#define DRIVER_CONTEXT_IRP_ENTRY	3


#define FUSER_IRP_PENDING_TIMEOUT	(1000 * 15) // in millisecond
#define FUSER_IRP_PENDING_TIMEOUT_RESET_MAX (1000 * 60 * 5) // in millisecond
#define FUSER_CHECK_INTERVAL		(1000 * 5) // in millisecond
#define FUSER_KEEPALIVE_TIMEOUT		(1000 * 15) // in millisecond



#if _WIN32_WINNT < 0x0501
	extern PFN_FSRTLTEARDOWNPERSTREAMCONTEXTS FuserFsRtlTeardownPerStreamContexts;
#endif

#if _WIN32_WINNT > 0x501
	#define FDbgPrint(...) \
	if (g_Debug) { KdPrintEx((DPFLTR_IHVDRIVER_ID, DPFLTR_TRACE_LEVEL, "[FuserFS] " __VA_ARGS__ )); }	
#else
	#define FDbgPrint(...) \
		if (g_Debug) { DbgPrint("[FuserFS] " __VA_ARGS__); }
		
#endif


extern UNICODE_STRING	FcbFileNameNull;
#define FuserPrintFileName(FileObject) \
	FDbgPrint("  FileName: %wZ FCB.FileName: %wZ\n", \
		&FileObject->FileName, \
		FileObject->FsContext2 ? \
			(((PFuserCCB)FileObject->FsContext2)->Fcb ? \
				&((PFuserCCB)FileObject->FsContext2)->Fcb->FileName : &FcbFileNameNull) : \
			&FcbFileNameNull)






//Redefine ExAllocatePool
#define TAG (ULONG)'esuF'
#ifdef ExAllocatePool
	#undef ExAllocatePool
#endif
#define ExAllocatePool(size)	ExAllocatePoolWithTag(NonPagedPool, size, TAG) // TODO: Windows8 change type to NonPagedPoolNx


extern NPAGED_LOOKASIDE_LIST	FuserIrpEntryLookasideList;
#define FuserAllocateIrpEntry()		ExAllocateFromNPagedLookasideList(&FuserIrpEntryLookasideList)
#define FuserFreeIrpEntry(IrpEntry)	ExFreeToNPagedLookasideList(&FuserIrpEntryLookasideList, IrpEntry)



// ************************************************
// *  FSD_IDENTIFIER_TYPE:                        *
// ************************************************	
// To identify the structures:
//
typedef enum _FSD_IDENTIFIER_TYPE {
// TODO: rename DGL in FGL
	DGL = ':DGL', // Fuser Global
    DCB = ':DCB', // Disk Control Block
    VCB = ':VCB', // Volume Control Block
    FCB = ':FCB', // File Control Block
    CCB = ':CCB', // Context Control Block
} FSD_IDENTIFIER_TYPE;


// ************************************************
// *  FSD_IDENTIFIER:                             *
// ************************************************	
// Header put in the beginning of every structure
//
typedef struct _FSD_IDENTIFIER {
    FSD_IDENTIFIER_TYPE     Type;
    ULONG                   Size;
} FSD_IDENTIFIER, *PFSD_IDENTIFIER;


#define GetIdentifierType(Obj) (((PFSD_IDENTIFIER)Obj)->Type)




// Data struct:



typedef struct _IRP_LIST {
	LIST_ENTRY		ListHead;
	KEVENT			NotEmpty;
	KSPIN_LOCK		ListLock;
} IRP_LIST, *PIRP_LIST;


// IRP list which has pending status
// this structure is also used to store event notification IRP
typedef struct _IRP_ENTRY {
	LIST_ENTRY			ListEntry;
	ULONG				SerialNumber;
	PIRP				Irp;
	PIO_STACK_LOCATION	IrpSp;
	PFILE_OBJECT		FileObject;
	BOOLEAN				CancelRoutineFreeMemory;
	ULONG				Flags;
	LARGE_INTEGER		TickCount;
	PIRP_LIST			IrpList;
} IRP_ENTRY, *PIRP_ENTRY;




// Identification Struct


typedef struct _FUSER_GLOBAL {
	FSD_IDENTIFIER	Identifier;
	ERESOURCE		Resource;
	PDEVICE_OBJECT	DeviceObject;
	ULONG			MountId;
	// the list of waiting IRP for mount service
	IRP_LIST		PendingService;
	IRP_LIST		NotifyService;		
} FUSER_GLOBAL, *PFUSER_GLOBAL;







// make sure Identifier is the top of struct
typedef struct _FuserDiskControlBlock {
	FSD_IDENTIFIER			Identifier;
	ERESOURCE				Resource;

	PFUSER_GLOBAL			Global;
	PDRIVER_OBJECT			DriverObject;
	PDEVICE_OBJECT			DeviceObject;
	
	PVOID					Vcb;

	// the list of waiting Event
	IRP_LIST				PendingIrp;
	IRP_LIST				PendingEvent;
	IRP_LIST				NotifyEvent;

	PUNICODE_STRING			DiskDeviceName;
	PUNICODE_STRING			FileSystemDeviceName;
	PUNICODE_STRING			SymbolicLinkName;

	DEVICE_TYPE				DeviceType;
	ULONG					DeviceCharacteristics;
	HANDLE					MupHandle;
	UNICODE_STRING			MountedDeviceInterfaceName;
	UNICODE_STRING			DiskDeviceInterfaceName;

	// When timeout is occuerd, KillEvent is triggered.
	KEVENT					KillEvent;

	KEVENT					ReleaseEvent;

	// the thread to deal with timeout
	PKTHREAD				TimeoutThread;
	PKTHREAD				EventNotificationThread;

	// When UseAltStream is 1, use Alternate stream
	USHORT					UseAltStream;
	USHORT					UseKeepAlive;
	USHORT					Mounted;

	// to make a unique id for pending IRP
	ULONG					SerialNumber;

	ULONG					MountId;

	LARGE_INTEGER			TickCount;

	CACHE_MANAGER_CALLBACKS CacheManagerCallbacks;
    CACHE_MANAGER_CALLBACKS CacheManagerNoOpCallbacks;

} FuserDCB, *PFuserDCB;






typedef struct _FuserVolumeControlBlock {
	FSD_IDENTIFIER				Identifier;
	FSRTL_ADVANCED_FCB_HEADER	VolumeFileHeader;
	SECTION_OBJECT_POINTERS		SectionObjectPointers;
	FAST_MUTEX					AdvancedFCBHeaderMutex;

	ERESOURCE					Resource;
	PDEVICE_OBJECT				DeviceObject;
	PFuserDCB					Dcb;

	LIST_ENTRY					NextFCB;

	// NotifySync is used by notify directory change
    PNOTIFY_SYNC				NotifySync;
    LIST_ENTRY					DirNotifyList;

	ULONG						FcbAllocated;
	ULONG						FcbFreed;
	ULONG						CcbAllocated;
	ULONG						CcbFreed;

} FuserVCB, *PFuserVCB;



typedef struct _FuserFileControlBlock
{
	FSD_IDENTIFIER				Identifier;

	FSRTL_ADVANCED_FCB_HEADER	AdvancedFCBHeader;
	SECTION_OBJECT_POINTERS		SectionObjectPointers;
	
	FAST_MUTEX				AdvancedFCBHeaderMutex;

	ERESOURCE				MainResource;
	ERESOURCE				PagingIoResource;
	
	PFuserVCB				Vcb;
	LIST_ENTRY				NextFCB;
	ERESOURCE				Resource;
	LIST_ENTRY				NextCCB;

	ULONG					FileCount;

	ULONG					Flags;

	UNICODE_STRING			FileName;

	//uint32 ReferenceCount;
	//uint32 OpenHandleCount;
} FuserFCB, *PFuserFCB;



typedef struct _FuserContextControlBlock
{
	FSD_IDENTIFIER		Identifier;
	ERESOURCE			Resource;
	PFuserFCB			Fcb;
	LIST_ENTRY			NextCCB;
	ULONG64				Context;
	ULONG64				UserContext;
	
	PWCHAR				SearchPattern;
	ULONG				SearchPatternLength;

	ULONG				Flags;

	int					FileCount;
	ULONG				MountId;
} FuserCCB, *PFuserCCB;


typedef struct _DRIVER_EVENT_CONTEXT {
	LIST_ENTRY		ListEntry;
	PKEVENT			Completed;
	EVENT_CONTEXT	EventContext;
} DRIVER_EVENT_CONTEXT, *PDRIVER_EVENT_CONTEXT;




DRIVER_INITIALIZE DriverEntry;

__drv_dispatchType(IRP_MJ_CREATE)					DRIVER_DISPATCH FuserDispatchCreate;
__drv_dispatchType(IRP_MJ_CLOSE)					DRIVER_DISPATCH FuserDispatchClose;
__drv_dispatchType(IRP_MJ_READ)						DRIVER_DISPATCH FuserDispatchRead;
__drv_dispatchType(IRP_MJ_WRITE)					DRIVER_DISPATCH FuserDispatchWrite;
__drv_dispatchType(IRP_MJ_FLUSH_BUFFERS)			DRIVER_DISPATCH FuserDispatchFlush;
__drv_dispatchType(IRP_MJ_CLEANUP)					DRIVER_DISPATCH FuserDispatchCleanup;
__drv_dispatchType(IRP_MJ_DEVICE_CONTROL)			DRIVER_DISPATCH FuserDispatchDeviceControl;
__drv_dispatchType(IRP_MJ_FILE_SYSTEM_CONTROL)		DRIVER_DISPATCH FuserDispatchFileSystemControl;
__drv_dispatchType(IRP_MJ_DIRECTORY_CONTROL)		DRIVER_DISPATCH FuserDispatchDirectoryControl;
__drv_dispatchType(IRP_MJ_QUERY_INFORMATION)		DRIVER_DISPATCH FuserDispatchQueryInformation;
__drv_dispatchType(IRP_MJ_SET_INFORMATION)			DRIVER_DISPATCH FuserDispatchSetInformation;
__drv_dispatchType(IRP_MJ_QUERY_VOLUME_INFORMATION)	DRIVER_DISPATCH FuserDispatchQueryVolumeInformation;
__drv_dispatchType(IRP_MJ_SET_VOLUME_INFORMATION)	DRIVER_DISPATCH FuserDispatchSetVolumeInformation;
__drv_dispatchType(IRP_MJ_SHUTDOWN)					DRIVER_DISPATCH FuserDispatchShutdown;
__drv_dispatchType(IRP_MJ_PNP)						DRIVER_DISPATCH FuserDispatchPnp;
__drv_dispatchType(IRP_MJ_LOCK_CONTROL)				DRIVER_DISPATCH FuserDispatchLock;

__drv_dispatchType(IRP_MJ_QUERY_SECURITY)			DRIVER_DISPATCH FuserDispatchQuerySecurity; // TODO: remove support for filesecurity
__drv_dispatchType(IRP_MJ_SET_SECURITY)				DRIVER_DISPATCH FuserDispatchSetSecurity;   // TODO: remove support for filesecurity

//DRIVER_CANCEL FuserEventCancelRoutine; // TODO: Check where is this method used
DRIVER_UNLOAD	FuserUnload;
DRIVER_CANCEL	FuserIrpCancelRoutine;
DRIVER_DISPATCH FuserRegisterPendingIrpForEvent;
DRIVER_DISPATCH FuserRegisterPendingIrpForService;
DRIVER_DISPATCH FuserCompleteIrp;
DRIVER_DISPATCH FuserResetPendingIrpTimeout;
DRIVER_DISPATCH FuserGetAccessToken;
DRIVER_DISPATCH FuserEventStart;
DRIVER_DISPATCH FuserEventWrite;

	

NTSTATUS
FuserCreateGlobalDiskDevice(
	__in PDRIVER_OBJECT DriverObject,
	__out PFUSER_GLOBAL* FuserGlobal);

VOID
FuserInitIrpList(
	 __in PIRP_LIST		IrpList);	
	 
VOID
FuserPrintNTStatus(
	NTSTATUS	Status);

VOID
PrintIdType(
	__in VOID* Id);	
	
NTSTATUS
FuserFreeFCB(
  __in PFuserFCB Fcb);	
  
 
ULONG GetBinaryVersion(); 

PEVENT_CONTEXT
AllocateEventContext(
	__in PFuserDCB	Dcb,
	__in PIRP				Irp,
	__in ULONG				EventContextLength,
	__in PFuserCCB			Ccb);


PEVENT_CONTEXT
AllocateEventContextRaw(
	__in ULONG	EventContextLength
	);	


NTSTATUS
FuserRegisterPendingIrp(
    __in PDEVICE_OBJECT DeviceObject,
    __in PIRP			Irp,
	__in PEVENT_CONTEXT	EventContext,
	__in ULONG			Flags);



VOID
FuserEventNotification(
	__in PIRP_LIST		NotifyEvent,
	__in PEVENT_CONTEXT	EventContext);


	
VOID
FuserFreeEventContext(
	__in PEVENT_CONTEXT	EventContext);	
	
	
	

VOID
FuserUpdateTimeout(
	__out PLARGE_INTEGER KickCount,
	__in ULONG Timeout);	
	

BOOLEAN
FuserNoOpAcquire(
    __in PVOID Fcb,
    __in BOOLEAN Wait);


VOID
FuserNoOpRelease (
    __in PVOID Fcb);	
	

NTSTATUS
FuserCreateDiskDevice(
	__in PDRIVER_OBJECT DriverObject,
	__in ULONG			MountId,
	__in PWCHAR			BaseGuid,
	__in PFUSER_GLOBAL	FuserGlobal,
	__in DEVICE_TYPE	DeviceType,
	__in ULONG			DeviceCharacteristics,
	__out PFuserDCB* Dcb);

	
NTSTATUS
FuserStartEventNotificationThread(
	__in PFuserDCB	Dcb);

	
NTSTATUS
FuserStartCheckThread(
	__in PFuserDCB	Dcb);
	

NTSTATUS
FuserEventRelease(
	__in PDEVICE_OBJECT DeviceObject);


VOID
FuserStopCheckThread(
	__in PFuserDCB	Dcb);



VOID
FuserStopEventNotificationThread(
	__in PFuserDCB	Dcb);



VOID
FuserDeleteDeviceObject(
	__in PFuserDCB Dcb);
	


VOID
FuserUnmount(
	__in PFuserDCB Dcb);	
	


VOID
FuserCompleteCreate(
	__in PIRP_ENTRY			IrpEntry,
	__in PEVENT_INFORMATION	EventInfo);



VOID
FuserNotifyReportChange(
	__in PFuserFCB	Fcb,
	__in ULONG		FilterMatch,
	__in ULONG		Action);	
	



BOOLEAN
FuserCheckCCB(
	__in PFuserDCB	Dcb,
	__in PFuserCCB	Ccb);	
	


NTSTATUS
FuserFreeCCB(
  __in PFuserCCB Ccb);

  
  
VOID
FuserCompleteCleanup(
	__in PIRP_ENTRY			IrpEntry,
	__in PEVENT_INFORMATION	EventInfo);







NTSTATUS
FuserAllocateMdl(
	__in PIRP	Irp,
	__in ULONG	Length);

	

VOID
FuserCompleteDirectoryControl(
	__in PIRP_ENTRY			IrpEntry,
	__in PEVENT_INFORMATION	EventInfo);	
	
  




VOID
FuserFreeMdl(
	__in PIRP	Irp);
	
	


VOID
FuserCompleteQueryInformation(
	__in PIRP_ENTRY			IrpEntry,
	__in PEVENT_INFORMATION	EventInfo);	


VOID
FuserCompleteSetInformation(
	__in PIRP_ENTRY			IrpEntry,
	__in PEVENT_INFORMATION EventInfo);	



VOID
FuserNotifyReportChange0(
	__in PFuserFCB				Fcb,
	__in PUNICODE_STRING		FileName,
	__in ULONG					FilterMatch,
	__in ULONG					Action);
	


VOID
FuserCompleteQueryVolumeInformation(
	__in PIRP_ENTRY			IrpEntry,
	__in PEVENT_INFORMATION	EventInfo);	
	
	

VOID
FuserCompleteRead(
	__in PIRP_ENTRY			IrpEntry,
	__in PEVENT_INFORMATION	EventInfo);	
	



VOID
FuserCompleteWrite(
	__in PIRP_ENTRY			IrpEntry,
	__in PEVENT_INFORMATION	EventInfo);

	

VOID
FuserCompleteFlush(
	__in PIRP_ENTRY			IrpEntry,
	__in PEVENT_INFORMATION	EventInfo);



VOID
FuserCompleteLock(
	__in PIRP_ENTRY			IrpEntry,
	__in PEVENT_INFORMATION	EventInfo);
	



VOID
FuserCompleteQuerySecurity( // TODO: remove support for filesecurity
	__in PIRP_ENTRY		IrpEntry,
	__in PEVENT_INFORMATION EventInfo);

VOID
FuserCompleteSetSecurity( // TODO: remove support for filesecurity
	__in PIRP_ENTRY		IrpEntry,
	__in PEVENT_INFORMATION EventInfo);



/* TODO: no longer needed and can be removed
	#define VOLUME_LABEL  FUSER_DEFAULT_VOLUME_LABEL	
	

#if _WIN32_WINNT > 0x501
	#define DDbgPrint(...) \
	if (g_Debug) { KdPrintEx((DPFLTR_IHVDRIVER_ID, DPFLTR_TRACE_LEVEL, "[FuserFS] " __VA_ARGS__ )); }	
#else
	#define DDbgPrint(...) \
		if (g_Debug) { DbgPrint("[FuserFS] " __VA_ARGS__); }
		
#endif
	
	
	
unused:
NTSTATUS
FuserUnmountNotification(
	__in PFuserDCB	Dcb,
	__in PEVENT_CONTEXT		EventContext);

PFuserFCB
FuserAllocateFCB(
	__in PFuserVCB Vcb);

PFuserCCB
FuserAllocateCCB(
	__in PFuserDCB Dcb,
	__in PFuserFCB	Fcb);

*/

#endif
