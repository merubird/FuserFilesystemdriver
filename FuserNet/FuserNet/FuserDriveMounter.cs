using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading;
using FuserLowlevelDriver;

namespace FuserNet{

    public enum FuserMountFinishStatus {
        Success = 0,
        Version_Error = -1,
        Event_Load_Error = -2,
        Bad_Mountpoint_Error = -3,
        Driver_Install_Error = -4,
        Driver_Start_Error = -5,
        Mount_Error = -6,

        General_Unknown_Error = -1000,
        Driver_Not_Found_Error = -1001, //DLL not found
    }


    public class FuserDriveMounter
    {
        private string MountPoint;
        private IFuserlDrive drive;
        private Thread thWorking;


        private FuserDevice FuserMountDevice;
        private FuserDefinition.FUSER_MOUNT_PARAMETER FuserMountParam;

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

                this.FuserMountParam = new FuserDefinition.FUSER_MOUNT_PARAMETER();
                this.FuserMountParam.ThreadsCount = 0;
                this.FuserMountParam.MountPoint = mountPoint;

                // TODO: Use options
                this.FuserMountParam.Flags = 0;
                this.FuserMountParam.Flags |= this.pDebugMode ? FuserDefinition.FUSER_MOUNT_PARAMETER_FLAG_STDERR : 0;
                



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
            FuserMountFinishStatus mountReturnCode = FuserMountFinishStatus.General_Unknown_Error;
            
            try {
                int dllstatus = 0;
                
                dllstatus = FuserLinkLibraryCall.DeviceMount(this.FuserMountParam, this.FuserMountDevice);
                
                switch (dllstatus) {
                    case FuserDefinition.FUSER_DEVICEMOUNT_SUCCESS: mountReturnCode = FuserMountFinishStatus.Success; break;
                    case FuserDefinition.FUSER_DEVICEMOUNT_VERSION_ERROR: mountReturnCode = FuserMountFinishStatus.Version_Error; break;
                    case FuserDefinition.FUSER_DEVICEMOUNT_EVENT_LOAD_ERROR: mountReturnCode = FuserMountFinishStatus.Event_Load_Error; break;
                    case FuserDefinition.FUSER_DEVICEMOUNT_BAD_MOUNT_POINT_ERROR: mountReturnCode = FuserMountFinishStatus.Bad_Mountpoint_Error; break;
                    case FuserDefinition.FUSER_DEVICEMOUNT_DRIVER_INSTALL_ERROR: mountReturnCode = FuserMountFinishStatus.Driver_Install_Error; break;
                    case FuserDefinition.FUSER_DEVICEMOUNT_DRIVER_START_ERROR: mountReturnCode = FuserMountFinishStatus.Driver_Start_Error; break;
                    case FuserDefinition.FUSER_DEVICEMOUNT_MOUNT_ERROR: mountReturnCode = FuserMountFinishStatus.Mount_Error; break;

                    default: mountReturnCode = FuserMountFinishStatus.General_Unknown_Error; break;
                }                
            
            }catch (DllNotFoundException de){
                de.ToString();
                mountReturnCode = FuserMountFinishStatus.Driver_Not_Found_Error;
            } catch (Exception e) {
                e.ToString();
                mountReturnCode = FuserMountFinishStatus.General_Unknown_Error;
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
                string mp = "";
                if (this.FuserMountDevice != null) {
                    mp = this.FuserMountDevice.getMountPoint();
                }
                if (mp == "") {
                    mp = this.MountPoint;
                }

                FuserLinkLibraryCall.DeviceUnmount(mp);
            } catch {
                return;
            }
        }


        public static string GetFuserVersion(){
            return FuserLinkLibraryCall.FuserVersion();
        }

    }


    


}
