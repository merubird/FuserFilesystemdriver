using System;
using System.Collections.Generic;
using System.IO;
using FuserLowlevelDriver;

namespace FuserNet
{
    internal class DebugAndLogProxyFS : IFuserFilesystemDevice
    {
        private IFuserFilesystemDevice sourceSystem;

        public DebugAndLogProxyFS(IFuserFilesystemDevice sourceSystem) {
            this.sourceSystem = sourceSystem;
        }

        private void LogEvent(string funcName, Win32Returncode returnValue, string Param, FuserFileHandler hFile) {
            string filename;

            if (hFile == null) {
                filename = "";
            } else {
                filename = hFile.filename;
            }

            LogMessage(funcName + "[" + (returnValue.ToString() + " " + Param).Trim() + "]: " + filename);

            //if (filename == @"\test") {
            //    LogMessage(funcName + "[" + (returnValue + " " + Param).Trim() + "]: " + filename);
            //}

            //LogMessage(funcName + ": " + InfoToString(info));
        }

        private void LogException(string funcName, Exception exception)
        {            
            LogMessage(">>>>>>>> EXCEPTION <<<<<<<< Function: " + funcName + ": " + exception.Message);
        }

        private void LogMessage(string message) {         
            Console.WriteLine(message);
        }

        private string InfoToString(FuserFileHandler hFile)
        {            
            string tmp = "";
            if (hFile.fileHandle != null) {
                tmp = hFile.fileHandle.ToString();
            }

            return tmp + "Context: " + tmp +
                        "hID:" + 0;// + ", PID:" + hFile.ProcessId + "," + getStrBool("Dir", hFile.IsDirectory) + "," + getStrBool("Delete", hFile.DeleteOnClose) + ","
                        //+ getStrBool("Paging", hFile.PagingIo) + "," + getStrBool("Sync", hFile.SynchronousIo) + "," + getStrBool("EndOfFile", hFile.WriteToEndOfFile) + ",InternalContext:" + hFile.FuserContext;
        }

        private string getStrBool(string key, bool value)
        {
            if (value)
            {
                return key + ":T";
            }
            else {
                return key + ":F";
            }
        }



        public void LogErrorMessage(string functionname, string message) {
            LogMessage(@"///////////////////////////////////////////////////////////////");
            LogException(functionname, new Exception(message));
            System.Diagnostics.Debugger.Break(); // TODO: remove
        }



        public Win32Returncode CreateFile(FuserFileHandler hFile, FuserFileAccess access, FileShare share, FileMode mode, FileOptions options, FileAttributes attribut) {
            string funcname = "CreateFile";
            string param = "";

            try {
                param = access + ";" + share + ";" + mode + ";" + options + " Attribute: " + attribut;
                Win32Returncode r = sourceSystem.CreateFile(hFile, access, share, mode, options, attribut );

                LogEvent(funcname, r,  param, hFile);
                return r;
            } catch (Exception e) {
                LogEvent(funcname, Win32Returncode.DEFAULT_UNKNOWN_ERROR,  param, hFile);
                LogException(funcname, e);
                return Win32Returncode.DEFAULT_UNKNOWN_ERROR;
            }
        }

        public Win32Returncode OpenDirectory(FuserFileHandler hFile) {
            string funcname = "OpenDirectory";
            string param = "";

            try {
                //param = ;
                Win32Returncode r = sourceSystem.OpenDirectory( hFile);

                LogEvent(funcname, r,  param, hFile);
                return r;
            } catch (Exception e) {
                LogEvent(funcname, Win32Returncode.DEFAULT_UNKNOWN_ERROR,  param, hFile);
                LogException(funcname, e);
                return Win32Returncode.DEFAULT_UNKNOWN_ERROR;
            }
        }

