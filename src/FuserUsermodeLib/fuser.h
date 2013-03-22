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

// TODO: Check Fuser.def exports and check which are no longer needed, remove them in FuserNet as well.



#ifndef _FUSER_H_
#define _FUSER_H_



#ifdef __cplusplus
extern "C" {
#endif

#ifndef _M_X64	
  #ifdef _EXPORTING
	#define FUSERAPI __declspec(dllimport) __stdcall
  #else
	#define FUSERAPI __declspec(dllexport) __stdcall
  #endif
#else
  #define FUSERAPI
#endif
// TODO: check the removal of all FUSERAPI parameters

#define FUSER_CALLBACK __stdcall







// <- C#.Net Library ->
	#define FUSER_MOUNT_PARAMETER_FLAG_DEBUG			1   // ouput debug message
	#define FUSER_MOUNT_PARAMETER_FLAG_STDERR			2   // ouput debug message to stderr
	#define FUSER_MOUNT_PARAMETER_FLAG_USEADS			4	// use Alternativ Data Stream (e.g. C:\TEMP\TEST.TXT:ADS.file)
	#define FUSER_MOUNT_PARAMETER_FLAG_HEARTBEAT 		8 	//  use heartbeat-check for device alive-control
	#define FUSER_MOUNT_PARAMETER_FLAG_TYPE_REMOVABLE 	16  //  DeviceType: Removable-Device	
// <- C#.Net Library ->

// <- C#.Net Library ->
	#define FUSER_DEVICEMOUNT_VERSION_ERROR			-1 // Version incompatible with this version
	#define FUSER_DEVICEMOUNT_EVENT_LOAD_ERROR		-2 // Error while loading the events.
	#define FUSER_DEVICEMOUNT_BAD_MOUNT_POINT_ERROR -3 // mountpoint invalid
	#define FUSER_DEVICEMOUNT_DRIVER_INSTALL_ERROR	-4 // driver not installed
	#define FUSER_DEVICEMOUNT_DRIVER_START_ERROR	-5 // driver not started (FuserStart-method)
	#define FUSER_DEVICEMOUNT_MOUNT_ERROR			-6 // device can't mount (FuserDeviceAgent)	
	#define FUSER_DEVICEMOUNT_SUCCESS				0 // device successfully unmount
	// TODO: add different success reasons for mount
// <- C#.Net Library ->

// <- C#.Net Library ->
	#define FUSER_EVENT_MOUNT					1
	#define FUSER_EVENT_UNMOUNT					2
	#define FUSER_EVENT_GET_VOLUME_INFORMATION	3
	#define FUSER_EVENT_GET_DISK_FREESPACE		4
	#define FUSER_EVENT_CREATE_FILE				5
	#define FUSER_EVENT_CREATE_DIRECTORY		6
	#define FUSER_EVENT_OPEN_DIRECTORY			7
	#define FUSER_EVENT_CLOSE_FILE				8
	#define FUSER_EVENT_CLEANUP					9
	#define FUSER_EVENT_READ_FILE				10
	#define FUSER_EVENT_WRITE_FILE				11
	#define FUSER_EVENT_FLUSH_FILEBUFFERS		12
	#define FUSER_EVENT_FIND_FILES				13
	#define FUSER_EVENT_FIND_FILES_WITH_PATTERN	14
	#define FUSER_EVENT_GET_FILE_INFORMATION	15
	#define FUSER_EVENT_SET_FILE_ATTRIBUTES		16
	#define FUSER_EVENT_SET_FILE_TIME			17
	#define FUSER_EVENT_SET_End_OF_FILE			18
	#define FUSER_EVENT_SET_ALLOCATIONSIZE		19
	#define FUSER_EVENT_LOCK_FILE				20
	#define FUSER_EVENT_UNLOCK_FILE				21
	#define FUSER_EVENT_DELETE_FILE				22
	#define FUSER_EVENT_DELETE_DIRECTORY		23
	#define FUSER_EVENT_MOVE_FILE				24
	#define FUSER_EVENT_GET_FILESECURITY		25
	#define FUSER_EVENT_SET_FILESECURITY		26
// <- C#.Net Library ->






// TODO: Adapt name also in fuser.dll and FuserNet
// TODO: Also revise function, what is really used
typedef struct _FUSER_FILE_INFO {   // <- C#.Net Library ->
	ULONG64	Context;      // FileSystem can use this variable
	ULONG64	FuserContext; // Don't touch this	
	ULONG	ProcessId;    // process id for the thread that originally requested a given I/O operation
	UCHAR	IsDirectory;  // requesting a directory file
	UCHAR	DeleteOnClose; // Delete on when "cleanup" is called
	UCHAR	PagingIo;	// Read or write is paging IO.
	UCHAR	SynchronousIo;  // Read or write is synchronous IO.
	UCHAR	Nocache;
	UCHAR	WriteToEndOfFile; //  If true, write to the current end of file instead of Offset parameter.

} FUSER_FILE_INFO, *PFUSER_FILE_INFO;


// FillFileData
//   add an entry in FindFiles
//   return 1 if buffer is full, otherwise 0
//   (currently never return 1)
typedef int (WINAPI *PFillFindData) (PWIN32_FIND_DATAW, PFUSER_FILE_INFO);



// <- C#.Net Library ->
	typedef struct _FUSER_EVENT { 		
		union {
			ULONG CallPointer;
			int (FUSER_CALLBACK *Mount) 				(LPCWSTR, LPCWSTR );
			int (FUSER_CALLBACK *Unmount) 				(PFUSER_FILE_INFO);
			int (FUSER_CALLBACK *GetVolumeInformation)	(LPWSTR, DWORD,	LPDWORD, LPDWORD, LPDWORD, LPWSTR, DWORD, PFUSER_FILE_INFO);
			int (FUSER_CALLBACK *GetDiskFreeSpace) 		(PULONGLONG, PULONGLONG, PULONGLONG, PFUSER_FILE_INFO);	
			int (FUSER_CALLBACK *CreateFile) 			(LPCWSTR, DWORD, DWORD, DWORD, DWORD, PFUSER_FILE_INFO);
			int (FUSER_CALLBACK *CreateDirectory) 		(LPCWSTR, PFUSER_FILE_INFO);			
			int (FUSER_CALLBACK *OpenDirectory) 		(LPCWSTR, PFUSER_FILE_INFO);
			int (FUSER_CALLBACK *CloseFile) 			(LPCWSTR, PFUSER_FILE_INFO);			
			int (FUSER_CALLBACK *Cleanup) 				(LPCWSTR, PFUSER_FILE_INFO);
			int (FUSER_CALLBACK *ReadFile) 				(LPCWSTR, LPVOID, DWORD, LPDWORD, LONGLONG, PFUSER_FILE_INFO);	
			int (FUSER_CALLBACK *WriteFile) 			(LPCWSTR, LPCVOID, DWORD, LPDWORD, LONGLONG, PFUSER_FILE_INFO);
			int (FUSER_CALLBACK *FlushFileBuffers) 		(LPCWSTR, PFUSER_FILE_INFO);
			int (FUSER_CALLBACK *FindFiles) 			(LPCWSTR, PFillFindData, PFUSER_FILE_INFO);
			int (FUSER_CALLBACK *FindFilesWithPattern)	(LPCWSTR, LPCWSTR, PFillFindData, PFUSER_FILE_INFO);			
			int (FUSER_CALLBACK *GetFileInformation) 	(LPCWSTR, LPBY_HANDLE_FILE_INFORMATION, PFUSER_FILE_INFO);			
			int (FUSER_CALLBACK *SetFileAttributes) 	(LPCWSTR, DWORD, PFUSER_FILE_INFO);
			int (FUSER_CALLBACK *SetFileTime) 			(LPCWSTR, CONST FILETIME*, CONST FILETIME*, CONST FILETIME*, PFUSER_FILE_INFO);
			int (FUSER_CALLBACK *SetEndOfFile) 			(LPCWSTR, LONGLONG, PFUSER_FILE_INFO);
			int (FUSER_CALLBACK *SetAllocationSize) 	(LPCWSTR, LONGLONG, PFUSER_FILE_INFO);
			int (FUSER_CALLBACK *LockFile) 				(LPCWSTR, LONGLONG, LONGLONG, PFUSER_FILE_INFO);
			int (FUSER_CALLBACK *UnlockFile) 			(LPCWSTR, LONGLONG,	LONGLONG, PFUSER_FILE_INFO);		
			int (FUSER_CALLBACK *DeleteFile) 			(LPCWSTR, PFUSER_FILE_INFO);
			int (FUSER_CALLBACK *DeleteDirectory) 		(LPCWSTR, PFUSER_FILE_INFO);
			int (FUSER_CALLBACK *MoveFile) 				(LPCWSTR, LPCWSTR, BOOL, PFUSER_FILE_INFO);
			int (FUSER_CALLBACK *GetFileSecurity)		(LPCWSTR, PSECURITY_INFORMATION, PSECURITY_DESCRIPTOR, ULONG, PULONG, PFUSER_FILE_INFO);
			int (FUSER_CALLBACK *SetFileSecurity) 		(LPCWSTR, PSECURITY_INFORMATION, PSECURITY_DESCRIPTOR, ULONG, PFUSER_FILE_INFO);
		};
	} FUSER_EVENT, *PFUSER_EVENT;
// <- C#.Net Library ->	


// <- C#.Net Library ->
typedef struct _FUSER_MOUNT_PARAMETER {
	USHORT	StructVersion;	// used struct Version, currently: 1, each change of this structure must increase this value.	
	LPCWSTR	MountPoint; 	//  mount point "V:\" (drive letter) or "C:\DATA\fuser" (path in NTFS)
	ULONG	Flags;	 		// combination of FUSER_MOUNT_PARAMETER_*
	USHORT	ThreadsCount; 	// number of threads to be used		
	int (FUSER_CALLBACK *EventLoader) (ULONG, PFUSER_EVENT); //Callback methode for event loading, ULONG = counter, FUSER_EVENT = EventHandler , returnvalue = EventID
} FUSER_MOUNT_PARAMETER, *PFUSER_MOUNT_PARAMETER;
// <- C#.Net Library ->





typedef struct _FUSER_EVENT_CALLBACKS {
	// TODO: Revise parameters and return values

	int (FUSER_CALLBACK *Mount) (
		LPCWSTR, // MountPoint
		LPCWSTR  // DeviceName
		);		

	int (FUSER_CALLBACK *Unmount) (
		PFUSER_FILE_INFO
		);
		
	int (FUSER_CALLBACK *GetVolumeInformation) ( // see Win32 API GetVolumeInformation
		LPWSTR, // VolumeNameBuffer
		DWORD,	// VolumeNameSize in num of chars
		LPDWORD,// VolumeSerialNumber
		LPDWORD,// MaximumComponentLength in num of chars
		LPDWORD,// FileSystemFlags
		LPWSTR,	// FileSystemNameBuffer
		DWORD,	// FileSystemNameSize in num of chars
		PFUSER_FILE_INFO
		);
		
	// Neither GetDiskFreeSpace nor GetVolumeInformation
	// save the FuserFileContext->Context.
	// Before these methods are called, CreateFile may not be called.
	// (ditto CloseFile and Cleanup)

	// see Win32 API GetDiskFreeSpaceEx
	int (FUSER_CALLBACK *GetDiskFreeSpace) (
		PULONGLONG, // FreeBytesAvailable
		PULONGLONG, // TotalNumberOfBytes
		PULONGLONG, // TotalNumberOfFreeBytes
		PFUSER_FILE_INFO
		);
		

	// CreateFile
	//   If file is a directory, CreateFile (not OpenDirectory) may be called.
	//   In this case, CreateFile should return 0 when that directory can be opened.
	//   You should set TRUE on FuserFileInfo->IsDirectory when file is a directory.
	//   When CreationDisposition is CREATE_ALWAYS or OPEN_ALWAYS and a file already exists,
	//   you should return ERROR_ALREADY_EXISTS(183) (not negative value)
	int (FUSER_CALLBACK *CreateFile) (
		LPCWSTR,      // FileName
		DWORD,        // DesiredAccess
		DWORD,        // ShareMode
		DWORD,        // CreationDisposition
		DWORD,        // FlagsAndAttributes
		PFUSER_FILE_INFO
		);

	int (FUSER_CALLBACK *CreateDirectory) (
		LPCWSTR,				// FileName
		PFUSER_FILE_INFO
		);			
		
	int (FUSER_CALLBACK *OpenDirectory) (
		LPCWSTR,				// FileName
		PFUSER_FILE_INFO);

	int (FUSER_CALLBACK *CloseFile) (
		LPCWSTR,      // FileName
		PFUSER_FILE_INFO
		);
		
	// When FileInfo->DeleteOnClose is true, you must delete the file in Cleanup.
	int (FUSER_CALLBACK *Cleanup) (
		LPCWSTR,      // FileName
		PFUSER_FILE_INFO
		);

	int (FUSER_CALLBACK *ReadFile) (
		LPCWSTR,  // FileName
		LPVOID,   // Buffer
		DWORD,    // NumberOfBytesToRead
		LPDWORD,  // NumberOfBytesRead
		LONGLONG, // Offset
		PFUSER_FILE_INFO
		);
	
	int (FUSER_CALLBACK *WriteFile) (
		LPCWSTR,  // FileName
		LPCVOID,  // Buffer
		DWORD,    // NumberOfBytesToWrite
		LPDWORD,  // NumberOfBytesWritten
		LONGLONG, // Offset
		PFUSER_FILE_INFO
		);

	int (FUSER_CALLBACK *FlushFileBuffers) (
		LPCWSTR, // FileName
		PFUSER_FILE_INFO
		);
		
	int (FUSER_CALLBACK *FindFiles) (
		LPCWSTR,			// PathName
		PFillFindData,		// call this function with PWIN32_FIND_DATAW
		PFUSER_FILE_INFO
		);  //  (see PFillFindData definition)


	// TODO: FindFilesWithPattern remove, as not used
	// You should implement either FindFiles or FindFilesWithPattern
	int (FUSER_CALLBACK *FindFilesWithPattern) (
		LPCWSTR,			// PathName
		LPCWSTR,			// SearchPattern
		PFillFindData,		// call this function with PWIN32_FIND_DATAW
		PFUSER_FILE_INFO
		);
		
	int (FUSER_CALLBACK *GetFileInformation) (
		LPCWSTR,          // FileName
		LPBY_HANDLE_FILE_INFORMATION, // Buffer
		PFUSER_FILE_INFO
		);
	
	int (FUSER_CALLBACK *SetFileAttributes) (
		LPCWSTR, // FileName
		DWORD,   // FileAttributes
		PFUSER_FILE_INFO
		);

	int (FUSER_CALLBACK *SetFileTime) (
		LPCWSTR,		// FileName
		CONST FILETIME*, // CreationTime
		CONST FILETIME*, // LastAccessTime
		CONST FILETIME*, // LastWriteTime
		PFUSER_FILE_INFO
		);
		
	int (FUSER_CALLBACK *SetEndOfFile) (
		LPCWSTR,  // FileName
		LONGLONG, // Length
		PFUSER_FILE_INFO
		);
	
	int (FUSER_CALLBACK *SetAllocationSize) (
		LPCWSTR,  // FileName
		LONGLONG, // Length
		PFUSER_FILE_INFO
		);
	
	int (FUSER_CALLBACK *LockFile) (
		LPCWSTR, // FileName
		LONGLONG, // ByteOffset
		LONGLONG, // Length
		PFUSER_FILE_INFO
		);

	int (FUSER_CALLBACK *UnlockFile) (
		LPCWSTR, // FileName
		LONGLONG,// ByteOffset
		LONGLONG,// Length
		PFUSER_FILE_INFO
		);

	// You should not delete file on DeleteFile or DeleteDirectory.
	// When DeleteFile or DeleteDirectory, you must check whether
	// you can delete the file or not, and return 0 (when you can delete it)
	// or appropriate error codes such as -ERROR_DIR_NOT_EMPTY,
	// -ERROR_SHARING_VIOLATION.
	// When you return 0 (ERROR_SUCCESS), you get Cleanup with
	// FileInfo->DeleteOnClose set TRUE and you have to delete the
	// file in Close.
	int (FUSER_CALLBACK *DeleteFile) (
		LPCWSTR, // FileName
		PFUSER_FILE_INFO
		);

	int (FUSER_CALLBACK *DeleteDirectory) ( 
		LPCWSTR, // FileName
		PFUSER_FILE_INFO
		);

	int (FUSER_CALLBACK *MoveFile) (
		LPCWSTR, // ExistingFileName
		LPCWSTR, // NewFileName
		BOOL,	// ReplaceExisiting
		PFUSER_FILE_INFO
		);

	int (FUSER_CALLBACK *GetFileSecurity) (
		LPCWSTR, // FileName
		PSECURITY_INFORMATION, // A pointer to SECURITY_INFORMATION value being requested
		PSECURITY_DESCRIPTOR, // A pointer to SECURITY_DESCRIPTOR buffer to be filled
		ULONG, // length of Security descriptor buffer
		PULONG, // LengthNeeded
		PFUSER_FILE_INFO
		);

	int (FUSER_CALLBACK *SetFileSecurity) (
		LPCWSTR, // FileName
		PSECURITY_INFORMATION,
		PSECURITY_DESCRIPTOR, // SecurityDescriptor
		ULONG, // SecurityDescriptor length
		PFUSER_FILE_INFO
		);

} FUSER_EVENT_CALLBACKS, *PFUSER_EVENT_CALLBACKS;








int FUSERAPI FuserDeviceMount(PFUSER_MOUNT_PARAMETER MountParameter);


ULONG FUSERAPI FuserVersion();


BOOL FUSERAPI
FuserDeviceUnmount(
	LPCWSTR MountPoint);
	



// FuserResetTimeout
//   extends the time out of the current IO operation in driver.
// TODO: Check if method can be removed or implemented differently -> FuserKeepAlive , remove export in fuser.def too.
BOOL 
FuserResetTimeout(
	ULONG				Timeout,	// timeout in millisecond
	PFUSER_FILE_INFO	FuserFileInfo);

	
//Sends the heartbeat signal
BOOL FUSERAPI
FuserSendHeartbeat(LPCWSTR MountPoint, LPCWSTR DeviceName);
	
	
	
VOID DebugLogWrite(LPCWSTR z); // TODO: remove this




/*
// Obsolete and removed:
//#define FUSER_DRIVER_NAME	L"fuser.sys" No longer needed
*/


#ifdef __cplusplus
}
#endif

#endif // _FUSER_H_