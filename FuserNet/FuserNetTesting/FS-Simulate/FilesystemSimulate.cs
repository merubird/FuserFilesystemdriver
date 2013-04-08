using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;

using System.IO;

using FuserNet;

namespace Testsystem {
    class FilesystemSimulate : IFuserlDrive {
        private IFuserFilesystemDirectory root;

        public FilesystemSimulate() {
            this.root = getnewRoot();
        }

        private static MemoryStream getData(int size) {
            MemoryStream data = new MemoryStream();

            string test = "1234567890";
            byte[] bt = System.Text.UTF8Encoding.UTF8.GetBytes(test);

            int r = 0;
            while (data.Length < size) {
                r = size - (int)data.Length;
                if (r > bt.Length) r = bt.Length;

                data.Write(bt, 0, r);
            }


            return data;
        }


        private static MemoryStream getData(string textData) {
            MemoryStream data = new MemoryStream();

            byte[] bt = System.Text.UTF8Encoding.UTF8.GetBytes(textData);
            data.Write(bt, 0, bt.Length);

            return data;
        }

        private IFuserFilesystemDirectory getnewRoot() {


            SimRoot root = new SimRoot();
            SimSubdirectory subA = new SimSubdirectory(null, "SubA");
            SimSubdirectory subB = new SimSubdirectory(null, "SubB");

            SimSubdirectory TestDir = new SimSubdirectory(null, "test");

            SimFile testFile = new SimFile("test.txt", getData("This is a test!"), FileAttributes.Archive);

            SimFile file1 = new SimFile("Datei 1.txt", getData(100), FileAttributes.Archive);
            SimFile file2 = new SimFile("Datei 2.dat", getData(2024), FileAttributes.Archive);
            SimFile file3 = new SimFile("Datei 3.bmp", getData(33333), FileAttributes.Archive);

            SimFile file4 = new SimFile("Ganz Neue Dat-Datei.dat", getData(123456), FileAttributes.Archive);



            root.AddDir(new SimSubdirectory(null, "Leeres Verzeichnis"));
            root.AddDir(new SimSubdirectory(null, "nochmals leer"));
            root.AddDir(subA);

            subA.AddDir(new SimSubdirectory(null, "Leer a1"));
            subA.AddDir(new SimSubdirectory(null, "Leer a2"));
            subA.AddDir(subB);

            subB.AddDir(new SimSubdirectory(null, "Leer b1"));
            subB.AddDir(new SimSubdirectory(null, "Leer b2"));


            root.AddFile(file1);
            root.AddFile(file2);
            root.AddFile(file3);
            subB.AddFile(file4);

            root.AddDir(TestDir);

            root.AddFile(testFile);

            return root;
        }



        public IFuserFilesystemDirectory Root {
            get { return this.root; }
        }

        public string Volumelabel {
            get { return "FuserFilesystemDemo"; }
        }

        public string Filesystem {
            get { return "TestFS Dev"; }
        }

        public uint Serialnumber {
            get { return 0x12345678; }
        }


        public void Unmounted(FuserMountFinishStatus returncode) {
            Console.WriteLine("Beendet: " + returncode);
        }



        public ulong DiskspaceTotalBytes {
            get { return 3738339534848; }
        }

        public ulong DiskspaceFreeBytes {
            get { return 879609302220; }
        }


        public void Mounted(string MountPoint, string RawDevice) {
            Console.WriteLine("Mounted: " + MountPoint + "  Device: " + RawDevice);            
        }


    }


}
