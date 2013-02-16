
  Fuser Libary

  Copyright (C) 2011 - 2013 Christian Auer christian.auer@gmx.ch
  Copyright (C) 2007 - 2011 Hiroki Asakawa http://dokan-dev.net/en



What is Fuser Library
=====================

When you want to create a new file system on Windows, for example to
improve FAT or NTFS, you need to develope a file system
driver. Developing a device driver that works in kernel mode on
windows is extremley difficult.By using Fuser library, you can create
your own file systems very easily without writing device driver. Fuser
Library is similar to FUSE(Linux user mode file system) but works on
Windows.


Licensing
=========

Fuser library contains LGPL, MIT licensed programs.

- user-mode library (fuser.dll)  LGPL
- driver (fuser.sys)             LGPL
- control program (fuserctl.exe) MIT
- mount service (mouter.exe)     MIT
- samples (mirror.c)             MIT

For detals, please check license files.
LGPL license.lgpl.txt
GPL  license.gpl.txt
MIT  license.mit.txt



Environment
===========

Fuser Library works on Windowx XP,2003,Vista,2008,7 x86 and Windows
2003,Vista,2008,7 x64.


How it works
============

Fuser library contains a user mode DLL (fuser.dll) and a kernel mode
file system driver (fuser.sys). Once Fuser file system driver is
installed, you can create file systems which can be seen as normal
file systems in Windows. The application that creates file systems
using Fuser library is called File system application. File operation
requests from user programs (e.g., CreateFile, ReadFile, WriteFile,
...) will be sent to the Windows I/O subsystem (runs in kernel mode)
which will subsequently forward the requests to the Fuser file system
driver (fuser.sys). By using functions provided by the Fuser user mode
library (fuser.dll), file system applications are able to register
callback functions to the file system driver. The file system driver
will invoke these callback routines in order to response to the
requests it received. The results of the callback routines will be
sent back to the user program. For example, when Windows Explorer
requests to open a directory, the OpenDirectory request will be sent
to Fuser file system driver and the driver will invoke the
OpenDirectory callback provided by the file system application. The
results of this routine are sent back to Windows Explorer as the
response to the OpenDirectory request. Therefore, the Fuser file
system driver acts as a proxy between user programs and file system
applications. The advantage of this approach is that it allows
programmers to develop file systems in user mode which is safe and
easy to debug.


Components of the Library and installation
==========================================

When the installer executes, it will install Fuser file system driver
(fuser.sys), register Fuser mount service (mouter.exe) and several
libraries. The detailed list of files installed is as follows:

SystemFolder\fuser.dll
   Fuser user mode library

SystemFolder\drivers\fuser.sys
   Fuser File System Driver

ProgramFilesFolder\Fuser\FuserLibrary\mounter.exe
   Fuser mouter service

ProgramFilesFolder\Fuser\FuserLibrary\fuserctl.exe
   Fuser control program

ProgramFilesFolder\Fuser\FuserLibrary\fuser.lib
   Fuser import library

ProgramFilesFolder\Fuser\FuserLibrary\fuser.h
   Fuser library header

ProgramFilesFolder\Fuser\FuserLibrary\readme.txt
   this file

You can use Add/Remove programs in Control Panel to uninstall Fuser.
It is required to restart your computer after uninstallation.


How to create your file systems
===============================

To create file system, an application needs to implement functions in
FUSER_OPERATIONS structure (declared in fuser.h). Once implemented,
you can invoke FuserMain function with FUSER_OPERATIONS as parameter
in order to mount the file system. The semantics of functions in
FUSER_OPERATIONS is just similar to Windows APIs with the same
name. The parameters for these functions are therefore the same as for
the counterpart Windows APIs. These functions are called from many
threads so they need to be thread-safe, otherwise many problems may
occur.

These functions are typically invoked in this sequence:

1. CreateFile(OpenDirectory, OpenDirectory)
2. Other functions
3. Cleanup
4. CloseFile

