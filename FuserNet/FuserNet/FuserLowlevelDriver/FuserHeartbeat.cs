using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading;


namespace FuserLowlevelDriver {
    class FuserHeartbeat {
        
        private string MountPoint;
        private string DeviceName;
        private Thread thHeartbeat;
                
        public FuserHeartbeat(string MountPoint, string DeviceName) {            
            this.MountPoint = MountPoint;
            this.DeviceName = DeviceName;

            this.thHeartbeat = new Thread(pvSendHeartbeat);
            this.thHeartbeat.Start();             
        }
        
        private void pvSendHeartbeat(){
            try {
                while (true) {
                    FuserLinkLibraryCall.SendHeartbeat(this.MountPoint, this.DeviceName);
                    Thread.Sleep(2000);
                }
            } catch {

            }
        }
         

        public void Stop() {
            try {
                thHeartbeat.Interrupt();
                thHeartbeat.Join();
            } catch {
                return;
            }
        }
        


    }
}
