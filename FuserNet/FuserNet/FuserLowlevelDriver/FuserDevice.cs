using System;
using System.Collections.Generic;
using System.Text;
using System.IO;
using System.Runtime.InteropServices;
using FuserNet;
using ComTypes = System.Runtime.InteropServices.ComTypes;

namespace FuserLowlevelDriver {
    internal class FuserDevice {
        private IFuserFilesystemDevice fsDevice;        
        private FuserHeartbeat heartbeat;
        private FuserHandlerManager hManager;

        private string volumelabel;
        private string filesystem;
        private uint serialnumber;

        private FuserNet.ACLProxy aclProxy = new FuserNet.ACLProxy(); // TODO: remove
           
        public FuserDevice(IFuserFilesystemDevice fsDevice, string Volumelabel, string Filesystem, uint Serialnumber) {
            this.fsDevice = fsDevice;
            this.heartbeat = null;
            this.hManager = new FuserHandlerManager();

            this.volumelabel = Volumelabel;
            this.filesystem = Filesystem;
            this.serialnumber = Serialnumber;                        
        }
      
        public FuserDefinition.FUSER_OPERATIONS FuserLinkedDelegates{
            get {
                FuserDefinition.FUSER_OPERATIONS linkedDelegates = new FuserDefinition.FUSER_OPERATIONS();

                linkedDelegates.CreateFile = this.CreateFile;
                linkedDelegates.OpenDirectory = this.OpenDirectory;
                linkedDelegates.CreateDirectory = this.CreateDirectory;
                linkedDelegates.Cleanup = this.Cleanup;
                linkedDelegates.CloseFile = this.CloseFile;
                linkedDelegates.ReadFile = this.ReadFile;
                linkedDelegates.WriteFile = this.WriteFile;
                linkedDelegates.FlushFileBuffers = this.FlushFileBuffers;
                linkedDelegates.GetFileInformation = this.GetFileInformation;
                linkedDelegates.FindFiles = this.FindFiles;
                linkedDelegates.SetFileAttributes = this.SetFileAttributes;
                linkedDelegates.SetFileTime = this.SetFileTime;
                linkedDelegates.DeleteFile = this.DeleteFile;
                linkedDelegates.DeleteDirectory = this.DeleteDirectory;
                linkedDelegates.MoveFile = this.MoveFile;
                linkedDelegates.SetEndOfFile = this.SetEndOfFile;
                linkedDelegates.SetAllocationSize = this.SetAllocationSize;
                linkedDelegates.LockFile = this.LockFile;
                linkedDelegates.UnlockFile = this.UnlockFile;
                linkedDelegates.GetDiskFreeSpace = this.GetDiskFreeSpace;
                linkedDelegates.GetVolumeInformation = this.GetVolumeInformation;
                linkedDelegates.Mount = this.Mount;
                linkedDelegates.Unmount = this.Unmount;
                linkedDelegates.GetFileSecurity = this.GetFileSecurity;
                linkedDelegates.SetFileSecurity = this.SetFileSecurity;

                return linkedDelegates;
            }
        }

        public void HeartbeatStop() {
            // This method is not always executed, (programme abort)
            if (this.heartbeat != null) {
                this.heartbeat.Stop();
            }
        }

        private int ConvReturnCodeToInt(Win32Returncode rcode) {
            // TODO: remove this method
            return -(int)rcode;
        }

        private string GetFilename(IntPtr filename) {
            return Marshal.PtrToStringUni(filename);
        }


        private void ConvLongToRAW(long input, out int dwHigh,out int dwLow) {
            dwHigh = (int)(input >> 32);
            dwLow =  (int)(input & 0xffffffff);
        }

        private ComTypes.FILETIME ConvLongToFILETIME(long input) {
            ComTypes.FILETIME ret = new ComTypes.FILETIME();

            int h;
            int l;

            ConvLongToRAW(input, out h, out l);

            ret.dwHighDateTime = h;
            ret.dwLowDateTime = l;

            return ret;
        }

