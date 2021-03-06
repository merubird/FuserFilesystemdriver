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

#include "fuseri.h"
#include "fileinfo.h"
#include "list.h"

// TODO: belong to the method FuserIsNameInExpression, check if can be removed
#define DOS_STAR                        (L'<')
#define DOS_QM                          (L'>')
#define DOS_DOT                         (L'"')




#if _MSC_VER < 1300 // VC6
typedef ULONG ULONG_PTR;
#endif

typedef struct _FUSER_FIND_DATA {
	WIN32_FIND_DATAW	FindData;
	LIST_ENTRY			ListEntry;
} FUSER_FIND_DATA, *PFUSER_FIND_DATA;




VOID
ClearFindData(
  PLIST_ENTRY	ListHead)
{
	// free all list entries
	while(!IsListEmpty(ListHead)) {
		PLIST_ENTRY entry = RemoveHeadList(ListHead);
		PFUSER_FIND_DATA find = CONTAINING_RECORD(entry, FUSER_FIND_DATA, ListEntry);
		free(find);
	}
}




int WINAPI
FuserFillFileData(
	PWIN32_FIND_DATAW	FindData,
	PFUSER_FILE_INFO	FileInfo)
{
	PLIST_ENTRY listHead = ((PFUSER_OPEN_INFO)FileInfo->FuserContext)->DirListHead;
	PFUSER_FIND_DATA	findData;
	
	findData = malloc(sizeof(FUSER_FIND_DATA));
	ZeroMemory(findData, sizeof(FUSER_FIND_DATA));
	InitializeListHead(&findData->ListEntry);

	findData->FindData = *FindData;

	InsertTailList(listHead, &findData->ListEntry);
	return 0;
}




VOID
FuserFillDirInfo(
	PFILE_DIRECTORY_INFORMATION	Buffer,
	PWIN32_FIND_DATAW			FindData,
	ULONG						Index)
{
	ULONG nameBytes = wcslen(FindData->cFileName) * sizeof(WCHAR);

	Buffer->FileIndex = Index;
	Buffer->FileAttributes = FindData->dwFileAttributes;
	Buffer->FileNameLength = nameBytes;

	Buffer->EndOfFile.HighPart = FindData->nFileSizeHigh;
	Buffer->EndOfFile.LowPart   = FindData->nFileSizeLow;
	Buffer->AllocationSize.HighPart = FindData->nFileSizeHigh;
	Buffer->AllocationSize.LowPart = FindData->nFileSizeLow;

	Buffer->CreationTime.HighPart = FindData->ftCreationTime.dwHighDateTime;
	Buffer->CreationTime.LowPart  = FindData->ftCreationTime.dwLowDateTime;

	Buffer->LastAccessTime.HighPart = FindData->ftLastAccessTime.dwHighDateTime;
	Buffer->LastAccessTime.LowPart  = FindData->ftLastAccessTime.dwLowDateTime;

	Buffer->LastWriteTime.HighPart = FindData->ftLastWriteTime.dwHighDateTime;
	Buffer->LastWriteTime.LowPart  = FindData->ftLastWriteTime.dwLowDateTime;

	Buffer->ChangeTime.HighPart = FindData->ftLastWriteTime.dwHighDateTime;
	Buffer->ChangeTime.LowPart  = FindData->ftLastWriteTime.dwLowDateTime;

	RtlCopyMemory(Buffer->FileName, FindData->cFileName, nameBytes);
}


