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
#ifndef _PUBLIC_H_
#define _PUBLIC_H_


#define FUSER_DRIVER_VERSION	  0x0000190   // TODO: change value

#define EVENT_CONTEXT_MAX_SIZE    (1024*32)

#define IOCTL_TEST                CTL_CODE(FILE_DEVICE_UNKNOWN, 0x800, METHOD_BUFFERED,   FILE_ANY_ACCESS)
#define IOCTL_SET_DEBUG_MODE      CTL_CODE(FILE_DEVICE_UNKNOWN, 0x801, METHOD_BUFFERED,   FILE_ANY_ACCESS)
#define IOCTL_EVENT_WAIT          CTL_CODE(FILE_DEVICE_UNKNOWN, 0x802, METHOD_BUFFERED,   FILE_ANY_ACCESS)
#define IOCTL_EVENT_INFO          CTL_CODE(FILE_DEVICE_UNKNOWN, 0x803, METHOD_BUFFERED,   FILE_ANY_ACCESS)
#define IOCTL_EVENT_RELEASE       CTL_CODE(FILE_DEVICE_UNKNOWN, 0x804, METHOD_BUFFERED,   FILE_ANY_ACCESS)
#define IOCTL_EVENT_START         CTL_CODE(FILE_DEVICE_UNKNOWN, 0x805, METHOD_BUFFERED,   FILE_ANY_ACCESS)
#define IOCTL_EVENT_WRITE         CTL_CODE(FILE_DEVICE_UNKNOWN, 0x806, METHOD_OUT_DIRECT, FILE_ANY_ACCESS)
#define IOCTL_KEEPALIVE           CTL_CODE(FILE_DEVICE_UNKNOWN, 0x809, METHOD_NEITHER,    FILE_ANY_ACCESS)
#define IOCTL_SERVICE_WAIT        CTL_CODE(FILE_DEVICE_UNKNOWN, 0x80A, METHOD_BUFFERED,   FILE_ANY_ACCESS)
#define IOCTL_RESET_TIMEOUT       CTL_CODE(FILE_DEVICE_UNKNOWN, 0x80B, METHOD_BUFFERED,   FILE_ANY_ACCESS)
#define IOCTL_GET_ACCESS_TOKEN    CTL_CODE(FILE_DEVICE_UNKNOWN, 0x80C, METHOD_BUFFERED,   FILE_ANY_ACCESS)


// TOCO: change names, unify
#define FUSER_MOUNTED		1
#define FUSER_START_FAILED	3

// Drive Geometrie Data:
#define FUSER_SECTOR_SIZE			512
#define FUSER_ALLOCATION_UNIT_SIZE	512 // used with fuser.dll, volume.c to calculate size


// used in FUSER_START->DeviceType
#define FUSER_DISK_FILE_SYSTEM		0
#define FUSER_NETWORK_FILE_SYSTEM	1 // TODO: remove support

#define FUSER_EVENT_ALTERNATIVE_STREAM_ON	1
#define FUSER_EVENT_KEEP_ALIVE_ON			2
#define FUSER_EVENT_REMOVABLE				4



// used in CCB->Flags and FCB->Flags   // TODO: revise names, all occur in createmethods and others
#define FUSER_FILE_DIRECTORY			1
#define FUSER_FILE_OPENED				4
#define FUSER_DIR_MATCH_ALL				8
#define FUSER_DELETE_ON_CLOSE			16
#define FUSER_PAGING_IO					32
#define FUSER_SYNCHRONOUS_IO			64
#define FUSER_WRITE_TO_END_OF_FILE 		128
#define FUSER_NOCACHE					256

// Context-Struct

typedef struct _CREATE_CONTEXT {
	ULONG	FileAttributes;
	ULONG	CreateOptions;
	ULONG	DesiredAccess;
	ULONG	ShareAccess;
	
	ULONG	FileNameLength;
	WCHAR	FileName[1];

} CREATE_CONTEXT, *PCREATE_CONTEXT;


typedef struct _CLEANUP_CONTEXT {
	ULONG	FileNameLength;
	WCHAR	FileName[1];

} CLEANUP_CONTEXT, *PCLEANUP_CONTEXT;


typedef struct _CLOSE_CONTEXT {
	ULONG	FileNameLength;
	WCHAR	FileName[1];

} CLOSE_CONTEXT, *PCLOSE_CONTEXT;

typedef struct _DIRECTORY_CONTEXT {
	ULONG	FileInformationClass;
	ULONG	FileIndex;
	ULONG	BufferLength;
	ULONG	DirectoryNameLength;
	ULONG	SearchPatternLength;
	ULONG	SearchPatternOffset;
	WCHAR	DirectoryName[1];
	WCHAR	SearchPatternBase[1];

} DIRECTORY_CONTEXT, *PDIRECTORY_CONTEXT;

typedef struct _READ_CONTEXT {
	LARGE_INTEGER	ByteOffset;
	ULONG			BufferLength;
	ULONG			FileNameLength;
	WCHAR			FileName[1];
} READ_CONTEXT, *PREAD_CONTEXT;


typedef struct _WRITE_CONTEXT {
	LARGE_INTEGER	ByteOffset;
	ULONG			BufferLength;
	ULONG			BufferOffset;
	ULONG			RequestLength;
	ULONG			FileNameLength;
	WCHAR			FileName[2];
	// "2" means to keep last null of contents to write
} WRITE_CONTEXT, *PWRITE_CONTEXT;


