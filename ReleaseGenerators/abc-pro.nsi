Unicode true
RequestExecutionLevel user
SetCompressor /SOLID lzma

!include "MUI2.nsh"

!ifndef SOURCE_DIR
  !error "Pass /DSOURCE_DIR=<portable package directory>"
!endif
!ifndef LAUNCHER
  !error "Pass /DLAUNCHER=<abc.pro.exe>"
!endif
!ifndef BRANDED_IDE
  !error "Pass /DBRANDED_IDE=<PascalABCNET.exe>"
!endif
!ifndef NOTEBOOK_LAUNCHER
  !error "Pass /DNOTEBOOK_LAUNCHER=<abc.pro-notebook.cmd>"
!endif
!ifndef IDE_COMMAND
  !error "Pass /DIDE_COMMAND=<PascalABCNET.cmd>"
!endif
!ifndef NOTEBOOK_COMMAND
  !error "Pass /DNOTEBOOK_COMMAND=<PascalABCNotebook.cmd>"
!endif
!ifndef OUTPUT_FILE
  !error "Pass /DOUTPUT_FILE=<installer path>"
!endif
!ifndef APP_VERSION
  !error "Pass /DAPP_VERSION=<four-part version>"
!endif

!define UNINSTALL_KEY "Software\Microsoft\Windows\CurrentVersion\Uninstall\abc.pro"
!define PRODUCT_KEY "Software\abc.pro"
!define MUI_ICON "..\VisualPascalABCNET\_ABC26.ico"
!define MUI_UNICON "..\VisualPascalABCNET\_ABC26.ico"
!define MUI_ABORTWARNING
!define MUI_FINISHPAGE_RUN "$INSTDIR\PascalABCNET.cmd"
!define MUI_FINISHPAGE_RUN_TEXT "Запустить abc.pro"

Name "abc.pro"
OutFile "${OUTPUT_FILE}"
InstallDir "$LOCALAPPDATA\Programs\abc.pro"
InstallDirRegKey HKCU "${PRODUCT_KEY}" "InstallDir"
BrandingText "abc.pro"
VIProductVersion "${APP_VERSION}"
VIAddVersionKey "ProductName" "abc.pro"
VIAddVersionKey "FileDescription" "abc.pro installer"
VIAddVersionKey "FileVersion" "${APP_VERSION}"

!insertmacro MUI_PAGE_WELCOME
!insertmacro MUI_PAGE_DIRECTORY
!insertmacro MUI_PAGE_INSTFILES
!insertmacro MUI_PAGE_FINISH
!insertmacro MUI_UNPAGE_CONFIRM
!insertmacro MUI_UNPAGE_INSTFILES
!insertmacro MUI_LANGUAGE "Russian"
!insertmacro MUI_LANGUAGE "English"

Section "abc.pro" MainSection
  SetShellVarContext current
  IfFileExists "$INSTDIR\bin\PascalABCNET.exe" 0 install
  IfFileExists "$INSTDIR\abc.pro.exe" install 0
  MessageBox MB_ICONSTOP "В выбранной папке уже есть другая сборка PascalABC.NET. Выберите отдельную папку для abc.pro."
  Abort
