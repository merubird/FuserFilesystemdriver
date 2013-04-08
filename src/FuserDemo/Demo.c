/*

Copyright (C) 2011 - 2013 Christian Auer christian.auer@gmx.ch
Copyright (c) 2007, 2008 Hiroki Asakawa info@dokan-dev.net

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
#include <process.h>
#include <stdio.h>
#include <stdlib.h>
#include "fuser.h"
//  #include "fileinfo.h"

typedef struct _HEARTBEAT_DATA {	
	//Heartbeat:
	BOOL			HeartbeatActive;
	HANDLE			HeartbeatThread;	
	BOOL			HeartbeatAbort;
	WCHAR			MountPoint[MAX_PATH];
	WCHAR			DeviceName[MAX_PATH];
	
	
} HEARTBEAT_DATA, *PHEARTBEAT_DATA;





BOOL g_UseStdErr;
BOOL g_DebugMode;

static WCHAR RootDirectory[MAX_PATH] = L"C:";
static WCHAR MountPoint[MAX_PATH] = L"M:";
static HEARTBEAT_DATA HeartbeatData;

#define MirrorCheckFlag(val, flag) if (val&flag) { DbgPrint(L"\t" L#flag L"\n"); }








static void DbgPrint(LPCWSTR format, ...)
{
	if (g_DebugMode) {
		WCHAR buffer[512];
		va_list argp;
		va_start(argp, format);
		vswprintf_s(buffer, sizeof(buffer)/sizeof(WCHAR), format, argp);
		va_end(argp);
		if (g_UseStdErr) {
			fwprintf(stderr, buffer);
		} else {
			OutputDebugStringW(buffer);
		}
	}
}



/*
static void
PrintUserName(PFUSER_FILE_INFO	FuserFileInfo)
{
	HANDLE	handle;
	UCHAR buffer[1024];
	DWORD returnLength;
	WCHAR accountName[256];
	WCHAR domainName[256];
	DWORD accountLength = sizeof(accountName) / sizeof(WCHAR);
	DWORD domainLength = sizeof(domainName) / sizeof(WCHAR);
	PTOKEN_USER tokenUser;
	SID_NAME_USE snu;

	handle = FuserOpenRequestorToken(FuserFileInfo);
	if (handle == INVALID_HANDLE_VALUE) {
		DbgPrint(L"  FuserOpenRequestorToken failed\n");
		return;
	}

	if (!GetTokenInformation(handle, TokenUser, buffer, sizeof(buffer), &returnLength)) {
		DbgPrint(L"  GetTokenInformaiton failed: %d\n", GetLastError());
		CloseHandle(handle);
		return;
	}

	CloseHandle(handle);

	tokenUser = (PTOKEN_USER)buffer;
	if (!LookupAccountSid(NULL, tokenUser->User.Sid, accountName,
			&accountLength, domainName, &domainLength, &snu)) {
		DbgPrint(L"  LookupAccountSid failed: %d\n", GetLastError());
		return;
	}

	DbgPrint(L"  AccountName: %s, DomainName: %s\n", accountName, domainName);
}
*/


static void
GetFilePath(
	PWCHAR	filePath,
	ULONG	numberOfElements,
	LPCWSTR FileName)
{
	RtlZeroMemory(filePath, numberOfElements * sizeof(WCHAR));
	wcsncpy_s(filePath, numberOfElements, RootDirectory, wcslen(RootDirectory));
	wcsncat_s(filePath, numberOfElements, FileName, wcslen(FileName));
}

DWORD WINAPI
Heartbeat (PHEARTBEAT_DATA hData)
{
	int c = 0;
	while(1){
		if (hData->HeartbeatAbort){
			break;
		}
		
		c++;
		if (c >= 5){
			FuserSendHeartbeat(hData->MountPoint, hData->DeviceName);
			c = 0;
		}		
		Sleep(100);		
	}
	
	
	HeartbeatData.HeartbeatThread = NULL;
	return 0;
}