        private void ConvertFileInfoToRAW(FuserFileInformation fi, ref FuserDefinition.BY_HANDLE_FILE_INFORMATION rawHandleFileInformation) {
            // TODO: boundary values and behaviour testing
            rawHandleFileInformation.dwFileAttributes = (uint)fi.Attributes;

            rawHandleFileInformation.ftCreationTime = ConvLongToFILETIME(fi.CreationTime.ToFileTime());
            rawHandleFileInformation.ftLastAccessTime = ConvLongToFILETIME(fi.LastAccessTime.ToFileTime());
            rawHandleFileInformation.ftLastWriteTime = ConvLongToFILETIME(fi.LastWriteTime.ToFileTime());
                        
            //ConvLongToRAW(fi.Length, out rawHandleFileInformation.nFileSizeHigh, out rawHandleFileInformation.nFileSizeLow);
            // TODO: Check if the parameter nFileSizeHigh/nFileSizeLow also works as INT, then only the above line leave it.
            int h;
            int l;
            ConvLongToRAW(fi.Length, out h, out l); 
            rawHandleFileInformation.nFileSizeHigh = (uint) h;
            rawHandleFileInformation.nFileSizeLow = (uint)l;            
        }

        private DateTime ConvDateTimeFromRAW(ComTypes.FILETIME filetime) {

            if (filetime.dwHighDateTime == -1 || filetime.dwLowDateTime == -1) {
                return DateTime.MinValue;
            } else {
                long newTime = ((long)filetime.dwHighDateTime << 32) + (uint)filetime.dwLowDateTime;
                if (newTime == 0) {
                    return DateTime.MinValue;
                } else {
                    return DateTime.FromFileTime(newTime);
                }
            }
        }




        public delegate int CreateFileDelegate(IntPtr rawFilname, uint rawAccessMode, uint rawShare, uint rawCreationDisposition, uint rawFlagsAndAttributes, ref FuserDefinition.FUSER_FILE_INFO rawHFile);
        public          int CreateFile        (IntPtr rawFilname, uint rawAccessMode, uint rawShare, uint rawCreationDisposition, uint rawFlagsAndAttributes, ref FuserDefinition.FUSER_FILE_INFO rawHFile){
            try {
                FuserFileHandler hFile = this.hManager.RegisterFileHandler(GetFilename(rawFilname), ref rawHFile);


                FuserFileAccess access = FuserFileAccess.None;
                FileAttributes newFileAttr;

                try {
                    newFileAttr = (FileAttributes)rawFlagsAndAttributes;
                } catch {
                    newFileAttr = 0;
                }

                FileShare share = FileShare.None;
                FileMode mode = FileMode.Open;
                FileOptions options = FileOptions.None;


                if ((rawAccessMode & FuserDefinition.FILE_READ_DATA) != 0 && (rawAccessMode & FuserDefinition.FILE_WRITE_DATA) != 0) {
                    access = FuserFileAccess.ReadWrite;
                } else if ((rawAccessMode & FuserDefinition.FILE_WRITE_DATA) != 0) {
                    access = FuserFileAccess.Write;
                } else if ((rawAccessMode & FuserDefinition.FILE_READ_DATA) != 0) {
                    access = FuserFileAccess.Read;
                }


                if ((rawShare & FuserDefinition.FILE_SHARE_READ) != 0) {
                    share = FileShare.Read;
                }

                if ((rawShare & FuserDefinition.FILE_SHARE_WRITE) != 0) {
                    share |= FileShare.Write;
                }

                if ((rawShare & FuserDefinition.FILE_SHARE_DELETE) != 0) {
                    share |= FileShare.Delete;
                }

                switch (rawCreationDisposition) {
                    case FuserDefinition.CREATE_NEW:
                        mode = FileMode.CreateNew;
                        break;
                    case FuserDefinition.CREATE_ALWAYS:
                        mode = FileMode.Create;
                        break;
                    case FuserDefinition.OPEN_EXISTING:
                        mode = FileMode.Open;
                        break;
                    case FuserDefinition.OPEN_ALWAYS:
                        mode = FileMode.OpenOrCreate;
                        break;
                    case FuserDefinition.TRUNCATE_EXISTING:
                        mode = FileMode.Truncate;
                        break;
                }

                Win32Returncode ret = this.fsDevice.CreateFile(hFile, access, share, mode, options, newFileAttr);

                if (hFile.IsDirectory) {
                    rawHFile.IsDirectory = 1;
                    // TODO: directory problem
                    //rawFlagsAndAttributes |= 0x02000000;
                }

                if (ret != Win32Returncode.SUCCESS){
                    this.hManager.RemoveFileHandler(ref rawHFile);
                }

                return ConvReturnCodeToInt(ret);
            } catch (Exception e) {
                this.hManager.RemoveFileHandler(ref rawHFile);
                this.fsDevice.LogErrorMessage("CreateFile", e.Message);
                return ConvReturnCodeToInt(Win32Returncode.ERROR_FILE_NOT_FOUND);
            }
        }


