; ParaView install script designed for a nmake build

;--------------------------------
; You must define these values

  !define VERSION "1.6"
  !define PATCH  "2"
  !define PV_BIN "C:\martink\ParaView\ParaView16Rel"
  !define PV_SRC "C:\martink\ParaView\ParaView16"


;--------------------------------
;Variables

  Var MUI_TEMP
  Var STARTMENU_FOLDER

;--------------------------------
;Include Modern UI

  !include "MUI.nsh"

;--------------------------------
;General

  ;Name and file
  Name "ParaView ${VERSION}"
  OutFile "paraview-${VERSION}.${PATCH}-win32.exe"

  ;Default installation folder
  InstallDir "$PROGRAMFILES\ParaView ${VERSION}"
  
  ;Get installation folder from registry if available
  InstallDirRegKey HKCU "Software\Kitware\ParaView ${VERSION}" ""

;--------------------------------
;Interface Settings

  !define MUI_HEADERIMAGE
  !define MUI_ABORTWARNING

;--------------------------------
;Pages

  !insertmacro MUI_PAGE_DIRECTORY
  !insertmacro MUI_PAGE_INSTFILES
  
  ;Start Menu Folder Page Configuration
  !define MUI_STARTMENUPAGE_REGISTRY_ROOT "HKCU" 
  !define MUI_STARTMENUPAGE_REGISTRY_KEY "Software\Kitware\ParaView ${VERSION}" 
  !define MUI_STARTMENUPAGE_REGISTRY_VALUENAME "Start Menu Folder"
  !insertmacro MUI_PAGE_STARTMENU Application $STARTMENU_FOLDER

  !insertmacro MUI_UNPAGE_CONFIRM
  !insertmacro MUI_UNPAGE_INSTFILES
  
;--------------------------------
;Languages
 
  !insertmacro MUI_LANGUAGE "English"

;--------------------------------
;Installer Sections

Section "Dummy Section" SecDummy

  SetOutPath "$INSTDIR\bin"
  File "${PV_BIN}\bin\paraview.exe"
  File "${PV_BIN}\bin\tcl84.dll"
  File "${PV_BIN}\bin\tk84.dll"
  File "C:\martink\msvcr71.dll"
  File "C:\martink\msvcp71.dll"
  
  SetOutPath "$INSTDIR"
  File /r "${PV_BIN}\lib"
  File /r "${PV_SRC}\GUI\Demos"

  SetOutPath "$INSTDIR\Data"
  File /r "${PV_SRC}\Data\Data"

  ;Store installation folder
  WriteRegStr HKCU "Software\Kitware\ParaView ${VERSION}" "" $INSTDIR
  
  ;Create uninstaller
  WriteUninstaller "$INSTDIR\Uninstall.exe"

  !insertmacro MUI_STARTMENU_WRITE_BEGIN Application
    
  ;Create shortcuts
  CreateDirectory "$SMPROGRAMS\$STARTMENU_FOLDER"
  CreateShortCut "$SMPROGRAMS\$STARTMENU_FOLDER\paraview.lnk" "$INSTDIR\bin\paraview.exe"
  CreateShortCut "$SMPROGRAMS\$STARTMENU_FOLDER\Uninstall.lnk" "$INSTDIR\Uninstall.exe"
  
  !insertmacro MUI_STARTMENU_WRITE_END

  ; back up old value of .opt
  !define Index "Line${__LINE__}"
  ReadRegStr $1 HKCR ".pvs" ""
  StrCmp $1 "" "${Index}-NoBackup"
    StrCmp $1 "ParaView.FileType" "${Index}-NoBackup"
    WriteRegStr HKCR ".pvs" "backup_val" $1
"${Index}-NoBackup:"
  WriteRegStr HKCR ".pvs" "" "ParaView.FileType"
  ReadRegStr $0 HKCR "ParaView.FileType" ""
  StrCmp $0 "" 0 "${Index}-Skip"
	WriteRegStr HKCR "ParaView.FileType" "" "ParaView Files"
	WriteRegStr HKCR "ParaView.FileType\shell" "" "open"
	WriteRegStr HKCR "ParaView.FileType\DefaultIcon" "" "$INSTDIR\bin\paraview.exe,0"
"${Index}-Skip:"
  WriteRegStr HKCR "ParaView.FileType\shell\open\command" "" \
    '$INSTDIR\bin\paraview.exe "%1"'
  WriteRegStr HKCR "ParaView.FileType\shell\edit" "" "Edit Options File"
  WriteRegStr HKCR "ParaView.FileType\shell\edit\command" "" \
    '$INSTDIR\bin\paraview.exe "%1"'
!undef Index

SectionEnd


;--------------------------------
;Uninstaller Section

Section "Uninstall"

  ;ADD YOUR OWN FILES HERE...

  Delete "$INSTDIR\Uninstall.exe"

  RMDir "$INSTDIR"

  !insertmacro MUI_STARTMENU_GETFOLDER Application $MUI_TEMP
    
  Delete "$SMPROGRAMS\$MUI_TEMP\Uninstall.lnk"
  
  ;Delete empty start menu parent diretories
  StrCpy $MUI_TEMP "$SMPROGRAMS\$MUI_TEMP"
 
  startMenuDeleteLoop:
    RMDir $MUI_TEMP
    GetFullPathName $MUI_TEMP "$MUI_TEMP\.."
    
    IfErrors startMenuDeleteLoopDone
  
    StrCmp $MUI_TEMP $SMPROGRAMS startMenuDeleteLoopDone startMenuDeleteLoop
  startMenuDeleteLoopDone:

  DeleteRegKey /ifempty HKCU "Software\Kitware\ParaView ${VERSION}"

SectionEnd