static int
MirrorCreateFile(
	LPCWSTR					FileName,
	DWORD					AccessMode,
	DWORD					ShareMode,
	DWORD					CreationDisposition,
	DWORD					FlagsAndAttributes,
	PFUSER_FILE_INFO		FuserFileInfo)
{
	WCHAR filePath[MAX_PATH];
	HANDLE handle;
	DWORD fileAttr;

	GetFilePath(filePath, MAX_PATH, FileName);

	DbgPrint(L"CreateFile : %s\n", filePath);

	//PrintUserName(FuserFileInfo);

	if (CreationDisposition == CREATE_NEW)
		DbgPrint(L"\tCREATE_NEW\n");
	if (CreationDisposition == OPEN_ALWAYS)
		DbgPrint(L"\tOPEN_ALWAYS\n");
	if (CreationDisposition == CREATE_ALWAYS)
		DbgPrint(L"\tCREATE_ALWAYS\n");
	if (CreationDisposition == OPEN_EXISTING)
		DbgPrint(L"\tOPEN_EXISTING\n");
	if (CreationDisposition == TRUNCATE_EXISTING)
		DbgPrint(L"\tTRUNCATE_EXISTING\n");

	/*
	if (ShareMode == 0 && AccessMode & FILE_WRITE_DATA)
		ShareMode = FILE_SHARE_WRITE;
	else if (ShareMode == 0)
		ShareMode = FILE_SHARE_READ;
	*/

	DbgPrint(L"\tShareMode = 0x%x\n", ShareMode);

	MirrorCheckFlag(ShareMode, FILE_SHARE_READ);
	MirrorCheckFlag(ShareMode, FILE_SHARE_WRITE);
	MirrorCheckFlag(ShareMode, FILE_SHARE_DELETE);

	DbgPrint(L"\tAccessMode = 0x%x\n", AccessMode);

	MirrorCheckFlag(AccessMode, GENERIC_READ);
	MirrorCheckFlag(AccessMode, GENERIC_WRITE);
	MirrorCheckFlag(AccessMode, GENERIC_EXECUTE);
	
	MirrorCheckFlag(AccessMode, DELETE);
	MirrorCheckFlag(AccessMode, FILE_READ_DATA);
	MirrorCheckFlag(AccessMode, FILE_READ_ATTRIBUTES);
	MirrorCheckFlag(AccessMode, FILE_READ_EA);
	MirrorCheckFlag(AccessMode, READ_CONTROL);
	MirrorCheckFlag(AccessMode, FILE_WRITE_DATA);
	MirrorCheckFlag(AccessMode, FILE_WRITE_ATTRIBUTES);
	MirrorCheckFlag(AccessMode, FILE_WRITE_EA);
	MirrorCheckFlag(AccessMode, FILE_APPEND_DATA);
	MirrorCheckFlag(AccessMode, WRITE_DAC);
	MirrorCheckFlag(AccessMode, WRITE_OWNER);
	MirrorCheckFlag(AccessMode, SYNCHRONIZE);
	MirrorCheckFlag(AccessMode, FILE_EXECUTE);
	MirrorCheckFlag(AccessMode, STANDARD_RIGHTS_READ);
	MirrorCheckFlag(AccessMode, STANDARD_RIGHTS_WRITE);
	MirrorCheckFlag(AccessMode, STANDARD_RIGHTS_EXECUTE);

	// When filePath is a directory, needs to change the flag so that the file can be opened.
	fileAttr = GetFileAttributes(filePath);
	if (fileAttr && fileAttr & FILE_ATTRIBUTE_DIRECTORY) {
		FlagsAndAttributes |= FILE_FLAG_BACKUP_SEMANTICS;
		//AccessMode = 0;
	}
	DbgPrint(L"\tFlagsAndAttributes = 0x%x\n", FlagsAndAttributes);

	MirrorCheckFlag(FlagsAndAttributes, FILE_ATTRIBUTE_ARCHIVE);
	MirrorCheckFlag(FlagsAndAttributes, FILE_ATTRIBUTE_ENCRYPTED);
	MirrorCheckFlag(FlagsAndAttributes, FILE_ATTRIBUTE_HIDDEN);
	MirrorCheckFlag(FlagsAndAttributes, FILE_ATTRIBUTE_NORMAL);
	MirrorCheckFlag(FlagsAndAttributes, FILE_ATTRIBUTE_NOT_CONTENT_INDEXED);
	MirrorCheckFlag(FlagsAndAttributes, FILE_ATTRIBUTE_OFFLINE);
	MirrorCheckFlag(FlagsAndAttributes, FILE_ATTRIBUTE_READONLY);
	MirrorCheckFlag(FlagsAndAttributes, FILE_ATTRIBUTE_SYSTEM);
	MirrorCheckFlag(FlagsAndAttributes, FILE_ATTRIBUTE_TEMPORARY);
	MirrorCheckFlag(FlagsAndAttributes, FILE_FLAG_WRITE_THROUGH);
	MirrorCheckFlag(FlagsAndAttributes, FILE_FLAG_OVERLAPPED);
	MirrorCheckFlag(FlagsAndAttributes, FILE_FLAG_NO_BUFFERING);
	MirrorCheckFlag(FlagsAndAttributes, FILE_FLAG_RANDOM_ACCESS);
	MirrorCheckFlag(FlagsAndAttributes, FILE_FLAG_SEQUENTIAL_SCAN);
	MirrorCheckFlag(FlagsAndAttributes, FILE_FLAG_DELETE_ON_CLOSE);
	MirrorCheckFlag(FlagsAndAttributes, FILE_FLAG_BACKUP_SEMANTICS);
	MirrorCheckFlag(FlagsAndAttributes, FILE_FLAG_POSIX_SEMANTICS);
	MirrorCheckFlag(FlagsAndAttributes, FILE_FLAG_OPEN_REPARSE_POINT);
	MirrorCheckFlag(FlagsAndAttributes, FILE_FLAG_OPEN_NO_RECALL);
	MirrorCheckFlag(FlagsAndAttributes, SECURITY_ANONYMOUS);
	MirrorCheckFlag(FlagsAndAttributes, SECURITY_IDENTIFICATION);
	MirrorCheckFlag(FlagsAndAttributes, SECURITY_IMPERSONATION);
	MirrorCheckFlag(FlagsAndAttributes, SECURITY_DELEGATION);
	MirrorCheckFlag(FlagsAndAttributes, SECURITY_CONTEXT_TRACKING);
	MirrorCheckFlag(FlagsAndAttributes, SECURITY_EFFECTIVE_ONLY);
	MirrorCheckFlag(FlagsAndAttributes, SECURITY_SQOS_PRESENT);

	handle = CreateFile(
		filePath,
		AccessMode,//GENERIC_READ|GENERIC_WRITE|GENERIC_EXECUTE,
		ShareMode,
		NULL, // security attribute
		CreationDisposition,
		FlagsAndAttributes,// |FILE_FLAG_NO_BUFFERING,
		NULL); // template file handle

	if (handle == INVALID_HANDLE_VALUE) {
		DWORD error = GetLastError();
		DbgPrint(L"\terror code = %d\n\n", error);
		return error * -1; // error codes are negated value of Windows System Error codes
	}

	DbgPrint(L"\n");

	// save the file handle in Context
	FuserFileInfo->Context = (ULONG64)handle;
	return 0;
}



