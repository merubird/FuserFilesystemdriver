@echo off
if NOT %1x == x goto start
echo start make all?
pause

..\..\..\Tools\FuserVersionAutoincrement\FuserVersionAutoincrement\bin\Release\FuserVersionAutoincrement.exe ..\..\ auto silent
%0 WAIT
goto end
:start

CALL _make-all-internal.bat   FuserDriver       %1 %2 %3 %4 %5
CALL _make-all-internal.bat   FuserUsermodeLib  %1 %2 %3 %4 %5
CALL _make-all-internal.bat   FuserDeviceAgent  %1 %2 %3 %4 %5
CALL _make-all-internal.bat   FuserDemo         %1 %2 %3 %4 %5


echo completed
:end





