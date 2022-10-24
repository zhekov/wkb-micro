#
# Copyright (C) 2019-2022 Dimitar Toshkov Zhekov <dimitar.zhekov@gmail.com>
#
# This program is free software; you can redistribute it and/or modify it
# under the terms of the GNU General Public License as published by the Free
# Software Foundation; either version 3 of the License, or (at your option)
# any later version.
#
# This program is distributed in the hope that it will be useful, but
# WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY
# or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License
# for more details.
#
# You should have received a copy of the GNU General Public License along
# with this program; if not, write to the Free Software Foundation, Inc.,
# 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
#

Unicode true
Name "WKB Layout"
OutFile "out\wkb-install-2.02-x64.exe"
RequestExecutionLevel admin
InstallDir "C:\Program Files\WKB Layout"  # /D= is accepted
CRCCheck force
ManifestDPIAware true


VIProductVersion "2.0.2.0"
VIAddVersionKey ProductName "WKB Layout"
VIAddVersionKey LegalCopyright "Copyright (C) 2016-2022 Dimitar Toshkov Zhekov"
VIAddVersionKey FileVersion 2.02
VIAddVersionKey ProductVersion 2.02
VIAddVersionKey FileDescription "WKB Layout Installer"


!include FileFunc.nsh
!include LogicLib.nsh
!include MUI2.nsh
!include WinVer.nsh
!include x64.nsh


# -- INTERFACE --
!define MUI_ICON "gucharmap1.ico"
!define MUI_UNICON "gucharmap1.ico"
!define MUI_WELCOMEFINISHPAGE_BITMAP "${NSISDIR}\Contrib\Graphics\Wizard\orange.bmp"

!insertmacro MUI_PAGE_WELCOME
!define MUI_LICENSEPAGE_RADIOBUTTONS
!insertmacro MUI_PAGE_LICENSE "gpl-3.0.txt"
Page custom customPageCreate
!insertmacro MUI_PAGE_INSTFILES
!define MUI_FINISHPAGE_NOAUTOCLOSE
!define MUI_FINISHPAGE_RUN
!define MUI_FINISHPAGE_RUN_FUNCTION runWkbMicroInstall
!define MUI_FINISHPAGE_SHOWREADME
!define MUI_FINISHPAGE_SHOWREADME_FUNCTION showWkbMicroReadMe
!define MUI_FINISHPAGE_SHOWREADME_NOTCHECKED
!insertmacro MUI_PAGE_FINISH

!insertmacro MUI_UNPAGE_INSTFILES
!insertmacro MUI_LANGUAGE "English"

Function customPageCreate
	!insertmacro MUI_HEADER_TEXT "Notes about WKB Layout" "From the ReadMe file."
	nsDialogs::Create 1018

	${NSD_CreateLabel} 0 0 100% 24u 'WKB Layout does not work with programs started "as administrator", \
		unless it$\'s also started as administrator, for example by the installer.'

	${NSD_CreateLabel} 0 24u 100% 24u 'Any assigned keys may still be used in combinations, such as \
		Ctrl+C, Alt-Click etc. They may work as standalone keys in some games as well.'

	${NSD_CreateLabel} 0 48u 100% 24u 'After adding or removing a keyboard from Windows Settings or \
		Control Panel, you will need to restart WKB Layout, or invoke and confirm WKB Settings.'

	nsDialogs::Show
FunctionEnd

# MUI_FINISHPAGE_RUN and MUI_FINISHPAGE_SHOWREADME do not show errors
!define wkbMicroExe "$INSTDIR\wkb-micro.exe"

Function runWkbMicroInstall
	ClearErrors
	Exec '"${wkbMicroExe}" /install'
	${If} ${Errors}
		MessageBox MB_OK "Failed to run WKB Layout.$\n\
			Use the Windows Start Menu to invoke it."
	${EndIf}
FunctionEnd