static int
MirrorOpenDirectory(
	LPCWSTR					FileName,
	PFUSER_FILE_INFO		FuserFileInfo)
{
	WCHAR filePath[MAX_PATH];
	HANDLE handle;
	DWORD attr;

	GetFilePath(filePath, MAX_PATH, FileName);

	DbgPrint(L"OpenDirectory : %s\n", filePath);

	attr = GetFileAttributes(filePath);
	if (attr == INVALID_FILE_ATTRIBUTES) {
		DWORD error = GetLastError();
		DbgPrint(L"\terror code = %d\n\n", error);
		return error * -1;
	}
	if (!(attr & FILE_ATTRIBUTE_DIRECTORY)) {
		return -1;
	}

	handle = CreateFile(
		filePath,
		0,
		FILE_SHARE_READ|FILE_SHARE_WRITE,
		NULL,
		OPEN_EXISTING,
		FILE_FLAG_BACKUP_SEMANTICS,
		NULL);

	if (handle == INVALID_HANDLE_VALUE) {
		DWORD error = GetLastError();
		DbgPrint(L"\terror code = %d\n\n", error);
		return error * -1;
	}

	DbgPrint(L"\n");

	FuserFileInfo->Context = (ULONG64)handle;

	return 0;
}



static int
MirrorCreateDirectory(
	LPCWSTR					FileName,
	PFUSER_FILE_INFO		FuserFileInfo)
{
	WCHAR filePath[MAX_PATH];
	GetFilePath(filePath, MAX_PATH, FileName);

	DbgPrint(L"CreateDirectory : %s\n", filePath);
	if (!CreateDirectory(filePath, NULL)) {
		DWORD error = GetLastError();
		DbgPrint(L"\terror code = %d\n\n", error);
		return error * -1; // error codes are negated value of Windows System Error codes
	}
	return 0;
}




static int
MirrorCloseFile(
	LPCWSTR					FileName,
	PFUSER_FILE_INFO		FuserFileInfo)
{
	WCHAR filePath[MAX_PATH];
	GetFilePath(filePath, MAX_PATH, FileName);

	if (FuserFileInfo->Context) {
		DbgPrint(L"CloseFile: %s\n", filePath);
		DbgPrint(L"\terror : not cleanuped file\n\n");
		CloseHandle((HANDLE)FuserFileInfo->Context);
		FuserFileInfo->Context = 0;
	} else {
		//DbgPrint(L"Close: %s\n\tinvalid handle\n\n", filePath);
		DbgPrint(L"Close: %s\n\n", filePath);
		return 0;
	}

	//DbgPrint(L"\n");
	return 0;
}


static int
MirrorCleanup(
	LPCWSTR					FileName,
	PFUSER_FILE_INFO		FuserFileInfo)
{
	WCHAR filePath[MAX_PATH];
	GetFilePath(filePath, MAX_PATH, FileName);

	if (FuserFileInfo->Context) {
		DbgPrint(L"Cleanup: %s\n\n", filePath);
		CloseHandle((HANDLE)FuserFileInfo->Context);
		FuserFileInfo->Context = 0;

		if (FuserFileInfo->DeleteOnClose) {// TODO: deleted here, convert accordingly in .net library
			DbgPrint(L"\tDeleteOnClose\n");
			if (FuserFileInfo->IsDirectory) {
				DbgPrint(L"  DeleteDirectory ");
				if (!RemoveDirectory(filePath)) {
					DbgPrint(L"error code = %d\n\n", GetLastError());
				} else {
					DbgPrint(L"success\n\n");
				}
			} else {
				DbgPrint(L"  DeleteFile ");
				if (DeleteFile(filePath) == 0) {
					DbgPrint(L" error code = %d\n\n", GetLastError());
				} else {
					DbgPrint(L"success\n\n");
				}
			}
		}

	} else {
		DbgPrint(L"Cleanup: %s\n\tinvalid handle\n\n", filePath);
		return -1;
	}

	return 0;
}



static int
MirrorReadFile(
	LPCWSTR				FileName,
	LPVOID				Buffer,
	DWORD				BufferLength,
	LPDWORD				ReadLength,
	LONGLONG			Offset,
	PFUSER_FILE_INFO	FuserFileInfo)
{
	WCHAR	filePath[MAX_PATH];
	HANDLE	handle = (HANDLE)FuserFileInfo->Context;
	ULONG	offset = (ULONG)Offset;
	BOOL	opened = FALSE;

	GetFilePath(filePath, MAX_PATH, FileName);

	DbgPrint(L"ReadFile : %s\n", filePath);

	if (!handle || handle == INVALID_HANDLE_VALUE) {
		DbgPrint(L"\tinvalid handle, cleanuped?\n");
		handle = CreateFile(
			filePath,
			GENERIC_READ,
			FILE_SHARE_READ,
			NULL,
			OPEN_EXISTING,
			0,
			NULL);
		if (handle == INVALID_HANDLE_VALUE) {
			DbgPrint(L"\tCreateFile error : %d\n\n", GetLastError());
			return -1;
		}
		opened = TRUE;
	}
	
	if (SetFilePointer(handle, offset, NULL, FILE_BEGIN) == 0xFFFFFFFF) {
		DbgPrint(L"\tseek error, offset = %d\n\n", offset);
		if (opened)
			CloseHandle(handle);
		return -1;
	}

		
	if (!ReadFile(handle, Buffer, BufferLength, ReadLength,NULL)) {
		DbgPrint(L"\tread error = %u, buffer length = %d, read length = %d\n\n",
			GetLastError(), BufferLength, *ReadLength);
		if (opened)
			CloseHandle(handle);
		return -1;

	} else {
		DbgPrint(L"\tread %d, offset %d\n\n", *ReadLength, offset);
	}

	if (opened)
		CloseHandle(handle);

	return 0;
}


