using System;
using System.Collections.Generic;
using System.IO;
using FuserLowlevelDriver;

namespace FuserNet {
    class DebugSynLock : IFuserFilesystemDevice {

        private IFuserFilesystemDevice sourceSystem;

        public DebugSynLock(IFuserFilesystemDevice sourceSystem) {            
            this.sourceSystem = sourceSystem;
        }

        public void LogErrorMessage(string functionname, string message) {
            lock (this) {
                this.sourceSystem.LogErrorMessage(functionname, message);
            }
        }


        public Win32Returncode CreateFile(FuserFileHandler hFile, FuserFileAccess access, FileShare share, FileMode mode, FileOptions options, FileAttributes attribut) {
            lock (this) {
                return this.sourceSystem.CreateFile(hFile, access, share, mode, options, attribut );
            }
        }

        public Win32Returncode OpenDirectory(FuserFileHandler hFile) {
            lock (this) {
                return this.sourceSystem.OpenDirectory( hFile);
            }
        }

        public Win32Returncode CreateDirectory(FuserFileHandler hFile) {
            lock (this) {
                return this.sourceSystem.CreateDirectory(hFile);
            }
        }

        public Win32Returncode Cleanup(FuserFileHandler hFile) {
            lock (this) {
                return this.sourceSystem.Cleanup( hFile);
            }
        }

        public Win32Returncode CloseFile(FuserFileHandler hFile) {
            lock (this) {
                return this.sourceSystem.CloseFile( hFile);
            }
        }

        public Win32Returncode FlushFileBuffers(FuserFileHandler hFile) {
            lock (this) {
                return this.sourceSystem.FlushFileBuffers( hFile);
            }
        }

        public Win32Returncode DeleteFile(FuserFileHandler hFile) {
            lock (this) {
                return this.sourceSystem.DeleteFile( hFile);
            }
        }

        public Win32Returncode DeleteDirectory( FuserFileHandler hFile) {
            lock (this) {
                return this.sourceSystem.DeleteDirectory(hFile);
            }
        }

        public Win32Returncode MoveFile(FuserFileHandler hFile, string newFilename, bool replace) {
            lock (this) {
                return this.sourceSystem.MoveFile(hFile, newFilename, replace);
            }
        }

        public Win32Returncode SetEndOfFile(FuserFileHandler hFile,  long length) {
            lock (this) {
                return this.sourceSystem.SetEndOfFile(hFile, length);
            }
        }

        public Win32Returncode Mount(string MountPoint, string RawDevice) {
            { lock (this) { return this.sourceSystem.Mount(MountPoint, RawDevice); } }
        }


        public Win32Returncode ReadFile(FuserFileHandler hFile,  byte[] buffer, ref uint readBytes, long offset) {
            lock (this) {
                return this.sourceSystem.ReadFile(hFile, buffer, ref readBytes, offset );
            }
        }

        public Win32Returncode WriteFile(FuserFileHandler hFile,  byte[] buffer, ref uint writtenBytes, long offset) {
            lock (this) {
                return this.sourceSystem.WriteFile(hFile, buffer, ref writtenBytes, offset);
            }
        }

        public Win32Returncode GetFileInformation(FuserFileHandler hFile,  FuserFileInformation fileinfo) {
            lock (this) {
                return this.sourceSystem.GetFileInformation(hFile, fileinfo);
            }
        }

        public Win32Returncode FindFiles(FuserFileHandler hFile,  List<FuserFileInformation> files) {
            lock (this) {
                return this.sourceSystem.FindFiles(hFile, files );
            }
        }

        public Win32Returncode SetFileAttributes(FuserFileHandler hFile, FileAttributes attr ) {
            lock (this) {
                return this.sourceSystem.SetFileAttributes(hFile, attr);
            }
        }

        public Win32Returncode SetFileTime(FuserFileHandler hFile, DateTime CreationTime, DateTime LastAccessTime, DateTime LastWriteTime) {
            lock (this) {
                return this.sourceSystem.SetFileTime(hFile, CreationTime, LastAccessTime, LastWriteTime);
            }
        }

        public Win32Returncode SetAllocationSize(FuserFileHandler hFile,  long length) {
            lock (this) {
                return this.sourceSystem.SetAllocationSize(hFile,length );
            }
        }

        public Win32Returncode LockFile         (FuserFileHandler hFile, long offset, long length ) { lock (this) { return this.sourceSystem.LockFile   (hFile, offset, length ); } }
        public Win32Returncode UnlockFile       (FuserFileHandler hFile, long offset, long length ) { lock (this) { return this.sourceSystem.UnlockFile (hFile, offset, length); } }
        public Win32Returncode GetDiskFreeSpace (ref ulong freeBytesAvailable, ref ulong totalBytes, ref ulong totalFreeBytes) { lock (this) { return this.sourceSystem.GetDiskFreeSpace(ref freeBytesAvailable, ref totalBytes, ref totalFreeBytes); } }

        
    }
}
