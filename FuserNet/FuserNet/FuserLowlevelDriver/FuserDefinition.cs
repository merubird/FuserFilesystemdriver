using System;
using System.Runtime.InteropServices;
using System.IO;
using ComTypes = System.Runtime.InteropServices.ComTypes;

namespace FuserLowlevelDriver {
    internal static class FuserDefinition {
        
        [StructLayout(LayoutKind.Sequential, Pack = 4)]
        public struct FUSER_OPERATIONS {
            public FuserDevice.CreateFileDelegate CreateFile;
            public FuserDevice.OpenDirectoryDelegate OpenDirectory;
            public FuserDevice.CreateDirectoryDelegate CreateDirectory;
            public FuserDevice.CleanupDelegate Cleanup;
            public FuserDevice.CloseFileDelegate CloseFile;
            public FuserDevice.ReadFileDelegate ReadFile;
            public FuserDevice.WriteFileDelegate WriteFile;
            public FuserDevice.FlushFileBuffersDelegate FlushFileBuffers;
            public FuserDevice.GetFileInformationDelegate GetFileInformation;
            public FuserDevice.FindFilesDelegate FindFiles;
            public IntPtr FindFilesWithPattern;
            public FuserDevice.SetFileAttributesDelegate SetFileAttributes;
            public FuserDevice.SetFileTimeDelegate SetFileTime;
            public FuserDevice.DeleteFileDelegate DeleteFile;
            public FuserDevice.DeleteDirectoryDelegate DeleteDirectory;
            public FuserDevice.MoveFileDelegate MoveFile;
            public FuserDevice.SetEndOfFileDelegate SetEndOfFile;
            public FuserDevice.SetAllocationSizeDelegate SetAllocationSize;
            public FuserDevice.LockFileDelegate LockFile;
            public FuserDevice.UnlockFileDelegate UnlockFile;
            public FuserDevice.GetDiskFreeSpaceDelegate GetDiskFreeSpace;
            public FuserDevice.GetVolumeInformationDelegate GetVolumeInformation;
            public FuserDevice.MountDelegate Mount;
            public FuserDevice.UnmountDelegate Unmount;
            public FuserDevice.GetFileSecurityDelegate GetFileSecurity;
            public FuserDevice.SetFileSecurityDelegate SetFileSecurity;            
        }


        [StructLayout(LayoutKind.Sequential, CharSet = CharSet.Auto, Pack = 4)]
        public struct FUSER_OPTIONS {
            public ushort Version;
            public ushort ThreadCount; // number of threads to be used
            public uint Options;
            public ulong GlobalContext; // must not be used
            [MarshalAs(UnmanagedType.LPWStr)]
            public string MountPoint;
        }



        [StructLayout(LayoutKind.Sequential, Pack = 4)]
        public struct FUSER_FILE_INFO {
            // TODO: Completely revise with driver / check values if necessary / check in driver
            public ulong Context; // TODO: change datatype, long or int
            public ulong FuserContext;
            public IntPtr FuserOptions;
            public uint ProcessId;
            public byte IsDirectory;
            public byte DeleteOnClose;
            public byte PagingIo;
            public byte SynchronousIo;
            public byte Nocache;
            public byte WriteToEndOfFile;
        }
        
        // TODO: Check which one is needed for what and adjust DriveMounter.Mount() too.
        // -> change method DriveMounter.Mount()
        public const int FUSER_SUCCESS = 0;
        public const int FUSER_ERROR = -1; // General Error
        public const int FUSER_DRIVE_LETTER_ERROR = -2; // Bad Drive letter
        public const int FUSER_DRIVER_INSTALL_ERROR = -3; // Can't install driver
        public const int FUSER_START_ERROR = -4; // Driver something wrong
        public const int FUSER_MOUNT_ERROR = -5; // Can't assign drive letter
        public const int FUSER_MOUNT_POINT_ERROR = -6; /* Mount point is invalid */

        public const uint FUSER_OPTION_DEBUG = 1;
        public const uint FUSER_OPTION_STDERR = 2;
        public const uint FUSER_OPTION_ALT_STREAM = 4; // Alternate Data Streams -> File:Secondfile
        public const uint FUSER_OPTION_KEEP_ALIVE = 8;
        public const uint FUSER_OPTION_NETWORK = 16;
        public const uint FUSER_OPTION_REMOVABLE = 32;
        public const uint FUSER_OPTION_HEARTBEAT = 256;





