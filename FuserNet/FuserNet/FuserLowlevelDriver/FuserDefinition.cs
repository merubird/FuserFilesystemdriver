using System;
using System.Runtime.InteropServices;
using System.IO;
using ComTypes = System.Runtime.InteropServices.ComTypes;

namespace FuserLowlevelDriver {
    internal static class FuserDefinition {
        

        public enum FUSER_EVENT : int {
            FUSER_EVENT_MOUNT = 1,
            FUSER_EVENT_UNMOUNT = 2,
            FUSER_EVENT_GET_VOLUME_INFORMATION = 3,
            FUSER_EVENT_GET_DISK_FREESPACE = 4,
            FUSER_EVENT_CREATE_FILE = 5,
            FUSER_EVENT_CREATE_DIRECTORY = 6,
            FUSER_EVENT_OPEN_DIRECTORY = 7,
            FUSER_EVENT_CLOSE_FILE = 8,
            FUSER_EVENT_CLEANUP = 9,
            FUSER_EVENT_READ_FILE = 10,
            FUSER_EVENT_WRITE_FILE = 11,
            FUSER_EVENT_FLUSH_FILEBUFFERS = 12,
            FUSER_EVENT_FIND_FILES = 13,
            FUSER_EVENT_FIND_FILES_WITH_PATTERN = 14,
            FUSER_EVENT_GET_FILE_INFORMATION = 15,
            FUSER_EVENT_SET_FILE_ATTRIBUTES = 16,
            FUSER_EVENT_SET_FILE_TIME = 17,
            FUSER_EVENT_SET_End_OF_FILE = 18,
            FUSER_EVENT_SET_ALLOCATIONSIZE = 19,
            FUSER_EVENT_LOCK_FILE = 20,
            FUSER_EVENT_UNLOCK_FILE = 21,
            FUSER_EVENT_DELETE_FILE = 22,
            FUSER_EVENT_DELETE_DIRECTORY = 23,
            FUSER_EVENT_MOVE_FILE = 24,
            FUSER_EVENT_GET_FILESECURITY = 25,
            FUSER_EVENT_SET_FILESECURITY = 26,
        }



 
        [StructLayout(LayoutKind.Sequential, Pack = 4)]
        public struct FUSER_FILE_INFO {            
            public ulong FuserContext;  //only for internal (driver) use. don't touch this.
            public ulong Context; // TODO: change datatype, long or int
            public uint ProcessId;      //process id for the thread that originally requested a given I/O operation

            public byte IsDirectory;        // requesting a directory file
            public byte DeleteOnClose;      // Delete on when "cleanup" is called
            public byte PagingIo;           // Read or write is paging IO.
            public byte SynchronousIo;      // Read or write is synchronous IO.
            public byte Nocache;            
            public byte WriteToEndOfFile;   // If true, write to the current end of file instead of Offset parameter.
        }


        
        
        // -> change method FuserDriverMounter.pvStart()
        public const int FUSER_DEVICEMOUNT_VERSION_ERROR         = -1; // Version incompatible with this version
        public const int FUSER_DEVICEMOUNT_EVENT_LOAD_ERROR		 = -2; // Error while loading the events.
        public const int FUSER_DEVICEMOUNT_BAD_MOUNT_POINT_ERROR = -3; // mountpoint invalid
        public const int FUSER_DEVICEMOUNT_DRIVER_INSTALL_ERROR	 = -4; // driver not installed
        public const int FUSER_DEVICEMOUNT_DRIVER_START_ERROR	 = -5; // driver not started (FuserStart-method)
        public const int FUSER_DEVICEMOUNT_MOUNT_ERROR			 = -6; // device can't mount (FuserDeviceAgent)	
        public const int FUSER_DEVICEMOUNT_SUCCESS				 =  0; // device successfully unmount
                
        public const uint FUSER_MOUNT_PARAMETER_FLAG_DEBUG		=	1;   // ouput debug message
        public const uint FUSER_MOUNT_PARAMETER_FLAG_STDERR		=	2;   // ouput debug message to stderr
        public const uint FUSER_MOUNT_PARAMETER_FLAG_USEADS		=	4;	// use Alternate Data Streams (e.g. C:\TEMP\TEST.TXT:ADS.file)        
        public const uint FUSER_MOUNT_PARAMETER_FLAG_TYPE_REMOVABLE = 8;  //  DeviceType: Removable-Device	





        
        [StructLayout(LayoutKind.Sequential, CharSet = CharSet.Auto, Pack = 4)]
        public struct FUSER_MOUNT_PARAMETER { // TODO:
            public ushort Version;
            [MarshalAs(UnmanagedType.LPWStr)] public string MountPoint;
            public uint Flags;
            public ushort ThreadsCount; // number of threads to be used
            public FuserDevice.EventLoaderDelegate EventLoaderFunc;
        }
   



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