VOID
FuserFillFullDirInfo(
	PFILE_FULL_DIR_INFORMATION	Buffer,
	PWIN32_FIND_DATAW			FindData,
	ULONG						Index)
{
	ULONG nameBytes = wcslen(FindData->cFileName) * sizeof(WCHAR);

	Buffer->FileIndex = Index;
	Buffer->FileAttributes = FindData->dwFileAttributes;
	Buffer->FileNameLength = nameBytes;

	Buffer->EndOfFile.HighPart = FindData->nFileSizeHigh;
	Buffer->EndOfFile.LowPart   = FindData->nFileSizeLow;
	Buffer->AllocationSize.HighPart = FindData->nFileSizeHigh;
	Buffer->AllocationSize.LowPart = FindData->nFileSizeLow;

	Buffer->CreationTime.HighPart = FindData->ftCreationTime.dwHighDateTime;
	Buffer->CreationTime.LowPart  = FindData->ftCreationTime.dwLowDateTime;

	Buffer->LastAccessTime.HighPart = FindData->ftLastAccessTime.dwHighDateTime;
	Buffer->LastAccessTime.LowPart  = FindData->ftLastAccessTime.dwLowDateTime;

	Buffer->LastWriteTime.HighPart = FindData->ftLastWriteTime.dwHighDateTime;
	Buffer->LastWriteTime.LowPart  = FindData->ftLastWriteTime.dwLowDateTime;

	Buffer->ChangeTime.HighPart = FindData->ftLastWriteTime.dwHighDateTime;
	Buffer->ChangeTime.LowPart  = FindData->ftLastWriteTime.dwLowDateTime;

	Buffer->EaSize = 0;

	RtlCopyMemory(Buffer->FileName, FindData->cFileName, nameBytes);
}


VOID
FuserFillNamesInfo(
	PFILE_NAMES_INFORMATION	Buffer,
	PWIN32_FIND_DATAW		FindData,
	ULONG					Index)
{
	ULONG nameBytes = wcslen(FindData->cFileName) * sizeof(WCHAR);

	Buffer->FileIndex = Index;
	Buffer->FileNameLength = nameBytes;

	RtlCopyMemory(Buffer->FileName, FindData->cFileName, nameBytes);
}


VOID
FuserFillBothDirInfo(
	PFILE_BOTH_DIR_INFORMATION		Buffer,
	PWIN32_FIND_DATAW				FindData,
	ULONG							Index)
{
	ULONG nameBytes = wcslen(FindData->cFileName) * sizeof(WCHAR);

	Buffer->FileIndex = Index;
	Buffer->FileAttributes = FindData->dwFileAttributes;
	Buffer->FileNameLength = nameBytes;
	Buffer->ShortNameLength = 0;

	Buffer->EndOfFile.HighPart = FindData->nFileSizeHigh;
	Buffer->EndOfFile.LowPart   = FindData->nFileSizeLow;
	Buffer->AllocationSize.HighPart = FindData->nFileSizeHigh;
	Buffer->AllocationSize.LowPart = FindData->nFileSizeLow;

	Buffer->CreationTime.HighPart = FindData->ftCreationTime.dwHighDateTime;
	Buffer->CreationTime.LowPart  = FindData->ftCreationTime.dwLowDateTime;

	Buffer->LastAccessTime.HighPart = FindData->ftLastAccessTime.dwHighDateTime;
	Buffer->LastAccessTime.LowPart  = FindData->ftLastAccessTime.dwLowDateTime;

	Buffer->LastWriteTime.HighPart = FindData->ftLastWriteTime.dwHighDateTime;
	Buffer->LastWriteTime.LowPart  = FindData->ftLastWriteTime.dwLowDateTime;

	Buffer->ChangeTime.HighPart = FindData->ftLastWriteTime.dwHighDateTime;
	Buffer->ChangeTime.LowPart  = FindData->ftLastWriteTime.dwLowDateTime;

	Buffer->EaSize = 0;

	RtlCopyMemory(Buffer->FileName, FindData->cFileName, nameBytes);
}