static int
MirrorWriteFile(
	LPCWSTR		FileName,
	LPCVOID		Buffer,
	DWORD		NumberOfBytesToWrite,
	LPDWORD		NumberOfBytesWritten,
	LONGLONG			Offset,
	PFUSER_FILE_INFO	FuserFileInfo)
{
	WCHAR	filePath[MAX_PATH];
	HANDLE	handle = (HANDLE)FuserFileInfo->Context;
	ULONG	offset = (ULONG)Offset;
	BOOL	opened = FALSE;

	GetFilePath(filePath, MAX_PATH, FileName);

	DbgPrint(L"WriteFile : %s, offset %I64d, length %d\n", filePath, Offset, NumberOfBytesToWrite);

	// reopen the file
	if (!handle || handle == INVALID_HANDLE_VALUE) {
		DbgPrint(L"\tinvalid handle, cleanuped?\n");
		handle = CreateFile(
			filePath,
			GENERIC_WRITE,
			FILE_SHARE_WRITE,
			NULL,
			OPEN_EXISTING,
			0,
			NULL);
		if (handle == INVALID_HANDLE_VALUE) {
			DbgPrint(L"\tCreateFile error : %d\n\n", GetLastError());
			return -1;
		}
		opened = TRUE;
	}

	if (FuserFileInfo->WriteToEndOfFile) {// TODO: also implement this in .net library
		if (SetFilePointer(handle, 0, NULL, FILE_END) == INVALID_SET_FILE_POINTER) {
			DbgPrint(L"\tseek error, offset = EOF, error = %d\n", GetLastError());
			return -1;
		}
	} else if (SetFilePointer(handle, offset, NULL, FILE_BEGIN) == INVALID_SET_FILE_POINTER) {
		DbgPrint(L"\tseek error, offset = %d, error = %d\n", offset, GetLastError());
		return -1;
	}

		
	if (!WriteFile(handle, Buffer, NumberOfBytesToWrite, NumberOfBytesWritten, NULL)) {
		DbgPrint(L"\twrite error = %u, buffer length = %d, write length = %d\n",
			GetLastError(), NumberOfBytesToWrite, *NumberOfBytesWritten);
		return -1;

	} else {
		DbgPrint(L"\twrite %d, offset %d\n\n", *NumberOfBytesWritten, offset);
	}

	// close the file when it is reopened
	if (opened)
		CloseHandle(handle);

	return 0;
}


static int
MirrorFlushFileBuffers(
	LPCWSTR		FileName,
	PFUSER_FILE_INFO	FuserFileInfo)
{
	WCHAR	filePath[MAX_PATH];
	HANDLE	handle = (HANDLE)FuserFileInfo->Context;

	GetFilePath(filePath, MAX_PATH, FileName);

	DbgPrint(L"FlushFileBuffers : %s\n", filePath);

	if (!handle || handle == INVALID_HANDLE_VALUE) {
		DbgPrint(L"\tinvalid handle\n\n");
		return 0;
	}

	if (FlushFileBuffers(handle)) {
		return 0;
	} else {
		DbgPrint(L"\tflush error code = %d\n", GetLastError());
		return -1;
	}

}



static int
MirrorGetFileInformation(
	LPCWSTR							FileName,
	LPBY_HANDLE_FILE_INFORMATION	HandleFileInformation,
	PFUSER_FILE_INFO				FuserFileInfo)
{
	WCHAR	filePath[MAX_PATH];
	HANDLE	handle = (HANDLE)FuserFileInfo->Context;
	BOOL	opened = FALSE;

	GetFilePath(filePath, MAX_PATH, FileName);

	DbgPrint(L"GetFileInfo : %s\n", filePath);

	if (!handle || handle == INVALID_HANDLE_VALUE) {
		DbgPrint(L"\tinvalid handle\n\n");

		// If CreateDirectory returned FILE_ALREADY_EXISTS and 
		// it is called with FILE_OPEN_IF, that handle must be opened.
		handle = CreateFile(filePath, 0, FILE_SHARE_READ, NULL, OPEN_EXISTING,
			FILE_FLAG_BACKUP_SEMANTICS, NULL);
		if (handle == INVALID_HANDLE_VALUE)
			return -1;
		opened = TRUE;
	}

	if (!GetFileInformationByHandle(handle,HandleFileInformation)) {
		DbgPrint(L"\terror code = %d\n", GetLastError());

		// FileName is a root directory
		// in this case, FindFirstFile can't get directory information
		if (wcslen(FileName) == 1) {
			DbgPrint(L"  root dir\n");
			HandleFileInformation->dwFileAttributes = GetFileAttributes(filePath);

		} else {
			WIN32_FIND_DATAW find;
			ZeroMemory(&find, sizeof(WIN32_FIND_DATAW));
			handle = FindFirstFile(filePath, &find);
			if (handle == INVALID_HANDLE_VALUE) {
				DbgPrint(L"\tFindFirstFile error code = %d\n\n", GetLastError());
				return -1;
			}
			HandleFileInformation->dwFileAttributes = find.dwFileAttributes;
			HandleFileInformation->ftCreationTime = find.ftCreationTime;
			HandleFileInformation->ftLastAccessTime = find.ftLastAccessTime;
			HandleFileInformation->ftLastWriteTime = find.ftLastWriteTime;
			HandleFileInformation->nFileSizeHigh = find.nFileSizeHigh;
			HandleFileInformation->nFileSizeLow = find.nFileSizeLow;
			DbgPrint(L"\tFindFiles OK, file size = %d\n", find.nFileSizeLow);
			FindClose(handle);
		}
	} else {
		DbgPrint(L"\tGetFileInformationByHandle success, file size = %d\n",
			HandleFileInformation->nFileSizeLow);
	}

	DbgPrint(L"\n");

	if (opened) {
		CloseHandle(handle);
	}

	return 0;
}