        public delegate int OpenDirectoryDelegate(IntPtr rawFilename, ref FuserDefinition.FUSER_FILE_INFO rawHFile);
        public          int OpenDirectory        (IntPtr rawFilename, ref FuserDefinition.FUSER_FILE_INFO rawHFile){
            try {
                Win32Returncode ret = this.fsDevice.OpenDirectory(this.hManager.RegisterFileHandler(GetFilename(rawFilename), ref rawHFile));

                if (ret != Win32Returncode.SUCCESS) {
                    this.hManager.RemoveFileHandler(ref rawHFile);
                }

                return ConvReturnCodeToInt(ret);
            } catch (Exception e) {
                this.hManager.RemoveFileHandler(ref rawHFile);
                this.fsDevice.LogErrorMessage("OpenDirectory", e.Message);
                return ConvReturnCodeToInt(Win32Returncode.DEFAULT_UNKNOWN_ERROR);
            }
        }


        public delegate int CreateDirectoryDelegate(IntPtr rawFilename, ref FuserDefinition.FUSER_FILE_INFO rawHFile);
        public          int CreateDirectory        (IntPtr rawFilename, ref FuserDefinition.FUSER_FILE_INFO rawHFile){
            try {
                Win32Returncode ret = this.fsDevice.CreateDirectory(this.hManager.RegisterFileHandler(GetFilename(rawFilename), ref rawHFile));

                if (ret != Win32Returncode.SUCCESS) {
                    this.hManager.RemoveFileHandler(ref rawHFile);
                }

                return ConvReturnCodeToInt(ret);
            } catch (Exception e) {
                this.hManager.RemoveFileHandler(ref rawHFile);
                this.fsDevice.LogErrorMessage("Cleanup", e.Message);
                return ConvReturnCodeToInt(Win32Returncode.DEFAULT_UNKNOWN_ERROR);
            }
        }


        public delegate int CleanupDelegate(IntPtr rawFilename, ref FuserDefinition.FUSER_FILE_INFO rawHFile);
        public          int Cleanup        (IntPtr rawFilename, ref FuserDefinition.FUSER_FILE_INFO rawHFile){
            try {
                return ConvReturnCodeToInt(this.fsDevice.Cleanup(this.hManager.GetFileHandler(GetFilename(rawFilename), ref rawHFile)));
            } catch (Exception e) {
                this.fsDevice.LogErrorMessage("Cleanup", e.Message);
                return ConvReturnCodeToInt(Win32Returncode.DEFAULT_UNKNOWN_ERROR);
            }
        }

        public delegate int CloseFileDelegate(IntPtr rawFilename, ref FuserDefinition.FUSER_FILE_INFO rawHFile);
        public          int CloseFile        (IntPtr rawFilename, ref FuserDefinition.FUSER_FILE_INFO rawHFile){
            try {
                Win32Returncode ret = this.fsDevice.CloseFile(this.hManager.GetFileHandler(GetFilename(rawFilename), ref rawHFile));
                this.hManager.RemoveFileHandler(ref rawHFile);

                return ConvReturnCodeToInt(ret);
            } catch (Exception e) {
                this.fsDevice.LogErrorMessage("CloseFile", e.Message);
                return ConvReturnCodeToInt(Win32Returncode.DEFAULT_UNKNOWN_ERROR);
            }
        }


        
        public delegate int ReadFileDelegate(IntPtr rawFilename, IntPtr rawBuffer, uint rawBufferLength, ref uint rawReadLength, long rawOffset, ref FuserDefinition.FUSER_FILE_INFO rawHFile);
        public int          ReadFile        (IntPtr rawFilename, IntPtr rawBuffer, uint rawBufferLength, ref uint rawReadLength, long rawOffset, ref FuserDefinition.FUSER_FILE_INFO rawHFile){
            try {               
                byte[] buf = new byte[rawBufferLength];

                uint readLength = 0;
                Win32Returncode ret = this.fsDevice.ReadFile(this.hManager.GetFileHandler(GetFilename(rawFilename), ref rawHFile), buf, ref readLength, rawOffset);
                if (ret == Win32Returncode.SUCCESS) {
                    rawReadLength = readLength;
                    Marshal.Copy(buf, 0, rawBuffer, (int)rawBufferLength); // TODO: check if change from uint to int is possible for rawBufferLength, then remove casting
                }
                return ConvReturnCodeToInt(ret);

            } catch (Exception e) {
                this.fsDevice.LogErrorMessage("ReadFile", e.Message);
                return ConvReturnCodeToInt(Win32Returncode.DEFAULT_UNKNOWN_ERROR);
            }
        }


