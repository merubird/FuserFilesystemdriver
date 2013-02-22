using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.IO;

using FuserNet;


namespace Testsystem {
    class SimSubdirectory : IFuserFilesystemDirectory {
        FileLockManager fLock = new FileLockManager(); public FileLockManager Filelock { get { return this.fLock; } }

        private List<IFuserFilesystemItem> directoryContent;
        private string name;

        private IFuserFilesystemDirectory parentDir;

        public SimSubdirectory(IFuserFilesystemDirectory parentDir, string name) {
            this.directoryContent = new List<IFuserFilesystemItem>();
            this.name = name;
            this.attrA = false;
            this.parentDir = parentDir;

            this.timeA = DateTime.Now;
            this.timeC = this.timeA;
            this.timeW = this.timeA;
        }

        public void SetParent(IFuserFilesystemDirectory newParent) {
            this.parentDir = newParent;
        }

        public void SetNewName(string newname) {
            this.name = newname;
        }

        public void AddDir(SimSubdirectory newDir) {
            directoryContent.Add(newDir);
            newDir.SetParent(this);
        }
        public void AddFile(IFuserFilesystemFile newFile) {
            directoryContent.Add(newFile);
        }
        public void AddItem(IFuserFilesystemItem n) {
            directoryContent.Add(n);
        }

        public void DebugAddEntry(IFuserFilesystemItem n) {
            directoryContent.Add(n);
        }


        public IFuserFilesystemItem[] GetContentList() {
            return this.directoryContent.ToArray();
        }

               

        public string Name {
            get { return this.name; }
        }

        private bool attrA; public bool isArchive { get { return attrA; } set { attrA = value; } }
        private bool attrR; public bool isReadOnly { get { return attrR; } set { attrR = value; } }
        private bool attrH; public bool isHidden { get { return attrH; } set { attrH = value; } }
        private bool attrS; public bool isSystem { get { return attrS; } set { attrS = value; } }

        private DateTime timeC; public DateTime CreationTime { get { return this.timeC; } set { this.timeC = value; } }
        private DateTime timeA; public DateTime LastAccessTime { get { return this.timeA; } set { this.timeA = value; } }
        private DateTime timeW; public DateTime LastWriteTime { get { return this.timeW; } set { this.timeW = value; } }




        public IFuserFilesystemItem GetItem(string itemname) {

            foreach (IFuserFilesystemItem dc in directoryContent) {
                if (dc.Name.ToUpper() == itemname.ToUpper()) {
                    return dc;
                }
            }
            throw new KeyNotFoundException();
        }





        public void CreateDirectory(string directoryname) {
            //throw new NotImplementedException();
            if (directoryname == "")
                throw new Exception();


            directoryContent.Add(new SimSubdirectory(this, directoryname));

        }


        public void CreateFile(string filename, FileAttributes newAttributes) {
            //throw new NotImplementedException();
            directoryContent.Add(new SimFile(filename, new System.IO.MemoryStream(), newAttributes));
        }


        public void Delete(IFuserFilesystemItem item) {
            if (!directoryContent.Remove(item)) {
                throw new Exception("File Not found");
            } else {
                if (item is SimFile) {
                    SimFile vFile = (SimFile)item;
                    vFile.destroy();
                }
            }

        }


        public void MoveTo(IFuserFilesystemItem item, IFuserFilesystemDirectory destination, string newname) {
            if (item is SimRoot) {
                throw new Exception("Access error");
            }
            if (item is SimFile) {
                SimFile vFile = (SimFile)item;
                if (vFile.Name != newname) {
                    vFile.SetNewName(newname);
                }
            }

            if (item is SimSubdirectory) {
                SimSubdirectory vDir = (SimSubdirectory)item;
                if (vDir.Name != newname) {
                    vDir.SetNewName(newname);
                }
            }

            if (destination == null) return;



            if (destination is SimSubdirectory) {
                SimSubdirectory vDstDir = (SimSubdirectory)destination;
                directoryContent.Remove(item);
                vDstDir.AddItem(item);
                return;
            }

            if (destination is SimRoot) {
                SimRoot vDstRoot = (SimRoot)destination;
                directoryContent.Remove(item);
                vDstRoot.AddItem(item);
                return;
            }



            throw new Exception("Mode not supported");
        }


        public bool DeletionAllow(IFuserFilesystemItem item) {
            return true;
        }


        public IFuserFilesystemDirectory Parent {
            get { return this.parentDir; }
        }
    }
}
