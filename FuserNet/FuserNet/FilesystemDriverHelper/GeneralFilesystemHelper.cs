using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;

using FuserLowlevelDriver;
using System.IO;

namespace FuserNet {
    internal static class GeneralFilesystemHelper {

        /// <summary>
        /// Sets the specified accesses and shares to the file. If false is returned, the lock cannot be set.
        /// </summary>
        /// <param name="fileHandle"></param>
        /// <param name="item"></param>
        /// <param name="access"></param>
        /// <param name="share"></param>
        /// <returns></returns>
        public static bool FilelockSet(FileHandler fileHandle, IFuserFilesystemItem item, FuserFileAccess access, FileShare share) {
            FileLockItem lItem = null;
                       
            try {
                lItem = new FileLockItem((item is IFuserFilesystemDirectory), (access == FuserFileAccess.Read || access == FuserFileAccess.ReadWrite), (access == FuserFileAccess.Write || access == FuserFileAccess.ReadWrite), share);
            } catch {
                return false;
            }
            return FilelockSet(fileHandle, item, lItem);
        }

        /// <summary>
        /// Sets the specified FileLock on the file and saves it in the file handle. Returns whether the setting is successful;
        /// if False is returned, the file is already blocked by another process.
        /// </summary>
        /// <param name="fileHandle"></param>
        /// <param name="fsItem"></param>
        /// <param name="LockItem"></param>
        /// <returns></returns>
        public static bool FilelockSet(FileHandler fileHandle, IFuserFilesystemItem fsItem, FileLockItem LockItem) {
            if (LockItem == null || fileHandle== null || fsItem ==null ) 
                return false;
            

            try {
                bool ret = false;
                FileLockManager lockMgr = fsItem.Filelock;

                if (lockMgr == null)  
                    return false; 

                lock (lockMgr) {
                    ret = lockMgr.RegisterLock(LockItem);                    
                }

                if (ret && fsItem is IFuserFilesystemDirectory && LockItem.IsDirectory && !LockItem.IsOpLock && !LockItem.Share.HasFlag(FileShare.Delete)) {
                    RegisterBlockDeletePermission((IFuserFilesystemDirectory)fsItem, LockItem);
                }


                if (ret) {
                    lock (fileHandle) {
                        fileHandle.AddFileLock(LockItem);
                    }
                } else {
                    return false; // access violation occurred
                }
            } catch {
                return false;
            }
            return true;
        }

        /// <summary>
        /// Release a filelock
        /// </summary>
        /// <param name="fileHandle"></param>
        /// <param name="item"></param>
        public static void FilelockRelease(FileHandler fileHandle, IFuserFilesystemItem item) {
            try {
                if (fileHandle == null || item == null)
                    return;

                FileLockManager lockMgr = item.Filelock;
                FileLockItem[] locks = null;


                lock (fileHandle) {
                    locks = fileHandle.GetFileLockList();
                }

                if (locks != null) {
                    IFuserFilesystemDirectory dirItem = null;
                    if (item is IFuserFilesystemDirectory) {
                        dirItem = (IFuserFilesystemDirectory)item;
                    }


                    lock (lockMgr) {
                        foreach (FileLockItem lItem in locks) {
                            lockMgr.UnregisterLock(lItem);
                            if (dirItem != null) {
                                ReleaseBlockDeletePermission(dirItem, lItem);
                            }
                        }
                    }
                }

            } catch {
                return;
            }
        }


        

        /// <summary>
        /// Sets the lock for deleting directories for all directories (recursive up to the root).
        /// </summary>
        /// <param name="blockDirectory"></param>
        /// <param name="flock"></param>
        private static void RegisterBlockDeletePermission(IFuserFilesystemDirectory blockDirectory, FileLockItem flock) {
            if (blockDirectory == null)
                return; // Abort criterion for recursive method.
            if (flock == null)
                return;

            try {
                FileLockManager lockMgr = blockDirectory.Filelock;
                if (lockMgr == null)
                    return;

                lock (lockMgr) {
                    lockMgr.RegisterBlockDeletePermission(flock);
                }
            } catch {
                return;
            }

            RegisterBlockDeletePermission(blockDirectory.Parent, flock); // Set recursively up to the root for all
        }

        /// <summary>
        /// Removes the lock for deleting directories for all directories (recursive up to the root).
        /// </summary>
        /// <param name="blockDirectory"></param>
        /// <param name="flock"></param>
        private static void ReleaseBlockDeletePermission(IFuserFilesystemDirectory blockDirectory, FileLockItem flock) {
            if (blockDirectory == null)
                return; // Abort criterion for recursive method.
            if (flock == null)
                return;

            try {
                FileLockManager lockMgr = blockDirectory.Filelock;
                if (lockMgr == null)
                    return;

                lock (lockMgr) {
                    lockMgr.ReleaseBlockDeletePermission(flock);
                }
            } catch {
                return;
            }

            ReleaseBlockDeletePermission(blockDirectory.Parent, flock);  // recursively reset to root for all
        }




