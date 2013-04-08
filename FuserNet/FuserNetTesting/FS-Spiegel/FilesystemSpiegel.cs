using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.IO;

using FuserNet;

namespace Testsystem {
    class FilesystemSpiegel : IFuserlDrive {
        private IFuserFilesystemDirectory root;
        private DriveInfo drvInfo;

        public FilesystemSpiegel() {

            this.drvInfo = new DriveInfo(@"C:\");


            this.root = new FSSpiegelDirectory(null, new System.IO.DirectoryInfo(this.drvInfo.RootDirectory.FullName));
        }



        public IFuserFilesystemDirectory Root {
            get { return this.root; }
        }

        public string Volumelabel {
            get { return this.drvInfo.VolumeLabel; }
        }

        public string Filesystem {
            get { return this.drvInfo.DriveFormat; }
        }

        public uint Serialnumber {
            get { return 0x12345678; }
        }

        public void Unmounted(FuserMountFinishStatus returncode) {
            Console.WriteLine("Beendet: " + returncode);
        }

        public ulong DiskspaceTotalBytes {
            get { return (ulong)this.drvInfo.TotalSize; }
        }

        public ulong DiskspaceFreeBytes {
            get { return (ulong)this.drvInfo.TotalFreeSpace; }
        }



        public void Mounted(string MountPoint, string RawDevice) {
            Console.WriteLine("Mounted: " + MountPoint + "  Device: " + RawDevice);            
        }


    }
}
