using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.IO;
using FuserNet;

namespace FuserLowlevelDriver {
    interface IFuserFilesystemDevice {

        void LogErrorMessage(string functionname, string message);

        Win32Returncode CreateFile         (FuserFileHandler hFile, FuserFileAccess access, FileShare share, FileMode mode, FileOptions options, FileAttributes attribut);
        Win32Returncode OpenDirectory      (FuserFileHandler hFile);
        Win32Returncode CreateDirectory    (FuserFileHandler hFile);
        Win32Returncode Cleanup            (FuserFileHandler hFile);
        Win32Returncode CloseFile          (FuserFileHandler hFile);
        Win32Returncode ReadFile           (FuserFileHandler hFile, byte[] buffer, ref uint readBytes,    long offset);
        Win32Returncode WriteFile          (FuserFileHandler hFile, byte[] buffer, ref uint writtenBytes, long offset);
        Win32Returncode FlushFileBuffers   (FuserFileHandler hFile);
        Win32Returncode GetFileInformation (FuserFileHandler hFile, FuserFileInformation fileinfo);
        Win32Returncode FindFiles          (FuserFileHandler hFile, List<FuserFileInformation> files);
        Win32Returncode SetFileAttributes  (FuserFileHandler hFile, FileAttributes attr);
        Win32Returncode SetFileTime        (FuserFileHandler hFile, DateTime CreationTime, DateTime LastAccessTime, DateTime LastWriteTime);

        Win32Returncode DeleteFile         (FuserFileHandler hFile);
        Win32Returncode DeleteDirectory    (FuserFileHandler hFile);

        Win32Returncode MoveFile           (FuserFileHandler hFile , string newFilename, bool replace);

        Win32Returncode SetEndOfFile       (FuserFileHandler hFile, long length);
        Win32Returncode SetAllocationSize  (FuserFileHandler hFile, long length);

        Win32Returncode LockFile           (FuserFileHandler hFile, long offset, long length);
        Win32Returncode UnlockFile         (FuserFileHandler hFile, long offset, long length);
        
        Win32Returncode GetDiskFreeSpace   (ref ulong freeBytesAvailable, ref ulong totalBytes, ref ulong totalFreeBytes);
        Win32Returncode Mount              (string MountPoint, string RawDevice);
    }
}