static int
MirrorFindFiles(
	LPCWSTR				FileName,
	PFillFindData		FillFindData, // function pointer
	PFUSER_FILE_INFO	FuserFileInfo)
{
	WCHAR				filePath[MAX_PATH];
	HANDLE				hFind;
	WIN32_FIND_DATAW	findData;
	DWORD				error;
	PWCHAR				yenStar = L"\\*";
	int count = 0;

	GetFilePath(filePath, MAX_PATH, FileName);

	wcscat_s(filePath, MAX_PATH, yenStar);
	DbgPrint(L"FindFiles :%s\n", filePath);

	hFind = FindFirstFile(filePath, &findData);

	if (hFind == INVALID_HANDLE_VALUE) {
		DbgPrint(L"\tinvalid file handle. Error is %u\n\n", GetLastError());
		return -1;
	}

	FillFindData(&findData, FuserFileInfo);
	count++;

	while (FindNextFile(hFind, &findData) != 0) {
 		FillFindData(&findData, FuserFileInfo);
		count++;
	}
	
	error = GetLastError();
	FindClose(hFind);

	if (error != ERROR_NO_MORE_FILES) {
		DbgPrint(L"\tFindNextFile error. Error is %u\n\n", error);
		return -1;
	}

	DbgPrint(L"\tFindFiles return %d entries in %s\n\n", count, filePath);

	return 0;
}


static int
MirrorSetFileAttributes(
	LPCWSTR				FileName,
	DWORD				FileAttributes,
	PFUSER_FILE_INFO	FuserFileInfo)
{
	WCHAR	filePath[MAX_PATH];
	
	GetFilePath(filePath, MAX_PATH, FileName);

	DbgPrint(L"SetFileAttributes %s\n", filePath);

	if (!SetFileAttributes(filePath, FileAttributes)) {
		DWORD error = GetLastError();
		DbgPrint(L"\terror code = %d\n\n", error);
		return error * -1;
	}

	DbgPrint(L"\n");
	return 0;
}


static int
MirrorSetFileTime(
	LPCWSTR				FileName,
	CONST FILETIME*		CreationTime,
	CONST FILETIME*		LastAccessTime,
	CONST FILETIME*		LastWriteTime,
	PFUSER_FILE_INFO	FuserFileInfo)
{
	WCHAR	filePath[MAX_PATH];
	HANDLE	handle;

	GetFilePath(filePath, MAX_PATH, FileName);

	DbgPrint(L"SetFileTime %s\n", filePath);

	handle = (HANDLE)FuserFileInfo->Context;

	if (!handle || handle == INVALID_HANDLE_VALUE) {
		DbgPrint(L"\tinvalid handle\n\n");
		return -1;
	}

	if (!SetFileTime(handle, CreationTime, LastAccessTime, LastWriteTime)) {
		DWORD error = GetLastError();
		DbgPrint(L"\terror code = %d\n\n", error);
		return error * -1;
	}

	DbgPrint(L"\n");
	return 0;
}


static int
MirrorDeleteFile(
	LPCWSTR				FileName,
	PFUSER_FILE_INFO	FuserFileInfo)
{
	WCHAR	filePath[MAX_PATH];
	HANDLE	handle = (HANDLE)FuserFileInfo->Context;

	GetFilePath(filePath, MAX_PATH, FileName);

	DbgPrint(L"DeleteFile %s\n", filePath);

	return 0;
}


static int
MirrorDeleteDirectory(
	LPCWSTR				FileName,
	PFUSER_FILE_INFO	FuserFileInfo)
{
	WCHAR	filePath[MAX_PATH];
	HANDLE	handle = (HANDLE)FuserFileInfo->Context;
	HANDLE	hFind;
	WIN32_FIND_DATAW	findData;
	ULONG	fileLen;

	ZeroMemory(filePath, sizeof(filePath));
	GetFilePath(filePath, MAX_PATH, FileName);

	DbgPrint(L"DeleteDirectory %s\n", filePath);

	fileLen = wcslen(filePath);
	if (filePath[fileLen-1] != L'\\') {
		filePath[fileLen++] = L'\\';
	}
	filePath[fileLen] = L'*';

	hFind = FindFirstFile(filePath, &findData);
	while (hFind != INVALID_HANDLE_VALUE) {
		if (wcscmp(findData.cFileName, L"..") != 0 &&
			wcscmp(findData.cFileName, L".") != 0) {
			FindClose(hFind);
			DbgPrint(L"  Directory is not empty: %s\n", findData.cFileName);
			return -(int)ERROR_DIR_NOT_EMPTY;
		}
		if (!FindNextFile(hFind, &findData)) {
			break;
		}
	}
	FindClose(hFind);

	if (GetLastError() == ERROR_NO_MORE_FILES) {
		return 0;
	} else {
		return -1;
	}
}


static int
MirrorMoveFile(
	LPCWSTR				FileName, // existing file name
	LPCWSTR				NewFileName,
	BOOL				ReplaceIfExisting,
	PFUSER_FILE_INFO	FuserFileInfo)
{
	WCHAR			filePath[MAX_PATH];
	WCHAR			newFilePath[MAX_PATH];
	BOOL			status;

	GetFilePath(filePath, MAX_PATH, FileName);
	GetFilePath(newFilePath, MAX_PATH, NewFileName);

	DbgPrint(L"MoveFile %s -> %s\n\n", filePath, newFilePath);

	if (FuserFileInfo->Context) {
		// should close? or rename at closing?
		CloseHandle((HANDLE)FuserFileInfo->Context);
		FuserFileInfo->Context = 0;
	}

	if (ReplaceIfExisting)
		status = MoveFileEx(filePath, newFilePath, MOVEFILE_REPLACE_EXISTING);
	else
		status = MoveFile(filePath, newFilePath);

	if (status == FALSE) {
		DWORD error = GetLastError();
		DbgPrint(L"\tMoveFile failed status = %d, code = %d\n", status, error);
		return -(int)error;
	} else {
		return 0;
	}
}


