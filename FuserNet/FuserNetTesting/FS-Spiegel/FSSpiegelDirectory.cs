using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;

using System.IO;
using FuserNet;

namespace Testsystem {
    class FSSpiegelDirectory : IFuserFilesystemDirectory {
        FileLockManager fLock = new FileLockManager(); public FileLockManager Filelock { get { return this.fLock; } }


        private DirectoryInfo realDir;

        private List<IFuserFilesystemItem> content;

        private bool flag;

        private IFuserFilesystemDirectory parentDir;

        public FSSpiegelDirectory(IFuserFilesystemDirectory parentDir, DirectoryInfo realDir) {
            this.realDir = realDir;
            this.content = null;
            this.flag = false;
            this.parentDir = parentDir;
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
            if (fullname.Trim().ToUpper() == realDir.FullName.Trim().ToUpper()) {
                return true;
            } else {
                return false;
            }
        }

        private void ensureContent() {

            this.realDir.Refresh();

            if (content == null) {
                content = new List<IFuserFilesystemItem>();
            }



            foreach (IFuserFilesystemItem vfi in content) {
                if (vfi is FSSpiegelDirectory) {
                    FSSpiegelDirectory vfidir = (FSSpiegelDirectory)vfi;
                    vfidir.FlagReset();
                }
                if (vfi is FSSpiegelFile) {
                    FSSpiegelFile vfifile = (FSSpiegelFile)vfi;
                    vfifile.FlagReset();
                }
            }



            DirectoryInfo[] dis = this.realDir.GetDirectories();
            foreach (DirectoryInfo di in dis) {
                bool doinsert = true;
                foreach (IFuserFilesystemItem vfi in content) {
                    if (vfi is FSSpiegelDirectory) {
                        FSSpiegelDirectory vfidir = (FSSpiegelDirectory)vfi;
                        if (vfidir.checkFullname(di.FullName)) {
                            vfidir.FlagSet();
                            doinsert = false;
                            break;
                        }

                    }
                }
                if (doinsert) {
                    FSSpiegelDirectory vfidir = new FSSpiegelDirectory(this, di);
                    vfidir.FlagSet();
                    content.Add(vfidir);
                }
            }


            FileInfo[] fis = this.realDir.GetFiles();
            foreach (FileInfo fi in fis) {

                bool doinsert = true;
                foreach (IFuserFilesystemItem vfi in content) {
                    if (vfi is FSSpiegelFile) {
                        FSSpiegelFile vfifile = (FSSpiegelFile)vfi;
                        if (vfifile.checkFullname(fi.FullName)) {
                            vfifile.FlagSet();
                            doinsert = false;
                            break;
                        }

                    }
                }
                if (doinsert) {
                    FSSpiegelFile vfifile = new FSSpiegelFile(fi);
                    vfifile.FlagSet();
                    content.Add(vfifile);
                }
            }


            IFuserFilesystemItem[] rmlist = content.ToArray();
            foreach (IFuserFilesystemItem vfi in rmlist) {
                if (vfi is FSSpiegelDirectory) {
                    FSSpiegelDirectory vfidir = (FSSpiegelDirectory)vfi;
                    if (!vfidir.FlagGet()) {
                        content.Remove(vfidir);
                    }
                }
                if (vfi is FSSpiegelFile) {
                    FSSpiegelFile vfifile = (FSSpiegelFile)vfi;
                    if (!vfifile.FlagGet()) {
                        content.Remove(vfifile);
                    }
                }
            }



        }


        public IFuserFilesystemItem[] GetContentList() {
            IFuserFilesystemItem[] l;
            lock (this.realDir) {
                ensureContent();
                l = this.content.ToArray();
            }
            return l;
        }


        public IFuserFilesystemItem GetItem(string itemname) {
            IFuserFilesystemItem[] l;
            lock (this.realDir) {
                ensureContent();
                l = this.content.ToArray();
            }



            foreach (IFuserFilesystemItem dc in l) {
                if (dc.Name.ToUpper() == itemname.ToUpper()) {
                    return dc;
                }
            }
            throw new KeyNotFoundException();

        }

        public string Name {
            get { return this.realDir.Name; }
        }
        


        public bool isArchive { get { return this.realDir.Attributes.HasFlag(FileAttributes.Archive); } set { if (value) { this.realDir.Attributes |= FileAttributes.Archive; } else { this.realDir.Attributes &= ~FileAttributes.Archive; } } }
        public bool isReadOnly { get { return this.realDir.Attributes.HasFlag(FileAttributes.ReadOnly); } set { if (value) { this.realDir.Attributes |= FileAttributes.ReadOnly; } else { this.realDir.Attributes &= ~FileAttributes.ReadOnly; } } }
        public bool isHidden { get { return this.realDir.Attributes.HasFlag(FileAttributes.Hidden); } set { if (value) { this.realDir.Attributes |= FileAttributes.Hidden; } else { this.realDir.Attributes &= ~FileAttributes.Hidden; } } }
        public bool isSystem { get { return this.realDir.Attributes.HasFlag(FileAttributes.System); } set { if (value) { this.realDir.Attributes |= FileAttributes.System; } else { this.realDir.Attributes &= ~FileAttributes.System; } } }

        public DateTime CreationTime { get { return this.realDir.CreationTime; } set { this.realDir.CreationTime = value; } }
        public DateTime LastAccessTime { get { return this.realDir.LastAccessTime; } set { this.realDir.LastAccessTime = value; } }
        public DateTime LastWriteTime { get { return this.realDir.LastWriteTime; } set { this.realDir.LastWriteTime = value; } }




        public void DebugAddEntry(IFuserFilesystemItem n) {
            throw new NotImplementedException();
        }


        public void CreateDirectory(string directoryname) {

            DirectoryInfo di = new DirectoryInfo(System.IO.Path.Combine(realDir.FullName, directoryname));
            di.Create();

        }




        public void CreateFile(string filename, FileAttributes newAttributes) {
            FileInfo fi = new FileInfo(System.IO.Path.Combine(realDir.FullName, filename));
            fi.Attributes = newAttributes;
            FileStream fs = fi.Create();
            fs.Close();
        }


        public void Delete(IFuserFilesystemItem item) {
            throw new NotImplementedException();
        }




        public void MoveTo(IFuserFilesystemItem item, IFuserFilesystemDirectory destination, string newname) {
            throw new NotImplementedException();
        }








        public bool DeletionAllow(IFuserFilesystemItem item) {
            return true;
        }


        public IFuserFilesystemDirectory Parent {
            get { return this.parentDir; }
        }
    }
}
