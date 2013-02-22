using System;
using System.IO;

namespace FuserNet
{
    /// <summary>
    /// All methods are thread-save, but this does not apply to parameters.
    /// </summary>
    public interface IFuserFilesystemFile : IFuserFilesystemItem
    {
        long Length{ get; }

        Stream FileOpen(FileAccess access, FileShare share);
        void FileClose(Stream filedata);
    }
}