VOID
FuserFillIdBothDirInfo(
	PFILE_ID_BOTH_DIR_INFORMATION	Buffer,
	PWIN32_FIND_DATAW				FindData,
	ULONG							Index)
{
	ULONG nameBytes = wcslen(FindData->cFileName) * sizeof(WCHAR);

	Buffer->FileIndex = Index;
	Buffer->FileAttributes = FindData->dwFileAttributes;
	Buffer->FileNameLength = nameBytes;
	Buffer->ShortNameLength = 0;

	Buffer->EndOfFile.HighPart = FindData->nFileSizeHigh;
	Buffer->EndOfFile.LowPart   = FindData->nFileSizeLow;
	Buffer->AllocationSize.HighPart = FindData->nFileSizeHigh;
	Buffer->AllocationSize.LowPart = FindData->nFileSizeLow;

	Buffer->CreationTime.HighPart = FindData->ftCreationTime.dwHighDateTime;
	Buffer->CreationTime.LowPart  = FindData->ftCreationTime.dwLowDateTime;

	Buffer->LastAccessTime.HighPart = FindData->ftLastAccessTime.dwHighDateTime;
	Buffer->LastAccessTime.LowPart  = FindData->ftLastAccessTime.dwLowDateTime;

	Buffer->LastWriteTime.HighPart = FindData->ftLastWriteTime.dwHighDateTime;
	Buffer->LastWriteTime.LowPart  = FindData->ftLastWriteTime.dwLowDateTime;

	Buffer->ChangeTime.HighPart = FindData->ftLastWriteTime.dwHighDateTime;
	Buffer->ChangeTime.LowPart  = FindData->ftLastWriteTime.dwLowDateTime;

	Buffer->EaSize = 0;
	Buffer->FileId.QuadPart = 0;

	RtlCopyMemory(Buffer->FileName, FindData->cFileName, nameBytes);
}







ULONG
FuserFillDirectoryInformation(
	FILE_INFORMATION_CLASS	DirectoryInfo,
	PVOID					Buffer,
	PULONG					LengthRemaining,
	PWIN32_FIND_DATAW		FindData,
	ULONG					Index)
{
	ULONG	nameBytes;
	ULONG	thisEntrySize;
	
	nameBytes = wcslen(FindData->cFileName) * sizeof(WCHAR);

	thisEntrySize = nameBytes;

	switch (DirectoryInfo) {
	case FileDirectoryInformation:
		thisEntrySize += sizeof(FILE_DIRECTORY_INFORMATION);
		break;
	case FileFullDirectoryInformation:
		thisEntrySize += sizeof(FILE_FULL_DIR_INFORMATION);
		break;
	case FileNamesInformation:
		thisEntrySize += sizeof(FILE_NAMES_INFORMATION);
		break;
	case FileBothDirectoryInformation:
		thisEntrySize += sizeof(FILE_BOTH_DIR_INFORMATION);
		break;
	case FileIdBothDirectoryInformation:
		thisEntrySize += sizeof(FILE_ID_BOTH_DIR_INFORMATION);
		break;
	default:
		break;
	}

	// Must be align on a 8-byte boundary.
	thisEntrySize = QuadAlign(thisEntrySize);

	// no more memory, don't fill any more
	if (*LengthRemaining < thisEntrySize) {
		DbgPrint("  no memory\n");
		return 0;
	}

	RtlZeroMemory(Buffer, thisEntrySize);

	switch (DirectoryInfo) {
	case FileDirectoryInformation:
		FuserFillDirInfo(Buffer, FindData, Index);
		break;
	case FileFullDirectoryInformation:
		FuserFillFullDirInfo(Buffer, FindData, Index);
		break;
	case FileNamesInformation:
		FuserFillNamesInfo(Buffer, FindData, Index);
		break;
	case FileBothDirectoryInformation:
		FuserFillBothDirInfo(Buffer, FindData, Index);
		break;
	case FileIdBothDirectoryInformation:
		FuserFillIdBothDirInfo(Buffer, FindData, Index);
		break;
	default:
		break;
	}

	*LengthRemaining -= thisEntrySize;

	return thisEntrySize;
}




