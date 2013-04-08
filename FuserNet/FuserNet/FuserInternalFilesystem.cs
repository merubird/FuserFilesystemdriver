using System;
using System.Collections.Generic;
using System.IO;
using FuserLowlevelDriver;

namespace FuserNet
{
    internal class FuserInternalFilesystem : IFuserFilesystemDevice
    {
        private IFuserFilesystemDirectory root; //Root-Directory on the Drive
        private IFuserlDrive drive; //Mounted Drive
        private bool DebugModeEnabled;

        public FuserInternalFilesystem(IFuserlDrive drive, bool DebugMode) {
            this.DebugModeEnabled = DebugMode;
            if (drive != null) {
                this.drive = drive;
                this.root = this.drive.Root;
            } else {
                this.drive = null;
                this.root = null;
            }

            if (this.DebugModeEnabled)
                LogDebug("(Constructor)", "Warning: debug mode active",false);            
        }

        /// <summary>
        /// Resolves a path(filename) and sets the filehandle.
        /// </summary>
        /// <param name="filename"></param>
        /// <param name="info"></param>
        /// <returns></returns>
        private PathResolver newResolvePath(string filename, FuserFileHandler hFile) {
            PathResolver pr;
            lock (this) {                
                pr = new PathResolver(this.root, filename);
                if (pr.PathInvalid) {
                    // path not found or invalid
                    hFile.fileHandle.CurrentFilesystemItem = null;
                    hFile.fileHandle.FolderContainer = null;
                    throw new FSException(pr.LastErrorCode);
                } else {
                    hFile.fileHandle.CurrentFilesystemItem = pr.fileitem;
                    hFile.fileHandle.FolderContainer = pr.path.currentDirectory;
                    if (filename == @"\")
                        hFile.fileHandle.FolderContainer = null;
                }
            }
            return pr;
        }

        /// <summary>
        /// Outputs a debug message and interrupts execution if DebugModeEnable is set.
        /// </summary>
        /// <param name="method"></param>
        /// <param name="message"></param>
        private void LogDebug(string method, string message, bool doBreak) {
            if (!this.DebugModeEnabled)
                return;

            Console.WriteLine("Method: " + method + "    " + message);
            if (doBreak)
                System.Diagnostics.Debugger.Break();
        }

        public void LogErrorMessage(string functionname, string message) {
            LogDebug(functionname, message, true);
        }


        public Win32Returncode CreateFile(FuserFileHandler hFile, FuserFileAccess access, FileShare share, FileMode mode, FileOptions options, FileAttributes attribut) {
            IFuserFilesystemItem vfi;
            FileAccess accessNativ = FileAccess.Read;
            PathResolver pr;
                        
            bool doCreateNewFile = false;
            bool doTruncateFile = false;
            bool setLastAccessWithTruncate = false; // If true, the date of the last access is set when the truncate function is executed.
            bool fileshareAllreadySet = false;
            string filename = hFile.filename;
                        

            try {                
                try { pr = newResolvePath(filename, hFile); } catch (FSException fe) { return fe.Error; } catch (Exception e) { e.ToString(); return Win32Returncode.DEFAULT_UNKNOWN_ERROR; }
                vfi = pr.fileitem;


                if (vfi != null) {
                    FileLockManager lockMgr = vfi.Filelock;
                    if (lockMgr != null) {
                        lock (lockMgr) {
                            if (lockMgr.DeleteOnClose) {
                                // File/Directory is to be deleted, no more new handles are to be created:
                                return Win32Returncode.ERROR_ACCESS_DENIED; // TODO: perhaps replace this code with another one
                            }
                        }
                    }
                }


                if (access == FuserFileAccess.None) {
                    if (mode != FileMode.Open) {
                        // access mode is different to Open
                        LogDebug("CreateFile", "[1] Access=none -> mode:" + mode,true);
                        return Win32Returncode.DEFAULT_UNKNOWN_ERROR;
                    }
                    
                    if (vfi != null) {
                        if (vfi is IFuserFilesystemDirectory) { hFile.IsDirectory = true; }
                        if (!GeneralFilesystemHelper.FilelockSet(hFile.fileHandle, vfi, access, share)) { return Win32Returncode.ERROR_SHARING_VIOLATION; }
                        return Win32Returncode.SUCCESS; // Allow only for information requests
                    }
                    return Win32Returncode.ERROR_FILE_NOT_FOUND;
                }
                
                if (vfi != null) {
                    if (vfi is IFuserFilesystemDirectory) {
                        hFile.IsDirectory = true;
                        if (mode == FileMode.Open) {
                            // in case of directory, to change attributes, access:write is passed
                            if (!GeneralFilesystemHelper.FilelockSet(hFile.fileHandle, vfi, access, share)) { return Win32Returncode.ERROR_SHARING_VIOLATION; }
                            return Win32Returncode.SUCCESS; // Allow only for DirectoryListings, unfortunately no distinction can be made whether access to a file or directory was desired.
                        } else {
                            return Win32Returncode.ERROR_ACCESS_DENIED; //  <- Error for access to directory
                        }
                    }
                }

                if (hFile.fileHandle.Data != null) {
                    // handle already has an open file:
                    return Win32Returncode.ERROR_ACCESS_DENIED;
                }


                if (access == FuserFileAccess.Read) { accessNativ = FileAccess.Read; } else if (access == FuserFileAccess.Write) { accessNativ = FileAccess.Write; } else if (access == FuserFileAccess.ReadWrite) { accessNativ = FileAccess.ReadWrite; }


                switch (mode) {
                    case FileMode.Open:
                        //
                        // Summary:
                        //     Specifies that the operating system should open an existing file. The ability
                        //     to open the file is dependent on the value specified by the System.IO.FileAccess
                        //     enumeration. A System.IO.FileNotFoundException exception is thrown if the file
                        //     does not exist.
                        // Standard mode, do not take any special actions
                        break;

                    case FileMode.Append:
                        //
                        // Summary:
                        //     Opens the file if it exists and seeks to the end of the file, or creates a new
                        //     file. This requires System.Security.Permissions.FileIOPermissionAccess.Append
                        //     permission. FileMode.Append can be used only in conjunction with FileAccess.Write.
                        //     Trying to seek to a position before the end of the file throws an System.IO.IOException
                        //     exception, and any attempt to read fails and throws a System.NotSupportedException
                        //     exception.
                        // Append can never be called: is implemented by the operating system
                        if (access != FuserFileAccess.Write) {
                            // Access not allowed for this mode:
                            return Win32Returncode.ERROR_ACCESS_DENIED;
                        }
                        if (vfi == null) {
                            doCreateNewFile = true; // create new file, as no file exists yet
                        }
                        break;

                    case FileMode.Create:
                        //
                        // Summary:
                        //     Specifies that the operating system should create a new file. If the file already
                        //     exists, it will be overwritten. This requires System.Security.Permissions.FileIOPermissionAccess.Write
                        //     permission. FileMode.Create is equivalent to requesting that if the file does
                        //     not exist, use System.IO.FileMode.CreateNew; otherwise, use System.IO.FileMode.Truncate.
                        //     If the file already exists but is a hidden file, an System.UnauthorizedAccessException
                        //     exception is thrown.
                        if (!(access == FuserFileAccess.ReadWrite || access == FuserFileAccess.Write)) {
                            // Access not allowed for this mode:
                            return Win32Returncode.ERROR_ACCESS_DENIED;
                        }
                        if (vfi == null) {
                            doCreateNewFile = true; // create new file, as no file exists yet
                        }
                        doTruncateFile = true; // Then treat as truncate mode
                        setLastAccessWithTruncate=true; // this mode resets the last access date
                        break;

                    case FileMode.CreateNew:
                        // Summary:
                        //     Specifies that the operating system should create a new file. This requires System.Security.Permissions.FileIOPermissionAccess.Write
                        //     permission. If the file already exists, an System.IO.IOException exception is
                        //     thrown.
                        if (!(access == FuserFileAccess.ReadWrite || access == FuserFileAccess.Write)) {
                            // Access not allowed for this mode:
                            return Win32Returncode.ERROR_ACCESS_DENIED;
                        }

                        doCreateNewFile = true; // create new file
                        break;

                    case FileMode.OpenOrCreate:
                        //
                        // Summary:
                        //     Specifies that the operating system should open a file if it exists; otherwise,
                        //     a new file should be created. If the file is opened with FileAccess.Read, System.Security.Permissions.FileIOPermissionAccess.Read
                        //     permission is required. If the file access is FileAccess.Write, System.Security.Permissions.FileIOPermissionAccess.Write
                        //     permission is required. If the file is opened with FileAccess.ReadWrite, both
                        //     System.Security.Permissions.FileIOPermissionAccess.Read and System.Security.Permissions.FileIOPermissionAccess.Write
                        //     permissions are required.
                        if (vfi == null) {
                            doCreateNewFile = true; // create new file, as no file exists yet
                        }
                        break;

                    case FileMode.Truncate:
                        // Summary:
                        //     Specifies that the operating system should open an existing file. When the file
                        //     is opened, it should be truncated so that its size is zero bytes. This requires
                        //     System.Security.Permissions.FileIOPermissionAccess.Write permission. Attempts
                        //     to read from a file opened with FileMode.Truncate cause an System.ArgumentException
                        //     exception.
                        // Never executed: Operating system opens file for writing and then sets Allocation-Size:0

                        if (!(access == FuserFileAccess.ReadWrite || access == FuserFileAccess.Write)) {
                            // Access not allowed for this mode:
                            return Win32Returncode.ERROR_ACCESS_DENIED;
                        }
                        doTruncateFile = true;
                        break;

                    default:
                        LogDebug("CreateFile", "[3] Mode not supported: " + mode, true);
                        return Win32Returncode.DEFAULT_UNKNOWN_ERROR;
                }
            } catch {
                return Win32Returncode.DEFAULT_UNKNOWN_ERROR;
            }


            try {
                if (doCreateNewFile) {// Creates a new file in the selected folder, file must not already exist:
                    if (vfi != null) {
                        return Win32Returncode.ERROR_ALREADY_EXISTS;
                    }
                    if (pr.path == null) {
                        return Win32Returncode.ERROR_PATH_NOT_FOUND;
                    } else {
                        if (pr.path.currentDirectory == null) {
                            return Win32Returncode.ERROR_PATH_NOT_FOUND;
                        } else {
                            string newfilename = pr.path.itemname;

                            if (newfilename == "") {
                                return Win32Returncode.ERROR_INVALID_NAME;
                            }
                            if (!PathResolver.PathValidateCheckItemname(newfilename, false)) {
                                return Win32Returncode.ERROR_INVALID_NAME;
                            }

                            attribut |= FileAttributes.Archive;
                            lock (pr.path.currentDirectory) {
                                if (GeneralFilesystemHelper.CheckIsDirectoryDeletedOnClose(pr.path.currentDirectory)) {
                                    return Win32Returncode.ERROR_ACCESS_DENIED; // TODO: maybe replace error message
                                }
                                pr.path.currentDirectory.CreateFile(newfilename, attribut);
                            }
                            GeneralFilesystemHelper.SetLastWrite(pr.path.currentDirectory, true);
                            
                            // again resolve path:
                            try { pr = newResolvePath(filename, hFile); } catch (FSException fe) { return fe.Error; } catch (Exception e) { e.ToString(); return Win32Returncode.DEFAULT_UNKNOWN_ERROR; }
                            vfi = pr.fileitem;
                            if (vfi == null) {
                                return Win32Returncode.ERROR_FILE_NOT_FOUND;
                            }
                            if (vfi is IFuserFilesystemDirectory) {
                                return Win32Returncode.ERROR_ACCESS_DENIED;
                            }
                        }
                    }
                } // New file now exists with certainty


                IFuserFilesystemFile vFile = null;
                if (vfi == null) {
                    // file not found:
                    return Win32Returncode.ERROR_FILE_NOT_FOUND;
                }
                if (vfi is IFuserFilesystemFile) {
                    vFile = (IFuserFilesystemFile)vfi;
                } else {
                    // access not for file:
                    LogDebug("CreateFile", "[2] Open not for type file!", true);
                    return Win32Returncode.ERROR_ACCESS_DENIED;
                }

                // check and block access:
                if (!GeneralFilesystemHelper.FilelockSet(hFile.fileHandle, vfi, access, share)) { return Win32Returncode.ERROR_SHARING_VIOLATION; }
                fileshareAllreadySet = true; // Fileshare is not set
                if (share == FileShare.None) {
                    hFile.fileHandle.OPLockActive = false;// access is not shared, OPLocks are not possible, therefore also do not check
                }
                
                // open the file:
                lock (vFile) {
                    if (vFile.isReadOnly && (access == FuserFileAccess.ReadWrite || access == FuserFileAccess.Write)) {
                        hFile.fileHandle.Data = null; // file is read-only, prevent opening.
                    } else {
                        hFile.fileHandle.Data = vFile.FileOpen(accessNativ, share);
                    }                    
                }

                if (hFile.fileHandle.Data == null) {
                    // handle was not opened
                    GeneralFilesystemHelper.FilelockRelease(hFile.fileHandle, vfi);
                    return Win32Returncode.ERROR_ACCESS_DENIED;
                }
                if (doTruncateFile) {
                    // reset the contents of the file:
                    lock (hFile.fileHandle.Data) {
                        hFile.fileHandle.Data.SetLength(0);
                    }
                    GeneralFilesystemHelper.SetLastWrite(vfi, setLastAccessWithTruncate);
                    GeneralFilesystemHelper.WriteLastWriteAndAccess(vfi);
                }

                // set access mode correctly:
                if (access == FuserFileAccess.Read || access == FuserFileAccess.ReadWrite) hFile.fileHandle.IsOpenRead = true;
                if (access == FuserFileAccess.Write || access == FuserFileAccess.ReadWrite) hFile.fileHandle.IsOpenWrite = true;

            } catch (FSException fe) {
                if (fileshareAllreadySet) { GeneralFilesystemHelper.FilelockRelease(hFile.fileHandle, vfi); }
                return fe.Error;
            } catch {
                if (fileshareAllreadySet) { GeneralFilesystemHelper.FilelockRelease(hFile.fileHandle, vfi); }
                return Win32Returncode.ERROR_ACCESS_DENIED;
            }

            return Win32Returncode.SUCCESS;
        }


        public Win32Returncode OpenDirectory(FuserFileHandler hFile) {            
            try {
                PathResolver pr; try { pr = newResolvePath(hFile.filename, hFile); } catch (FSException fe) { return fe.Error; } catch (Exception e) { e.ToString(); return Win32Returncode.DEFAULT_UNKNOWN_ERROR; }

                IFuserFilesystemItem vfi = pr.fileitem;
                if (vfi != null) {
                    if (vfi is IFuserFilesystemDirectory) {
                        if (!GeneralFilesystemHelper.FilelockSet(hFile.fileHandle, vfi, FuserFileAccess.None, FileShare.None)) { return Win32Returncode.ERROR_SHARING_VIOLATION; }// Unfortunately, the parameters for FileAccess and FileShare are missing.
                        return Win32Returncode.SUCCESS;// directory valid and open
                    }
                }
            } catch (FSException fe) {
                return fe.Error;
            } catch {
                return Win32Returncode.ERROR_ACCESS_DENIED;
            }

            return Win32Returncode.ERROR_PATH_NOT_FOUND;
        }


        public Win32Returncode Cleanup(FuserFileHandler hFile) {
            // close file share accesses here, not in close-method
            //   Note: when user uses memory mapped file, WriteFile
            //   or ReadFile function may be invoked after Cleanup in order to complete
            //   the I/O operations.            
            try {
                if (hFile.fileHandle == null) {
                    LogDebug("Cleanup", "File never opened=NoContext exists: " + hFile.filename , true);
                    return Win32Returncode.SUCCESS; // regardless return 0
                }
                IFuserFilesystemItem vfi = hFile.fileHandle.CurrentFilesystemItem;

                if (vfi == null) {
                    LogDebug("Cleanup", "File not exists " + hFile.filename , false);
                } else {
                    GeneralFilesystemHelper.FilelockRelease(hFile.fileHandle, vfi); // release Filelock
                }


                return Win32Returncode.SUCCESS;
            } catch (FSException fe) {
                return fe.Error;
            } catch {
                return Win32Returncode.ERROR_ACCESS_DENIED;
            }
        }


        public Win32Returncode CloseFile(FuserFileHandler hFile) {            
            try {
                if (hFile.fileHandle == null) {
                    LogDebug("CloseFile", "File never opened=NoContext exists: " + hFile.filename , true);
                    return Win32Returncode.SUCCESS;// should never occur
                }

                IFuserFilesystemItem vfi = hFile.fileHandle.CurrentFilesystemItem;
                if (vfi != null)
                    GeneralFilesystemHelper.WriteLastWriteAndAccess(vfi);

                if (hFile.fileHandle.Data != null) {
                    // was opened for Read/Write:
                    IFuserFilesystemFile vFile = null;

                    if (vfi != null) {
                        if (vfi is IFuserFilesystemFile)
                            vFile = (IFuserFilesystemFile)vfi;
                    }


                    if (vFile != null) {
                        lock (vFile) {
                            lock (hFile.fileHandle.Data) {
                                vFile.FileClose(hFile.fileHandle.Data);
                            }
                        }
                    }

                    hFile.fileHandle.Data = null;
                    hFile.fileHandle.IsOpenRead = false;
                    hFile.fileHandle.IsOpenWrite = false;
                }

                if (vfi == null)
                    return Win32Returncode.SUCCESS;// no file available


                // TODO: check how the driver implements delete, notify operating system
                // check whether file/directory should be deleted on closing:
                if (hFile.DeleteOnClose) {
                    FileLockManager lockMgr = vfi.Filelock;
                    if (lockMgr != null) {
                        lock (lockMgr) {
                            lockMgr.DeleteOnClose = true;
                        }
                    }
                }

                // physical deletion is now started:
                if (vfi != null && hFile.fileHandle.FolderContainer != null) {
                    IFuserFilesystemDirectory folder = hFile.fileHandle.FolderContainer;
                    bool doDelete = false;

                    FileLockManager lockMgr = vfi.Filelock;
                    if (lockMgr != null) {
                        lock (lockMgr) {
                            if (lockMgr.Count == 0) {
                                // delete only when no more FileLocks are open:
                                doDelete = lockMgr.DeleteOnClose; // true = file should be deleted when closing.
                            }
                        }
                    }

                    if (doDelete) {
                        if (vfi is IFuserFilesystemDirectory) {
                            IFuserFilesystemDirectory vDir = (IFuserFilesystemDirectory)vfi;

                            lock (vDir) {
                                if (vDir.GetContentList().Length != 0) { doDelete = false; } // directory is not empty and cannot be deleted.
                            }
                        }

                        if (doDelete) {
                            try {
                                lock (folder) {
                                    folder.Delete(vfi);
                                }
                                GeneralFilesystemHelper.SetLastWrite(folder, true);
                            } catch {
                                // if errors occur during deletion, no action is taken. The return value must always be 0 for CloseFile.
                            }
                        }
                    }
                }
            } catch {
                // no operation, if errors occur no action is taken. The return value must always be 0 for CloseFile.
            }
            return Win32Returncode.SUCCESS;
        }


        public Win32Returncode ReadFile(FuserFileHandler hFile, byte[] buffer, ref uint readBytes, long offset) {
            try {
                if (hFile.fileHandle == null) {
                    LogDebug("ReadFile", "File never opened=NoContext exists: " + hFile.filename , true);
                    return Win32Returncode.ERROR_ACCESS_DENIED;
                }

                IFuserFilesystemItem vfi = hFile.fileHandle.CurrentFilesystemItem;
                Stream s = hFile.fileHandle.Data;
                if (vfi == null || s == null)
                    return Win32Returncode.ERROR_FILE_NOT_FOUND;

                if (!hFile.fileHandle.IsOpenRead) {
                    return Win32Returncode.ERROR_ACCESS_DENIED;
                }

                if (!GeneralFilesystemHelper.FileOPLockCheck(hFile.fileHandle, vfi, offset, buffer.Length)) { return Win32Returncode.ERROR_LOCK_VIOLATION; }
                lock (s) {
                    if (offset >= s.Length) {
                        readBytes = 0; // reading range exceeded:
                    } else {
                        if (offset != s.Position) { s.Position = offset; }
                        int r = s.Read(buffer, 0, buffer.Length);
                        if (r <= 0) {
                            readBytes = 0;
                        } else {
                            readBytes = (uint)r;
                        }
                    }
                }

                if (readBytes != 0)
                    GeneralFilesystemHelper.SetLastAccess(vfi);

            } catch (FSException fe) {
                return fe.Error;
            } catch {
                return Win32Returncode.ERROR_ACCESS_DENIED;
            }
            return Win32Returncode.SUCCESS;
        }


        public Win32Returncode WriteFile(FuserFileHandler hFile, byte[] buffer, ref uint writtenBytes, long offset) {
            try {
                if (hFile.fileHandle == null) {
                    LogDebug("WriteFile", "File never opened=NoContext exists: " + hFile.filename , true);
                    return Win32Returncode.ERROR_ACCESS_DENIED;
                }

                IFuserFilesystemItem vfi = hFile.fileHandle.CurrentFilesystemItem;
                Stream s = hFile.fileHandle.Data;
                if (vfi == null || s == null)
                    return Win32Returncode.ERROR_FILE_NOT_FOUND;

                if (!hFile.fileHandle.IsOpenWrite) {
                    return Win32Returncode.ERROR_ACCESS_DENIED;
                }


                if (!GeneralFilesystemHelper.FileOPLockCheck(hFile.fileHandle, vfi, offset, buffer.Length)) { return Win32Returncode.ERROR_LOCK_VIOLATION; }
                lock (s) {
                    if (offset != s.Position) { s.Position = offset; }

                    s.Write(buffer, 0, buffer.Length);
                    writtenBytes = (uint)buffer.Length;
                }
                GeneralFilesystemHelper.SetLastWrite(vfi, true);
            } catch (FSException fe) {
                return fe.Error;
            } catch {
                return Win32Returncode.ERROR_ACCESS_DENIED;
            }
            return Win32Returncode.SUCCESS;
        }


        public Win32Returncode FlushFileBuffers(FuserFileHandler hFile) {
            try {
                if (hFile.fileHandle == null) {
                    LogDebug("FlushFileBuffers", "File never opened=NoContext exists: " + hFile.filename , true);
                    return Win32Returncode.ERROR_ACCESS_DENIED;
                }

                IFuserFilesystemItem vfi = hFile.fileHandle.CurrentFilesystemItem;
                Stream s = hFile.fileHandle.Data;
                if (vfi == null || s == null)
                    return Win32Returncode.ERROR_FILE_NOT_FOUND;

                if (!hFile.fileHandle.IsOpenWrite) {
                    return Win32Returncode.ERROR_ACCESS_DENIED;
                }

                lock (s) {
                    s.Flush();
                }

                GeneralFilesystemHelper.WriteLastWriteAndAccess(vfi);
            } catch (FSException fe) {
                return fe.Error;
            } catch {
                return Win32Returncode.ERROR_ACCESS_DENIED;
            }
            return Win32Returncode.SUCCESS;
        }


        public Win32Returncode GetFileInformation(FuserFileHandler hFile, FuserFileInformation fileinfo) {
            try {
                if (hFile.fileHandle == null) {
                    LogDebug("GetFileInformation", "File never opened=NoContext exists: " + hFile.filename , true);
                    return Win32Returncode.ERROR_ACCESS_DENIED;
                }

                IFuserFilesystemItem vfi = hFile.fileHandle.CurrentFilesystemItem;
                if (vfi == null)
                    return Win32Returncode.ERROR_FILE_NOT_FOUND;
            
            
                GeneralFilesystemHelper.convertFileInformation(vfi, fileinfo); // lock is executed in this method.
                if (vfi == this.root) {
                    fileinfo.Attributes = FileAttributes.Directory | FileAttributes.Hidden | FileAttributes.System;
                    fileinfo.Length = 0;
                    fileinfo.Filename = "";
                }
            } catch (FSException fe) {
                return fe.Error;
            } catch {
                return Win32Returncode.ERROR_ACCESS_DENIED;
            }
            return Win32Returncode.SUCCESS;
        }


        public Win32Returncode FindFiles(FuserFileHandler hFile, List<FuserFileInformation> files ) {                        
            try {
                if (hFile.fileHandle == null) {
                    LogDebug("FindFiles", "File never opened=NoContext exists: " + hFile.filename , true);
                    return Win32Returncode.ERROR_ACCESS_DENIED;
                }
                IFuserFilesystemItem vfi = hFile.fileHandle.CurrentFilesystemItem;
                if (vfi == null)
                    return Win32Returncode.ERROR_FILE_NOT_FOUND;

                if (vfi is IFuserFilesystemDirectory) {
                    IFuserFilesystemDirectory vDir = (IFuserFilesystemDirectory)vfi;

                    if (vDir != this.root) {
                        FuserFileInformation tmpFI;
                        tmpFI = GeneralFilesystemHelper.convertFileInformation(vDir); tmpFI.Attributes = FileAttributes.Directory; tmpFI.Filename = ".";
                        files.Add(tmpFI);
                        tmpFI = GeneralFilesystemHelper.convertFileInformation(vDir); tmpFI.Attributes = FileAttributes.Directory; tmpFI.Filename = "..";
                        files.Add(tmpFI);
                    }

                    IFuserFilesystemItem[] dirContent;
                    lock (vDir){
                        dirContent = vDir.GetContentList();
                    }
                    if (dirContent != null){
                        foreach (IFuserFilesystemItem fi in dirContent) 
                            files.Add(GeneralFilesystemHelper.convertFileInformation(fi)); // lock is executed in this method.                       
                    }
                    GeneralFilesystemHelper.SetLastAccess(vfi);
                } else {
                    return Win32Returncode.ERROR_PATH_NOT_FOUND; // path is not a directory
                }
            } catch (FSException fe) {
                return fe.Error;
            } catch {
                return Win32Returncode.ERROR_ACCESS_DENIED;
            }
            return Win32Returncode.SUCCESS;
        }


        public Win32Returncode SetFileAttributes(FuserFileHandler hFile, FileAttributes attr) {
            try {
                if (hFile.fileHandle == null) {
                    LogDebug("SetFileAttributes", "File never opened=NoContext exists: " + hFile.filename , true);
                    return Win32Returncode.ERROR_ACCESS_DENIED;
                }

                IFuserFilesystemItem vfi = hFile.fileHandle.CurrentFilesystemItem;
                if (vfi == null)
                    return Win32Returncode.ERROR_FILE_NOT_FOUND;
                                       
                bool normal = attr.HasFlag(FileAttributes.Normal);
                bool a = attr.HasFlag(FileAttributes.Archive);
                bool h = attr.HasFlag(FileAttributes.Hidden);
                bool r = attr.HasFlag(FileAttributes.ReadOnly);
                bool s = attr.HasFlag(FileAttributes.System);

                if (!normal) {
                    return Win32Returncode.SUCCESS; // do not pass on any attributes, so no change anything
                }

                lock (vfi) { 
                    if (vfi.isArchive != a)
                        vfi.isArchive = a;

                    if (vfi.isHidden != h)
                        vfi.isHidden = h;

                    if (vfi.isReadOnly != r)
                        vfi.isReadOnly = r;

                    if (vfi.isSystem != s)
                        vfi.isSystem = s;
                }

            } catch (FSException fe) {
                return fe.Error;
            } catch {
                return Win32Returncode.ERROR_ACCESS_DENIED;
            }
            return Win32Returncode.SUCCESS;
        }


        public Win32Returncode SetFileTime(FuserFileHandler hFile, DateTime CreationTime, DateTime LastAccessTime, DateTime LastWriteTime) {
            try {
                if (hFile.fileHandle == null) {
                    LogDebug("SetFileTime", "File never opened=NoContext exists: " + hFile.filename , true);
                    return Win32Returncode.ERROR_ACCESS_DENIED;
                }
                IFuserFilesystemItem vfi = hFile.fileHandle.CurrentFilesystemItem;
                if (vfi == null)
                    return Win32Returncode.ERROR_FILE_NOT_FOUND;

                if (CreationTime == DateTime.MinValue && LastAccessTime == DateTime.MinValue && LastWriteTime == DateTime.MinValue)
                    return Win32Returncode.SUCCESS;// no change to be made.

                lock (vfi) {
                    if (CreationTime != DateTime.MinValue) {
                        vfi.CreationTime = CreationTime;
                    }

                    if (LastAccessTime != DateTime.MinValue) {
                        vfi.LastAccessTime = LastAccessTime;
                    }

                    if (LastWriteTime != DateTime.MinValue) {
                        vfi.LastWriteTime = LastWriteTime;
                    }
                }
            } catch (FSException fe) {
                return fe.Error;
            } catch {
                return Win32Returncode.ERROR_ACCESS_DENIED;
            }
            return Win32Returncode.SUCCESS;
        }


        public Win32Returncode DeleteFile(FuserFileHandler hFile) {
            // do not delete now, the operating system sets the flag for deletion. it is only checked whether deletion is allowed.
            
            try {
                if (hFile.fileHandle == null) {
                    LogDebug("DeleteFile", "File never opened=NoContext exists: " + hFile.filename , true);
                    return Win32Returncode.ERROR_ACCESS_DENIED;
                }
                IFuserFilesystemItem vfi = hFile.fileHandle.CurrentFilesystemItem;
                if (vfi == null)
                    return Win32Returncode.ERROR_FILE_NOT_FOUND;

                if (hFile.fileHandle.FolderContainer == null)
                    return Win32Returncode.ERROR_ACCESS_DENIED;

                if (vfi is IFuserFilesystemFile) {
                    IFuserFilesystemFile vFile = (IFuserFilesystemFile)vfi;
                    
                    lock (vFile) {
                        if (vFile.isReadOnly) {
                            return Win32Returncode.ERROR_ACCESS_DENIED;
                        }
                    }

                    if (!GeneralFilesystemHelper.DeletionAllow(hFile.fileHandle, vfi)) {
                        return Win32Returncode.ERROR_SHARING_VIOLATION;
                    }

                    lock (hFile.fileHandle.FolderContainer) {
                        if (!hFile.fileHandle.FolderContainer.DeletionAllow(vFile)) {
                            return Win32Returncode.ERROR_ACCESS_DENIED;
                        }
                    }

                } else {
                    return Win32Returncode.ERROR_ACCESS_DENIED;
                }
            } catch (FSException fe) {
                return fe.Error;
            } catch {
                return Win32Returncode.ERROR_ACCESS_DENIED;
            }
            return Win32Returncode.SUCCESS;
        }


        public Win32Returncode CreateDirectory(FuserFileHandler hFile) {
            // A parameter with file attributes would have to be transferred.
            
            try {
                string filename = hFile.filename;
                PathResolver pr; try { pr = newResolvePath(filename, hFile); } catch (FSException fe) { return fe.Error; } catch (Exception e) { e.ToString(); return Win32Returncode.DEFAULT_UNKNOWN_ERROR; }

                if (pr.fileitem != null) {
                    return Win32Returncode.ERROR_ALREADY_EXISTS;
                }

                string newdirname = pr.path.itemname;

                if (newdirname == "") {
                    return Win32Returncode.ERROR_INVALID_NAME;
                }
                if (!PathResolver.PathValidateCheckItemname(newdirname, false)) {
                    return Win32Returncode.ERROR_INVALID_NAME;
                }

                if (GeneralFilesystemHelper.CheckIsDirectoryDeletedOnClose(pr.path.currentDirectory)) {
                    return Win32Returncode.ERROR_ACCESS_DENIED; // TODO: Possibly take another error message
                }
                lock (pr.path.currentDirectory) {
                    pr.path.currentDirectory.CreateDirectory(newdirname);
                }
                GeneralFilesystemHelper.SetLastWrite(pr.path.currentDirectory, true);

                // again resolve path:
                try { pr = newResolvePath(filename, hFile); } catch (FSException fe) { return fe.Error; } catch (Exception e) { e.ToString(); return Win32Returncode.DEFAULT_UNKNOWN_ERROR; }
                if (!hFile.IsDirectory) {
                    LogDebug("CreateDirectory", "DirectoryFlag not set after creation: " + filename, true);
                }
                hFile.IsDirectory = true;
                if (!GeneralFilesystemHelper.FilelockSet(hFile.fileHandle, pr.fileitem, FuserFileAccess.None, FileShare.None)) { return Win32Returncode.ERROR_SHARING_VIOLATION; }

            } catch (FSException fe) {
                return fe.Error;
            } catch (NotImplementedException) {
                return Win32Returncode.ERROR_NOT_READY;
            } catch {
                return Win32Returncode.ERROR_ACCESS_DENIED;
            }
            return Win32Returncode.SUCCESS;
        }


        public Win32Returncode DeleteDirectory(FuserFileHandler hFile) {
            // do not delete now, the operating system sets the flag for deletion. it is only checked whether deletion is allowed.

            try {
                if (hFile.fileHandle == null) {
                    LogDebug("DeleteDirectory", "File never opened=NoContext exists: " + hFile.filename , true);
                    return Win32Returncode.ERROR_ACCESS_DENIED;
                }

                IFuserFilesystemItem vfi = hFile.fileHandle.CurrentFilesystemItem;
                if (vfi == null)
                    return Win32Returncode.ERROR_FILE_NOT_FOUND;

                if (hFile.fileHandle.FolderContainer == null)
                    return Win32Returncode.ERROR_ACCESS_DENIED;

                if (vfi is IFuserFilesystemDirectory) {
                    IFuserFilesystemDirectory vDir = (IFuserFilesystemDirectory)vfi;

                    lock (vDir) {
                        if (vDir.GetContentList().Length != 0) {
                            return Win32Returncode.ERROR_DIR_NOT_EMPTY;
                        }

                        if (vDir.isReadOnly) {
                            return Win32Returncode.ERROR_ACCESS_DENIED;
                        }
                    }

                    if (!GeneralFilesystemHelper.DeletionAllow(hFile.fileHandle, vDir)) {
                        return Win32Returncode.ERROR_SHARING_VIOLATION;
                    }
                    lock (hFile.fileHandle.FolderContainer) {
                        if (!hFile.fileHandle.FolderContainer.DeletionAllow(vDir)) {
                            return Win32Returncode.ERROR_ACCESS_DENIED;
                        }
                    }

                } else {
                    return Win32Returncode.ERROR_ACCESS_DENIED;
                }
            } catch (FSException fe) {
                return fe.Error;
            } catch {
                return Win32Returncode.ERROR_ACCESS_DENIED;
            }
            return Win32Returncode.SUCCESS;
        }


        public Win32Returncode MoveFile(FuserFileHandler hFile, string newFilename, bool replace) {            
            try {
                string filename = hFile.filename;

                if (hFile.fileHandle == null) {
                    LogDebug("MoveFile", "File never opened=NoContext exists: " + filename, true);
                    return Win32Returncode.ERROR_ACCESS_DENIED;
                }
                IFuserFilesystemItem vfi = hFile.fileHandle.CurrentFilesystemItem;
                if (vfi == null)
                    return Win32Returncode.ERROR_FILE_NOT_FOUND;

                if (hFile.fileHandle.FolderContainer == null)
                    return Win32Returncode.ERROR_ACCESS_DENIED;

                // resolve path for destination:
                PathResolver prNew = null;
                try {
                    prNew = new PathResolver(this.root, newFilename); if (prNew.PathInvalid) { return Win32Returncode.ERROR_PATH_NOT_FOUND; }
                } catch {
                    return Win32Returncode.ERROR_PATH_NOT_FOUND;
                }

                if (prNew == null) { return Win32Returncode.ERROR_PATH_NOT_FOUND; }
                if (prNew.path == null) { return Win32Returncode.ERROR_PATH_NOT_FOUND; }
                if (prNew.path.currentDirectory == null) { return Win32Returncode.ERROR_PATH_NOT_FOUND; }
                if (prNew.path.itemname == "") { return Win32Returncode.ERROR_PATH_NOT_FOUND; }
                if (!PathResolver.PathValidateCheckItemname(prNew.path.itemname, false)) { return Win32Returncode.ERROR_INVALID_NAME; }

                // search for new item:
                IFuserFilesystemItem vfiNew = null;
                try {
                    lock (prNew.path.currentDirectory) {
                        vfiNew = prNew.path.currentDirectory.GetItem(prNew.path.itemname);
                    }
                } catch {
                    vfiNew = null;
                }


                if (vfiNew != null) {
                    if (vfiNew == vfi) {
                        return Win32Returncode.SUCCESS;// no operation so return OK (destination=source)
                    }
                    if (replace) {
                        if (vfiNew is IFuserFilesystemDirectory) {// directories cannot be overwritten
                            return Win32Returncode.ERROR_ACCESS_DENIED;
                        }
                    } else {
                        // item already exists:
                        return Win32Returncode.ERROR_ALREADY_EXISTS;
                    }
                }

                if (!GeneralFilesystemHelper.DeletionAllow(hFile.fileHandle, vfi)) {
                    // this right is also needed to rename and move the files.
                    return Win32Returncode.ERROR_SHARING_VIOLATION;
                }

                IFuserFilesystemDirectory dest = prNew.path.currentDirectory;
                if (dest == hFile.fileHandle.FolderContainer) {
                    dest = null; // source and destination directory is the same
                }

                if (vfiNew != null) {
                    if (replace) {
                        if (vfiNew is IFuserFilesystemFile) {
                            lock (prNew.path.currentDirectory) {
                                prNew.path.currentDirectory.Delete(vfiNew);
                            }
                        } else {
                            return Win32Returncode.ERROR_ACCESS_DENIED;
                        }
                    } else {
                        return Win32Returncode.ERROR_ACCESS_DENIED;
                    }
                }

                lock (hFile.fileHandle.FolderContainer) {
                    hFile.fileHandle.FolderContainer.MoveTo(vfi, dest, prNew.path.itemname);
                }

                if (vfi is IFuserFilesystemFile) {
                    // always set the archive attribute for files:
                    lock (vfi) {
                        vfi.isArchive = true;
                    }
                }
                GeneralFilesystemHelper.SetLastWrite(hFile.fileHandle.FolderContainer, true);
            } catch (FSException fe) {
                return fe.Error;
            } catch {
                return Win32Returncode.ERROR_ACCESS_DENIED;
            }
            return Win32Returncode.SUCCESS;
        }


        public Win32Returncode SetEndOfFile(FuserFileHandler hFile, long length) {
            try {
                if (hFile.fileHandle == null) {
                    LogDebug("SetEndOfFile", "File never opened=NoContext exists: " + hFile.filename , true);
                    return Win32Returncode.ERROR_ACCESS_DENIED;
                }

                IFuserFilesystemItem vfi = hFile.fileHandle.CurrentFilesystemItem;
                Stream s = hFile.fileHandle.Data;
                if (vfi == null || s == null)
                    return Win32Returncode.ERROR_FILE_NOT_FOUND;

                if (!hFile.fileHandle.IsOpenWrite || length < 0)
                    return Win32Returncode.ERROR_ACCESS_DENIED;
                            
                lock (s) {
                    s.SetLength(length);
                }

                GeneralFilesystemHelper.SetLastWrite(vfi, false);
                GeneralFilesystemHelper.WriteLastWriteAndAccess(vfi);
            } catch (FSException fe) {
                return fe.Error;
            } catch {
                return Win32Returncode.ERROR_ACCESS_DENIED;
            }
            return Win32Returncode.SUCCESS;
        }


        public Win32Returncode SetAllocationSize(FuserFileHandler hFile,   long length ) {
            return SetEndOfFile(hFile, length);// works exactly the same and must be able to adjust the size in both directions. TODO: check whether this is really the case, look at dokanSSH
        }


        public Win32Returncode LockFile(FuserFileHandler hFile, long offset, long length) {
            try {
                if (hFile.fileHandle == null) {
                    LogDebug("LockFile", "File never opened=NoContext exists: " + hFile.filename , true);
                    return Win32Returncode.ERROR_ACCESS_DENIED;
                }

                IFuserFilesystemItem vfi = hFile.fileHandle.CurrentFilesystemItem;
                if (vfi == null)
                    return Win32Returncode.ERROR_FILE_NOT_FOUND;

                if (!(vfi is IFuserFilesystemFile))
                    return Win32Returncode.ERROR_ACCESS_DENIED;

                if (!GeneralFilesystemHelper.FilelockSet(hFile.fileHandle, vfi, new FileLockItem(offset, length))) {
                    return Win32Returncode.ERROR_OPLOCK_NOT_GRANTED; // access violation occurred
                }
            } catch (FSException fe) {
                return fe.Error;
            } catch {
                return Win32Returncode.ERROR_ACCESS_DENIED;
            }
            return Win32Returncode.SUCCESS;
        }


        public Win32Returncode UnlockFile(FuserFileHandler hFile,  long offset, long length) {
            // to unlock, the offset and length must be exactly the same as when locking.
            try {
                if (hFile.fileHandle == null) {
                    LogDebug("UnlockFile", "File never opened=NoContext exists: " + hFile.filename, true);
                    return Win32Returncode.ERROR_ACCESS_DENIED;
                }
                IFuserFilesystemItem vfi = hFile.fileHandle.CurrentFilesystemItem;
                if (vfi == null)
                    return Win32Returncode.ERROR_FILE_NOT_FOUND;

                if (!(vfi is IFuserFilesystemFile))
                    return Win32Returncode.ERROR_ACCESS_DENIED;
                
                FileLockManager lockMgr = vfi.Filelock;
                FileLockItem[] locks = null;
                FileLockItem unlockItem = null;

                lock (hFile.fileHandle) {
                    locks = hFile.fileHandle.GetFileLockList();
                }

                if (locks != null) {
                    foreach (FileLockItem lItem in locks) {
                        if (lItem.IsOpLock) {
                            if (lItem.OPLockOffset == offset && lItem.OPLockLength == length) 
                                unlockItem = lItem;// item found
                        }
                    }
                }

                if (unlockItem != null) {
                    lock (lockMgr) {
                        lockMgr.UnregisterLock(unlockItem);
                    }
                    lock (hFile.fileHandle) {
                        hFile.fileHandle.RemoveFileLock(unlockItem);
                    }                    
                } else {
                    return Win32Returncode.DEFAULT_UNKNOWN_ERROR; // TODO: change this error code, error codes are not currently supported by Fuser
                }
            } catch (FSException fe) {
                return fe.Error;
            } catch {
                return Win32Returncode.ERROR_ACCESS_DENIED;
            }
            return Win32Returncode.SUCCESS;
        }


        public Win32Returncode GetDiskFreeSpace(ref ulong freeBytesAvailable, ref ulong totalBytes, ref ulong totalFreeBytes) {
            try { // TODO: this method does not need a FileHandler parameter
                freeBytesAvailable = this.drive.DiskspaceFreeBytes;
                totalBytes = this.drive.DiskspaceTotalBytes;
                totalFreeBytes = freeBytesAvailable;

            } catch (FSException fe) {
                freeBytesAvailable = 0; totalBytes = 0; totalFreeBytes = 0;
                return fe.Error;
            } catch {
                freeBytesAvailable = 0; totalBytes = 0; totalFreeBytes = 0;
                return Win32Returncode.ERROR_ACCESS_DENIED;
            }
            return Win32Returncode.SUCCESS;    
        }


        public Win32Returncode Mount(string MountPoint, string RawDevice) {
            // forward to the virtual drive
            try {
                this.drive.Mounted(MountPoint, RawDevice);
            } catch {                
            }
            return Win32Returncode.SUCCESS;
        }


       
    }
}