static int
MirrorSetEndOfFile(
	LPCWSTR				FileName,
	LONGLONG			ByteOffset,
	PFUSER_FILE_INFO	FuserFileInfo)
{
	WCHAR			filePath[MAX_PATH];
	HANDLE			handle;
	LARGE_INTEGER	offset;

	GetFilePath(filePath, MAX_PATH, FileName);

	DbgPrint(L"SetEndOfFile %s, %I64d\n", filePath, ByteOffset);

	handle = (HANDLE)FuserFileInfo->Context;
	if (!handle || handle == INVALID_HANDLE_VALUE) {
		DbgPrint(L"\tinvalid handle\n\n");
		return -1;
	}

	offset.QuadPart = ByteOffset;
	if (!SetFilePointerEx(handle, offset, NULL, FILE_BEGIN)) {
		DbgPrint(L"\tSetFilePointer error: %d, offset = %I64d\n\n",
				GetLastError(), ByteOffset);
		return GetLastError() * -1;
	}

	if (!SetEndOfFile(handle)) {
		DWORD error = GetLastError();
		DbgPrint(L"\terror code = %d\n\n", error);
		return error * -1;
	}

	return 0;
}


static int
MirrorSetAllocationSize(
	LPCWSTR				FileName,
	LONGLONG			AllocSize,
	PFUSER_FILE_INFO	FuserFileInfo)
{
	WCHAR			filePath[MAX_PATH];
	HANDLE			handle;
	LARGE_INTEGER	fileSize;

	GetFilePath(filePath, MAX_PATH, FileName);

	DbgPrint(L"SetAllocationSize %s, %I64d\n", filePath, AllocSize);

	handle = (HANDLE)FuserFileInfo->Context;
	if (!handle || handle == INVALID_HANDLE_VALUE) {
		DbgPrint(L"\tinvalid handle\n\n");
		return -1;
	}

	if (GetFileSizeEx(handle, &fileSize)) {
		if (AllocSize < fileSize.QuadPart) {
			fileSize.QuadPart = AllocSize;
			if (!SetFilePointerEx(handle, fileSize, NULL, FILE_BEGIN)) {
				DbgPrint(L"\tSetAllocationSize: SetFilePointer eror: %d, "
					L"offset = %I64d\n\n", GetLastError(), AllocSize);
				return GetLastError() * -1;
			}
			if (!SetEndOfFile(handle)) {
				DWORD error = GetLastError();
				DbgPrint(L"\terror code = %d\n\n", error);
				return error * -1;
			}
		}
	} else {// TODO: Check this implementation in .net again
		DWORD error = GetLastError();
		DbgPrint(L"\terror code = %d\n\n", error);
		return error * -1;
	}
	return 0;
}


static int
MirrorLockFile(
	LPCWSTR				FileName,
	LONGLONG			ByteOffset,
	LONGLONG			Length,
	PFUSER_FILE_INFO	FuserFileInfo)
{
	WCHAR	filePath[MAX_PATH];
	HANDLE	handle;
	LARGE_INTEGER offset;
	LARGE_INTEGER length;

	GetFilePath(filePath, MAX_PATH, FileName);

	DbgPrint(L"LockFile %s\n", filePath);

	handle = (HANDLE)FuserFileInfo->Context;
	if (!handle || handle == INVALID_HANDLE_VALUE) {
		DbgPrint(L"\tinvalid handle\n\n");
		return -1;
	}

	length.QuadPart = Length;
	offset.QuadPart = ByteOffset;

	if (LockFile(handle, offset.HighPart, offset.LowPart, length.HighPart, length.LowPart)) {
		DbgPrint(L"\tsuccess\n\n");
		return 0;
	} else {
		DbgPrint(L"\tfail\n\n");
		return -1;
	}
}


static int
MirrorUnlockFile(
	LPCWSTR				FileName,
	LONGLONG			ByteOffset,
	LONGLONG			Length,
	PFUSER_FILE_INFO	FuserFileInfo)
{
	WCHAR	filePath[MAX_PATH];
	HANDLE	handle;
	LARGE_INTEGER	length;
	LARGE_INTEGER	offset;

	GetFilePath(filePath, MAX_PATH, FileName);

	DbgPrint(L"UnlockFile %s\n", filePath);

	handle = (HANDLE)FuserFileInfo->Context;
	if (!handle || handle == INVALID_HANDLE_VALUE) {
		DbgPrint(L"\tinvalid handle\n\n");
		return -1;
	}

	length.QuadPart = Length;
	offset.QuadPart = ByteOffset;

	if (UnlockFile(handle, offset.HighPart, offset.LowPart, length.HighPart, length.LowPart)) {
		DbgPrint(L"\tsuccess\n\n");
		return 0;
	} else {
		DbgPrint(L"\tfail\n\n");
		return -1;
	}
}