        /// <summary>
        /// Checks whether the selected item can be deleted. Only the FileLocks are taken into account. Required for Move and Delete
        /// </summary>
        /// <param name="fileHandle"></param>
        /// <param name="item"></param>
        /// <returns></returns>
        public static bool DeletionAllow(FileHandler fileHandle, IFuserFilesystemItem item) {           
            try {
                if (fileHandle == null) {
                    return false;
                }
                if (item == null) {
                    return false;
                }

                List<FileLockItem> locks;
                FileLockManager lockMgr = item.Filelock;
                FileLockItem[] locksHandle = null; // list with locks for the current fileHandle
                FileLockItem[] locksFile = null;   // list with locks for the current file
                FileLockItem[] locksBlockDelete = null; // list with deletion-locks for the current file

                lock (fileHandle) {
                    locksHandle = fileHandle.GetFileLockList();
                }

                lock (lockMgr) {
                    locksFile = lockMgr.GetFileLockList();
                    locksBlockDelete = lockMgr.GetBlockDeletePermissionList();
                }

                if (locksFile == null) {
                    return true; // no locks found, file not in use
                }


                // check: deletion block
                if (locksBlockDelete != null) {
                    locks = new List<FileLockItem>(locksBlockDelete);
                    if (locksHandle != null) {
                        foreach (FileLockItem lItem in locksHandle) {
                            locks.Remove(lItem);
                        }
                    }
                    if (locks.Count != 0) {
                        return false;// deletion was blocked
                    }
                }
                // check: deletion block
                

                locks = new List<FileLockItem>(locksFile);
                if (locksHandle != null) {
                    foreach (FileLockItem lItem in locksHandle) {
                        locks.Remove(lItem);
                    }
                }

                bool del = false;
                foreach (FileLockItem lck in locks) {
                    if (!lck.Share.HasFlag(FileShare.Delete)) {
                        del = true;
                    }
                }

                if (del == false) {
                    return true;
                } else {
                    return false;
                }
            } catch {
                return false;
            }
        }




        /// <summary>
        /// Checks whether the specified area has been locked by another file handle.
        /// </summary>
        /// <param name="fileHandle"></param>
        /// <param name="item"></param>
        /// <param name="offset"></param>
        /// <param name="length"></param>
        /// <returns></returns>
        public static bool FileOPLockCheck(FileHandler fileHandle, IFuserFilesystemItem item, long offset, long length) {            
            try {
                if (fileHandle == null) {
                    return false;
                }
                if (!fileHandle.OPLockActive) {
                    return true; // do not check any locks, according to the filehandle locks are not possible.
                }
                
                if (item == null) {
                    return false;
                }
                if (offset < 0) {
                    return true;
                }
                if (length <= 0) {
                    return true;
                }
                

                FileLockManager lockMgr = item.Filelock;
                FileLockItem[] locksHandle = null; // list of locks from this handle.
                FileLockItem[] locksFile = null; // list of locks from this file.


                lock (fileHandle) {
                    locksHandle = fileHandle.GetFileLockList();
                }

                lock (lockMgr) {
                    locksFile = lockMgr.GetFileLockList();
                }

                if (locksFile == null) {
                    return true;
                }

                List<FileLockItem> locks = new List<FileLockItem>(locksFile);
                if (locksHandle != null) {
                    foreach (FileLockItem lItem in locksHandle) {
                        locks.Remove(lItem);
                    }
                }

                foreach (FileLockItem lItem in locks) {
                    if (lItem.IsOpLock) {
                        if (!lItem.CheckOPLockRange(offset, length)) {
                            return false;// violation occurred!
                        }
                    }
                }
                return true;
            } catch {
                return false;
            }
        }




        /// <summary>
        /// Sets the information that the vItem was changed. If SetLastAccess=true, the information that the vItem was accessed is also set.
        /// </summary>
        /// <param name="vItem"></param>
        /// <param name="setLastAccess"></param>
        public static void SetLastWrite(IFuserFilesystemItem vItem, bool setLastAccess) {
            try {
                if (vItem == null) return;
                if (vItem.Filelock == null) return;
                FileLastAccessControl flac = vItem.Filelock.getLastAccessControl;
                if (flac == null) return;
                lock (flac) {
                    flac.SetLastWrite();
                    if (setLastAccess)
                        flac.SetLastAccess();
                }
            } catch {
                return;
            }
        }


