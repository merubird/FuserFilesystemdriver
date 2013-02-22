using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;

using System.IO;

using FuserNet;


namespace Testsystem {
    class SimFile : IFuserFilesystemFile {
        FileLockManager fLock = new FileLockManager(); public FileLockManager Filelock { get { return this.fLock; } }

        private bool attrA; public bool isArchive { get { return attrA; } set { attrA = value; } }
        private bool attrR; public bool isReadOnly { get { return attrR; } set { attrR = value; } }
        private bool attrH; public bool isHidden { get { return attrH; } set { attrH = value; } }
        private bool attrS; public bool isSystem { get { return attrS; } set { attrS = value; } }

        private string name;

        private MemoryStream data;

        long size;

        public SimFile(string name, MemoryStream data, FileAttributes defaultAttributes) {
            this.name = name;
            this.data = data;

            //this.attrA = true;
            this.timeA = DateTime.Now;
            this.timeC = DateTime.Now;
            this.timeW = DateTime.Now;

            this.size = 0;

            if (defaultAttributes.HasFlag(FileAttributes.Archive)) {
                this.attrA = true;
            }

            if (defaultAttributes.HasFlag(FileAttributes.System)) {
                this.attrS = true;
            }

            if (defaultAttributes.HasFlag(FileAttributes.ReadOnly)) {
                this.attrR = true;
            }
            if (defaultAttributes.HasFlag(FileAttributes.Hidden)) {
                this.attrH = true;
            }


        }

        public void destroy() {
            this.size = this.data.Length;
            this.data.SetLength(0);
            this.data.Close();
            this.data = null;
        }

        public void SetNewName(string newname) {
            this.name = newname;
        }

        public long Length {
            get {
                if (this.data == null) {
                    return this.size;
                } else {
                    return this.data.Length;
                }


            }
        }

        public bool isDirectory {
            get { return false; }
        }

        public string Name {
            get { return name; }
        }



        private DateTime timeC; public DateTime CreationTime { get { return this.timeC; } set { this.timeC = value; } }
        private DateTime timeA; public DateTime LastAccessTime { get { return this.timeA; } set { this.timeA = value; } }
        private DateTime timeW; public DateTime LastWriteTime { get { return this.timeW; } set { this.timeW = value; } }



        public Stream FileOpen(FileAccess access, FileShare share) {
            return this.data;
        }

        public void FileClose(Stream filedata) {
            // no operation, stream remains open

        }
    }
}
