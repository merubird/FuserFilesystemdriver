# Fuser filesystem driver
## User-mode file system driver for Windows based on Dokan library

This project is based on the original Dokan project by Hiroki Asakawa from 2013. I started the development without a version control system and without an IDE, I only worked with Notepad++ and a command line build tool. The big pain point of the project was the frequent bluescreens. I started building an agent service that would periodically check with a heartbeat signal if the application was still running in user mode, if not, the mounted drive was dismounted.

I abandoned the project because it was very time consuming. The current state of the project is running, but it is not reliable. The project has been **archived** and is no longer maintained, the last code change dates back to 2013. The dokany project is based on the same but is still being developed regularly. If you are interested in this project, have a look at the dokany project: https://github.com/dokan-dev/dokany


see [howtobuild.txt](docs/howtobuild.txt) for information about how to build

Licensing
=========

Fuser library contains LGPL, MIT licensed programs.

- user-mode library (FuserUsermodeLib)          LGPL
- driver (FuserDriver)                          LGPL
- fuser device agent (FuserDeviceAgent)         MIT
- samples (FuserDemo)                           MIT
- .NET Projects (FuserNet, FuserNetTesting)     MIT

## .NET C# Application
### FuserNet
Contains the core access library that communicates with the user mode library fuser.dll

### FuserNetTesting
Contains a simple test project that emulates a mirror drive (FS-Spiegel) and creates a pure memory drive (FS-Simulate)

## C Program (in directory src)
### FuserDriver
Contains the kernel driver that created the file system drive

### FuserUsermodeLib
Contains the user mode library that communicates with the driver

### FuserDemo
Contains a demo application that creates a mirror drive

### FuserDeviceAgent
Contains a Windows service. This service mounts and unmounts the drives and periodicly checks whether the application is still running in user mode. If not, the drive is unmounted.