Function showWkbMicroReadMe
	ClearErrors
	ExecShell "open" "$INSTDIR\ReadMe.txt"
	${If} ${Errors}
		MessageBox MB_OK "Failed to show the WKB Layout ReadMe.$\n\
			Use the Windows Start Menu to view it."
	${EndIf}
FunctionEnd


# -- STARTUP --
!define v1UninstallKey "HKLM Software\Microsoft\Windows\CurrentVersion\Uninstall\wkb_mini"

Function .onInit
	${IfNot} ${AtLeastWin7}
		MessageBox MB_ICONSTOP|MB_OK "WKB Layout 2 requires Windows 7 or later." /SD IDOK
		Abort
	${EndIf}

	${IfNot} ${RunningX64}
		MessageBox MB_ICONSTOP|MB_OK "WKB Layout 2 requires a 64-bit operating system." /SD IDOK
		Abort
	${EndIf}

	ReadRegStr $0 ${v1UninstallKey} "UninstallString"
	ReadRegStr $1 ${v1UninstallKey} "DisplayVersion"
	${If} $0 != ""
		${IfThen} $1 == 0 ${|} StrCpy $1 1 ${|}
		MessageBox MB_ICONINFORMATION|MB_OK "WKB Layout $1 is currently installed. Please uninstall it first." /SD IDOK
		Abort
	${EndIf}
FunctionEnd


# -- (UN)INSTALL --
!macro deleteFileMacro fileName suggestion
	ClearErrors
	Delete "${fileName}"
	${If} ${Errors}
		${If} "${suggestion}" != ""
			MessageBox MB_ICONSTOP|MB_OK 'Failed to delete "${fileName}"$\n$\n${suggestion}' /SD IDOK
			Abort
		${Else}
			DetailPrint 'Failed to delete "${fileName}"'
		${EndIf}
	${EndIf}
!macroend

!define deleteFile `!insertmacro deleteFileMacro`
!define deleteLink `${deleteFile}`

!macro destroyWkbBaseMacro caller microExePath
	SetShellVarContext all
	${deleteLink} "$SMSTARTUP\WKB Layout.lnk" "Please delete it, and run the ${caller} again."
	ExecWait '"${microExePath}\wkb-micro.exe" /stop'
	Sleep 250
	# no result check: we run as Administrator, and the file deletion will fail with a proper text

 	StrCpy $R0 "Please run the ${caller} again after the computer is restarted, or all users sign out (log off)."
	${deleteFile} "$INSTDIR\wkb-micro.exe" $R0
	${deleteFile} "$INSTDIR\wkb-hook.dll" $R0
	${deleteFile} "$INSTDIR\gpl-3.0.txt" ""
	${deleteFile} "$INSTDIR\ReadMe.txt" ""
!macroend

!define wkbLinksDir "$SMPROGRAMS\WKB Layout"
!define uninstallKey "HKLM64 Software\Microsoft\Windows\CurrentVersion\Uninstall\WkbLayout2"

