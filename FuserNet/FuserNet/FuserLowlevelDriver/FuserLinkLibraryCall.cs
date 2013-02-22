using System;
using System.Runtime.InteropServices;

namespace FuserLowlevelDriver {

    internal static class FuserLinkLibraryCall {        

        private static class DLLCoreCall {
            [DllImport("fuser.dll")]
            public static extern int FuserMain(ref FuserDefinition.FUSER_OPTIONS options, ref FuserDefinition.FUSER_OPERATIONS operations);

            [DllImport("fuser.dll")]
            public static extern int FuserUnmount(int driveLetter);

            [DllImport("fuser.dll")]
            public static extern int FuserRemoveMountPoint(IntPtr mountPoint);

            [DllImport("fuser.dll")]
            public static extern uint FuserVersion();

            [DllImport("fuser.dll")]
            public static extern uint FuserDriveVersion();

            [DllImport("fuser.dll")]
            public static extern bool FuserResetTimeout(uint timeout, ref FuserDefinition.FUSER_FILE_INFO rawHFile);

            [DllImport("fuser.dll")]
            public static extern bool FuserSendHeartbeat(IntPtr MountPoint, IntPtr DeviceName);
        }


        public static int FuserMain(FuserDefinition.FUSER_OPTIONS fuserOptions, FuserDevice device) {
            FuserDefinition.FUSER_OPERATIONS cmd = device.FuserLinkedDelegates;
            int ret = DLLCoreCall.FuserMain(ref fuserOptions, ref cmd);
            device.HeartbeatStop();
            return ret;
        }

        public static int FuserUnmount(char driveLetter) {
            return DLLCoreCall.FuserUnmount(driveLetter);
        }

        public static int FuserRemoveMountPoint(string mountPoint) {
            return DLLCoreCall.FuserRemoveMountPoint(Marshal.StringToHGlobalUni(mountPoint));
        }

        public static uint FuserVersion() {
            return DLLCoreCall.FuserVersion();
        }

        public static uint FuserDriverVersion() {
            return DLLCoreCall.FuserDriveVersion();
        }

        //public static bool FuserResetTimeout(uint timeout, FuserFileHandler fileinfo) { // TODO: remove
        //    FuserDefinition.FUSER_FILE_INFO rawFileInfo = new FuserDefinition.FUSER_FILE_INFO();
        //    rawFileInfo.FuserContext = fileinfo.FuserContext;
        //    return DLLCoreCall.FuserResetTimeout(timeout, ref rawFileInfo);
        //}

        public static bool FuserSendHeartbeat(string MountPoint, string DeviceName) {
            return DLLCoreCall.FuserSendHeartbeat(Marshal.StringToHGlobalUni(MountPoint), Marshal.StringToHGlobalUni(DeviceName));            
        }



    }


}
