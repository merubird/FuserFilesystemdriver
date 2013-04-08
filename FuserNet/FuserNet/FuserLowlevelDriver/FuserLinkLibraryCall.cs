using System;
using System.Runtime.InteropServices;

namespace FuserLowlevelDriver {

    internal static class FuserLinkLibraryCall {

        [StructLayout(LayoutKind.Explicit)]
        private struct VersionUnion {
            [FieldOffset(0)] public uint BinaryVersion;
            [FieldOffset(0)] public byte Major;
            [FieldOffset(1)] public byte Minor;
            [FieldOffset(2)] public ushort Revision;            
        }

        private static class DLLCoreCall {
            [DllImport("fuser.dll")]
            public static extern int FuserDeviceMount(ref FuserDefinition.FUSER_MOUNT_PARAMETER parameter);
            
            [DllImport("fuser.dll")]
            public static extern int FuserDeviceUnmount(IntPtr mountPoint);

            [DllImport("fuser.dll")]
            public static extern bool FuserSendHeartbeat(IntPtr MountPoint, IntPtr DeviceName);
                       

            [DllImport("fuser.dll")]
            public static extern uint FuserVersion();
        }


        public static int DeviceMount(FuserDefinition.FUSER_MOUNT_PARAMETER parameter, FuserDevice device) {            
            parameter.Version = 1;
            parameter.EventLoaderFunc = device.EventLoader;
            int ret = DLLCoreCall.FuserDeviceMount(ref parameter);
            device.HeartbeatStop();
            return ret;
        }


        public static int DeviceUnmount(string mountPoint) {            
            return DLLCoreCall.FuserDeviceUnmount(Marshal.StringToHGlobalUni(mountPoint));
        }
                  
        public static bool SendHeartbeat(string MountPoint, string DeviceName) {
            return DLLCoreCall.FuserSendHeartbeat(Marshal.StringToHGlobalUni(MountPoint), Marshal.StringToHGlobalUni(DeviceName));            
        }

        public static string FuserVersion() {
            // TODO: add trycatch
            VersionUnion n = new VersionUnion();
            n.BinaryVersion = DLLCoreCall.FuserVersion();
            if (n.BinaryVersion == 0) {
                return "";
            } else {
                return n.Major + "." + n.Minor + "." + n.Revision;
            }
        }

    }


}
