@echo off
echo start make all?
pause


cd sys
call b all
cd ..
echo.
echo next?
pause


cd fuser
call b all
cd ..
echo.
echo next?
pause




cd fuser_mount
call b all
cd ..
echo.
echo next?
pause




cd fuser_control
call b all
cd ..
echo.
echo next?
pause


cd fuser_mirror
call b all
cd ..
echo.

echo completed


