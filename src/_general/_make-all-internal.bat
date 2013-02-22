@echo off
IF %1x == x GOTO end
IF %2x == x GOTO end


call make.bat %1 all


IF %3x == sigx GOTO signature
goto next

:signature

IF %5 == x86 CALL sign.bat ..\%1\obj%4_%6_x86\i386     %2
IF %5 == x64 CALL sign.bat ..\%1\obj%4_%6_amd64\amd64  %2


:next
if %2x == silentx goto end
echo.
echo next?
pause
:end
