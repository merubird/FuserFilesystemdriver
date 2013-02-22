using System;
using System.Collections.Generic;
using System.IO;

namespace FuserNet
{
    internal class FileHandler {
        private string pFilename;
        private IFuserFilesystemItem curItem; // Target element to which the FileHandle points
        private IFuserFilesystemDirectory curDir; // Directory in which the element is located

        private List<FileLockItem> pLocks;
        private bool UseOPLocks;

        // determines whether the handle has been opened for I/O operations
        private bool pIsOpenRead;
        private bool pIsOpenWrite;
        private Stream pData;

        private FileLastAccessControl pFileLastAccess;
              
        public FileHandler(string filename) {
            this.pFilename = filename;            
            this.curItem = null;
            this.curDir = null;
            this.UseOPLocks = true;// by default, OPLocks must be checked

            this.pIsOpenRead = false;
            this.pIsOpenWrite = false;
            this.pData = null;

            this.pLocks = new List<FileLockItem>();
            this.pLocks.Clear();

            this.pFileLastAccess = new FileLastAccessControl();
        }

        public string Filename   {get {return this.pFilename;   }}
        public bool IsOpenRead   {get {return this.pIsOpenRead; } set {this.pIsOpenRead = value; }}
        public bool IsOpenWrite  {get {return this.pIsOpenWrite;} set {this.pIsOpenWrite = value;}}
        public bool OPLockActive {get {return this.UseOPLocks;  } set {this.UseOPLocks = value;  }}
        public Stream Data       {get {return this.pData;       } set {this.pData = value;       }}

        public IFuserFilesystemItem CurrentFilesystemItem { get { return this.curItem; } set { this.curItem = value; } }
        public IFuserFilesystemDirectory FolderContainer { get { return this.curDir; } set { this.curDir = value; } }
        public FileLastAccessControl FileLastAccess         { get { return this.pFileLastAccess; }}


        public void AddFileLock(FileLockItem lockItem) {
            if (lockItem == null) {
                return;
            } else {
                this.pLocks.Add(lockItem);
            }            
        }

        public FileLockItem[] GetFileLockList() {            
            return this.pLocks.ToArray();
        }

        public void RemoveFileLock(FileLockItem lockItem) {
            if (lockItem == null) {
                return;
            } else {
                this.pLocks.Remove(lockItem);
            }    
        }
                
    }
}