Section Install
	File "/oname=$PLUGINSDIR\wkb-micro.exe" "out\wkb-micro.exe"
	File "/oname=$PLUGINSDIR\wkb-hook.dll" "out\wkb-hook.dll"
	!insertmacro destroyWkbBaseMacro "installer" $PLUGINSDIR

	ClearErrors
	SetOutPath $INSTDIR
	WriteUninstaller "uninstall.exe"
	${If} ${Errors}
		MessageBox MB_ICONSTOP|MB_OK 'Failed to create "uninstall.exe" in "$INSTDIR".' /SD IDOK
		Abort
	${EndIf}

	WriteRegStr ${uninstallKey} "DisplayIcon" "${wkbMicroExe}"
	WriteRegStr ${uninstallKey} "DisplayName" "WKB Layout"
	WriteRegStr ${uninstallKey} "DisplayVersion" "2.02"
	WriteRegStr ${uninstallKey} "Publisher" "Dimitar Toshkov Zhekov"
	WriteRegStr ${uninstallKey} "InstallDir" "$INSTDIR"
	WriteRegStr ${uninstallKey} "UninstallString" '"$INSTDIR\uninstall.exe"'
	${If} ${Errors}
		MessageBox MB_ICONSTOP|MB_OK "Failed to create the uninstall registry keys." /SD IDOK
		Abort
	${EndIf}
	WriteRegDWORD ${uninstallKey} "NoModify" 1
	WriteRegDWORD ${uninstallKey} "NoRepair" 1
	WriteRegDWORD ${uninstallKey} "VersionMajor" 2
	WriteRegDWORD ${uninstallKey} "VersionMinor" 0
	WriteRegDWORD ${uninstallKey} "EstimatedSize" 172

	ClearErrors
	CopyFiles /SILENT /FILESONLY "$PLUGINSDIR\wkb-micro.exe" $INSTDIR
	CopyFiles /SILENT /FILESONLY "$PLUGINSDIR\wkb-hook.dll" $INSTDIR
	File "gpl-3.0.txt"
	File "ReadMe.txt"
	${If} ${Errors}
		MessageBox MB_ICONSTOP|MB_OK 'Failed to copy/extract file(s) to "$INSTDIR".' /SD IDOK
		Abort
	${EndIf}

	CreateShortCut "$SMSTARTUP\WKB Layout.lnk" "${wkbMicroExe}"
	${If} ${Errors}
		MessageBox MB_ICONSTOP|MB_OK 'Failed to create "WKB Layout.lnk" in "$SMSTARTUP".' /SD IDOK
		Abort
	${EndIf}

	CreateDirectory "${wkbLinksDir}"
	${If} ${Errors}
		MessageBox MB_ICONSTOP|MB_OK 'Failed to create "${wkbLinksDir}."' /SD IDOK
		Abort
	${EndIf}

	CreateShortCut "${wkbLinksDir}\WKB License.lnk" "$INSTDIR\gpl-3.0.txt"
	CreateShortCut "${wkbLinksDir}\WKB ReadMe.lnk" "$INSTDIR\ReadMe.txt"
	CreateShortCut "${wkbLinksDir}\WKB Settings.lnk" "${wkbMicroExe}" "/settings"
	CreateShortCut "${wkbLinksDir}\WKB Start.lnk" "${wkbMicroExe}"
	CreateShortCut "${wkbLinksDir}\WKB Stop.lnk" "${wkbMicroExe}" "/stop"
	${If} ${Errors}
		MessageBox MB_ICONSTOP|MB_OK 'Failed to create link(s) in "${wkbLinksDir}".' /SD IDOK
		Abort
	${EndIf}

	${If} ${Silent}
		${GetParameters} $0
		${GetOptions} $0 "/start" $1
		${IfNot} ${Errors}
			Exec '"${wkbMicroExe}"'
		${EndIf}
	${Else}
		SetDetailsView show
	${EndIf}
SectionEnd

Function un.onInit
	System::Call "User32::SetProcessDPIAware"
FunctionEnd

Section Uninstall
	MessageBox MB_ICONQUESTION|MB_YESNO "Do you really want to uninstall WKB Layout?" /SD IDYES IDYES +2
	Abort

	!insertmacro destroyWkbBaseMacro "uninstaller" $INSTDIR
	${deleteLink} "${wkbLinksDir}\WKB License.lnk"  ""
	${deleteLink} "${wkbLinksDir}\WKB ReadMe.lnk"   ""
	${deleteLink} "${wkbLinksDir}\WKB Settings.lnk" ""
	${deleteLink} "${wkbLinksDir}\WKB Start.lnk"    ""
	${deleteLink} "${wkbLinksDir}\WKB Stop.lnk"     ""
	RMDir "${wkbLinksDir}"
	
	Delete "$INSTDIR\uninstall.exe"
	${If} ${Errors}
		DetailPrint 'Failed to delete "$INSTDIR\uninstall.exe"'
	${Else}
		RMDir /REBOOTOK "$INSTDIR"
		DeleteRegKey ${uninstallKey}
	${EndIf}
	SetDetailsView show
SectionEnd
