# Microsoft Developer Studio Project File - Name="ParaView" - Package Owner=<4>
# Microsoft Developer Studio Generated Build File, Format Version 6.00
# ** DO NOT EDIT **

# TARGTYPE "Win32 (x86) Application" 0x0101

CFG=ParaView - Win32 Debug
!MESSAGE This is not a valid makefile. To build this project using NMAKE,
!MESSAGE use the Export Makefile command and run
!MESSAGE 
!MESSAGE NMAKE /f "ParaView.mak".
!MESSAGE 
!MESSAGE You can specify a configuration when running NMAKE
!MESSAGE by defining the macro CFG on the command line. For example:
!MESSAGE 
!MESSAGE NMAKE /f "ParaView.mak" CFG="ParaView - Win32 Debug"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "ParaView - Win32 Release" (based on "Win32 (x86) Application")
!MESSAGE "ParaView - Win32 Debug" (based on "Win32 (x86) Application")
!MESSAGE 

# Begin Project
# PROP AllowPerConfigDependencies 0
# PROP Scc_ProjName ""
# PROP Scc_LocalPath ""
CPP=cl.exe
MTL=midl.exe
RSC=rc.exe

!IF  "$(CFG)" == "ParaView - Win32 Release"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir "Release"
# PROP BASE Intermediate_Dir "Release"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 0
# PROP Output_Dir "Release"
# PROP Intermediate_Dir "Release"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
# ADD BASE CPP /nologo /W3 /GX /O2 /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "_MBCS" /YX /FD /c
# ADD CPP /nologo /MD /W3 /GX /O2 /I "..\..\vtk\pcmaker\xlib" /I "..\MercuryCommon" /I "..\Widgets" /I "C:\program files\html help workshop\include" /I "..\..\vtk\common" /I "..\..\vtk\graphics" /I "..\..\vtk\imaging" /I "..\..\vtk\contrib" /D "NDEBUG" /D "WIN32" /D "_WINDOWS" /D "_MBCS" /D "VTKDLL" /YX /FD /c
# ADD BASE MTL /nologo /D "NDEBUG" /mktyplib203 /win32
# ADD MTL /nologo /D "NDEBUG" /mktyplib203 /win32
# ADD BASE RSC /l 0x409 /d "NDEBUG"
# ADD RSC /l 0x409 /d "NDEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:windows /machine:I386
# ADD LINK32 ..\Widgets\Release\vtkKWWidgetsTcl.lib ..\..\vtk\pcmaker\tk82.lib ..\..\vtk\pcmaker\tcl82.lib vtktcl.lib vtkCommon.lib vtkImaging.lib vtkGraphics0.lib vtkGraphics1.lib vtkGraphics2.lib vtkGraphics3.lib vtkGraphics4.lib vtkContrib.lib htmlhelp.lib kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /dll /machine:I386 /out:"Release/vtkKWParaViewTcl.dll" /libpath:"..\..\vtkbin\lib" /libpath:"C:\program files\html help workshop\lib"
# SUBTRACT LINK32 /pdb:none

!ELSEIF  "$(CFG)" == "ParaView - Win32 Debug"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir "Debug"
# PROP BASE Intermediate_Dir "Debug"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 1
# PROP Output_Dir "Debug"
# PROP Intermediate_Dir "Debug"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
# ADD BASE CPP /nologo /W3 /Gm /GX /ZI /Od /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /D "_MBCS" /YX /FD /GZ /c
# ADD CPP /nologo /MD /W3 /Gm /GX /ZI /Od /I "..\..\vtk\pcmaker\xlib" /I "..\MercuryCommon" /I "..\Widgets" /I "C:\program files\html help workshop\include" /I "..\..\vtk\common" /I "..\..\vtk\graphics" /I "..\..\vtk\imaging" /I "..\..\vtk\contrib" /D "_DEBUG" /D "WIN32" /D "_WINDOWS" /D "_MBCS" /D "VTKDLL" /YX /FD /GZ /c
# ADD BASE MTL /nologo /D "_DEBUG" /mktyplib203 /win32
# ADD MTL /nologo /D "_DEBUG" /mktyplib203 /win32
# ADD BASE RSC /l 0x409 /d "_DEBUG"
# ADD RSC /l 0x409 /d "_DEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:windows /debug /machine:I386 /pdbtype:sept
# ADD LINK32 ..\Widgets\Debug\vtkKWWidgetsTcl.lib ..\..\vtk\pcmaker\tk82.lib ..\..\vtk\pcmaker\tcl82.lib vtktcl.lib vtkCommon.lib vtkImaging.lib vtkGraphics0.lib vtkGraphics1.lib vtkGraphics2.lib vtkGraphics3.lib vtkGraphics4.lib vtkContrib.lib htmlhelp.lib kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /dll /debug /machine:I386 /out:"Debug/vtkKWParaViewTcl.dll" /pdbtype:sept /libpath:"..\..\vtkbin\debug\lib" /libpath:"C:\program files\html help workshop\lib"
# SUBTRACT LINK32 /pdb:none

!ENDIF 

# Begin Target