        public Win32Returncode CreateDirectory(FuserFileHandler hFile) {
            string funcname = "CreateDirectory";
            string param = "";

            try {
                //param = ;
                Win32Returncode r = sourceSystem.CreateDirectory( hFile);

                LogEvent(funcname, r,  param, hFile);
                return r;
            } catch (Exception e) {
                LogEvent(funcname, Win32Returncode.DEFAULT_UNKNOWN_ERROR,  param, hFile);
                LogException(funcname, e);
                return Win32Returncode.DEFAULT_UNKNOWN_ERROR;
            }
        }

        public Win32Returncode Cleanup(FuserFileHandler hFile) {
            string funcname = "Cleanup";
            string param = "";

            try {
                //param = ;
                Win32Returncode r = sourceSystem.Cleanup( hFile);

                LogEvent(funcname, r,  param, hFile);
                return r;
            } catch (Exception e) {
                LogEvent(funcname, Win32Returncode.DEFAULT_UNKNOWN_ERROR,  param, hFile);
                LogException(funcname, e);
                return Win32Returncode.DEFAULT_UNKNOWN_ERROR;
            }
        }

        public Win32Returncode CloseFile(FuserFileHandler hFile) {
            string funcname = "CloseFile";
            string param = "";

            try {
                //param = ;
                Win32Returncode r = sourceSystem.CloseFile( hFile);

                LogEvent(funcname, r,  param, hFile);
                return r;
            } catch (Exception e) {
                LogEvent(funcname, Win32Returncode.DEFAULT_UNKNOWN_ERROR,  param, hFile);
                LogException(funcname, e);
                return Win32Returncode.DEFAULT_UNKNOWN_ERROR;
            }
        }

        public Win32Returncode ReadFile(FuserFileHandler hFile, byte[] buffer, ref uint readBytes, long offset) {
            string funcname = "ReadFile";
            string param = "";

            try {
                param = buffer.Length + ";" + readBytes + ";" + offset;
                Win32Returncode r = sourceSystem.ReadFile(hFile, buffer, ref readBytes, offset );

                LogEvent(funcname, r,  param, hFile);
                return r;
            } catch (Exception e) {
                LogEvent(funcname, Win32Returncode.DEFAULT_UNKNOWN_ERROR,  param, hFile);
                LogException(funcname, e);
                return Win32Returncode.DEFAULT_UNKNOWN_ERROR;
            }
        }

        public Win32Returncode WriteFile(FuserFileHandler hFile, byte[] buffer, ref uint writtenBytes, long offset) {
            string funcname = "WriteFile";
            string param = "";

            try {
                param = buffer.Length + ";" + writtenBytes + ";" + offset;
                Win32Returncode r = sourceSystem.WriteFile(hFile, buffer, ref writtenBytes, offset);

                LogEvent(funcname, r,  param, hFile);
                return r;
            } catch (Exception e) {
                LogEvent(funcname, Win32Returncode.DEFAULT_UNKNOWN_ERROR,  param, hFile);
                LogException(funcname, e);
                return Win32Returncode.DEFAULT_UNKNOWN_ERROR;
            }
        }

        public Win32Returncode FlushFileBuffers(FuserFileHandler hFile) {
            string funcname = "FlushFileBuffers";
            string param = "";

            try {
                //param = ;
                Win32Returncode r = sourceSystem.FlushFileBuffers( hFile);

                LogEvent(funcname, r,  param, hFile);
                return r;
            } catch (Exception e) {
                LogEvent(funcname, Win32Returncode.DEFAULT_UNKNOWN_ERROR,  param, hFile);
                LogException(funcname, e);
                return Win32Returncode.DEFAULT_UNKNOWN_ERROR;
            }
        }

        public Win32Returncode GetFileInformation(FuserFileHandler hFile, FuserFileInformation fileinfo ) {
            string funcname = "GetFileInformation";
            string param = "";

            try {
                //param = ;
                Win32Returncode r = sourceSystem.GetFileInformation(hFile, fileinfo);

                LogEvent(funcname, r,  param, hFile);
                return r;
            } catch (Exception e) {
                LogEvent(funcname, Win32Returncode.DEFAULT_UNKNOWN_ERROR,  param, hFile);
                LogException(funcname, e);
                return Win32Returncode.DEFAULT_UNKNOWN_ERROR;
            }
        }

