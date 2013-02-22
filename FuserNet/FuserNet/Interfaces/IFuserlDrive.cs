using System;

namespace FuserNet
{
    public interface IFuserlDrive
    {
        IFuserFilesystemDirectory Root { get; }

        
        
        string Volumelabel { get; }
        string Filesystem { get; }
        uint Serialnumber { get; }

        void Mounted(string MountPoint, string DeviceName);
        void Unmounted(FuserMountFinishStatus returncode);

        ulong DiskspaceTotalBytes { get; }
        ulong DiskspaceFreeBytes { get; }
    }
}
