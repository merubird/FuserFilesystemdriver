@echo off

IF EXIST %1\Fuser.sys              _sign-internal.bat %1\Fuser.sys              drv %2
IF EXIST %1\Fuser.dll              _sign-internal.bat %1\Fuser.dll              app %2
IF EXIST %1\Fuser.exe              _sign-internal.bat %1\Fuser.exe              app %2
IF EXIST %1\Demo.exe               _sign-internal.bat %1\Demo.exe               app %2
IF EXIST %1\FuserDeviceAgent.exe   _sign-internal.bat %1\FuserDeviceAgent.exe   app %2

echo.
echo.
echo.
echo !! WARNING No file found to sign !!
echo !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
echo.
echo.
pause