        public delegate int WriteFileDelegate(IntPtr rawFilename, IntPtr rawBuffer, uint rawNumberOfBytesToWrite, ref uint rawNumberOfBytesWritten, long rawOffset, ref FuserDefinition.FUSER_FILE_INFO rawHFile);
        public          int WriteFile        (IntPtr rawFilename, IntPtr rawBuffer, uint rawNumberOfBytesToWrite, ref uint rawNumberOfBytesWritten, long rawOffset, ref FuserDefinition.FUSER_FILE_INFO rawHFile){
            try {
                byte[] buf = new byte[rawNumberOfBytesToWrite];
                Marshal.Copy(rawBuffer, buf, 0, (int)rawNumberOfBytesToWrite); // TODO: check if change from uint to int is possible for rawNumberOfBytesToWrite, then remove casting

                uint bytesWritten = 0;
                Win32Returncode ret = this.fsDevice.WriteFile(this.hManager.GetFileHandler(GetFilename(rawFilename), ref rawHFile), buf, ref bytesWritten, rawOffset);
                if (ret == Win32Returncode.SUCCESS) {
                    rawNumberOfBytesWritten = bytesWritten;
                }
                return ConvReturnCodeToInt(ret);

            } catch (Exception e) {
                this.fsDevice.LogErrorMessage("WriteFile", e.Message);
                return ConvReturnCodeToInt(Win32Returncode.DEFAULT_UNKNOWN_ERROR);
            }
        }


        public delegate int FlushFileBuffersDelegate(IntPtr rawFilename, ref FuserDefinition.FUSER_FILE_INFO rawHFile);
        public          int FlushFileBuffers        (IntPtr rawFilename, ref FuserDefinition.FUSER_FILE_INFO rawHFile){
            try {
                return ConvReturnCodeToInt(this.fsDevice.FlushFileBuffers(this.hManager.GetFileHandler(GetFilename(rawFilename), ref rawHFile)));
            } catch (Exception e) {
                this.fsDevice.LogErrorMessage("FlushFileBuffers", e.Message);
                return ConvReturnCodeToInt(Win32Returncode.DEFAULT_UNKNOWN_ERROR);
            }
        }


        public delegate int GetFileInformationDelegate(IntPtr rawFilename, ref FuserDefinition.BY_HANDLE_FILE_INFORMATION rawHandleFileInformation, ref FuserDefinition.FUSER_FILE_INFO rawHFile);
        public          int GetFileInformation        (IntPtr rawFilename, ref FuserDefinition.BY_HANDLE_FILE_INFORMATION rawHandleFileInformation, ref FuserDefinition.FUSER_FILE_INFO rawHFile){
            try {                
                FuserFileInformation fi = new FuserFileInformation();

                Win32Returncode ret = this.fsDevice.GetFileInformation(this.hManager.GetFileHandler(GetFilename(rawFilename), ref rawHFile), fi);

                if (ret == Win32Returncode.SUCCESS) {
                    if (fi.CreationTime == DateTime.MinValue || fi.LastAccessTime == DateTime.MinValue || fi.LastWriteTime == DateTime.MinValue) {
                        ret = Win32Returncode.DEFAULT_UNKNOWN_ERROR;
                    }
                }

                if (ret == Win32Returncode.SUCCESS) {
                    ConvertFileInfoToRAW(fi, ref rawHandleFileInformation);
                }

                return ConvReturnCodeToInt(ret);            
            } catch (Exception e) {
                this.fsDevice.LogErrorMessage("GetFileInformation", e.Message);
                return ConvReturnCodeToInt(Win32Returncode.DEFAULT_UNKNOWN_ERROR);
            }            
        }

        

