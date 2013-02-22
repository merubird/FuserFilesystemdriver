@echo off
echo Signing %1
echo.


IF %2X == drvX goto drv
IF %2X == appX goto app

echo.
echo.
echo.
echo !! WARNING No mode has been set for signing !!
echo !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
echo.
echo.
pause
goto end


:drv
  signtool sign /v /ac "cert.crt" /t http://timestamp.globalsign.com/scripts/timestamp.dll /i "GlobalSign CodeSigning" /n "Christian Auer" %1
goto next


:app
  signtool sign /t http://timestamp.globalsign.com/scripts/timestamp.dll /i "GlobalSign CodeSigning" /n "Christian Auer" %1
goto next


:err
echo.
echo.
echo.
echo !! WARNING Error occurred !!
echo !!!!!!!!!!!!!!!!!!!!!!!!!!!!
echo.
echo.
pause
goto end


:next
IF NOT %ERRORLEVEL% == 0 goto Err

if %3x == silentx goto end
pause


:end
