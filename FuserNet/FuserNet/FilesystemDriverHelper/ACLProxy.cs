using System;

using System.IO;
using System.Security.AccessControl;
using System.Security.Principal;
using System.Runtime.InteropServices;

namespace FuserNet
{
    internal class ACLProxy
    {
        // TODO: Remove support for ACL completely
        [DllImport("advapi32.dll")]
        private static extern int GetFileSecurity(string lpFileName, FuserLowlevelDriver.FuserDefinition.SECURITY_INFORMATION RequestedInformation, [MarshalAs(UnmanagedType.Struct)] ref FuserLowlevelDriver.FuserDefinition.SECURITY_DESCRIPTOR pSecurityDescriptor, int nLength, ref uint lpnLengthNeeded);
        private string filename;

        public ACLProxy() {            
            this.filename = "";

            try {
                this.filename = System.Environment.GetFolderPath(System.Environment.SpecialFolder.CommonDocuments);
            } catch {
                this.filename = "";
            }

            if (this.filename == "") {
                this.filename = Path.GetTempPath();
            }

        }

        public int GetFileSecurity(FuserLowlevelDriver.FuserDefinition.SECURITY_INFORMATION RequestedInformation, [MarshalAs(UnmanagedType.Struct)] ref FuserLowlevelDriver.FuserDefinition.SECURITY_DESCRIPTOR pSecurityDescriptor, int nLength, ref uint lpnLengthNeeded) {
            if (this.filename == null)
                return -1;
            if (this.filename == "")
                return -1;

            return GetFileSecurity(this.filename, RequestedInformation, ref pSecurityDescriptor, nLength, ref lpnLengthNeeded);
        }



        /*
        private delegate int GetFileSecurityDelegate(IntPtr rawFilename, ref FuserDefinition.SECURITY_INFORMATION rawRequestedInformation, ref FuserDefinition.SECURITY_DESCRIPTOR rawSecurityDescriptor, uint rawSecurityDescriptorLength, ref uint rawSecurityDescriptorLengthNeeded, ref FuserDefinition.FUSER_FILE_INFO rawHFile);
        public           int GetFileSecurity        (IntPtr rawFilename, ref FuserDefinition.SECURITY_INFORMATION rawRequestedInformation, ref FuserDefinition.SECURITY_DESCRIPTOR rawSecurityDescriptor, uint rawSecurityDescriptorLength, ref uint rawSecurityDescriptorLengthNeeded, ref FuserDefinition.FUSER_FILE_INFO rawHFile){
            if (useACLSupport) {
                try {
                    if (this.aclProxy != null) {
                        return this.aclProxy.GetFileSecurity(rawRequestedInformation, ref rawSecurityDescriptor, (int)rawSecurityDescriptorLength, ref rawSecurityDescriptorLengthNeeded);
                    }
                } catch (Exception e) {
                    this.fsDevice.LogErrorMessage("GetFileSecurity", e.Message);
                    return ConvReturnCodeToInt(Win32Returncode.DEFAULT_UNKNOWN_ERROR);
                }
            }
            return ConvReturnCodeToInt(Win32Returncode.DEFAULT_UNKNOWN_ERROR);
        }

        private delegate int SetFileSecurityDelegate(IntPtr rawFilename, ref FuserDefinition.SECURITY_INFORMATION rawSecurityInformation, ref FuserDefinition.SECURITY_DESCRIPTOR rawSecurityDescriptor, uint rawSecurityDescriptorLengthNeeded, ref FuserDefinition.FUSER_FILE_INFO rawHFile);
        public           int SetFileSecurity        (IntPtr rawFilename, ref FuserDefinition.SECURITY_INFORMATION rawSecurityInformation, ref FuserDefinition.SECURITY_DESCRIPTOR rawSecurityDescriptor, uint rawSecurityDescriptorLengthNeeded, ref FuserDefinition.FUSER_FILE_INFO rawHFile){
            return ConvReturnCodeToInt(Win32Returncode.DEFAULT_UNKNOWN_ERROR);
        }
         */


    }
}