        public delegate int FindFilesDelegate(IntPtr rawFilename, IntPtr FunctionFillFindData, ref FuserDefinition.FUSER_FILE_INFO rawHFile);
        public          int FindFiles        (IntPtr rawFilename, IntPtr FunctionFillFindData, ref FuserDefinition.FUSER_FILE_INFO rawHFile){
            try {
                FuserDefinition.BY_HANDLE_FILE_INFORMATION rawFI = new FuserDefinition.BY_HANDLE_FILE_INFORMATION();

                List<FuserFileInformation> files = new List<FuserFileInformation>();
                Win32Returncode ret = this.fsDevice.FindFiles(this.hManager.GetFileHandler(GetFilename(rawFilename), ref rawHFile), files);

                FuserDefinition.FILL_FIND_DATA rawListAdd = (FuserDefinition.FILL_FIND_DATA)Marshal.GetDelegateForFunctionPointer(FunctionFillFindData, typeof(FuserDefinition.FILL_FIND_DATA)); // Function pointer

                if (ret == Win32Returncode.SUCCESS) {
                    foreach (FuserFileInformation fi in files) {                     
                        if (!(fi.CreationTime == DateTime.MinValue || fi.LastAccessTime == DateTime.MinValue || fi.LastWriteTime == DateTime.MinValue)) {                            
                            FuserDefinition.WIN32_FIND_DATA data = new FuserDefinition.WIN32_FIND_DATA();
                            
                            ConvertFileInfoToRAW(fi, ref rawFI);
                                                        
                            data.ftCreationTime =rawFI.ftCreationTime;                                                            
                            data.ftLastAccessTime =rawFI.ftLastAccessTime;                            
                            data.ftLastWriteTime = rawFI.ftLastWriteTime;                                                                                     
                            data.nFileSizeLow = rawFI.nFileSizeLow;
                            data.nFileSizeHigh = rawFI.nFileSizeHigh;
                            data.dwFileAttributes = fi.Attributes;
                                                            
                            data.cFileName = fi.Filename;

                            rawListAdd(ref data, ref rawHFile);
                        }
                    }
                }
                return ConvReturnCodeToInt(ret);
            } catch (Exception e) {
                this.fsDevice.LogErrorMessage("FindFiles", e.Message);
                return ConvReturnCodeToInt(Win32Returncode.DEFAULT_UNKNOWN_ERROR);
            }
        }



        public delegate int SetFileAttributesDelegate(IntPtr rawFilename, uint rawAttributes, ref FuserDefinition.FUSER_FILE_INFO rawHFile);
        public          int SetFileAttributes        (IntPtr rawFilename, uint rawAttributes, ref FuserDefinition.FUSER_FILE_INFO rawHFile){
            try {                
                FileAttributes attr = (FileAttributes)rawAttributes;
                return ConvReturnCodeToInt(this.fsDevice.SetFileAttributes(this.hManager.GetFileHandler(GetFilename(rawFilename), ref rawHFile), attr));

            } catch (Exception e) {
                this.fsDevice.LogErrorMessage("SetFileAttributes", e.Message);
                return ConvReturnCodeToInt(Win32Returncode.DEFAULT_UNKNOWN_ERROR);
            }
        }



        

        public delegate int SetFileTimeDelegate(IntPtr rawFilename, ref ComTypes.FILETIME rawCreationTime, ref ComTypes.FILETIME rawLastAccessTime, ref ComTypes.FILETIME rawLastWriteTime, ref FuserDefinition.FUSER_FILE_INFO rawHFile);
        public          int SetFileTime        (IntPtr rawFilename, ref ComTypes.FILETIME rawCreationTime, ref ComTypes.FILETIME rawLastAccessTime, ref ComTypes.FILETIME rawLastWriteTime, ref FuserDefinition.FUSER_FILE_INFO rawHFile){
            try {                                                        
                return ConvReturnCodeToInt(this.fsDevice.SetFileTime(this.hManager.GetFileHandler(GetFilename(rawFilename), ref rawHFile), ConvDateTimeFromRAW(rawCreationTime), ConvDateTimeFromRAW(rawLastAccessTime),  ConvDateTimeFromRAW(rawLastWriteTime)));
            } catch (Exception e) {
                this.fsDevice.LogErrorMessage("SetFileTime", e.Message);
                return ConvReturnCodeToInt(Win32Returncode.DEFAULT_UNKNOWN_ERROR);
            }
        }

