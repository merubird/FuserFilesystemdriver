@echo off
IF NOT exist bin GOTO ErrDir
IF NOT exist src GOTO ErrDir
IF NOT exist FuserNet GOTO ErrDir
IF NOT exist FuserSetup GOTO ErrDir

IF %1X == ParamCleanBuiltX GOTO doCleanBuilt
IF %1X == ParamCleanFullX GOTO doCleanFull

echo.
echo Cleans up the current release directory:
cd
echo.
echo Level 1 : Delete Built-Temp
echo Level 2 : Delete Built-Temp and Built-Binary, clean up completely
echo.
IF %1x == x GOTO ErrNoOption
IF %1 == 1 GOTO SelectCleanBuilt
IF %1 == 2 GOTO SelectCleanFull
GOTO ErrNoOption


:SelectCleanBuilt
echo. 
echo Level 1: Clean up Built-Temp Files only selected.
PAUSE
CALL %0 ParamCleanBuilt
GOTO end

:SelectCleanFull
echo. 
echo Level 2: Complete clean up selected, release bins are also deleted
PAUSE
CALL %0 ParamCleanBuilt
CALL %0 ParamCleanFull
GOTO end








:doCleanBuilt
	IF NOT EXIST src\FuserDemo GOTO ErrDir
	IF NOT EXIST src\FuserDeviceAgent GOTO ErrDir
	IF NOT EXIST src\FuserDriver GOTO ErrDir
	IF NOT EXIST src\FuserUsermodeLib GOTO ErrDir

	IF NOT %2x == x GOTO doCleanBuiltPart
	CALL %0 %1 FuserDemo
	CALL %0 %1 FuserDeviceAgent
	CALL %0 %1 FuserDriver
	CALL %0 %1 FuserUsermodeLib
	GOTO FINISH
	:doCleanBuiltPart
	IF NOT EXIST src\%2 GOTO ErrDir	
	IF EXIST src\%2\objchk_wxp_x86 RMDIR /s /q src\%2\objchk_wxp_x86
	IF EXIST src\%2\objfre_win7_amd64 RMDIR /s /q src\%2\objfre_win7_amd64
	IF EXIST src\%2\objchk_win7_amd64 RMDIR /s /q src\%2\objchk_win7_amd64
	IF EXIST src\%2\objchk_win7_x86 RMDIR /s /q src\%2\objchk_win7_x86
	IF EXIST src\%2\objchk_wlh_amd64 RMDIR /s /q src\%2\objchk_wlh_amd64
	IF EXIST src\%2\objchk_wlh_x86 RMDIR /s /q src\%2\objchk_wlh_x86
	IF EXIST src\%2\objchk_wnet_amd64 RMDIR /s /q src\%2\objchk_wnet_amd64
	IF EXIST src\%2\objchk_wnet_x86 RMDIR /s /q src\%2\objchk_wnet_x86
	IF EXIST src\%2\objchk_wxp_x86 RMDIR /s /q src\%2\objchk_wxp_x86
	IF EXIST src\%2\objfre_win7_x86 RMDIR /s /q src\%2\objfre_win7_x86
	IF EXIST src\%2\objfre_wlh_amd64 RMDIR /s /q src\%2\objfre_wlh_amd64
	IF EXIST src\%2\objfre_wlh_x86 RMDIR /s /q src\%2\objfre_wlh_x86
	IF EXIST src\%2\objfre_wnet_amd64 RMDIR /s /q src\%2\objfre_wnet_amd64
	IF EXIST src\%2\objfre_wnet_x86 RMDIR /s /q src\%2\objfre_wnet_x86
	IF EXIST src\%2\objfre_wxp_x86 RMDIR /s /q src\%2\objfre_wxp_x86
	
	IF EXIST src\%2\buildchk_wxp_x86.log DEL src\%2\buildchk_wxp_x86.log	
	IF EXIST src\%2\buildfre_win7_amd64.log DEL src\%2\buildfre_win7_amd64.log
	IF EXIST src\%2\buildfre_win7_x86.log DEL src\%2\buildfre_win7_x86.log
	IF EXIST src\%2\buildfre_wlh_amd64.log DEL src\%2\buildfre_wlh_amd64.log
	IF EXIST src\%2\buildfre_wlh_x86.log DEL src\%2\buildfre_wlh_x86.log
	IF EXIST src\%2\buildfre_wnet_amd64.log DEL src\%2\buildfre_wnet_amd64.log
	IF EXIST src\%2\buildfre_wnet_x86.log DEL src\%2\buildfre_wnet_x86.log
	IF EXIST src\%2\buildfre_wxp_x86.log DEL src\%2\buildfre_wxp_x86.log
	IF EXIST src\%2\buildchk_win7_amd64.log DEL src\%2\buildchk_win7_amd64.log
	IF EXIST src\%2\buildchk_win7_x86.log DEL src\%2\buildchk_win7_x86.log
	IF EXIST src\%2\buildchk_wlh_amd64.log DEL src\%2\buildchk_wlh_amd64.log
	IF EXIST src\%2\buildchk_wlh_x86.log DEL src\%2\buildchk_wlh_x86.log
	IF EXIST src\%2\buildchk_wnet_amd64.log DEL src\%2\buildchk_wnet_amd64.log
	IF EXIST src\%2\buildchk_wnet_x86.log DEL src\%2\buildchk_wnet_x86.log
	IF EXIST src\%2\buildchk_wxp_x86.log DEL src\%2\buildchk_wxp_x86.log	
GOTO FINISH



:doCleanFull
	IF NOT %2x == x GOTO doCleanFullPart
	CALL %0 %1 win7_amd64
	CALL %0 %1 win7_x86
	CALL %0 %1 wlh_amd64
	CALL %0 %1 wlh_x86
	CALL %0 %1 wnet_amd64
	CALL %0 %1 wnet_x86
	CALL %0 %1 wxp_x86	
	GOTO FINISH

	:doCleanFullPart
	IF EXIST bin\%2\Demo.exe DEL bin\%2\Demo.exe
	IF EXIST bin\%2\Fuser.dll DEL bin\%2\Fuser.dll
	IF EXIST bin\%2\Fuser.sys DEL bin\%2\Fuser.sys
	IF EXIST bin\%2\FuserDeviceAgent.exe DEL bin\%2\FuserDeviceAgent.exe	
GOTO FINISH




GOTO end
:ErrDir
echo.
echo Directory invalid!
GOTO END


GOTO END
:ErrNoOption
echo No parameter set!
GOTO END





:end
echo.
echo - completed -
echo.
pause
:FINISH

