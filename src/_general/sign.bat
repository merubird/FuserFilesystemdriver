@echo off

IF EXIST %1\fuser.sys     _sign-internal.bat %1\fuser.sys     drv %2
IF EXIST %1\fuser.dll     _sign-internal.bat %1\fuser.dll     app %2
IF EXIST %1\fuserctl.exe  _sign-internal.bat %1\fuserctl.exe  app %2
IF EXIST %1\mirror.exe    _sign-internal.bat %1\mirror.exe    app %2
IF EXIST %1\mounter.exe   _sign-internal.bat %1\mounter.exe   app %2

echo.
echo.
echo.
echo !! WARNING No file found to sign !!
echo !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
echo.
echo.
pause