        public delegate int DeleteFileDelegate(IntPtr rawFilename, ref FuserDefinition.FUSER_FILE_INFO rawHFile);
        public          int DeleteFile        (IntPtr rawFilename, ref FuserDefinition.FUSER_FILE_INFO rawHFile){
            try {
                return ConvReturnCodeToInt(this.fsDevice.DeleteFile(this.hManager.GetFileHandler(GetFilename(rawFilename), ref rawHFile)));

            } catch (Exception e) {
                this.fsDevice.LogErrorMessage("DeleteFile", e.Message);
                return ConvReturnCodeToInt(Win32Returncode.DEFAULT_UNKNOWN_ERROR);
            }
        }

        public delegate int DeleteDirectoryDelegate(IntPtr rawFilename, ref FuserDefinition.FUSER_FILE_INFO rawHFile);
        public          int DeleteDirectory        (IntPtr rawFilename, ref FuserDefinition.FUSER_FILE_INFO rawHFile){
            try {
                return ConvReturnCodeToInt(this.fsDevice.DeleteDirectory(this.hManager.GetFileHandler(GetFilename(rawFilename), ref rawHFile)));
            } catch (Exception e) {
                this.fsDevice.LogErrorMessage("DeleteDirectory", e.Message);
                return ConvReturnCodeToInt(Win32Returncode.DEFAULT_UNKNOWN_ERROR);
            }
        }


        public delegate int MoveFileDelegate(IntPtr rawFilename, IntPtr rawNewFilename, int rawReplaceIfExisting, ref FuserDefinition.FUSER_FILE_INFO rawHFile);
        public          int MoveFile        (IntPtr rawFilename, IntPtr rawNewFilename, int rawReplaceIfExisting, ref FuserDefinition.FUSER_FILE_INFO rawHFile){
            try {                
                string newFilename = GetFilename(rawNewFilename);
                Win32Returncode ret = this.fsDevice.MoveFile(this.hManager.GetFileHandler(GetFilename(rawFilename), ref rawHFile), newFilename, (rawReplaceIfExisting != 0));
                return ConvReturnCodeToInt(ret);

            } catch (Exception e) {
                this.fsDevice.LogErrorMessage("MoveFile", e.Message);
                return ConvReturnCodeToInt(Win32Returncode.DEFAULT_UNKNOWN_ERROR);
            }            
        }



        public delegate int SetEndOfFileDelegate(IntPtr rawFilename, long rawByteOffset, ref FuserDefinition.FUSER_FILE_INFO rawHFile);
        public          int SetEndOfFile        (IntPtr rawFilename, long rawByteOffset, ref FuserDefinition.FUSER_FILE_INFO rawHFile){
            try {
                return ConvReturnCodeToInt(this.fsDevice.SetEndOfFile(this.hManager.GetFileHandler(GetFilename(rawFilename), ref rawHFile), rawByteOffset));
            } catch (Exception e) {
                this.fsDevice.LogErrorMessage("SetEndOfFile", e.Message);
                return ConvReturnCodeToInt(Win32Returncode.DEFAULT_UNKNOWN_ERROR);
            }
        }


        public delegate int SetAllocationSizeDelegate(IntPtr rawFilename, long rawLength, ref FuserDefinition.FUSER_FILE_INFO rawHFile);
        public          int SetAllocationSize        (IntPtr rawFilename, long rawLength, ref FuserDefinition.FUSER_FILE_INFO rawHFile){
            try {
                return ConvReturnCodeToInt(this.fsDevice.SetAllocationSize(this.hManager.GetFileHandler(GetFilename(rawFilename), ref rawHFile), rawLength));
            } catch (Exception e) {
                this.fsDevice.LogErrorMessage("SetAllocationSize", e.Message);
                return ConvReturnCodeToInt(Win32Returncode.DEFAULT_UNKNOWN_ERROR);
            }
        }


