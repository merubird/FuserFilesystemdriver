

;--------------------------------
; Includes

	!include LogicLib.nsh
	!include x64.nsh
	!include WinVer.nsh
		
	!include scripts\languages.nsi

;--------------------------------




;--------------------------------
; Version Information

	!define VERSION "pre0.0.C"
	;VIProductVersion "${VERSION}"
	VIProductVersion "1.2.3.4"
	VIAddVersionKey /LANG=${LANG_ENGLISH} "ProductName" "FuserFilesystem Driver Setup"
	VIAddVersionKey /LANG=${LANG_ENGLISH} "Comments" "Fuser Filesystem Driver - simulates virtual drives"
	VIAddVersionKey /LANG=${LANG_ENGLISH} "CompanyName" "Christian Auer"
	VIAddVersionKey /LANG=${LANG_ENGLISH} "LegalCopyright" "Copyright Christian Auer"
	VIAddVersionKey /LANG=${LANG_ENGLISH} "FileDescription" "FuserFilesystem Driver Setup"
	VIAddVersionKey /LANG=${LANG_ENGLISH} "FileVersion" "${VERSION}"
	
;--------------------------------



;--------------------------------
; General Settings:

	Name "FuserFilesystem Driver Setup ${VERSION}"
	OutFile "FuserFilesystemDriver-${VERSION}-Setup.exe"
	InstallDir $PROGRAMFILES64\FuserFilesystemDriver
	RequestExecutionLevel admin	
	ShowUninstDetails show	

;--------------------------------

;--------------------------------
; Pages
	
	Page components
	Page instfiles

	UninstPage uninstConfirm
	UninstPage instfiles

;--------------------------------



!macro driverinstaller opt
	${If} ${RunningX64}
		ExecWait '"$PROGRAMFILES64\FuserFilesystemDriver\fuserctl.exe" ${opt}' $0
	${Else}
		ExecWait '"$PROGRAMFILES\FuserFilesystemDriver\fuserctl.exe" ${opt}' $0
	${EndIf}	
	DetailPrint "fuserctl returned $0"
!macroend


!macro X86Files os

  SetOutPath $PROGRAMFILES\FuserFilesystemDriver
  
    File ..\bin\${os}_x86\fuserctl.exe
    File ..\bin\${os}_x86\mounter.exe
    File ..\bin\${os}_x86\mirror.exe

  SetOutPath $SYSDIR

    File ..\bin\${os}_x86\fuser.dll

!macroend


!macro X64Files os

  SetOutPath $PROGRAMFILES64\FuserFilesystemDriver
  
	File ..\bin\${os}_amd64\fuserctl.exe
    File ..\bin\${os}_amd64\mounter.exe
    File ..\bin\${os}_amd64\mirror.exe	

  ${DisableX64FSRedirection}

  SetOutPath $SYSDIR
	/*  Installing 64bit Usermode Treiber */
    File ..\bin\${os}_amd64\fuser.dll

  ${EnableX64FSRedirection}
  
  SetOutPath $SYSDIR
	/*  Installing 32bit Usermode Treiber */
    File ..\bin\${os}_x86\fuser.dll

!macroend



!macro X86Driver os
  SetOutPath $SYSDIR\drivers
    File ..\bin\${os}_x86\fuser.sys
!macroend

!macro X64Driver os
  ${DisableX64FSRedirection}

  SetOutPath $SYSDIR\drivers

    File ..\bin\${os}_amd64\fuser.sys

  ${EnableX64FSRedirection}
!macroend




!macro FuserSetup
	!insertmacro driverinstaller "/i a"

	${If} ${RunningX64}
		WriteUninstaller $PROGRAMFILES64\FuserFilesystemDriver\FuserUninstall.exe
		WriteRegStr HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\FuserFilesystemDriver" "UninstallString" '"$PROGRAMFILES64\FuserFilesystemDriver\FuserUninstall.exe"'
	${Else}
		WriteUninstaller $PROGRAMFILES\FuserFilesystemDriver\FuserUninstall.exe
		WriteRegStr HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\FuserFilesystemDriver" "UninstallString" '"$PROGRAMFILES\FuserFilesystemDriver\FuserUninstall.exe"'
	${EndIf}
   

	; Write the uninstall keys for Windows
	WriteRegStr HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\FuserFilesystemDriver" "DisplayName" "Fuser Filesystem Driver ${VERSION}"  
	WriteRegDWORD HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\FuserFilesystemDriver" "NoModify" 1
	WriteRegDWORD HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\FuserFilesystemDriver" "NoRepair" 1
!macroend



Section "Fuser Driver x86" section_x86
  ${If} ${IsWin7}
    !insertmacro X86Files "win7"
  ${ElseIf} ${IsWin2008R2}
    !insertmacro X86Files "win7"
  ${ElseIf} ${IsWinVista}
    !insertmacro X86Files "wlh"
  ${ElseIf} ${IsWin2008}
    !insertmacro X86Files "wlh"
  ${ElseIf} ${IsWin2003}
    !insertmacro X86Files "wnet"
  ${ElseIf} ${IsWinXp}
    !insertmacro X86Files "wxp"
  ${EndIf}
SectionEnd