static int
MirrorGetFileSecurity(
	LPCWSTR					FileName,
	PSECURITY_INFORMATION	SecurityInformation,
	PSECURITY_DESCRIPTOR	SecurityDescriptor,
	ULONG				BufferLength,
	PULONG				LengthNeeded,
	PFUSER_FILE_INFO	FuserFileInfo)
{
	HANDLE	handle;
	WCHAR	filePath[MAX_PATH];

	GetFilePath(filePath, MAX_PATH, FileName);

	DbgPrint(L"GetFileSecurity %s\n", filePath);

	handle = (HANDLE)FuserFileInfo->Context;
	if (!handle || handle == INVALID_HANDLE_VALUE) {
		DbgPrint(L"\tinvalid handle\n\n");
		return -1;
	}

	if (!GetUserObjectSecurity(handle, SecurityInformation, SecurityDescriptor,
			BufferLength, LengthNeeded)) {
		int error = GetLastError();
		if (error == ERROR_INSUFFICIENT_BUFFER) {
			DbgPrint(L"  GetUserObjectSecurity failed: ERROR_INSUFFICIENT_BUFFER\n");
			return error * -1;
		} else {
			DbgPrint(L"  GetUserObjectSecurity failed: %d\n", error);
			return -1;
		}
	}
	return 0;
}


static int
MirrorSetFileSecurity(
	LPCWSTR					FileName,
	PSECURITY_INFORMATION	SecurityInformation,
	PSECURITY_DESCRIPTOR	SecurityDescriptor,
	ULONG				SecurityDescriptorLength,
	PFUSER_FILE_INFO	FuserFileInfo)
{
	HANDLE	handle;
	WCHAR	filePath[MAX_PATH];

	GetFilePath(filePath, MAX_PATH, FileName);

	DbgPrint(L"SetFileSecurity %s\n", filePath);

	handle = (HANDLE)FuserFileInfo->Context;
	if (!handle || handle == INVALID_HANDLE_VALUE) {
		DbgPrint(L"\tinvalid handle\n\n");
		return -1;
	}

	if (!SetUserObjectSecurity(handle, SecurityInformation, SecurityDescriptor)) {
		int error = GetLastError();
		DbgPrint(L"  SetUserObjectSecurity failed: %d\n", error);
		return -1;
	}
	return 0;
}


static int
MirrorGetVolumeInformation(
	LPWSTR		VolumeNameBuffer,
	DWORD		VolumeNameSize,
	LPDWORD		VolumeSerialNumber,
	LPDWORD		MaximumComponentLength,
	LPDWORD		FileSystemFlags,
	LPWSTR		FileSystemNameBuffer,
	DWORD		FileSystemNameSize,
	PFUSER_FILE_INFO	FuserFileInfo)
{
	wcscpy_s(VolumeNameBuffer, VolumeNameSize / sizeof(WCHAR), L"FUSER");
	*VolumeSerialNumber = 0x19831116;
	*MaximumComponentLength = 256;
	*FileSystemFlags = FILE_CASE_SENSITIVE_SEARCH | 
						FILE_CASE_PRESERVED_NAMES | 
						FILE_SUPPORTS_REMOTE_STORAGE |
						FILE_UNICODE_ON_DISK |
						FILE_PERSISTENT_ACLS;

	wcscpy_s(FileSystemNameBuffer, FileSystemNameSize / sizeof(WCHAR), L"Fuser");

	return 0;
}


static int
MirrorUnmount(
	PFUSER_FILE_INFO	FuserFileInfo)
{
	DbgPrint(L"Unmount\n");
	return 0;
}


static int
MirrorMount(
	LPCWSTR	MountPoint, 
	LPCWSTR	DeviceName, 
	LPCWSTR	RawDevice)
{
	HANDLE	threadId = 0;
	
	
	wcscpy_s(HeartbeatData.MountPoint,  MAX_PATH / sizeof(WCHAR), MountPoint);
	wcscpy_s(HeartbeatData.DeviceName,  MAX_PATH / sizeof(WCHAR), DeviceName);
	
	threadId = (HANDLE)_beginthreadex(
			NULL, // Security Atributes
			0, //stack size
			Heartbeat,
			&HeartbeatData, // param
			0, // create flag
			NULL);
	
	HeartbeatData.HeartbeatThread = threadId;
	return 0;
}



	

static int EventLoader (ULONG c, PFUSER_EVENT ev){
	switch (c) {
		case 0:		ev->CreateFile = MirrorCreateFile; 						return	FUSER_EVENT_CREATE_FILE;			break;
		case 1:		ev->OpenDirectory = MirrorOpenDirectory;				return	FUSER_EVENT_OPEN_DIRECTORY;			break;
		case 2:		ev->CreateDirectory = MirrorCreateDirectory;			return	FUSER_EVENT_CREATE_DIRECTORY;		break;
		case 3:		ev->Cleanup = MirrorCleanup;							return	FUSER_EVENT_CLEANUP;				break;
		case 4:		ev->CloseFile = MirrorCloseFile;						return	FUSER_EVENT_CLOSE_FILE;				break;
		case 5:		ev->ReadFile = MirrorReadFile;							return	FUSER_EVENT_READ_FILE;				break;
		case 6:		ev->WriteFile = MirrorWriteFile;						return	FUSER_EVENT_WRITE_FILE;				break;
		case 7:		ev->FlushFileBuffers = MirrorFlushFileBuffers;			return	FUSER_EVENT_FLUSH_FILEBUFFERS;		break;
		case 8:		ev->GetFileInformation = MirrorGetFileInformation;		return	FUSER_EVENT_GET_FILE_INFORMATION;	break;
		case 9:		ev->FindFiles = MirrorFindFiles;						return	FUSER_EVENT_FIND_FILES;				break;
		case 10:	ev->SetFileAttributes = MirrorSetFileAttributes;		return	FUSER_EVENT_SET_FILE_ATTRIBUTES;	break;
		case 11:	ev->SetFileTime = MirrorSetFileTime;					return	FUSER_EVENT_SET_FILE_TIME;			break;
		case 12:	ev->DeleteFile = MirrorDeleteFile;						return	FUSER_EVENT_DELETE_FILE;			break;
		case 13:	ev->DeleteDirectory = MirrorDeleteDirectory;			return	FUSER_EVENT_DELETE_DIRECTORY;		break;
		case 14:	ev->MoveFile = MirrorMoveFile;							return	FUSER_EVENT_MOVE_FILE;				break;
		case 15:	ev->SetEndOfFile = MirrorSetEndOfFile;					return	FUSER_EVENT_SET_End_OF_FILE;		break;
		case 16:	ev->SetAllocationSize = MirrorSetAllocationSize;		return	FUSER_EVENT_SET_ALLOCATIONSIZE;		break;
		case 17:	ev->LockFile = MirrorLockFile;							return	FUSER_EVENT_LOCK_FILE;				break;
		case 18:	ev->UnlockFile = MirrorUnlockFile;						return	FUSER_EVENT_UNLOCK_FILE;			break;
		case 19:	ev->GetFileSecurity = MirrorGetFileSecurity;			return	FUSER_EVENT_GET_FILESECURITY;		break;
		case 20:	ev->SetFileSecurity = MirrorSetFileSecurity;			return	FUSER_EVENT_SET_FILESECURITY;		break;
		case 21:	ev->GetVolumeInformation = MirrorGetVolumeInformation;	return	FUSER_EVENT_GET_VOLUME_INFORMATION;	break;
		case 22:	ev->Unmount = MirrorUnmount;							return	FUSER_EVENT_UNMOUNT;				break;
		case 23:	ev->Mount = MirrorMount;								return	FUSER_EVENT_MOUNT;					break;
		default:
			return 0;
	}
}