        public Win32Returncode FindFiles(FuserFileHandler hFile, List<FuserFileInformation> files) {
            string funcname = "FindFiles";
            string param = "";

            try {
                //param = ;
                Win32Returncode r = sourceSystem.FindFiles(hFile, files);

                LogEvent(funcname, r,  param, hFile);
                return r;
            } catch (Exception e) {
                LogEvent(funcname, Win32Returncode.DEFAULT_UNKNOWN_ERROR,  param, hFile);
                LogException(funcname, e);
                return Win32Returncode.DEFAULT_UNKNOWN_ERROR;
            }
        }

        public Win32Returncode SetFileAttributes(FuserFileHandler hFile, FileAttributes attr) {
            string funcname = "SetFileAttributes";
            string param = "";

            try {
                //param = ;
                Win32Returncode r = sourceSystem.SetFileAttributes(hFile, attr );

                LogEvent(funcname, r,  param, hFile);
                return r;
            } catch (Exception e) {
                LogEvent(funcname, Win32Returncode.DEFAULT_UNKNOWN_ERROR,  param, hFile);
                LogException(funcname, e);
                return Win32Returncode.DEFAULT_UNKNOWN_ERROR;
            }
        }

        public Win32Returncode SetFileTime(FuserFileHandler hFile, DateTime CreationTime, DateTime LastAccessTime, DateTime LastWriteTime) {
            string funcname = "SetFileTime";
            string param = "";

            try {
                param = CreationTime + ";" + LastAccessTime + ";" + LastWriteTime;
                Win32Returncode r = sourceSystem.SetFileTime(hFile, CreationTime, LastAccessTime, LastWriteTime);

                LogEvent(funcname, r,  param, hFile);
                return r;
            } catch (Exception e) {
                LogEvent(funcname, Win32Returncode.DEFAULT_UNKNOWN_ERROR,  param, hFile);
                LogException(funcname, e);
                return Win32Returncode.DEFAULT_UNKNOWN_ERROR;
            }
        }

        public Win32Returncode DeleteFile( FuserFileHandler hFile) {
            string funcname = "DeleteFile";
            string param = "";

            try {
                //param = ;
                Win32Returncode r = sourceSystem.DeleteFile( hFile);

                LogEvent(funcname, r,  param, hFile);
                return r;
            } catch (Exception e) {
                LogEvent(funcname, Win32Returncode.DEFAULT_UNKNOWN_ERROR,  param, hFile);
                LogException(funcname, e);
                return Win32Returncode.DEFAULT_UNKNOWN_ERROR;
            }
        }

        public Win32Returncode DeleteDirectory( FuserFileHandler hFile) {
            string funcname = "DeleteDirectory";
            string param = "";

            try {
                //param = ;
                Win32Returncode r = sourceSystem.DeleteDirectory(hFile);

                LogEvent(funcname, r, param, hFile);
                return r;
            } catch (Exception e) {
                LogEvent(funcname, Win32Returncode.DEFAULT_UNKNOWN_ERROR, param, hFile);
                LogException(funcname, e);
                return Win32Returncode.DEFAULT_UNKNOWN_ERROR;
            }
        }

        public Win32Returncode MoveFile(FuserFileHandler hFile, string newFilename, bool replace) {
            string funcname = "MoveFile";
            string param = "";

            try {
                param = newFilename + ";" + replace;
                Win32Returncode r = sourceSystem.MoveFile(hFile, newFilename, replace);

                LogEvent(funcname, r,  param, hFile);
                return r;
            } catch (Exception e) {
                LogEvent(funcname, Win32Returncode.DEFAULT_UNKNOWN_ERROR,  param, hFile);
                LogException(funcname, e);
                return Win32Returncode.DEFAULT_UNKNOWN_ERROR;
            }
        }

