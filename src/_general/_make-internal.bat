@echo off

IF %1x == allx GOTO allc
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

:allc
echo.
echo Auto-All compile...
GOTO compile

:compile

echo ;BuildVersion  >..\_general\_build.ver
echo #define VER_YEAR "%date:~-4%"   >>..\_general\_build.ver
IF %_BUILDARCH% == x86    echo #define VER_ARCH "32bit"   >>..\_general\_build.ver
IF %_BUILDARCH% == AMD64  echo #define VER_ARCH "64bit"   >>..\_general\_build.ver

IF %_BuildType% == chk    echo #define VER_BUILDDEBUG "--DEBUG VERSION --"   >>..\_general\_build.ver
IF %_BuildType% == fre    echo #define VER_BUILDDEBUG " "   >>..\_general\_build.ver

build /wcbg