Before file access operations (listing directory, reading file
attributes, ...), file creation functions (OpenDirectory, CreateFile,
...) are always invoked. On other hand, the function Cleanup always
get called by the Fuser file system driver when the file is closed by
the CloseFile Windows API.  Each function should return 0 when the
operation succeeded, otherwise it should return a negative value
represents error code. The error codes are negated Windows System
Error Codes. For examaple, when CreateFile can't open a file, you
should return -2( -1 * ERROR_FILE_NOT_FOUND).

The last parameter of each function is a FUSER_FILE_INFO structure :

   typedef struct _FUSER_FILE_INFO {

       ULONG64 Context;
       ULONG64 FuserContext;
       ULONG   ProcessId;
       BOOL    IsDirectory;

   } FUSER_FILE_INFO, *PFUSER_FILE_INFO;

Each file handle from user mode is associated with a FUSER_FILE_INFO
struct. Hence, the content of the struct does not change if the same
file handle is used. The struct is created when the file is opened by
CreateFile system call and destroyed when the file is closed (by
CloseFile system call).  The meaning of each field in the struct is as
follows:

  Context : a specific value assigned by the file system
  application. File system application can freely use this variable to
  store values that are constant in a file access session (the period
  from CreateFile to CloseFile) such as file handle, etc.

  FuserContext : reserved. Used by Fuser library.

  ProcessId : Process ID of the process that opened the file.

  IsDirectory : determine if the opened file is a directory, see
  exceptions bellow.


   int (*CreateFile) (
       LPCWSTR,      // FileName
       DWORD,        // DesiredAccess
       DWORD,        // ShareMode
       DWORD,        // CreationDisposition
       DWORD,        // FlagsAndAttributes
       PFUSER_FILE_INFO);

   int (*OpenDirectory) (
       LPCWSTR,          // FileName
       PFUSER_FILE_INFO);

   int (*CreateDirectory) (
       LPCWSTR,          // FileName
       PFUSER_FILE_INFO);

When the variable IsDirectory is set to TRUE, the file under the
operation is a directory. When it is FALSE, the file system
application programmers are required to set the variable to TRUE if
the current operation acts on a directory. If the value is FALSE and
the current operation is not acting on a directory, the programmers
should not change the variable. Note that setting the variable to TRUE
when a directory is accessed is very important for the Fuser
library. If it is not correctly set, the library does not know the
operation is acting on a directory and many problems may occur.
CreateFile should return ERROR_ALREADY_EXISTS (183) when the
CreationDisposition is CREATE_ALWAYS or OPEN_ALWAYS and the file under
question has already existed.

   int (*Cleanup) (
       LPCWSTR,      // FileName
       PFUSER_FILE_INFO);

   int (*CloseFile) (
       LPCWSTR,      // FileName
       PFUSER_FILE_INFO);

Cleanup is invoked when the function CloseHandle in Windows API is
executed. If the file system application stored file handle in the
Context variable when the function CreateFile is invoked, this should
be closed in the Cleanup function, not in CloseFile function. If the
user application calls CloseHandle and subsequently open the same
file, the CloseFile function of the file system application may not be
invoked before the CreateFile API is called. This may cause sharing
violation error.  Note: when user uses memory mapped file, WriteFile
or ReadFile function may be invoked after Cleanup in order to complete
the I/O operations. The file system application should also properly
work in this case.

   int (*FindFiles) (
       LPCWSTR,           // PathName
       PFillFindData,     // call this function with PWIN32_FIND_DATAW
       PFUSER_FILE_INFO); //  (see PFillFindData definition)


   // You should implement either FindFires or FindFilesWithPattern
   int (*FindFilesWithPattern) (
       LPCWSTR,           // PathName
       LPCWSTR,           // SearchPattern
       PFillFindData,     // call this function with PWIN32_FIND_DATAW
       PFUSER_FILE_INFO);