// check whether Name matches Expression
// Expression can contain "?"(any one character) and "*" (any string)
// when IgnoreCase is TRUE, do case insenstive matching
//
// http://msdn.microsoft.com/en-us/library/ff546850(v=VS.85).aspx
// * (asterisk) Matches zero or more characters.
// ? (question mark) Matches a single character.
// DOS_DOT Matches either a period or zero characters beyond the name string.
// DOS_QM Matches any single character or, upon encountering a period or end
//        of name string, advances the expression to the end of the set of
//        contiguous DOS_QMs.
// DOS_STAR Matches zero or more characters until encountering and matching
//          the final . in the name.
// TODO: perhaps remove this method
BOOL FuserIsNameInExpression(
	LPCWSTR		Expression, // matching pattern
	LPCWSTR		Name, // file name
	BOOL		IgnoreCase)
{
	ULONG ei = 0;
	ULONG ni = 0;

	while (Expression[ei] != '\0') {

		if (Expression[ei] == L'*') {
			ei++;
			if (Expression[ei] == '\0')
				return TRUE;

			while (Name[ni] != '\0') {
				if (FuserIsNameInExpression(&Expression[ei], &Name[ni], IgnoreCase))
					return TRUE;
				ni++;
			}

		} else if (Expression[ei] == DOS_STAR) {

			ULONG p = ni;
			ULONG lastDot = 0;
			ei++;
			
			while (Name[p] != '\0') {
				if (Name[p] == L'.')
					lastDot = p;
				p++;
			}
			

			while (TRUE) {
				if (Name[ni] == '\0' || ni == lastDot)
					break;

				if (FuserIsNameInExpression(&Expression[ei], &Name[ni], IgnoreCase))
					return TRUE;
				ni++;
			}
			
		} else if (Expression[ei] == DOS_QM)  {
			
			ei++;
			if (Name[ni] != L'.') {
				ni++;
			} else {

				ULONG p = ni + 1;
				while (Name[p] != '\0') {
					if (Name[p] == L'.')
						break;
					p++;
				}

				if (Name[p] == L'.')
					ni++;
			}

		} else if (Expression[ei] == DOS_DOT) {
			ei++;

			if (Name[ni] == L'.')
				ni++;

		} else {
			if (Expression[ei] == L'?') {
				ei++; ni++;
			} else if(IgnoreCase && towupper(Expression[ei]) == towupper(Name[ni])) {
				ei++; ni++;
			} else if(!IgnoreCase && Expression[ei] == Name[ni]) {
				ei++; ni++;
			} else {
				return FALSE;
			}
		}
	}

	if (ei == wcslen(Expression) && ni == wcslen(Name))
		return TRUE;
	

	return FALSE;
}



// add entry which matches the pattern specifed in EventContext
// to the buffer specifed in EventInfo
//
LONG
MatchFiles(
	PEVENT_CONTEXT			EventContext,
	PEVENT_INFORMATION		EventInfo,
	PLIST_ENTRY				FindDataList,
	BOOLEAN					PatternCheck)
{
	PLIST_ENTRY	thisEntry, listHead, nextEntry;

	ULONG	lengthRemaining = EventInfo->BufferLength;
	PVOID	currentBuffer	= EventInfo->Buffer;
	PVOID	lastBuffer		= currentBuffer;
	ULONG	index = 0;

	PWCHAR pattern = NULL;
	
	// search patten is specified
	if (PatternCheck && EventContext->Directory.SearchPatternLength != 0) {
		pattern = (PWCHAR)((SIZE_T)&EventContext->Directory.SearchPatternBase[0]
					+ (SIZE_T)EventContext->Directory.SearchPatternOffset);
	}

	listHead = FindDataList;

    for(thisEntry = listHead->Flink;
		thisEntry != listHead;
		thisEntry = nextEntry) {

		PFUSER_FIND_DATA	find;
		nextEntry = thisEntry->Flink;

		find = CONTAINING_RECORD(thisEntry, FUSER_FIND_DATA, ListEntry);

		DbgPrintW(L"FileMatch? : %s (%s,%d,%d)\n", find->FindData.cFileName,
			(pattern ? pattern : L"null"),
			EventContext->Directory.FileIndex, index);

		// pattern is not specified or pattern match is ignore cases
		if (!pattern || FuserIsNameInExpression(pattern, find->FindData.cFileName, TRUE)) {

			if(EventContext->Directory.FileIndex <= index) {
				// index+1 is very important, should use next entry index

				ULONG entrySize = FuserFillDirectoryInformation(
									EventContext->Directory.FileInformationClass,
									currentBuffer, &lengthRemaining, &find->FindData, index+1);
				// buffer is full
				if (entrySize == 0)
					break;
			
				// pointer of the current last entry
				lastBuffer = currentBuffer;

				// end if needs to return single entry
				if (EventContext->Flags & SL_RETURN_SINGLE_ENTRY) {
					DbgPrint("  =>return single entry\n");
					index++;
					break;
				}

				DbgPrint("  =>return\n");

				// the offset of next entry
				((PFILE_BOTH_DIR_INFORMATION)currentBuffer)->NextEntryOffset = entrySize;

				// next buffer position
				(PCHAR)currentBuffer += entrySize;
			}
			index++;
		}

	}

	// Since next of the last entry doesn't exist, clear next offset
	((PFILE_BOTH_DIR_INFORMATION)lastBuffer)->NextEntryOffset = 0;

	// acctualy used length of buffer
	EventInfo->BufferLength = EventContext->Directory.BufferLength - lengthRemaining;

	// NO_MORE_FILES
	if (index <= EventContext->Directory.FileIndex)
		return -1;

	return index;
}


