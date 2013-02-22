using System;

namespace FuserNet
{
    /// <summary>
    /// All methods are thread-save, but this does not apply to parameters.
    /// </summary>
    public interface IFuserFilesystemItem
    {
        FileLockManager Filelock { get; }
        string Name { get; }
                
        bool isArchive { get; set; }
        bool isReadOnly { get; set; }
        bool isHidden { get; set; }
        bool isSystem { get; set; }

        DateTime CreationTime { get; set; }
        DateTime LastAccessTime { get; set; }
        DateTime LastWriteTime { get; set; }
    }
}