        public Win32Returncode SetEndOfFile(FuserFileHandler hFile, long length) {
            string funcname = "SetEndOfFile";
            string param = "";

            try {
                param = length.ToString();
                Win32Returncode r = sourceSystem.SetEndOfFile(hFile, length);

                LogEvent(funcname, r,  param, hFile);
                return r;
            } catch (Exception e) {
                LogEvent(funcname, Win32Returncode.DEFAULT_UNKNOWN_ERROR, param, hFile);
                LogException(funcname, e);
                return Win32Returncode.DEFAULT_UNKNOWN_ERROR;
            }
        }

        public Win32Returncode SetAllocationSize(FuserFileHandler hFile, long length) {
            string funcname = "SetAllocationSize";
            string param = "";

            try {
                param = length.ToString();
                Win32Returncode r = sourceSystem.SetAllocationSize(hFile, length );

                LogEvent(funcname, r, param, hFile);
                return r;
            } catch (Exception e) {
                LogEvent(funcname, Win32Returncode.DEFAULT_UNKNOWN_ERROR, param, hFile);
                LogException(funcname, e);
                return Win32Returncode.DEFAULT_UNKNOWN_ERROR;
            }
        }

        public Win32Returncode LockFile(FuserFileHandler hFile, long offset, long length) {
            string funcname = "LockFile";
            string param = "";

            try {
                //param = ;
                Win32Returncode r = sourceSystem.LockFile(hFile, offset, length);

                LogEvent(funcname, r, param, hFile);
                return r;
            } catch (Exception e) {
                LogEvent(funcname, Win32Returncode.DEFAULT_UNKNOWN_ERROR,  param, hFile);
                LogException(funcname, e);
                return Win32Returncode.DEFAULT_UNKNOWN_ERROR;
            }
        }

        public Win32Returncode UnlockFile(FuserFileHandler hFile, long offset, long length) {
            string funcname = "UnlockFile";
            string param = "";

            try {
                //param = ;
                Win32Returncode r = sourceSystem.UnlockFile(hFile, offset, length);

                LogEvent(funcname, r,  param, hFile);
                return r;
            } catch (Exception e) {
                LogEvent(funcname, Win32Returncode.DEFAULT_UNKNOWN_ERROR,  param, hFile);
                LogException(funcname, e);
                return Win32Returncode.DEFAULT_UNKNOWN_ERROR;
            }
        }

        public Win32Returncode GetDiskFreeSpace(ref ulong freeBytesAvailable, ref ulong totalBytes, ref ulong totalFreeBytes) {
            string funcname = "GetDiskFreeSpace";
            string param = "";

            try {
                param = freeBytesAvailable + ";" + totalBytes + ";" + totalFreeBytes;
                Win32Returncode r = sourceSystem.GetDiskFreeSpace(ref freeBytesAvailable, ref  totalBytes, ref  totalFreeBytes);

                LogEvent(funcname, r,  param, null);
                return r;
            } catch (Exception e) {
                LogEvent(funcname, Win32Returncode.DEFAULT_UNKNOWN_ERROR,  param, null);
                LogException(funcname, e);
                return Win32Returncode.DEFAULT_UNKNOWN_ERROR;
            }
        }

        public Win32Returncode Mount(string MountPoint, string DeviceName) {
            string funcname = "Mount";
            string param = "MP: " + MountPoint + " DN: " + DeviceName;

            try {
                //param = ;
                Win32Returncode r = sourceSystem.Mount(MountPoint, DeviceName);

                LogEvent(funcname, r, param, null);
                return r;
            } catch (Exception e) {
                LogEvent(funcname, Win32Returncode.DEFAULT_UNKNOWN_ERROR, param, null);
                LogException(funcname, e);
                return Win32Returncode.DEFAULT_UNKNOWN_ERROR;
            }
        }





    }
}
