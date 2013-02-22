using System;
using System.Collections.Generic;

namespace FuserNet
{
    //Error list: http://msdn.microsoft.com/en-us/library/cc231199.aspx
    
        
    public class FSException : Exception {
        private Win32Returncode pError;
        
        public FSException(Win32Returncode errorcode) :base(errorcode.ToString()) {            
            this.pError = errorcode;
            if (this.pError == Win32Returncode.SUCCESS) {
                this.pError = Win32Returncode.DEFAULT_UNKNOWN_ERROR;
            }
        }
        public Win32Returncode Error {
            get {
                return this.pError;
            }
        }        
    }

}
