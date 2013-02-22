using System;
using System.IO;
using FuserNet;

namespace FuserLowlevelDriver {

    [Serializable]
    [Flags]
    internal enum FuserFileAccess {
        None = 0,
        Read = 1,
        Write = 2,
        ReadWrite = 3
    }


    internal class FuserFileHandler {        
        private string pFilename;
        private FileHandler pFileHandle;
        private bool pIsDirectory;        
        private bool pDeleteOnClose;
        
        public FuserFileHandler(string Filename, FileHandler SetFileHandler, bool SetIsDirectory, bool SetDeleteOnClose) {
            this.pFileHandle = SetFileHandler;
            this.pFilename = Filename;       
            this.pIsDirectory = SetIsDirectory;
            this.pDeleteOnClose = SetDeleteOnClose;        
        }

        public string      filename      { get {return this.pFilename;      }}
        public FileHandler fileHandle    { get {return this.pFileHandle;    }}
        public bool        DeleteOnClose { get {return this.pDeleteOnClose; }}
            
        public bool IsDirectory {
            get {
                return this.pIsDirectory;
            }
            set {
                this.pIsDirectory = value;
            }
        }
    }



    internal class FuserFileInformation
    {
        public string Filename;
        public long Length;        

        public FileAttributes Attributes;

        public DateTime CreationTime;
        public DateTime LastAccessTime;
        public DateTime LastWriteTime;

        public FuserFileInformation() {
            this.Attributes = 0;
            this.Length = 0;
            this.CreationTime = DateTime.MinValue;
            this.LastAccessTime = DateTime.MinValue;
            this.LastWriteTime = DateTime.MinValue;
            this.Filename = "";
        }
    }


    


}
