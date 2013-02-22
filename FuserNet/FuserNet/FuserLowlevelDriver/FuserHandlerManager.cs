using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;

using FuserNet;

namespace FuserLowlevelDriver {
    internal class FuserHandlerManager {
        private const int LIMIT_MIN_COUNT_RESERVE = 1024; // Sets the lower limit for the reserve memory with the unused IDs, so many always remain in the memory.
        private const double LIMIT_MIN_SECONDS_RESERVE = 30; // Indicates the minimum number of seconds a closed ID is in the reserve memory.


        private List<PVCStoredHandle> handlers; // Lock on this object
        private Queue<PVCStoredHandle> reserve;
        private int ListHandlerMaxID;
                
        public FuserHandlerManager() {
            this.handlers = new List<PVCStoredHandle>();
            this.reserve = new Queue<PVCStoredHandle>();
            this.handlers.Clear();
            this.handlers.Add(null); // always keep the element ID 0 empty.
            this.ListHandlerMaxID = 0;            
        }


        private FuserFileHandler CreateFileHandler(string filename, FileHandler fileHandle, FuserDefinition.FUSER_FILE_INFO rawHFile) {
            return new FuserFileHandler(filename, fileHandle, (rawHFile.IsDirectory == 1)  , (rawHFile.DeleteOnClose == 1) ); 
        }

        public FuserFileHandler RegisterFileHandler(string filename, ref FuserDefinition.FUSER_FILE_INFO rawHFile) {
            FileHandler fh = new FileHandler(filename);

            lock (this.handlers) {
                //int n = this.handlers.Count + 1;
                bool InsertNewItem = true;

                if (this.reserve.Count > 0 && this.reserve.Count >= LIMIT_MIN_COUNT_RESERVE) {
                    // try to take an item from the reserve list.
                    PVCStoredHandle sh = this.reserve.Peek();

                    if (sh == null) {
                        System.Diagnostics.Debugger.Break();
                    }
                    
                    if (sh.CheckReInsert()) {
                        sh = this.reserve.Dequeue();
                        int index = sh.GetHandleID;
                        if (this.handlers[index] != null) {                             
                            return null;
                        }
                        this.handlers[index] = new PVCStoredHandle(index, fh);
                       
                        if (this.handlers[index].GetHandleID != index) {                            
                            return null;
                        }
                        rawHFile.Context = (ulong)index; // TODO: remove casting
                        InsertNewItem = false;                         
                    }
                }


                if (InsertNewItem) {
                    ListHandlerMaxID++;
                    this.handlers.Add(new PVCStoredHandle(ListHandlerMaxID, fh));
                    if (this.handlers[ListHandlerMaxID].GetHandleID != ListHandlerMaxID) {                        
                        return null;
                    }
                    rawHFile.Context = (ulong)ListHandlerMaxID; // TODO: remove casting
                }


            }
            return CreateFileHandler(filename, fh, rawHFile);
        }


        public FuserFileHandler GetFileHandler(string filename, ref FuserDefinition.FUSER_FILE_INFO rawHFile) {

            FuserFileHandler ret = null;
            if (rawHFile.Context != 0) {
                int index = (int)rawHFile.Context; // TODO: remove casting
                lock (this.handlers) {
                    if (index > 0 && index <= this.ListHandlerMaxID) {
                        PVCStoredHandle sh = this.handlers[index];                        
                        if (sh != null) {                            
                            ret = CreateFileHandler(filename, sh.FileHandle, rawHFile);
                        }
                    }
                }
            }


            if (ret == null) {
                ret = RegisterFileHandler(filename,ref  rawHFile);
            }
            return ret; 
        }

        public void RemoveFileHandler(ref FuserDefinition.FUSER_FILE_INFO rawHFile) {
            if (rawHFile.Context != 0) {
                int index = (int)rawHFile.Context; // TODO: remove casting
                lock (this.handlers) {
                    rawHFile.Context = 0; // immediately removes the reference
                    if (index > 0 && index <= this.ListHandlerMaxID) {
                        PVCStoredHandle sh = this.handlers[index];
                        this.handlers[index] = null; // Removes saved handles
                        if (sh != null) {
                            sh.Close();
                            this.reserve.Enqueue(sh);
                        }
                    }
                }
            }
        }

        private class PVCStoredHandle {
            private int HandleID;
            private FileHandler pFileHandle;
            private bool IsValid = true;
            private DateTime CloseDate;

            public PVCStoredHandle(int HandleID, FileHandler SetFileHandler) {
                this.HandleID = HandleID;
                this.pFileHandle = SetFileHandler;
                this.IsValid = true;
            }

            public int GetHandleID { get { return this.HandleID; }}

            public FileHandler FileHandle {
                get {                    
                    if (!this.IsValid) {
                        return null;
                    }
                    return this.pFileHandle;
                }
            }

            public bool CheckReInsert() {
                // checks whether the element ID can already be added back to the list of handles
                try {
                    TimeSpan ts = DateTime.Now.Subtract(this.CloseDate);
                    Double d = ts.TotalSeconds;
                    if (d > LIMIT_MIN_SECONDS_RESERVE) {
                        return true;
                    }
                } catch {
                    return true;
                }
                return false;
            }

            public void Close() {                
                this.CloseDate = DateTime.Now;
                this.IsValid = false;
            }
        }






    }
}
