@echo off
IF %1x == x GOTO end


echo Compile: %1

cd ..\%1
cd
call ..\_general\_make-internal %2
cd ..\_general





:end