VOID
DispatchDirectoryInformation(
	HANDLE				Handle,
	PEVENT_CONTEXT		EventContext,
	PFUSER_INSTANCE		FuserInstance)
{
	PEVENT_INFORMATION	eventInfo;
	FUSER_FILE_INFO		fileInfo;
	PFUSER_OPEN_INFO	openInfo;
	int					status = 0;
	ULONG				fileInfoClass = EventContext->Directory.FileInformationClass;
	ULONG				sizeOfEventInfo = sizeof(EVENT_INFORMATION) - 8 + EventContext->Directory.BufferLength;

	BOOLEAN				patternCheck = TRUE;

	CheckFileName(EventContext->Directory.DirectoryName);

	eventInfo = DispatchCommon(
		EventContext, sizeOfEventInfo, FuserInstance, &fileInfo, &openInfo);

	// check whether this is handled FileInfoClass
	if (fileInfoClass != FileDirectoryInformation &&
		fileInfoClass != FileFullDirectoryInformation &&
		fileInfoClass != FileNamesInformation &&
		fileInfoClass != FileIdBothDirectoryInformation &&
		fileInfoClass != FileBothDirectoryInformation) {
		
		DbgPrint("not suported type %d\n", fileInfoClass);

		// send directory info to driver
		eventInfo->BufferLength = 0;
		eventInfo->Status = STATUS_NOT_IMPLEMENTED;
		SendEventInformation(Handle, eventInfo, sizeOfEventInfo, FuserInstance);
		free(eventInfo);
		return;
	}


	// IMPORTANT!!
	// this buffer length is fixed in MatchFiles funciton
	eventInfo->BufferLength		= EventContext->Directory.BufferLength; 

	if (openInfo->DirListHead == NULL) {
		openInfo->DirListHead = malloc(sizeof(LIST_ENTRY));
		InitializeListHead(openInfo->DirListHead);
	}

	if (EventContext->Directory.FileIndex == 0) {
		ClearFindData(openInfo->DirListHead);
	}

	if (IsListEmpty(openInfo->DirListHead)) {

		// if user defined FindFilesWithPattern
		if (FuserInstance->FuserEvents->FindFilesWithPattern) {
			LPCWSTR	pattern = L"*";
		
			// if search pattern is specified
			if (EventContext->Directory.SearchPatternLength != 0) {
				pattern = (PWCHAR)((SIZE_T)&EventContext->Directory.SearchPatternBase[0]
						+ (SIZE_T)EventContext->Directory.SearchPatternOffset);
			}

			patternCheck = FALSE; // do not recheck pattern later in MatchFiles

			status = FuserInstance->FuserEvents->FindFilesWithPattern(
						EventContext->Directory.DirectoryName,
						pattern,
						FuserFillFileData,
						&fileInfo);
	
		} else if (FuserInstance->FuserEvents->FindFiles) {

			patternCheck = TRUE; // do pattern check later in MachFiles

			// call FileSystem specifeid callback routine
			status = FuserInstance->FuserEvents->FindFiles(
						EventContext->Directory.DirectoryName,
						FuserFillFileData,
						&fileInfo);
		} else {
			status = -1;
		}
	}

	if (status < 0) {

		if (EventContext->Directory.FileIndex == 0) {
			DbgPrint("  STATUS_NO_SUCH_FILE\n");
			eventInfo->Status = STATUS_NO_SUCH_FILE;
		} else {
			DbgPrint("  STATUS_NO_MORE_FILES\n");
			eventInfo->Status = STATUS_NO_MORE_FILES;
		}

		eventInfo->BufferLength = 0;
		eventInfo->Directory.Index = EventContext->Directory.FileIndex;
		// free all of list entries
		ClearFindData(openInfo->DirListHead);
	} else {
		LONG	index;
		eventInfo->Status = STATUS_SUCCESS;

		DbgPrint("index from %d\n", EventContext->Directory.FileIndex);
		// extract entries that match search pattern from FindFiles result
		index = MatchFiles(EventContext, eventInfo, openInfo->DirListHead, patternCheck);

		// there is no matched file
		if (index <0) {
			if (EventContext->Directory.FileIndex == 0) {
				DbgPrint("  STATUS_NO_SUCH_FILE\n");
				eventInfo->Status = STATUS_NO_SUCH_FILE;
			} else {
				DbgPrint("  STATUS_NO_MORE_FILES\n");
				eventInfo->Status = STATUS_NO_MORE_FILES;
			}
			eventInfo->BufferLength = 0;
			eventInfo->Directory.Index = EventContext->Directory.FileIndex;

			ClearFindData(openInfo->DirListHead);

		} else {
			DbgPrint("index to %d\n", index);
			eventInfo->Directory.Index	= index;
		}
	}

	// information for FileSystem
	openInfo->UserContext = fileInfo.Context;

	// send directory information to driver
	SendEventInformation(Handle, eventInfo, sizeOfEventInfo, FuserInstance);
	free(eventInfo);
	return;
}