install:
  SetOutPath "$INSTDIR"
  File /r /x Work /x pabcworknet.ini /x PascalABCNET.cmd /x PascalABCNotebook.cmd "${SOURCE_DIR}\*"
  SetOutPath "$INSTDIR\bin"
  File /oname=PascalABCNET.exe "${BRANDED_IDE}"
  SetOutPath "$INSTDIR"
  File /oname=abc.pro.exe "${LAUNCHER}"
  File /oname=abc.pro-notebook.cmd "${NOTEBOOK_LAUNCHER}"
  File /oname=PascalABCNET.cmd "${IDE_COMMAND}"
  File /oname=PascalABCNotebook.cmd "${NOTEBOOK_COMMAND}"
  SetOutPath "$INSTDIR\Templates\Samples"
  File /r "${SOURCE_DIR}\Work\Samples\*"
  SetOutPath "$INSTDIR\Templates\Files"
  File /r "${SOURCE_DIR}\Work\Files\*"

  WriteUninstaller "$INSTDIR\Uninstall.exe"
  CreateDirectory "$SMPROGRAMS\abc.pro"
  CreateShortCut "$SMPROGRAMS\abc.pro\abc.pro.lnk" "$INSTDIR\PascalABCNET.cmd" "" "$INSTDIR\abc.pro.exe"
  CreateShortCut "$SMPROGRAMS\abc.pro\abc.pro Notebook.lnk" "$INSTDIR\abc.pro-notebook.cmd" "" "$INSTDIR\abc.pro.exe"
  CreateShortCut "$SMPROGRAMS\abc.pro\Отчёты об ошибках.lnk" "$INSTDIR\Open-logs.cmd" "" "$INSTDIR\abc.pro.exe"
  CreateShortCut "$SMPROGRAMS\abc.pro\Удалить abc.pro.lnk" "$INSTDIR\Uninstall.exe"

  WriteRegStr HKCU "${PRODUCT_KEY}" "InstallDir" "$INSTDIR"
  WriteRegStr HKCU "${UNINSTALL_KEY}" "DisplayName" "abc.pro"
  WriteRegStr HKCU "${UNINSTALL_KEY}" "DisplayVersion" "${APP_VERSION}"
  WriteRegStr HKCU "${UNINSTALL_KEY}" "DisplayIcon" "$INSTDIR\abc.pro.exe"
  WriteRegStr HKCU "${UNINSTALL_KEY}" "InstallLocation" "$INSTDIR"
  WriteRegStr HKCU "${UNINSTALL_KEY}" "UninstallString" '"$INSTDIR\Uninstall.exe"'
  WriteRegStr HKCU "${UNINSTALL_KEY}" "QuietUninstallString" '"$INSTDIR\Uninstall.exe" /S'
  WriteRegDWORD HKCU "${UNINSTALL_KEY}" "NoModify" 1
  WriteRegDWORD HKCU "${UNINSTALL_KEY}" "NoRepair" 1
SectionEnd

Section "Uninstall"
  SetShellVarContext current
  Delete "$SMPROGRAMS\abc.pro\abc.pro.lnk"
  Delete "$SMPROGRAMS\abc.pro\abc.pro Notebook.lnk"
  Delete "$SMPROGRAMS\abc.pro\Отчёты об ошибках.lnk"
  Delete "$SMPROGRAMS\abc.pro\Удалить abc.pro.lnk"
  RMDir "$SMPROGRAMS\abc.pro"
  DeleteRegKey HKCU "${UNINSTALL_KEY}"
  DeleteRegKey HKCU "${PRODUCT_KEY}"

  RMDir /r "$INSTDIR\bin"
  RMDir /r "$INSTDIR\bin-net10"
  RMDir /r "$INSTDIR\dotnet"
  RMDir /r "$INSTDIR\notebook"
  RMDir /r "$INSTDIR\Portable"
  RMDir /r "$INSTDIR\python"
  RMDir /r "$INSTDIR\Templates"
  Delete "$INSTDIR\abc.pro.exe"
  Delete "$INSTDIR\abc.pro-notebook.cmd"
  Delete "$INSTDIR\copyright.txt"
  Delete "$INSTDIR\License_en.txt"
  Delete "$INSTDIR\License.txt"
  Delete "$INSTDIR\pabcnetc-net10.cmd"
  Delete "$INSTDIR\pabcnetc.cmd"
  Delete "$INSTDIR\PascalABCNET.cmd"
  Delete "$INSTDIR\PascalABCNotebook.cmd"
  Delete "$INSTDIR\README.txt"
  Delete "$INSTDIR\run-net10.cmd"
  Delete "$INSTDIR\Open-logs.cmd"
  Delete "$INSTDIR\abc.pro.exe.config"
  Delete "$INSTDIR\Uninstall.exe"
  RMDir "$INSTDIR"
SectionEnd