Section "Fuser Driver x64" section_x64
  ${If} ${IsWin7}
    !insertmacro X64Files "win7"
  ${ElseIf} ${IsWin2008R2}
    !insertmacro X64Files "win7"
  ${ElseIf} ${IsWinVista}
    !insertmacro X64Files "wlh"
  ${ElseIf} ${IsWin2008}
    !insertmacro X64Files "wlh"
  ${ElseIf} ${IsWin2003}
    !insertmacro X64Files "wnet"  
  ${EndIf}  
SectionEnd


Section "Fuser Kernel-Mode-Driver x86" section_x86_driver

  ${If} ${IsWin7}
    !insertmacro X86Driver "win7"
  ${ElseIf} ${IsWinVista}
    !insertmacro X86Driver "wlh"
  ${ElseIf} ${IsWin2008}
    !insertmacro X86Driver "wlh"
  ${ElseIf} ${IsWin2003}
    !insertmacro X86Driver "wnet"
  ${ElseIf} ${IsWinXp}
    !insertmacro X86Driver "wxp"
  ${EndIf}

  !insertmacro FuserSetup
SectionEnd


Section "Fuser Kernel-Mode-Driver amd64" section_x64_driver
  ${If} ${IsWin7}
    !insertmacro X64Driver "win7"
  ${ElseIf} ${IsWin2008R2}
    !insertmacro X64Driver "win7"
  ${ElseIf} ${IsWinVista}
    !insertmacro X64Driver "wlh"
  ${ElseIf} ${IsWin2008}
    !insertmacro X64Driver "wlh"
  ${ElseIf} ${IsWin2003}
    !insertmacro X64Driver "wnet"
  ${EndIf}
  !insertmacro FuserSetup
SectionEnd








Section "Uninstall"
  !insertmacro driverinstaller "/r a"
  
  ${If} ${RunningX64}
	RMDir /r $PROGRAMFILES64\FuserFilesystemDriver  
	Delete $SYSDIR\fuser.dll
  
    ${DisableX64FSRedirection}
      Delete $SYSDIR\drivers\fuser.sys
	  Delete $SYSDIR\fuser.dll
    ${EnableX64FSRedirection}
  ${Else}
	RMDir /r $PROGRAMFILES\FuserFilesystemDriver  
	Delete $SYSDIR\fuser.dll
  
    Delete $SYSDIR\drivers\fuser.sys
  ${EndIf}

  ; Remove registry keys
  DeleteRegKey HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\FuserFilesystemDriver"

  IfSilent noreboot
    MessageBox MB_YESNO "A reboot is required to finish the uninstallation. Do you wish to reboot now?" IDNO noreboot
    Reboot
  noreboot:
SectionEnd



Function .onInit
  IntOp $0 ${SF_SELECTED} | ${SF_RO}
  ${If} ${RunningX64}
    SectionSetFlags ${section_x64} $0
	SectionSetFlags ${section_x86} ${SF_RO}  ; disable
    SectionSetFlags ${section_x86_driver} ${SF_RO}  ; disable
    SectionSetFlags ${section_x64_driver} $0
  ${Else}	 
    SectionSetFlags ${section_x86} $0
	SectionSetFlags ${section_x64} ${SF_RO}  ; disable
    SectionSetFlags ${section_x86_driver} $0
    SectionSetFlags ${section_x64_driver} ${SF_RO}  ; disable
  ${EndIf}
    

  ; Windows Version check

  ${If} ${RunningX64}
    ${If} ${IsWin2003}	
    ${ElseIf} ${IsWinVista}
    ${ElseIf} ${IsWin2008}
    ${ElseIf} ${IsWin2008R2}
    ${ElseIf} ${IsWin7}
    ${Else}
      MessageBox MB_OK "Your OS is not supported. Fuser Filesystem Driver supports Windows 2003, Vista, 2008, 2008R2 and 7 for x64."
      Abort
    ${EndIf}
  ${Else}
    ${If} ${IsWinXP}
    ${ElseIf} ${IsWin2003}
    ${ElseIf} ${IsWinVista}
    ${ElseIf} ${IsWin2008}
    ${ElseIf} ${IsWin7}
    ${Else}
      MessageBox MB_OK "Your OS is not supported. Fuser Filesystem Driver supports Windows XP, 2003, Vista, 2008 and 7 for x86."
      Abort
    ${EndIf}
  ${EndIf}

  ; Previous version
  ${If} ${RunningX64}
    ${DisableX64FSRedirection}
      IfFileExists $SYSDIR\drivers\fuser.sys HasPreviousVersionX64 NoPreviousVersionX64
      ; To make EnableX64FSRedirection called in both cases, needs duplicated MessageBox code. How can I avoid this?
      HasPreviousVersionX64:
        MessageBox MB_OK "Please unstall the previous version and restart your computer before running this installer."
        Abort
      NoPreviousVersionX64:
    ${EnableX64FSRedirection}
  ${Else}
    IfFileExists $SYSDIR\drivers\fuser.sys HasPreviousVersion NoPreviousVersion
    HasPreviousVersion:
      MessageBox MB_OK "Please unstall the previous version and restart your computer before running this installer."
      Abort
    NoPreviousVersion:
  ${EndIf}


FunctionEnd

Function .onInstSuccess
  IfSilent noshellopen
  
  ${If} ${RunningX64}
		ExecShell "open" "$PROGRAMFILES64\FuserFilesystemDriver"  
  ${Else}
		ExecShell "open" "$PROGRAMFILES\FuserFilesystemDriver"
  ${EndIf}
    
  noshellopen:
FunctionEnd

