@echo off

IF %1x == mergex GOTO mergec
IF %1x == testx GOTO testc
IF NOT %1x == x GOTO bc


echo.
echo -----------------------------------------------------------------------------------------------------------------------
echo -                                                 Compile Dev-Version                                                 -
echo -----------------------------------------------------------------------------------------------------------------------
echo.
GOTO compile

:bc
echo.
echo Unknown configuration selected
echo.
GOTO compile


:testc
echo.
echo.
echo !!!!      ATTENTION: TEST version selected      !!!!!
echo =====================================================
echo.
GOTO compile

:mergec
echo.
echo Merge-Version compile...
GOTO compile

:compile
build /wcbg