/* TODO: no longer used
VOID
FuserFillIdFullDirInfo(
	PFILE_ID_FULL_DIR_INFORMATION	Buffer,
	PWIN32_FIND_DATAW				FindData,
	ULONG							Index)
{
	ULONG nameBytes = wcslen(FindData->cFileName) * sizeof(WCHAR);

	Buffer->FileIndex = Index;
	Buffer->FileAttributes = FindData->dwFileAttributes;
	Buffer->FileNameLength = nameBytes;

	Buffer->EndOfFile.HighPart = FindData->nFileSizeHigh;
	Buffer->EndOfFile.LowPart   = FindData->nFileSizeLow;
	Buffer->AllocationSize.HighPart = FindData->nFileSizeHigh;
	Buffer->AllocationSize.LowPart = FindData->nFileSizeLow;

	Buffer->CreationTime.HighPart = FindData->ftCreationTime.dwHighDateTime;
	Buffer->CreationTime.LowPart  = FindData->ftCreationTime.dwLowDateTime;

	Buffer->LastAccessTime.HighPart = FindData->ftLastAccessTime.dwHighDateTime;
	Buffer->LastAccessTime.LowPart  = FindData->ftLastAccessTime.dwLowDateTime;

	Buffer->LastWriteTime.HighPart = FindData->ftLastWriteTime.dwHighDateTime;
	Buffer->LastWriteTime.LowPart  = FindData->ftLastWriteTime.dwLowDateTime;

	Buffer->ChangeTime.HighPart = FindData->ftLastWriteTime.dwHighDateTime;
	Buffer->ChangeTime.LowPart  = FindData->ftLastWriteTime.dwLowDateTime;

	Buffer->EaSize = 0;
	Buffer->FileId.QuadPart = 0;

	RtlCopyMemory(Buffer->FileName, FindData->cFileName, nameBytes);
}
*/



