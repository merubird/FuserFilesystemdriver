using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading;

using FuserNet;

namespace Testsystem
{
    class Program
    {        
        static void Main(string[] args)
        {
            //Console.WriteLine(FSException.GetError(FSErrorcodes.ERROR_DIR_NOT_EMPTY));
            //Console.ReadLine();return;

            

        
            Console.BufferWidth = 150;
            Console.WindowWidth = Console.BufferWidth;

            Program p = new Program();
            p.progStart();                
        }

        public Program()
        {
                        
        }

                

        public  void progStart(){
            IFuserlDrive vd = null;

            vd = new FilesystemSimulate();
            //vd = new FilesystemSpiegel();


            FuserDriveMounter vdm = new FuserDriveMounter(vd);
            vdm.DebugMode = true;

            string mountPoint;
            mountPoint = @"J:\";
            //mountPoint = @"C:\temp\ab";

            Console.WriteLine("--------  Start Fuser Drive -------");
            Console.WriteLine("Version : " + FuserDriveMounter.GetFuserVersion());
            Console.WriteLine("Start: " + vdm.Mount(mountPoint));
                       



            string tmp = "-";
            while (tmp != "") {
                tmp = Console.ReadLine();

                if (tmp != "") {
                    Console.Clear();
                }

            }



            vdm.Unmount();
        }



    }
}
