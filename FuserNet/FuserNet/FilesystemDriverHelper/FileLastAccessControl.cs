using System;

namespace FuserNet {
    public class FileLastAccessControl {

        private DateTime pLastWriteTime;
        private DateTime pLastAccessTime;
        private bool pLastWriteSet;
        private bool pLastAccessSet;

        public FileLastAccessControl() {
            Rest();
        }

        public bool IsLastWriteSet     { get { return this.pLastWriteSet;   }}
        public bool IsLastAccessSet    { get { return this.pLastAccessSet;  }}
        public DateTime LastWriteTime  { get { return this.pLastWriteTime;  }}
        public DateTime LastAccessTime { get { return this.pLastAccessTime; }}                

        public void Rest() {
            this.pLastWriteTime = DateTime.MinValue;
            this.pLastAccessTime = DateTime.MinValue;
            this.pLastWriteSet = false;
            this.pLastAccessSet = false;
        }

        public void SetLastWrite() {
            this.pLastWriteTime = DateTime.Now;
            this.pLastWriteSet = true;
        }
        public void SetLastAccess() {
            this.pLastAccessTime = DateTime.Now;
            this.pLastAccessSet = true;                
        }        
    }
}