typedef struct _FILEINFO_CONTEXT {
	ULONG	FileInformationClass;
	ULONG	BufferLength;
	ULONG	FileNameLength;
	WCHAR	FileName[1];
} FILEINFO_CONTEXT, *PFILEINFO_CONTEXT;


typedef struct _SETFILE_CONTEXT {
	ULONG	FileInformationClass;
	ULONG	BufferLength;
	ULONG	BufferOffset;
	ULONG	FileNameLength;
	WCHAR	FileName[1];
} SETFILE_CONTEXT, *PSETFILE_CONTEXT;

typedef struct _VOLUME_CONTEXT {
	ULONG	FsInformationClass;
	ULONG	BufferLength;
} VOLUME_CONTEXT, *PVOLUME_CONTEXT;

typedef struct _LOCK_CONTEXT {
	LARGE_INTEGER	ByteOffset;
	LARGE_INTEGER	Length;
	ULONG			Key;
	ULONG			FileNameLength;
	WCHAR			FileName[1];
} LOCK_CONTEXT, *PLOCK_CONTEXT;


typedef struct _FLUSH_CONTEXT {
	ULONG	FileNameLength;
	WCHAR	FileName[1];
} FLUSH_CONTEXT, *PFLUSH_CONTEXT;


typedef struct _UNMOUNT_CONTEXT {
	WCHAR	DeviceName[64];
} UNMOUNT_CONTEXT, *PUNMOUNT_CONTEXT;


typedef struct _SECURITY_CONTEXT {
	SECURITY_INFORMATION	SecurityInformation;
	ULONG	BufferLength;
	ULONG	FileNameLength;
	WCHAR	FileName[1];
} SECURITY_CONTEXT, *PSECURITY_CONTEXT;


typedef struct _SET_SECURITY_CONTEXT {
	SECURITY_INFORMATION	SecurityInformation;
	ULONG	BufferLength;
	ULONG	BufferOffset;
	ULONG	FileNameLength;
	WCHAR	FileName[1];
} SET_SECURITY_CONTEXT, *PSET_SECURITY_CONTEXT;


typedef struct _EVENT_CONTEXT {
	ULONG	Length;
	ULONG	MountId;
	ULONG	SerialNumber;
	ULONG	ProcessId;
	UCHAR	MajorFunction;
	UCHAR	MinorFunction;
	ULONG	Flags;
	ULONG	FileFlags;
	ULONG64	Context;
	union {
		DIRECTORY_CONTEXT	Directory;
		READ_CONTEXT		Read;
		WRITE_CONTEXT		Write;
		FILEINFO_CONTEXT	File;
		CREATE_CONTEXT		Create;
		CLOSE_CONTEXT		Close;
		SETFILE_CONTEXT		SetFile;
		CLEANUP_CONTEXT		Cleanup;					
		LOCK_CONTEXT		Lock;
		VOLUME_CONTEXT		Volume;
		FLUSH_CONTEXT		Flush;
		UNMOUNT_CONTEXT		Unmount;
		SECURITY_CONTEXT		Security;
		SET_SECURITY_CONTEXT	SetSecurity;
	};
} EVENT_CONTEXT, *PEVENT_CONTEXT;


typedef struct _EVENT_INFORMATION {
	ULONG		SerialNumber;
	ULONG		Status;
	ULONG		Flags;
	union {
		struct {
			ULONG	Index;
		} Directory;
		struct {
			ULONG	Flags;
			ULONG	Information;
		} Create;
		struct {
			LARGE_INTEGER CurrentByteOffset;
		} Read;
		struct {
			LARGE_INTEGER CurrentByteOffset;
		} Write;
		struct {
			UCHAR	DeleteOnClose;
		} Delete;
		struct {
			ULONG	Timeout;
		} ResetTimeout;
		struct {
			HANDLE	Handle;
		} AccessToken;
	};
	ULONG64		Context;
	ULONG		BufferLength;
	UCHAR		Buffer[8];

} EVENT_INFORMATION, *PEVENT_INFORMATION;


typedef struct _EVENT_DRIVER_INFO {
	ULONG	DriverVersion;
	ULONG	Status;
	ULONG	DeviceNumber;
	ULONG	MountId;
	WCHAR	DeviceName[64];
} EVENT_DRIVER_INFO, *PEVENT_DRIVER_INFO;

typedef struct _EVENT_START {
	ULONG	UserVersion;
	ULONG	DeviceType;
	ULONG	Flags;
	WCHAR	DriveLetter;
} EVENT_START, *PEVENT_START;


typedef struct _FUSER_RENAME_INFORMATION {
	BOOLEAN ReplaceIfExists;
	ULONG FileNameLength;
	WCHAR FileName[1];
} FUSER_RENAME_INFORMATION, *PFUSER_RENAME_INFORMATION;


/* TODO: remove these codes, are no longer needed
#include "devioctl.h" // TODO: move to right project
#define DRIVER_FUNC_INSTALL     0x01
#define DRIVER_FUNC_REMOVE      0x02
#define FUSER_USED			2
#define FUSER_DEVICE_MAX	10
#define FUSER_FILE_DELETED		2
#define WRITE_MAX_SIZE				(EVENT_CONTEXT_MAX_SIZE-sizeof(EVENT_CONTEXT)-256*sizeof(WCHAR))

typedef struct _FUSER_LINK_INFORMATION {
	BOOLEAN ReplaceIfExists;
	ULONG FileNameLength;
	WCHAR FileName[1];
} FUSER_LINK_INFORMATION, *PFUSER_LINK_INFORMATION;
*/



#endif // _PUBLIC_H_
