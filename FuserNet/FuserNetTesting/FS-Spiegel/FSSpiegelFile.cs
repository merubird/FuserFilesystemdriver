using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;

using System.IO;
using FuserNet;

namespace Testsystem {
    class FSSpiegelFile : IFuserFilesystemFile {
        FileLockManager fLock = new FileLockManager(); public FileLockManager Filelock { get { return this.fLock; } }

        private FileInfo realFile;
        private bool flag;

        public FSSpiegelFile(FileInfo realFile) {
            this.realFile = realFile;
            this.flag = false;
        }

        public void FlagReset() {
            this.flag = false;
        }
        public void FlagSet() {
            this.flag = true;
        }
        public bool FlagGet() {
            return this.flag;
        }


        public bool checkFullname(string fullname) {
            if (fullname.Trim().ToUpper() == realFile.FullName.Trim().ToUpper()) {
                return true;
            } else {
                return false;
            }
        }

        public long Length {
            get { return this.realFile.Length; }
        }

        public string Name {
            get { return this.realFile.Name; }
        }

        public bool isDirectory {
            get { return false; }
        }

        public bool isArchive { get { return this.realFile.Attributes.HasFlag(FileAttributes.Archive); } set { if (value) { this.realFile.Attributes |= FileAttributes.Archive; } else { this.realFile.Attributes &= ~FileAttributes.Archive; } } }
        public bool isReadOnly { get { return this.realFile.Attributes.HasFlag(FileAttributes.ReadOnly); } set { if (value) { this.realFile.Attributes |= FileAttributes.ReadOnly; } else { this.realFile.Attributes &= ~FileAttributes.ReadOnly; } } }
        public bool isHidden { get { return this.realFile.Attributes.HasFlag(FileAttributes.Hidden); } set { if (value) { this.realFile.Attributes |= FileAttributes.Hidden; } else { this.realFile.Attributes &= ~FileAttributes.Hidden; } } }
        public bool isSystem { get { return this.realFile.Attributes.HasFlag(FileAttributes.System); } set { if (value) { this.realFile.Attributes |= FileAttributes.System; } else { this.realFile.Attributes &= ~FileAttributes.System; } } }

        public DateTime CreationTime { get { return this.realFile.CreationTime; } set { this.realFile.CreationTime = value; } }
        public DateTime LastAccessTime { get { return this.realFile.LastAccessTime; } set { this.realFile.LastAccessTime = value; } }
        public DateTime LastWriteTime { get { return this.realFile.LastWriteTime; } set { this.realFile.LastWriteTime = value; } }







        public Stream FileOpen(FileAccess access, FileShare share) {
            return new FileStream(realFile.FullName, FileMode.Open, FileAccess.ReadWrite, FileShare.ReadWrite);
        }

        public void FileClose(Stream filedata) {
            filedata.Close();
        }
    }
}
