using System;
using System.Collections.Generic;

namespace FuserNet {    
    public class FileLockManager {

        private List<FileLockItem> fLocks;
        private List<FileLockItem> BlockDeletePermissionList;
        private FileLastAccessControl plastAccess;

        private bool pDeleteOnClose;
        
        public FileLockManager() {
            this.fLocks = new List<FileLockItem>();
            this.BlockDeletePermissionList = new List<FileLockItem>();
            this.pDeleteOnClose = false;
            this.plastAccess = new FileLastAccessControl();            
        }
       
        public FileLastAccessControl getLastAccessControl { get { return this.plastAccess; }}
        public int Count            { get { return this.fLocks.Count;   }}
        public bool DeleteOnClose   { get { return this.pDeleteOnClose; } set { this.pDeleteOnClose = value; }}
        
        
        public bool RegisterLock(FileLockItem flock) {            
            bool isDir = flock.IsDirectory;
                        
            foreach (FileLockItem el in this.fLocks) {
                if (el.IsDirectory != flock.IsDirectory) {
                    // Locks for directories may only be compared with those of directories.
                    return false;
                }
                if (!flock.CheckShareRuleIsPermit(el)) {                    
                    return false; // Access violation found
                }
            }            
            this.fLocks.Add(flock);
            return true;            
        }


        public void RegisterBlockDeletePermission(FileLockItem flock) {
            this.BlockDeletePermissionList.Add(flock);            
        }

        public void ReleaseBlockDeletePermission(FileLockItem flock) {
            this.BlockDeletePermissionList.Remove(flock);            
        }

                        
        public FileLockItem[] GetFileLockList() {
            return this.fLocks.ToArray();
        }
        public FileLockItem[] GetBlockDeletePermissionList() {
            return this.BlockDeletePermissionList.ToArray();
        }

        public void UnregisterLock(FileLockItem flock) {                        
            this.fLocks.Remove(flock);
        }
        
    }
}
