using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;

using System.IO;

using FuserNet;


namespace Testsystem {
    class SimRoot : IFuserFilesystemDirectory {
        FileLockManager fLock = new FileLockManager(); public FileLockManager Filelock { get { return this.fLock; } }

        private List<IFuserFilesystemItem> directoryContent;


        public SimRoot() {
            directoryContent = new List<IFuserFilesystemItem>();

        }

        public void AddDir(SimSubdirectory newDir) {
            directoryContent.Add(newDir);
            newDir.SetParent(this);
        }
        public void AddFile(IFuserFilesystemFile newFile) {
            directoryContent.Add(newFile);
        }
        public void DebugAddEntry(IFuserFilesystemItem n) {
            directoryContent.Add(n);
        }




        public void a() {
            throw new NotImplementedException();
        }


        public IFuserFilesystemItem[] GetContentList() {
            return this.directoryContent.ToArray();
        }



        public string Name {
            get { return "{root}"; }
        }


        public bool isArchive { get { return false; } set { } }
        public bool isReadOnly { get { return false; } set { } }
        public bool isHidden { get { return false; } set { } }
        public bool isSystem { get { return false; } set { } }

        public DateTime CreationTime { get { return DateTime.Now; } set { } }
        public DateTime LastAccessTime { get { return DateTime.Now; } set { } }
        public DateTime LastWriteTime { get { return DateTime.Now; } set { } }




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
            //throw new FSException(FSErrorcodes.ERROR_NOT_READY );

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

        public void AddItem(IFuserFilesystemItem n) {
            directoryContent.Add(n);
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
            get { return null; }
        }
    }
}