        public delegate int LockFileDelegate(IntPtr rawFilename, long rawByteOffset, long rawLength, ref FuserDefinition.FUSER_FILE_INFO rawHFile);
        public          int LockFile        (IntPtr rawFilename, long rawByteOffset, long rawLength, ref FuserDefinition.FUSER_FILE_INFO rawHFile){
            try {
                return ConvReturnCodeToInt(this.fsDevice.LockFile(this.hManager.GetFileHandler(GetFilename(rawFilename), ref rawHFile), rawByteOffset, rawLength));
            } catch (Exception e) {
                this.fsDevice.LogErrorMessage("LockFile", e.Message);
                return ConvReturnCodeToInt(Win32Returncode.DEFAULT_UNKNOWN_ERROR);
            }
        }


        public delegate int UnlockFileDelegate(IntPtr rawFilename, long rawByteOffset, long rawLength, ref FuserDefinition.FUSER_FILE_INFO rawHFile);
        public          int UnlockFile        (IntPtr rawFilename, long rawByteOffset, long rawLength, ref FuserDefinition.FUSER_FILE_INFO rawHFile){
            try {
                return ConvReturnCodeToInt(this.fsDevice.UnlockFile(this.hManager.GetFileHandler(GetFilename(rawFilename), ref rawHFile), rawByteOffset, rawLength));
            } catch (Exception e) {
                this.fsDevice.LogErrorMessage("UnlockFile", e.Message);
                return ConvReturnCodeToInt(Win32Returncode.DEFAULT_UNKNOWN_ERROR);
            }
        }

        public delegate int GetDiskFreeSpaceDelegate(ref ulong rawFreeBytesAvailable, ref ulong rawTotalNumberOfBytes, ref ulong rawTotalNumberOfFreeBytes, ref FuserDefinition.FUSER_FILE_INFO rawHFile); // TODO: remove rawHFile
        public          int GetDiskFreeSpace        (ref ulong rawFreeBytesAvailable, ref ulong rawTotalNumberOfBytes, ref ulong rawTotalNumberOfFreeBytes, ref FuserDefinition.FUSER_FILE_INFO rawHFile){
            try {                
                return ConvReturnCodeToInt(this.fsDevice.GetDiskFreeSpace(ref rawFreeBytesAvailable, ref rawTotalNumberOfBytes, ref rawTotalNumberOfFreeBytes));
            } catch (Exception e) {
                this.fsDevice.LogErrorMessage("GetDiskFreeSpace", e.Message);
                return ConvReturnCodeToInt(Win32Returncode.DEFAULT_UNKNOWN_ERROR);
            }
        }

        public delegate int GetVolumeInformationDelegate(IntPtr rawVolumenameBuffer, uint rawVolumenameSize, ref uint rawSerialnumber, ref uint rawMaximumComponentLength, ref uint rawFileSystemFlags, IntPtr rawFileSystemnameBuffer, uint rawFileSystemnameSize, ref FuserDefinition.FUSER_FILE_INFO rawHFile); // TODO: remove rawHFile
        public          int GetVolumeInformation        (IntPtr rawVolumenameBuffer, uint rawVolumenameSize, ref uint rawSerialnumber, ref uint rawMaximumComponentLength, ref uint rawFileSystemFlags, IntPtr rawFileSystemnameBuffer, uint rawFileSystemnameSize, ref FuserDefinition.FUSER_FILE_INFO rawHFile){
            try {
                byte[] volumelabel = System.Text.Encoding.Unicode.GetBytes(this.volumelabel);
                byte[] filesystem = System.Text.Encoding.Unicode.GetBytes(this.filesystem);
                Marshal.Copy(volumelabel, 0, rawVolumenameBuffer, Math.Min((int)rawVolumenameSize, volumelabel.Length)); // TODO: check if change from uint to int is possible for rawVolumenameSize, then remove casting
                Marshal.Copy(filesystem, 0, rawFileSystemnameBuffer, Math.Min((int)rawFileSystemnameSize, filesystem.Length)); // TODO: check if change from uint to int is possible for rawVolumenameSize, then remove casting

                rawSerialnumber = this.serialnumber; 
                
                //rawFileSystemFlags = (uint) (FuserDefinition.FileSystemFlags.FILE_CASE_PRESERVED_NAMES | FuserDefinition.FileSystemFlags.FILE_UNICODE_ON_DISK | FuserDefinition.FileSystemFlags.FILE_CASE_SENSITIVE_SEARCH);
                rawFileSystemFlags = 7; // The above code corresponds to 7
                rawMaximumComponentLength = 256;
               
                return ConvReturnCodeToInt(Win32Returncode.SUCCESS);
            } catch (Exception e) {
                this.fsDevice.LogErrorMessage("GetVolumeInformation", e.Message);
                return ConvReturnCodeToInt(Win32Returncode.DEFAULT_UNKNOWN_ERROR);
            }
        }


