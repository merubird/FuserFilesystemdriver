using System;
using System.IO;

namespace FuserNet
{
    /// <summary>
    /// All methods are thread-save, but this does not apply to parameters.
    /// </summary>
    public interface IFuserFilesystemDirectory : IFuserFilesystemItem
    {
        IFuserFilesystemDirectory Parent { get; }
        IFuserFilesystemItem GetItem(string itemname);
        IFuserFilesystemItem[] GetContentList();
        
        void CreateDirectory(string directoryname);
        void CreateFile(string filename, FileAttributes newAttributes);

        void Delete(IFuserFilesystemItem item);
        bool DeletionAllow(IFuserFilesystemItem item);

        void MoveTo(IFuserFilesystemItem item, IFuserFilesystemDirectory destination, string newname);
    }
}