FindFiles or FindFilesWithPattern is called in order to response to
directory listing requests. You should implement either FielFiles or
FileFilesWithPttern.  For each directory entry, file system
application should call the function FillFindData (passed as a
function pointer to FindFiles, FindFilesWithPattern) with the
WIN32_FIND_DATAW structure filled with directory information:
FillFindData( &win32FindDataw, FuserFileInfo ).  It is the
responsibility of file systems to process wildcard patterns because
shells in Windows are not designed to work properly with pattern
matching. When file system application provides FindFiles, the
wildcard patterns are automatically processed by the Fuser
library. You can control wildcard matching by implementing
FindFilesWithPattern function.  The function FuserIsNameInExpression
exported by the Fuser library (fuser.dll) can be used to process
wildcard matching.


Mounting
========

   #define FUSER_OPTION_DEBUG       1 // ouput debug message
   #define FUSER_OPTION_STDERR      2 // ouput debug message to stderr
   #define FUSER_OPTION_ALT_STREAM  4 // use alternate stream
   #define FUSER_OPTION_KEEP_ALIVE  8 // use auto unmount
   #define FUSER_OPTION_NETWORK    16 // use network drive,
                                      //you need to install Fuser network provider.
   #define FUSER_OPTION_REMOVABLE  32 // use removable drive

   typedef struct _FUSER_OPTIONS {
       USHORT  Version;  // Supported Fuser Version, ex. "530" (Fuser ver 0.5.3)
       ULONG   ThreadCount;  // number of threads to be used
       ULONG   Options;  // combination of FUSER_OPTIONS_*
       ULONG64 GlobalContext;  // FileSystem can use this variable
       LPCWSTR MountPoint;  // mount point "M:\" (drive letter) or
                            // "C:\mount\fuser" (path in NTFS)
   } FUSER_OPTIONS, *PFUSER_OPTIONS;

   int FUSERAPI FuserMain(
       PFUSER_OPTIONS    FuserOptions,
       PFUSER_OPERATIONS FuserOperations);

As stated above, the file system can be mounted by invoking FuserMain
function. The function blocks until the file system is unmounted. File
system applications should fill FuserOptions with options for Fuser
runtime library and FuserOperations with function pointers for file
system operations (such as CreateFile, ReadFile, CloseHandle, ...)
before passing these parameters to FuserMain function.  Functions in
FuserOperations structure need to be thread-safe, because they are
called in several threads (not the thread invoked FuserMain) with
different execution contexts.

Fuser options are as follows:

  Version :
    The version of Fuser library. You have to set a supported version.
    Fuser library may change the behavior based on this version number.
    ie. 530 (Fuser 0.5.3)
  ThreadCount :
    The number of threads internaly used by the Fuser library.
    If this value is set to 0, the default value will be used.
    When debugging the file system, file system application should
    set this value to 1 to avoid nondeterministic behaviors of
    multithreading.
  Options :
    A Combination of FUSER_OPTION_* constants.
  GlobalContext :
    Your filrsystem can use this variable to store a mount specific
    structure.
  MountPoint :
    A mount point. "M:\" drive letter or "C:\mount\fuser" a directory
    (needs empty) in NTFS

If the mount operation succeeded, the return value is FUSER_SUCCESS,
otherwise, the following error code is returned.

   #define FUSER_SUCCESS                0
   #define FUSER_ERROR                 -1 /* General Error */
   #define FUSER_DRIVE_LETTER_ERROR    -2 /* Bad Drive letter */
   #define FUSER_DRIVER_INSTALL_ERROR  -3 /* Can't install driver */
   #define FUSER_START_ERROR           -4 /* Driver something wrong */
   #define FUSER_MOUNT_ERROR           -5 /* Can't assign a drive letter or mount point */
   #define FUSER_MOUNT_POINT_ERROR     -6 /* Mount point is invalid */


Unmounting
==========

File system can be unmounted by calling the function FuserUnmount.  In
most cases when the programs or shells use the file system hang,
unmount operation will solve the problems by bringing the system to
the previous state when the file system is not mounted.

User may use FuserCtl to unmount file system like this:
   > fuserctl.exe /u DriveLetter


Misc
====

If there are bugs in Fuser library or file system applications which
use the library, you will get the Windows blue screen. Therefore, it
is strongly recommended to use Virtual Machine when you develop file
system applications.

