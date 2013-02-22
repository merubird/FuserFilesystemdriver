using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;

namespace FuserNet {


    public enum Win32Returncode : int {
        /// <summary>
        /// The operation completed successfully.
        /// </summary>
        SUCCESS = 0x00000000,

        /// <summary>
        /// The system cannot find the file specified.
        /// </summary>
        ERROR_FILE_NOT_FOUND = 0x00000002,

        /// <summary>
        /// The system cannot find the path specified.
        /// </summary>
        ERROR_PATH_NOT_FOUND = 0x00000003,

        /// <summary>
        /// Access is denied.
        /// </summary>
        ERROR_ACCESS_DENIED = 0x00000005,

        /// <summary>
        /// The device is not ready.
        /// </summary>
        ERROR_NOT_READY = 0x00000015,

        /// <summary>
        /// The process cannot access the file because it is being used by another process.
        /// </summary>
        ERROR_SHARING_VIOLATION = 0x00000020,

        /// <summary>
        /// The process cannot access the file because another process has locked a portion of the file.
        /// </summary>
        ERROR_LOCK_VIOLATION = 0x00000021,

        /// <summary>
        /// The file name, directory name, or volume label syntax is incorrect.
        /// </summary>
        ERROR_INVALID_NAME = 0x0000007B,

        /// <summary>
        /// The directory is not empty.
        /// </summary>
        ERROR_DIR_NOT_EMPTY = 0x00000091,

        /// <summary>
        /// Cannot create a file when that file already exists.
        /// </summary>
        ERROR_ALREADY_EXISTS = 0x000000B7,

        /// <summary>
        /// The oplock request is denied.
        /// </summary>
        ERROR_OPLOCK_NOT_GRANTED = 0x0000012C,



        // TODO: Support error codes
        //Win32 Error Codes
        //ERROR_CANNOT_MAKE = 82,   
        //test = 170,        
        //ERROR_DISK_FULL    



        /// <summary>
        /// Default error for all unknown errors
        /// </summary>
        DEFAULT_UNKNOWN_ERROR = ERROR_ACCESS_DENIED // TODO: put the original standard error from the driver here

    }
}