        // set by the operating system: TODO: check who sets this and defines the parameters
        public delegate int FILL_FIND_DATA(
            ref WIN32_FIND_DATA rawFindData,
            ref FuserDefinition.FUSER_FILE_INFO rawHFile);



        // set by the operating system:
        [StructLayout(LayoutKind.Sequential, Pack = 4)]
        public struct BY_HANDLE_FILE_INFORMATION {
            public uint dwFileAttributes;
            public ComTypes.FILETIME ftCreationTime;
            public ComTypes.FILETIME ftLastAccessTime;
            public ComTypes.FILETIME ftLastWriteTime;
            public uint dwVolumeSerialNumber;
            public uint nFileSizeHigh;
            public uint nFileSizeLow;
            public uint dwNumberOfLinks;
            public uint nFileIndexHigh;
            public uint nFileIndexLow;
        }


        // set by the operating system:
        [Flags]
        public enum SECURITY_INFORMATION : uint {
            OWNER_SECURITY_INFORMATION = 0x00000001,
            GROUP_SECURITY_INFORMATION = 0x00000002,
            DACL_SECURITY_INFORMATION = 0x00000004,
            SACL_SECURITY_INFORMATION = 0x00000008,
            UNPROTECTED_SACL_SECURITY_INFORMATION = 0x10000000,
            UNPROTECTED_DACL_SECURITY_INFORMATION = 0x20000000,
            PROTECTED_SACL_SECURITY_INFORMATION = 0x40000000,
            PROTECTED_DACL_SECURITY_INFORMATION = 0x80000000
        }

        // set by the operating system:
        [StructLayoutAttribute(LayoutKind.Sequential, Pack = 4)]
        public struct SECURITY_DESCRIPTOR {
            public byte revision;
            public byte size;
            public short control;
            public IntPtr owner;
            public IntPtr group;
            public IntPtr sacl;
            public IntPtr dacl;
        }

        // set by the operating system:
        public const uint FILE_READ_DATA = 0x0001;
        public const uint FILE_READ_ATTRIBUTES = 0x0080;
        public const uint FILE_READ_EA = 0x0008;
        public const uint FILE_WRITE_DATA = 0x0002;
        public const uint FILE_WRITE_ATTRIBUTES = 0x0100;
        public const uint FILE_WRITE_EA = 0x0010;

        // set by the operating system:
        public const uint FILE_SHARE_READ = 0x00000001;
        public const uint FILE_SHARE_WRITE = 0x00000002;
        public const uint FILE_SHARE_DELETE = 0x00000004;

        // set by the operating system:
        public const uint CREATE_NEW = 1;
        public const uint CREATE_ALWAYS = 2;
        public const uint OPEN_EXISTING = 3;
        public const uint OPEN_ALWAYS = 4;
        public const uint TRUNCATE_EXISTING = 5;



        // set by the operating system:
        public enum FileSystemFlags : uint {
            FILE_CASE_SENSITIVE_SEARCH = 0x00000001,
            FILE_CASE_PRESERVED_NAMES = 0x00000002,
            FILE_UNICODE_ON_DISK = 0x00000004,            
        }
        



        // set by the operating system:
        [StructLayout(LayoutKind.Sequential, CharSet = CharSet.Auto, Pack = 4)]
        public struct WIN32_FIND_DATA {
            public FileAttributes dwFileAttributes;
            public ComTypes.FILETIME ftCreationTime;
            public ComTypes.FILETIME ftLastAccessTime;
            public ComTypes.FILETIME ftLastWriteTime;
            public uint nFileSizeHigh;
            public uint nFileSizeLow;
            public uint dwReserved0;
            public uint dwReserved1;
            [MarshalAs(UnmanagedType.ByValTStr, SizeConst = 260)]
            public string cFileName;
            [MarshalAs(UnmanagedType.ByValTStr, SizeConst = 14)]
            public string cAlternateFileName;
        }

        


    }
}
