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

// TODO: FuserUnmount = is this method really still used?

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
	#define FUSER_OPTION_DEBUG		1   // ouput debug message
	#define FUSER_OPTION_STDERR		2   // ouput debug message to stderr
	#define FUSER_OPTION_ALT_STREAM	4   // use alternate stream
	#define FUSER_OPTION_KEEP_ALIVE	8   // use auto unmount
	#define FUSER_OPTION_NETWORK	16  // use network drive, you need to install Fuser network provider.
	#define FUSER_OPTION_REMOVABLE	32  // use removable drive
	#define FUSER_OPTION_HEARTBEAT	256 // use heartbeat control
// <- C#.Net Library ->



// The current Fuser version (ver 0.6.0). Please set this constant on FuserOptions->Version.
#define FUSER_VERSION		600 // TODO: Adjust values       <- C#.Net Library ->
// TODO: Revise version system, which data type should be used, how are versions ordered?



// TODO: Adjust name
// <- C#.Net Library ->
	#define FUSER_SUCCESS				 0
	#define FUSER_ERROR					-1 /* General Error */
	#define FUSER_DRIVE_LETTER_ERROR	-2 /* Bad Drive letter */
	#define FUSER_DRIVER_INSTALL_ERROR	-3 /* Can't install driver */
	#define FUSER_START_ERROR			-4 /* Driver something wrong */
	#define FUSER_MOUNT_ERROR			-5 /* Can't assign a drive letter or mount point */
	#define FUSER_MOUNT_POINT_ERROR		-6 /* Mount point is invalid */
// <- C#.Net Library ->



// TODO: Adapt name also in fuser.dll and FuserNet
typedef struct _FUSER_OPTIONS {  // <- C#.Net Library ->
	USHORT	Version; // Supported Fuser Version, ex. "530" (Fuser ver 0.5.3)
	USHORT	ThreadCount; // number of threads to be used
	ULONG	Options;	 // combination of FUSER_OPTIONS_*
	ULONG64	GlobalContext; // FileSystem can use this variable
	LPCWSTR	MountPoint; //  mount point "M:\" (drive letter) or "C:\mount\fuser" (path in NTFS)
} FUSER_OPTIONS, *PFUSER_OPTIONS;



