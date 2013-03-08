@echo off
IF %1x == x GOTO end
IF NOT exist ..\%1\sources goto err


echo Compile: %1

cd ..\%1
cd
call ..\_general\_make-internal %2
cd ..\_general

goto end

:err
echo.
echo Project %1 not found!
echo.
goto end



:end