# Name "ParaView - Win32 Release"
# Name "ParaView - Win32 Debug"
# Begin Group "Source Files"

# PROP Default_Filter "cpp;c;cxx;rc;def;r;odl;idl;hpj;bat"
# Begin Source File

SOURCE=.\tcl\KWParaViewInit.cxx
# End Source File
# Begin Source File

SOURCE=.\vtkFeldkampConeBeamReconstructionFactory.cxx
# End Source File
# Begin Source File

SOURCE=.\vtkFeldkampConeBeamReconstructionFactoryTcl.cxx
# End Source File
# Begin Source File

SOURCE=.\vtkKWFactoryBrowser.cxx
# End Source File
# Begin Source File

SOURCE=.\vtkKWFactoryBrowserTcl.cxx
# End Source File
# Begin Source File

SOURCE=.\vtkKWRenderView.cxx
# End Source File
# Begin Source File

SOURCE=.\vtkKWRenderViewTcl.cxx
# End Source File
# Begin Source File

SOURCE=.\vtkKWVolumeView.cxx
# End Source File
# Begin Source File

SOURCE=.\vtkKWVolumeViewTcl.cxx
# End Source File
# Begin Source File

SOURCE=.\vtkLoadGantryDataDialog.cxx
# End Source File
# Begin Source File

SOURCE=.\vtkLoadGantryDataDialogTcl.cxx
# End Source File
# Begin Source File

SOURCE=.\vtkParaViewApplication.cxx
# End Source File
# Begin Source File

SOURCE=.\vtkParaViewApplicationTcl.cxx
# End Source File
# Begin Source File

SOURCE=.\vtkParaViewVolumeComposite.cxx
# End Source File
# Begin Source File

SOURCE=.\vtkParaViewVolumeCompositeTcl.cxx
# End Source File
# Begin Source File

SOURCE=.\vtkParaViewVolumeView.cxx
# End Source File
# Begin Source File

SOURCE=.\vtkParaViewVolumeViewTcl.cxx
# End Source File
# Begin Source File

SOURCE=.\vtkParaViewWindow.cxx
# End Source File
# Begin Source File

SOURCE=.\vtkParaViewWindowTcl.cxx
# End Source File
# End Group
# Begin Group "Header Files"

# PROP Default_Filter "h;hpp;hxx;hm;inl"
# Begin Source File

SOURCE=.\vtkFeldkampConeBeamReconstructionFactory.h

!IF  "$(CFG)" == "ParaView - Win32 Release"

# Begin Custom Build
InputPath=.\vtkFeldkampConeBeamReconstructionFactory.h
InputName=vtkFeldkampConeBeamReconstructionFactory

"$(InputName)Tcl.cxx" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	..\..\vtk\pcmaker\vtkWrapTcl.exe $(InputName).h hints 1 > $(InputName)Tcl.cxx

# End Custom Build

!ELSEIF  "$(CFG)" == "ParaView - Win32 Debug"

# Begin Custom Build
InputPath=.\vtkFeldkampConeBeamReconstructionFactory.h
InputName=vtkFeldkampConeBeamReconstructionFactory

"$(InputName)Tcl.cxx" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	..\..\vtk\pcmaker\vtkWrapTcl.exe $(InputName).h hints 1 > $(InputName)Tcl.cxx

# End Custom Build

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\vtkKWFactoryBrowser.h

!IF  "$(CFG)" == "ParaView - Win32 Release"

# Begin Custom Build
InputPath=.\vtkKWFactoryBrowser.h
InputName=vtkKWFactoryBrowser

"$(InputName)Tcl.cxx" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	..\..\vtk\pcmaker\vtkWrapTcl.exe $(InputName).h hints 1 > $(InputName)Tcl.cxx

# End Custom Build

!ELSEIF  "$(CFG)" == "ParaView - Win32 Debug"

# Begin Custom Build
InputPath=.\vtkKWFactoryBrowser.h
InputName=vtkKWFactoryBrowser

"$(InputName)Tcl.cxx" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	..\..\vtk\pcmaker\vtkWrapTcl.exe $(InputName).h hints 1 > $(InputName)Tcl.cxx

# End Custom Build

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\vtkKWRenderView.h

!IF  "$(CFG)" == "ParaView - Win32 Release"

# Begin Custom Build
InputPath=.\vtkKWRenderView.h
InputName=vtkKWRenderView

"$(InputName)Tcl.cxx" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	..\..\vtk\pcmaker\vtkWrapTcl.exe $(InputName).h hints 1 > $(InputName)Tcl.cxx

# End Custom Build

!ELSEIF  "$(CFG)" == "ParaView - Win32 Debug"

# Begin Custom Build
InputPath=.\vtkKWRenderView.h
InputName=vtkKWRenderView

"$(InputName)Tcl.cxx" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	..\..\vtk\pcmaker\vtkWrapTcl.exe $(InputName).h hints 1 > $(InputName)Tcl.cxx

# End Custom Build

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\vtkKWVolumeView.h

!IF  "$(CFG)" == "ParaView - Win32 Release"

# Begin Custom Build
InputPath=.\vtkKWVolumeView.h
InputName=vtkKWVolumeView

