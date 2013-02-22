@echo off
if NOT %1x == x goto start
echo start make all?
pause
%0 WAIT
goto end
:start

CALL _make-all-internal.bat   sys            %1 %2 %3 %4 %5
CALL _make-all-internal.bat   fuser          %1 %2 %3 %4 %5
CALL _make-all-internal.bat   fuser_mount    %1 %2 %3 %4 %5
CALL _make-all-internal.bat   fuser_control  %1 %2 %3 %4 %5
CALL _make-all-internal.bat   fuser_mirror   %1 %2 %3 %4 %5


echo completed
:end