// TODO: Adapt name also in fuser.dll and FuserNet
// TODO: Also revise function, what is really used
typedef struct _FUSER_FILE_INFO {   // <- C#.Net Library ->
	ULONG64	Context;      // FileSystem can use this variable
	ULONG64	FuserContext; // Don't touch this
	PFUSER_OPTIONS FuserOptions; // A pointer to FUSER_OPTIONS which was  passed to FuserMain.
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



// TODO: Adapt name also in fuser.dll and FuserNet
typedef struct _FUSER_OPERATIONS { // <- C#.Net Library ->
	// When an error occurs, return negative value.
	// Usually you should return GetLastError() * -1.


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
		PFUSER_FILE_INFO);

	int (FUSER_CALLBACK *OpenDirectory) (
		LPCWSTR,				// FileName
		PFUSER_FILE_INFO);

	int (FUSER_CALLBACK *CreateDirectory) (
		LPCWSTR,				// FileName
		PFUSER_FILE_INFO);

	// When FileInfo->DeleteOnClose is true, you must delete the file in Cleanup.
	int (FUSER_CALLBACK *Cleanup) (
		LPCWSTR,      // FileName
		PFUSER_FILE_INFO);

	int (FUSER_CALLBACK *CloseFile) (
		LPCWSTR,      // FileName
		PFUSER_FILE_INFO);

	int (FUSER_CALLBACK *ReadFile) (
		LPCWSTR,  // FileName
		LPVOID,   // Buffer
		DWORD,    // NumberOfBytesToRead
		LPDWORD,  // NumberOfBytesRead
		LONGLONG, // Offset
		PFUSER_FILE_INFO);
	

	int (FUSER_CALLBACK *WriteFile) (
		LPCWSTR,  // FileName
		LPCVOID,  // Buffer
		DWORD,    // NumberOfBytesToWrite
		LPDWORD,  // NumberOfBytesWritten
		LONGLONG, // Offset
		PFUSER_FILE_INFO);

	int (FUSER_CALLBACK *FlushFileBuffers) (
		LPCWSTR, // FileName
		PFUSER_FILE_INFO);


	int (FUSER_CALLBACK *GetFileInformation) (
		LPCWSTR,          // FileName
		LPBY_HANDLE_FILE_INFORMATION, // Buffer
		PFUSER_FILE_INFO);
	

	int (FUSER_CALLBACK *FindFiles) (
		LPCWSTR,			// PathName
		PFillFindData,		// call this function with PWIN32_FIND_DATAW
		PFUSER_FILE_INFO);  //  (see PFillFindData definition)


		// TODO: FindFilesWithPattern remove, as not used
	// You should implement either FindFiles or FindFilesWithPattern
	int (FUSER_CALLBACK *FindFilesWithPattern) (
		LPCWSTR,			// PathName
		LPCWSTR,			// SearchPattern
		PFillFindData,		// call this function with PWIN32_FIND_DATAW
		PFUSER_FILE_INFO);


	int (FUSER_CALLBACK *SetFileAttributes) (
		LPCWSTR, // FileName
		DWORD,   // FileAttributes
		PFUSER_FILE_INFO);


	int (FUSER_CALLBACK *SetFileTime) (
		LPCWSTR,		// FileName
		CONST FILETIME*, // CreationTime
		CONST FILETIME*, // LastAccessTime
		CONST FILETIME*, // LastWriteTime
		PFUSER_FILE_INFO);


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
		PFUSER_FILE_INFO);

	int (FUSER_CALLBACK *DeleteDirectory) ( 
		LPCWSTR, // FileName
		PFUSER_FILE_INFO);


	int (FUSER_CALLBACK *MoveFile) (
		LPCWSTR, // ExistingFileName
		LPCWSTR, // NewFileName
		BOOL,	// ReplaceExisiting
		PFUSER_FILE_INFO);


	int (FUSER_CALLBACK *SetEndOfFile) (
		LPCWSTR,  // FileName
		LONGLONG, // Length
		PFUSER_FILE_INFO);


	int (FUSER_CALLBACK *SetAllocationSize) (
		LPCWSTR,  // FileName
		LONGLONG, // Length
		PFUSER_FILE_INFO);


	int (FUSER_CALLBACK *LockFile) (
		LPCWSTR, // FileName
		LONGLONG, // ByteOffset
		LONGLONG, // Length
		PFUSER_FILE_INFO);


	int (FUSER_CALLBACK *UnlockFile) (
		LPCWSTR, // FileName
		LONGLONG,// ByteOffset
		LONGLONG,// Length
		PFUSER_FILE_INFO);


	// Neither GetDiskFreeSpace nor GetVolumeInformation
	// save the FuserFileContext->Context.
	// Before these methods are called, CreateFile may not be called.
	// (ditto CloseFile and Cleanup)

	// see Win32 API GetDiskFreeSpaceEx
	int (FUSER_CALLBACK *GetDiskFreeSpace) (
		PULONGLONG, // FreeBytesAvailable
		PULONGLONG, // TotalNumberOfBytes
		PULONGLONG, // TotalNumberOfFreeBytes
		PFUSER_FILE_INFO);


	// see Win32 API GetVolumeInformation
	int (FUSER_CALLBACK *GetVolumeInformation) (
		LPWSTR, // VolumeNameBuffer
		DWORD,	// VolumeNameSize in num of chars
		LPDWORD,// VolumeSerialNumber
		LPDWORD,// MaximumComponentLength in num of chars
		LPDWORD,// FileSystemFlags
		LPWSTR,	// FileSystemNameBuffer
		DWORD,	// FileSystemNameSize in num of chars
		PFUSER_FILE_INFO);

	int (FUSER_CALLBACK *Mount) (
		LPCWSTR, // MountPoint
		LPCWSTR  // DeviceName
		);		

	int (FUSER_CALLBACK *Unmount) (
		PFUSER_FILE_INFO);
		
	


	// Suported since 0.6.0. You must specify the version at FUSER_OPTIONS.Version.
	int (FUSER_CALLBACK *GetFileSecurity) (
		LPCWSTR, // FileName
		PSECURITY_INFORMATION, // A pointer to SECURITY_INFORMATION value being requested
		PSECURITY_DESCRIPTOR, // A pointer to SECURITY_DESCRIPTOR buffer to be filled
		ULONG, // length of Security descriptor buffer
		PULONG, // LengthNeeded
		PFUSER_FILE_INFO);

	int (FUSER_CALLBACK *SetFileSecurity) (
		LPCWSTR, // FileName
		PSECURITY_INFORMATION,
		PSECURITY_DESCRIPTOR, // SecurityDescriptor
		ULONG, // SecurityDescriptor length
		PFUSER_FILE_INFO);

} FUSER_OPERATIONS, *PFUSER_OPERATIONS;




int FUSERAPI
FuserMain(
	PFUSER_OPTIONS	FuserOptions,
	PFUSER_OPERATIONS FuserOperations);



BOOL FUSERAPI
FuserUnmount(
	WCHAR	DriveLetter);	
	
ULONG FUSERAPI
FuserVersion();


ULONG FUSERAPI
FuserDriverVersion();


BOOL FUSERAPI
FuserRemoveMountPoint(
	LPCWSTR MountPoint);
	

	


// FuserIsNameInExpression
//   check whether Name can match Expression
//   Expression can contain wildcard characters (? and *)
BOOL FUSERAPI
FuserIsNameInExpression(  // TODO: check if method can be removed, but then also remove export in fuser.def
	LPCWSTR		Expression,		// matching pattern
	LPCWSTR		Name,			// file name
	BOOL		IgnoreCase);	




// FuserResetTimeout
//   extends the time out of the current IO operation in driver.
// TODO: Check if method can be removed or implemented differently -> FuserKeepAlive , remove export in fuser.def too.
BOOL FUSERAPI
FuserResetTimeout(
	ULONG				Timeout,	// timeout in millisecond
	PFUSER_FILE_INFO	FuserFileInfo);

	
//Sends the heartbeat signal
BOOL FUSERAPI
FuserSendHeartbeat(LPCWSTR MountPoint, LPCWSTR DeviceName);
	

// Get the handle to Access Token
// This method needs be called in CreateFile, OpenDirectory or CreateDirectly callback.
// The caller must call CloseHandle for the returned handle.
HANDLE FUSERAPI
FuserOpenRequestorToken(
	PFUSER_FILE_INFO	FuserFileInfo);	
	


	
	
VOID DebugLogWrite(LPCWSTR z); // TODO: remove this




/*
// Obsolete and removed:
//#define FUSER_DRIVER_NAME	L"fuser.sys" No longer needed
*/


#ifdef __cplusplus
}
#endif

#endif // _FUSER_H_