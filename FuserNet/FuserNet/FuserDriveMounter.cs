using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading;
using FuserLowlevelDriver;

namespace FuserNet{

    public enum FuserMountFinishStatus {
        SUCCESS = 0,
        GENERAL_UNKNOWN_ERROR = 1,
        DRIVER_NOT_FOUND_ERROR = 2, //DLL not found
        BAD_DRIVE_LETTER_ERROR = 3,
        DRIVER_INSTALL_ERROR = 4,
        START_ERROR = 5, // TODO: renaming, increase expressiveness
        MOUNT_ERROR = 6, // TODO: renaming, increase expressiveness
        MOUNT_POINT_ERROR = 7, // TODO: renaming, increase expressiveness
    }


    public class FuserDriveMounter
    {
        private string MountPoint;
        private IFuserlDrive drive;
        private Thread thWorking;


        private FuserDevice FuserMountDevice;
        private FuserDefinition.FUSER_OPTIONS FuserMountOptions;

        private bool isMounted;
        private bool pDebugMode;


        public FuserDriveMounter(IFuserlDrive drive) {
            this.pDebugMode = false;
            this.drive = drive;
            this.thWorking = null;
            this.FuserMountDevice = null;
            this.MountPoint = "";

            this.isMounted = false;
        }
        
        public bool DebugMode { get { return this.pDebugMode; } set { this.pDebugMode = value; } }

        public bool Mount(string mountPoint) {            
            lock(this){
                if (this.isMounted) {
                    return false;
                }
                this.isMounted = true;
            }
            
            try {
                IFuserFilesystemDevice fsDevice = new FuserInternalFilesystem(this.drive, this.pDebugMode);                
                
                if (this.pDebugMode) {
                    //this.driveLib = new DebugSynLock(this.driveLib);
                    fsDevice = new DebugAndLogProxyFS(fsDevice);
                }


                this.FuserMountDevice = new FuserDevice(fsDevice, this.drive.Volumelabel, this.drive.Filesystem, this.drive.Serialnumber);
                
                this.FuserMountOptions = new FuserDefinition.FUSER_OPTIONS();
                this.FuserMountOptions.Version = FuserDefinition.FUSER_VERSION;
                this.FuserMountOptions.ThreadCount = 0; // TODO: Determine and adjust the correct value
                this.FuserMountOptions.MountPoint = mountPoint;

                this.FuserMountOptions.Options = 0;
                this.FuserMountOptions.Options |= this.pDebugMode ? FuserDefinition.FUSER_OPTION_DEBUG : 0;                           
                this.FuserMountOptions.Options |= FuserDefinition.FUSER_OPTION_KEEP_ALIVE;
                this.FuserMountOptions.Options |= FuserDefinition.FUSER_OPTION_HEARTBEAT;



                this.MountPoint = mountPoint;
                this.thWorking = new Thread(pvStart);                
                this.thWorking.Start();
            } catch {
                lock(this){                
                    this.isMounted = false;
                }
                return false;
            }
           
            return true;
        }

        private void pvStart() {            
            FuserMountFinishStatus mountReturnCode = FuserMountFinishStatus.GENERAL_UNKNOWN_ERROR;
            
            try {
                int dllstatus = 0;
                dllstatus = FuserLinkLibraryCall.FuserMain(this.FuserMountOptions, this.FuserMountDevice);

                switch (dllstatus) {
                    case FuserDefinition.FUSER_SUCCESS:              mountReturnCode = FuserMountFinishStatus.SUCCESS;                break;
                    case FuserDefinition.FUSER_ERROR:                mountReturnCode = FuserMountFinishStatus.GENERAL_UNKNOWN_ERROR;  break;
                    case FuserDefinition.FUSER_DRIVE_LETTER_ERROR:   mountReturnCode = FuserMountFinishStatus.BAD_DRIVE_LETTER_ERROR; break;
                    case FuserDefinition.FUSER_DRIVER_INSTALL_ERROR: mountReturnCode = FuserMountFinishStatus.DRIVER_INSTALL_ERROR;   break;
                    case FuserDefinition.FUSER_START_ERROR:          mountReturnCode = FuserMountFinishStatus.START_ERROR;            break;
                    case FuserDefinition.FUSER_MOUNT_ERROR:          mountReturnCode = FuserMountFinishStatus.MOUNT_ERROR;            break;
                    case FuserDefinition.FUSER_MOUNT_POINT_ERROR:    mountReturnCode = FuserMountFinishStatus.MOUNT_POINT_ERROR;      break;
                    
                    default:                                         mountReturnCode = FuserMountFinishStatus.GENERAL_UNKNOWN_ERROR;  break;
                }



            }catch (DllNotFoundException de){
                de.ToString();
                mountReturnCode = FuserMountFinishStatus.DRIVER_NOT_FOUND_ERROR;
            } catch (Exception e) {
                e.ToString();
                mountReturnCode = FuserMountFinishStatus.GENERAL_UNKNOWN_ERROR;
            }

            try {
                this.drive.Unmounted(mountReturnCode);
            }
            catch {
            }
            
            lock (this) {
                this.isMounted = false;
            }            
        }


        public void Unmount() {
            lock (this) {
                if (!this.isMounted)
                    return;                
            }
            try {
                FuserLinkLibraryCall.FuserRemoveMountPoint(this.MountPoint);
            } catch {
                return;
            }
        }


    }


    


}