        /// <summary>
        /// Sets the information that the vItem has been accessed.
        /// </summary>
        /// <param name="vItem"></param>
        public static void SetLastAccess(IFuserFilesystemItem vItem) {
            try {
                if (vItem == null) return;
                if (vItem.Filelock == null) return;
                FileLastAccessControl flac = vItem.Filelock.getLastAccessControl;
                if (flac == null) return;
                lock (flac) {
                    flac.SetLastAccess();
                }
            } catch {
                return;
            }
        }

        
        /// <summary>
        /// Writes the LastAccessed and LastModified information, if they have been changed, to the file. For files, the archive attribute is also set if necessary.
        /// </summary>
        /// <param name="vItem"></param>
        public static void WriteLastWriteAndAccess(IFuserFilesystemItem vItem) {
            try {
                bool las = false;
                bool lws = false;
                bool IsDir = false;
                DateTime lat = DateTime.MinValue;
                DateTime lwt = DateTime.MinValue;

                if (vItem == null) return;
                if (vItem.Filelock == null) return;
                IsDir = (vItem is IFuserFilesystemDirectory);

                FileLastAccessControl flac = vItem.Filelock.getLastAccessControl;
                if (flac == null) return;
                lock (flac) {
                    las = flac.IsLastAccessSet;
                    lws = flac.IsLastWriteSet;

                    if (las)
                        lat = flac.LastAccessTime;
                    if (lws)
                        lwt = flac.LastWriteTime;

                    if (las || lws)
                        flac.Rest(); // if there are any changes, reset them all.
                }

                if (!las && !lws)
                    return; // nothing to do


                lock (vItem) {
                    if (las)
                        vItem.LastAccessTime = lat;

                    if (lws) {
                        vItem.LastWriteTime = lwt;
                        if (!IsDir)
                            vItem.isArchive = true;
                    }
                }

            } catch {
                return;
            }
        }



        /// <summary>
        /// Checks whether the specified directory is flagged for deletion.
        /// </summary>
        /// <param name="vDir"></param>
        /// <returns></returns>
        public static bool CheckIsDirectoryDeletedOnClose(IFuserFilesystemDirectory vDir) {
            // TODO: check if a certain error code must be returned when calling this function
            if (vDir == null) return false;
            if (vDir.Filelock == null) return false;

            bool selDeleted = false;
            try {
                FileLockManager lockMgr = vDir.Filelock;
                lock (lockMgr) {
                    selDeleted = lockMgr.DeleteOnClose;
                }
            } catch {
                return false;
            }
            return selDeleted;
        }


        /// <summary>
        /// Converts a passed FileItem into a file information structure. Exceptions can be thrown.
        /// </summary>
        /// <param name="vItem"></param>
        /// <returns></returns>
        public static FuserFileInformation convertFileInformation(IFuserFilesystemItem vItem) {
            FuserFileInformation fileInfo = new FuserFileInformation();
            convertFileInformation(vItem, fileInfo);
            return fileInfo;
        }

        /// <summary>
        /// Converts a passed FileItem into a file information structure. Exceptions can be thrown.
        /// </summary>
        /// <param name="FuserFileItem"></param>
        /// <returns></returns>
        public static void convertFileInformation(IFuserFilesystemItem vItem, FuserFileInformation overwriteFileInfo) {            
            lock (vItem) {
                overwriteFileInfo.Filename = vItem.Name;
                overwriteFileInfo.CreationTime = vItem.CreationTime;
                overwriteFileInfo.LastAccessTime = vItem.LastAccessTime;
                overwriteFileInfo.LastWriteTime = vItem.LastWriteTime;

                overwriteFileInfo.Attributes = 0;
                if (vItem.isArchive) { overwriteFileInfo.Attributes |= FileAttributes.Archive; }
                if (vItem.isReadOnly) { overwriteFileInfo.Attributes |= FileAttributes.ReadOnly; }
                if (vItem.isHidden) { overwriteFileInfo.Attributes |= FileAttributes.Hidden; }
                if (vItem.isSystem) { overwriteFileInfo.Attributes |= FileAttributes.System; }


                if (vItem is IFuserFilesystemDirectory) {                    
                    overwriteFileInfo.Attributes |= FileAttributes.Directory;
                    overwriteFileInfo.Length = 0;
                } else {
                    if (overwriteFileInfo.Attributes == 0)
                        overwriteFileInfo.Attributes = FileAttributes.Normal;

                    IFuserFilesystemFile file = (IFuserFilesystemFile)vItem;
                    overwriteFileInfo.Length = file.Length;
                }
            }
        }


    }
}
