using System;
using System.IO;

namespace FuserNet {
    public class FileLockItem {

        private bool pIsDirectory;
        private bool pIsOpLock;

        private bool pAccessRead;
        private bool pAccessWrite;
        private FileShare pShare;

        private long pOPLockOffset;
        private long pOPLockLength;
        
        public FileLockItem(bool IsDirectory, bool AccessRead, bool AccessWrite, FileShare share) {
            this.pIsDirectory = IsDirectory;
            this.pIsOpLock = false;

            this.pAccessRead = AccessRead;
            this.pAccessWrite = AccessWrite;
            this.pShare = share;

            this.pOPLockOffset = 0;
            this.pOPLockLength = 0;
        }
        
        public FileLockItem(long offset, long length) {
            // Lock on a specific area in a file:
            this.pIsDirectory = false;
            this.pIsOpLock = true;

            this.pAccessRead = false;
            this.pAccessWrite = false;
            this.pShare = FileShare.None;

            this.pOPLockOffset = offset;
            this.pOPLockLength = length;
        }

        public bool AccessModeWrite { get { return this.pAccessWrite;  }}
        public bool AccessModeRead  { get { return this.pAccessRead;   }}
        public FileShare Share      { get { return this.pShare;        }}
        public bool IsDirectory     { get { return this.pIsDirectory;  }}
        public bool IsOpLock        { get { return this.pIsOpLock;     }}
        public long OPLockOffset    { get { return this.pOPLockOffset; }}
        public long OPLockLength    { get { return this.pOPLockLength; }}
        

        private bool CheckShareOPLockIsPermit(FileLockItem otherRule) {
            if (this.pIsOpLock != otherRule.IsOpLock) {
                return true; // OPlock must always be valid with another rule, otherwise no OPLocks can be registered.
            }
            if (!this.pIsOpLock) {
                return true; // Should never occur anyway.
            }

            return CheckOPLockRange(otherRule.OPLockOffset, otherRule.OPLockLength);
        }

        public bool CheckOPLockRange(long offset, long length) { 
            if (!this.pIsOpLock) {
                return false; // This item is not an OPLock
            }

            long endeNeu;
            long endeBestehend;

            endeNeu = (offset + length);
            endeBestehend = (this.pOPLockOffset + this.pOPLockLength);
            

            if (offset >= this.pOPLockOffset && offset < endeBestehend) {                
                return false;
            }
            if (this.pOPLockOffset >= offset && this.pOPLockOffset < endeNeu) {                
                return false;
            }
            
            if (endeNeu > this.pOPLockOffset && endeNeu < endeBestehend) {                
                return false;
            }
            if (endeBestehend > offset && endeBestehend < endeNeu) {                
                return false;
            }
            
            return true;
        }

        public bool CheckShareRuleIsPermit(FileLockItem otherRule) {
            if (pIsDirectory != otherRule.IsDirectory) {
                return false;
            }
            if (this.pIsDirectory) {
                return true; // As long as CreateDirectory and OpenDirectory do not return correct FileShare/FileAccess values, this workaround must remain active.
            }

            if (this.pIsOpLock || otherRule.IsOpLock) {
                return CheckShareOPLockIsPermit(otherRule);
            }

            FileShare shareO = otherRule.Share;
            
            bool shareReadT = false;
            bool shareWriteT = false;            

            bool shareReadO = false;
            bool shareWriteO = false;            

            if (pShare.HasFlag(FileShare.Read       )){shareReadT=true;}
            if (pShare.HasFlag(FileShare.Write      )){shareWriteT=true;}
            if (pShare.HasFlag(FileShare.ReadWrite  )){shareReadT=true; shareWriteT=true;}            

            if (shareO.HasFlag(FileShare.Read       )){shareReadO = true; }
            if (shareO.HasFlag(FileShare.Write      )){shareWriteO = true; }
            if (shareO.HasFlag(FileShare.ReadWrite  )){shareReadO = true; shareWriteO = true; }                      

            if (shareReadT == false && shareWriteT == false) {
                if (otherRule.AccessModeWrite || otherRule.AccessModeRead) {
                    return false; // Authorised Share: none
                }
            }
            if (shareReadO == false && shareWriteO == false) {
                if (this.AccessModeWrite || this.AccessModeRead) {
                    return false; // Authorised Share: none
                }
            }


            if (shareReadT == true && shareWriteT == false) {
                if (otherRule.AccessModeWrite == true) {
                    return false;
                }
            }
            if (shareReadO == true && shareWriteO == false) {
                if (this.AccessModeWrite == true) {
                    return false;
                }
            }


            if (shareReadT == false && shareWriteT == true) {
                if (otherRule.AccessModeRead == true) {
                    return false;
                }
            }
            if (shareReadO == false && shareWriteO == true) {
                if (this.AccessModeRead == true) {
                    return false;
                }
            }

            
            if (shareReadT == true && shareWriteT == true) {                
            }
            if (shareReadO == true && shareWriteO == true) {
            }            


            return true;
        }



    }
}