"$(InputName)Tcl.cxx" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	..\..\vtk\pcmaker\vtkWrapTcl.exe $(InputName).h hints 1 > $(InputName)Tcl.cxx

# End Custom Build

!ELSEIF  "$(CFG)" == "ParaView - Win32 Debug"

# Begin Custom Build
InputPath=.\vtkKWVolumeView.h
InputName=vtkKWVolumeView

"$(InputName)Tcl.cxx" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	..\..\vtk\pcmaker\vtkWrapTcl.exe $(InputName).h hints 1 > $(InputName)Tcl.cxx

# End Custom Build

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\vtkLoadGantryDataDialog.h

!IF  "$(CFG)" == "ParaView - Win32 Release"

# Begin Custom Build
InputPath=.\vtkLoadGantryDataDialog.h
InputName=vtkLoadGantryDataDialog

"$(InputName)Tcl.cxx" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	..\..\vtk\pcmaker\vtkWrapTcl.exe $(InputName).h hints 1 > $(InputName)Tcl.cxx

# End Custom Build

!ELSEIF  "$(CFG)" == "ParaView - Win32 Debug"

# Begin Custom Build
InputPath=.\vtkLoadGantryDataDialog.h
InputName=vtkLoadGantryDataDialog

"$(InputName)Tcl.cxx" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	..\..\vtk\pcmaker\vtkWrapTcl.exe $(InputName).h hints 1 > $(InputName)Tcl.cxx

# End Custom Build

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\vtkParaViewApplication.h

!IF  "$(CFG)" == "ParaView - Win32 Release"

# Begin Custom Build
InputPath=.\vtkParaViewApplication.h
InputName=vtkParaViewApplication

"$(InputName)Tcl.cxx" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	..\..\vtk\pcmaker\vtkWrapTcl.exe $(InputName).h hints 1 > $(InputName)Tcl.cxx

# End Custom Build

!ELSEIF  "$(CFG)" == "ParaView - Win32 Debug"

# Begin Custom Build
InputPath=.\vtkParaViewApplication.h
InputName=vtkParaViewApplication

"$(InputName)Tcl.cxx" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	..\..\vtk\pcmaker\vtkWrapTcl.exe $(InputName).h hints 1 > $(InputName)Tcl.cxx

# End Custom Build

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\vtkParaViewVolumeComposite.h

!IF  "$(CFG)" == "ParaView - Win32 Release"

# Begin Custom Build
InputPath=.\vtkParaViewVolumeComposite.h
InputName=vtkParaViewVolumeComposite

"$(InputName)Tcl.cxx" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	..\..\vtk\pcmaker\vtkWrapTcl.exe $(InputName).h hints 1 > $(InputName)Tcl.cxx

# End Custom Build

!ELSEIF  "$(CFG)" == "ParaView - Win32 Debug"

# Begin Custom Build
InputPath=.\vtkParaViewVolumeComposite.h
InputName=vtkParaViewVolumeComposite

"$(InputName)Tcl.cxx" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	..\..\vtk\pcmaker\vtkWrapTcl.exe $(InputName).h hints 1 > $(InputName)Tcl.cxx

# End Custom Build

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\vtkParaViewVolumeView.h

!IF  "$(CFG)" == "ParaView - Win32 Release"

# Begin Custom Build
InputPath=.\vtkParaViewVolumeView.h
InputName=vtkParaViewVolumeView

"$(InputName)Tcl.cxx" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	..\..\vtk\pcmaker\vtkWrapTcl.exe $(InputName).h hints 1 > $(InputName)Tcl.cxx

# End Custom Build

!ELSEIF  "$(CFG)" == "ParaView - Win32 Debug"

# Begin Custom Build
InputPath=.\vtkParaViewVolumeView.h
InputName=vtkParaViewVolumeView

"$(InputName)Tcl.cxx" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	..\..\vtk\pcmaker\vtkWrapTcl.exe $(InputName).h hints 1 > $(InputName)Tcl.cxx

# End Custom Build

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\vtkParaViewWindow.h

!IF  "$(CFG)" == "ParaView - Win32 Release"

# Begin Custom Build
InputPath=.\vtkParaViewWindow.h
InputName=vtkParaViewWindow

"$(InputName)Tcl.cxx" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	..\..\vtk\pcmaker\vtkWrapTcl.exe $(InputName).h hints 1 > $(InputName)Tcl.cxx

# End Custom Build

!ELSEIF  "$(CFG)" == "ParaView - Win32 Debug"

# Begin Custom Build
InputPath=.\vtkParaViewWindow.h
InputName=vtkParaViewWindow

"$(InputName)Tcl.cxx" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	..\..\vtk\pcmaker\vtkWrapTcl.exe $(InputName).h hints 1 > $(InputName)Tcl.cxx

# End Custom Build

!ENDIF 

# End Source File
# End Group
# Begin Group "Resource Files"

# PROP Default_Filter "ico;cur;bmp;dlg;rc2;rct;bin;rgs;gif;jpg;jpeg;jpe"
# End Group
# End Target
# End Project