        public delegate int MountDelegate(IntPtr rawMountPoint, IntPtr rawDeviceName);
        public int          Mount        (IntPtr rawMountPoint, IntPtr rawDeviceName){
            try {
                if (this.heartbeat != null) {
                    return 0;
                }
            
                string mountPoint = Marshal.PtrToStringUni(rawMountPoint);
                string deviceName = Marshal.PtrToStringUni(rawDeviceName);

                this.heartbeat = new FuserHeartbeat(mountPoint, deviceName); // start Heartbeat

                return ConvReturnCodeToInt(this.fsDevice.Mount(mountPoint, deviceName));
            } catch (Exception e) {
                //System.Diagnostics.Debugger.Break();
                this.fsDevice.LogErrorMessage("Mount", e.Message);
                return ConvReturnCodeToInt(Win32Returncode.DEFAULT_UNKNOWN_ERROR);
            }
        }


        public delegate int UnmountDelegate(ref FuserDefinition.FUSER_FILE_INFO rawHFile);
        public          int Unmount        (ref FuserDefinition.FUSER_FILE_INFO rawHFile){ // TODO: remove or change
            return 0;
        }

        public delegate int GetFileSecurityDelegate(IntPtr rawFilename, ref FuserDefinition.SECURITY_INFORMATION rawRequestedInformation, ref FuserDefinition.SECURITY_DESCRIPTOR rawSecurityDescriptor, uint rawSecurityDescriptorLength, ref uint rawSecurityDescriptorLengthNeeded, ref FuserDefinition.FUSER_FILE_INFO rawHFile);
        public          int GetFileSecurity        (IntPtr rawFilename, ref FuserDefinition.SECURITY_INFORMATION rawRequestedInformation, ref FuserDefinition.SECURITY_DESCRIPTOR rawSecurityDescriptor, uint rawSecurityDescriptorLength, ref uint rawSecurityDescriptorLengthNeeded, ref FuserDefinition.FUSER_FILE_INFO rawHFile){
            try {
                if (this.aclProxy != null) {
                    return this.aclProxy.GetFileSecurity(rawRequestedInformation, ref rawSecurityDescriptor, (int)rawSecurityDescriptorLength, ref rawSecurityDescriptorLengthNeeded);
                }
            } catch (Exception e) {
                this.fsDevice.LogErrorMessage("GetFileSecurity", e.Message);
                return ConvReturnCodeToInt(Win32Returncode.DEFAULT_UNKNOWN_ERROR);
            }
            return ConvReturnCodeToInt(Win32Returncode.DEFAULT_UNKNOWN_ERROR);
        }

        public delegate int SetFileSecurityDelegate(IntPtr rawFilename, ref FuserDefinition.SECURITY_INFORMATION rawSecurityInformation, ref FuserDefinition.SECURITY_DESCRIPTOR rawSecurityDescriptor, uint rawSecurityDescriptorLengthNeeded, ref FuserDefinition.FUSER_FILE_INFO rawHFile);
        public          int SetFileSecurity        (IntPtr rawFilename, ref FuserDefinition.SECURITY_INFORMATION rawSecurityInformation, ref FuserDefinition.SECURITY_DESCRIPTOR rawSecurityDescriptor, uint rawSecurityDescriptorLengthNeeded, ref FuserDefinition.FUSER_FILE_INFO rawHFile){
            return ConvReturnCodeToInt(Win32Returncode.DEFAULT_UNKNOWN_ERROR);
        }

    }
}