int __cdecl
wmain(ULONG argc, PWCHAR argv[])
{

	int status;
	ULONG command;
	
	PFUSER_MOUNT_PARAMETER MountParameter = (PFUSER_MOUNT_PARAMETER)malloc(sizeof(FUSER_MOUNT_PARAMETER));		

	if (argc < 5) {
		// TODO: Change path/name
		fprintf(stderr, "Demo.exe\n"
			"  /r RootDirectory (ex. /r c:\\test)\n"
			"  /l DriveLetter (ex. /l m)\n"
			"  /t ThreadsCount (ex. /t 5)\n"
			"  /d (enable debug output)\n"
			"  /s (use stderr for output)\n"
			"  /m (use removable drive)\n");
		return -1;
	}

	g_DebugMode = FALSE;
	g_UseStdErr = FALSE;

	ZeroMemory(MountParameter, sizeof(FUSER_MOUNT_PARAMETER));
	MountParameter->StructVersion = 1; //Version for mount_parameter-struct
	MountParameter->ThreadsCount = 0; // use default

	for (command = 1; command < argc; command++) {
		switch (towlower(argv[command][1])) {		
		case L'r':
			command++;

			wcscpy_s(RootDirectory, sizeof(RootDirectory)/sizeof(WCHAR), argv[command]);
			DbgPrint(L"RootDirectory: %ls\n", RootDirectory);
			break;
		case L'l':
			command++;
			wcscpy_s(MountPoint, sizeof(MountPoint)/sizeof(WCHAR), argv[command]);
			MountParameter->MountPoint = MountPoint;
			break;
		case L't':
			command++;
			MountParameter->ThreadsCount = (USHORT)_wtoi(argv[command]);
			break;
		case L'd':
			g_DebugMode = TRUE;
			break;
		case L's':
			g_UseStdErr = TRUE;
			break;		
		case L'm':
			MountParameter->Flags |= FUSER_MOUNT_PARAMETER_FLAG_TYPE_REMOVABLE;
			break;		
		default:
			fwprintf(stderr, L"unknown command: %s\n", argv[command]);
			return -1;
		}
	}

	if (g_DebugMode) {
		MountParameter->Flags |= FUSER_MOUNT_PARAMETER_FLAG_DEBUG;
	}
	if (g_UseStdErr) {
		MountParameter->Flags |= FUSER_MOUNT_PARAMETER_FLAG_STDERR;
	}
	
	MountParameter->EventLoader = EventLoader;
	
	HeartbeatData.HeartbeatActive = FALSE;
	HeartbeatData.HeartbeatThread = NULL;
	HeartbeatData.HeartbeatAbort = FALSE;
		
	status = FuserDeviceMount(MountParameter);
	
	if (HeartbeatData.HeartbeatThread != NULL){
		HeartbeatData.HeartbeatAbort = TRUE;
		WaitForSingleObject(HeartbeatData.HeartbeatThread, INFINITE);
	}
	
	switch (status) {
	case FUSER_DEVICEMOUNT_SUCCESS:
		fprintf(stderr, "Success\n");
		break;
	case FUSER_DEVICEMOUNT_VERSION_ERROR:
		fprintf(stderr, "Version viaolation\n");
		break;
	case FUSER_DEVICEMOUNT_EVENT_LOAD_ERROR:
		fprintf(stderr, "Error while loading the events\n");
		break;
	case FUSER_DEVICEMOUNT_BAD_MOUNT_POINT_ERROR:
		fprintf(stderr, "Mountpoint invalid\n");
		break;
	case FUSER_DEVICEMOUNT_DRIVER_INSTALL_ERROR:
		fprintf(stderr, "driver not installed\n");
		break;
	case FUSER_DEVICEMOUNT_DRIVER_START_ERROR:
		fprintf(stderr, "driver not started\n");
		break;
	case FUSER_DEVICEMOUNT_MOUNT_ERROR:
		fprintf(stderr, "device can't mount\n");
		break;
	default:
		fprintf(stderr, "Unknown error: %d\n", status);
		break;
	}	
	

	free(MountParameter);
	return 0;
}
