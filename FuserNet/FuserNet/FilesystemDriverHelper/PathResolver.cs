using System;
using System.Collections.Generic;
using FuserLowlevelDriver;

namespace FuserNet
{
    internal class PathResolver {
        private const string PATHDELIMITTER = @"\";

        private FuserPathResolveResult pPath;
        private IFuserFilesystemItem pFileitem;
        private bool pPathInvalid;
        private Win32Returncode pLastErrorCode; // always returns an error code that best fits.

        public PathResolver(IFuserFilesystemDirectory root, string path) {
            this.pPath = null;
            this.pFileitem = null;
            this.pPathInvalid = true;
            this.pLastErrorCode = Win32Returncode.DEFAULT_UNKNOWN_ERROR;

            resolve(root, path);
        }

        public Win32Returncode LastErrorCode { get { return this.pLastErrorCode; } }
        public bool PathInvalid   { get { return this.pPathInvalid;   }}
        public FuserPathResolveResult path { get { return this.pPath; } }
        public IFuserFilesystemItem fileitem { get { return this.pFileitem; } }


        private void resolve(IFuserFilesystemDirectory root, string path) {
            FuserPathResolveResult vpr = null;
            
            try {
                vpr = ResolvePath(root, path);
                this.pPath = vpr;
                
                if (vpr.returncode != Win32Returncode.SUCCESS) {                    
                    this.pPathInvalid = true;
                    this.pLastErrorCode = vpr.returncode;
                    return;
                }
            } catch {
                // should never occur
                this.pPathInvalid = true;
                this.pLastErrorCode = Win32Returncode.ERROR_PATH_NOT_FOUND;
                this.pPath = null;
                return;
            }

            if (vpr != null){
                if (vpr.HasError ){
                    vpr = null;
                }
            }

            if (vpr != null){
                if (vpr.returncode != Win32Returncode.SUCCESS) {
                    vpr = null;
                }
            }


            if (vpr == null) {                
                this.pPathInvalid = true;
                this.pLastErrorCode = Win32Returncode.ERROR_PATH_NOT_FOUND;
                this.pPath = null;
            } else {
                this.pPathInvalid = false;
                this.pLastErrorCode = Win32Returncode.DEFAULT_UNKNOWN_ERROR;
                this.pFileitem = null;
                                
                if (vpr.currentDirectory == root && vpr.itemname == ""){
                    this.pFileitem = root;
                } else {
                    if (vpr.itemname == "") {
                        // path not found
                        this.pLastErrorCode = Win32Returncode.ERROR_PATH_NOT_FOUND;
                        this.pPathInvalid = true;
                    } else {
                        try {
                            lock (vpr.currentDirectory) {
                                this.pFileitem = vpr.currentDirectory.GetItem(vpr.itemname);
                            }
                        } catch {
                            this.pFileitem = null;
                        }
                    }
                }
            }
        }


        private FuserPathResolveResult ResolvePath(IFuserFilesystemDirectory root, string path) {
            FuserPathResolveResult ret = new FuserPathResolveResult(root, path);
            IFuserFilesystemDirectory currentdirectory = root;
            IFuserFilesystemItem vItem;
            string lastItem = "";
            string p;

            if (path == "")
                return new FuserPathResolveResult(new FSException(Win32Returncode.ERROR_INVALID_NAME));

            if (path.Length < 1)
                return new FuserPathResolveResult(new FSException(Win32Returncode.ERROR_INVALID_NAME));

            if (path.Substring(0, 1) != PATHDELIMITTER)
                return new FuserPathResolveResult(new FSException(Win32Returncode.ERROR_INVALID_NAME));

            try {
                path = path.Substring(1); // Remove starting \
                if (path != "") {
                    string[] part = path.Split(PATHDELIMITTER.ToCharArray()[0]);
                    lastItem = part[part.Length - 1]; // select last item

                    if (!PathValidateCheckItemname(lastItem, true)) {
                        throw new Exception("Invalid Path");
                    }

                    for (int i = 0; i < (part.Length - 1); i++) {
                        p = part[i].Trim();

                        if (PathValidateCheckItemname(p, false)) {

                            vItem = null;
                            lock (currentdirectory) {
                                vItem = currentdirectory.GetItem(p);
                            }

                            if (vItem == null)
                                throw new KeyNotFoundException();
                            if (!(vItem is IFuserFilesystemDirectory))
                                throw new Exception("Invalid path");

                            currentdirectory = (IFuserFilesystemDirectory)vItem;
                        } else {
                            throw new Exception("Invalid Path");
                        }
                    }
                }
            } catch (FSException fse) {
                return new FuserPathResolveResult(fse);
            } catch (Exception e) {
                e.ToString();
                return new FuserPathResolveResult(new FSException(Win32Returncode.ERROR_PATH_NOT_FOUND));
            }

            ret.currentDirectory = currentdirectory;
            ret.itemname = lastItem;

            return ret;
        }



        public static bool PathValidateCheckItemname(string itemname, bool allowSearchPattern) {
            /*
                Filename:
                34, 60, 62, 124
                0 - 31

                Path additionally:
                58, 42 (*), 63 (?), 92, 47
            */

            if (itemname == "")
                return false;
            if (itemname.Length == 0)
                return false;

            char[] f = itemname.ToCharArray();
            foreach (char fc in f) {
                if (fc >= 0 && fc <= 31)
                    return false;

                if (fc == 34 || fc == 60 || fc == 62 || fc == 124)
                    return false;

                if (fc == 58 || fc == 92 || fc == 47)
                    return false;

                if (!allowSearchPattern && (fc == 42 || fc == 63))
                    return false;
            }
            return true;
        }



    }




    internal class FuserPathResolveResult {
        private IFuserFilesystemDirectory root;
        private IFuserFilesystemDirectory curDir;
        private string path;
        private Win32Returncode errorcode;
        private string pItemname;
        private bool pHasError;

        public FuserPathResolveResult(IFuserFilesystemDirectory root, string path) {
            this.path = path;
            this.root = root;
            this.curDir = root;
            this.pItemname = "";

            this.pHasError = false;
            this.errorcode = Win32Returncode.SUCCESS;
        }

        public FuserPathResolveResult(FSException exception) {
            this.path = null;
            this.root = null;
            this.curDir = null;
            this.pItemname = "";

            this.pHasError = true;
            this.errorcode = exception.Error;
        }

        public IFuserFilesystemDirectory currentDirectory { get { return this.curDir; } set { this.curDir = value; } }
        public string itemname { get { return this.pItemname; } set { this.pItemname = value; } }
        public bool HasError { get { return this.pHasError; } }
        public Win32Returncode returncode { get { return this.errorcode; } }
    }



}
