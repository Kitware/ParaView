# Microsoft Developer Studio Project File - Name="Widgets" - Package Owner=<4>
# Microsoft Developer Studio Generated Build File, Format Version 6.00
# ** DO NOT EDIT **

# TARGTYPE "Win32 (x86) Dynamic-Link Library" 0x0102

CFG=Widgets - Win32 Debug
!MESSAGE This is not a valid makefile. To build this project using NMAKE,
!MESSAGE use the Export Makefile command and run
!MESSAGE 
!MESSAGE NMAKE /f "Widgets.mak".
!MESSAGE 
!MESSAGE You can specify a configuration when running NMAKE
!MESSAGE by defining the macro CFG on the command line. For example:
!MESSAGE 
!MESSAGE NMAKE /f "Widgets.mak" CFG="Widgets - Win32 Debug"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "Widgets - Win32 Release" (based on "Win32 (x86) Dynamic-Link Library")
!MESSAGE "Widgets - Win32 Debug" (based on "Win32 (x86) Dynamic-Link Library")
!MESSAGE 

# Begin Project
# PROP AllowPerConfigDependencies 0
# PROP Scc_ProjName ""
# PROP Scc_LocalPath ""
CPP=cl.exe
MTL=midl.exe
RSC=rc.exe

!IF  "$(CFG)" == "Widgets - Win32 Release"

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
# ADD BASE CPP /nologo /MT /W3 /GX /O2 /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "_MBCS" /D "_USRDLL" /D "Widgets_EXPORTS" /YX /FD /c
# ADD CPP /nologo /MD /W3 /GX /O2 /I "C:\program files\html help workshop\include" /I "..\..\vtk\common" /I "..\..\vtk\graphics" /I "..\..\vtk\imaging" /I "..\..\vtk\contrib" /I "..\..\Tcl8.0.5\generic" /I "..\..\Tk8.0.5\generic" /I "..\..\Tk8.0.5\xlib" /D "NDEBUG" /D "_USRDLL" /D "Widgets_EXPORTS" /D "WIN32" /D "_WINDOWS" /D "_MBCS" /D "VTKDLL" /YX /FD /Zm1000 /c
# ADD BASE MTL /nologo /D "NDEBUG" /mktyplib203 /win32
# ADD MTL /nologo /D "NDEBUG" /mktyplib203 /win32
# ADD BASE RSC /l 0x409 /d "NDEBUG"
# ADD RSC /l 0x409 /d "NDEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /dll /machine:I386
# ADD LINK32 ..\..\vtkbin\vtktcl\vtktcl.lib ..\..\vtkbin\vtkdll\vtkdll.lib ..\..\vtk\pcmaker\tk82.lib ..\..\vtk\pcmaker\tcl82.lib htmlhelp.lib kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /dll /machine:I386 /out:"Release/vtkKWWidgetsTcl.dll" /libpath:"C:\program files\html help workshop\lib"
# SUBTRACT LINK32 /pdb:none

!ELSEIF  "$(CFG)" == "Widgets - Win32 Debug"

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
# ADD BASE CPP /nologo /MTd /W3 /Gm /GX /ZI /Od /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /D "_MBCS" /D "_USRDLL" /D "Widgets_EXPORTS" /YX /FD /GZ /c
# ADD CPP /nologo /MDd /W3 /Gm /GX /ZI /Od /I "C:\program files\html help workshop\include" /I "..\..\vtk\common" /I "..\..\vtk\graphics" /I "..\..\vtk\imaging" /I "..\..\vtk\contrib" /I "..\..\vtk\pcmaker\xlib" /D "_DEBUG" /D "_USRDLL" /D "Widgets_EXPORTS" /D "WIN32" /D "_WINDOWS" /D "_MBCS" /D "VTKDLL" /YX /FD /GZ /Zm1000 /c
# ADD BASE MTL /nologo /D "_DEBUG" /mktyplib203 /win32
# ADD MTL /nologo /D "_DEBUG" /mktyplib203 /win32
# ADD BASE RSC /l 0x409 /d "_DEBUG"
# ADD RSC /l 0x409 /d "_DEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /dll /debug /machine:I386 /pdbtype:sept
# ADD LINK32 ..\..\vtk\pcmaker\tk82.lib ..\..\vtk\pcmaker\tcl82.lib vtktcl.lib vtkCommon.lib vtkImaging.lib vtkGraphics0.lib vtkGraphics1.lib vtkGraphics2.lib vtkGraphics3.lib vtkGraphics4.lib vtkGraphics5.lib vtkContrib.lib htmlhelp.lib kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /dll /debug /machine:I386 /out:"Debug/vtkKWWidgetsTcl.dll" /pdbtype:sept /libpath:"..\..\vtkbin\debug\lib" /libpath:"C:\program files\html help workshop\lib"
# SUBTRACT LINK32 /pdb:none

!ENDIF 

# Begin Target

# Name "Widgets - Win32 Release"
# Name "Widgets - Win32 Debug"
# Begin Group "Source Files"

# PROP Default_Filter "cpp;c;cxx;rc;def;r;odl;idl;hpj;bat"
# Begin Source File

SOURCE=.\tcl\KWWidgetsInit.cxx
# End Source File
# Begin Source File

SOURCE=.\vtkKWApplication.cxx
# End Source File
# Begin Source File

SOURCE=.\vtkKWApplicationTcl.cxx
# End Source File
# Begin Source File

SOURCE=.\vtkKWCheckButton.cxx
# End Source File
# Begin Source File

SOURCE=.\vtkKWCheckButtonTcl.cxx
# End Source File
# Begin Source File

SOURCE=.\vtkKWComposite.cxx
# End Source File
# Begin Source File

SOURCE=.\vtkKWCompositeCollection.cxx
# End Source File
# Begin Source File

SOURCE=.\vtkKWCompositeCollectionTcl.cxx
# End Source File
# Begin Source File

SOURCE=.\vtkKWCompositeTcl.cxx
# End Source File
# Begin Source File

SOURCE=.\vtkKWDialog.cxx
# End Source File
# Begin Source File

SOURCE=.\vtkKWDialogTcl.cxx
# End Source File
# Begin Source File

SOURCE=.\vtkKWEntry.cxx
# End Source File
# Begin Source File

SOURCE=.\vtkKWEntryTcl.cxx
# End Source File
# Begin Source File

SOURCE=.\vtkKWExtent.cxx
# End Source File
# Begin Source File

SOURCE=.\vtkKWExtentTcl.cxx
# End Source File
# Begin Source File

SOURCE=.\vtkKWGenericComposite.cxx
# End Source File
# Begin Source File

SOURCE=.\vtkKWGenericCompositeTcl.cxx
# End Source File
# Begin Source File

SOURCE=.\vtkKWLabeledFrame.cxx
# End Source File
# Begin Source File

SOURCE=.\vtkKWLabeledFrameTcl.cxx
# End Source File
# Begin Source File

SOURCE=.\vtkKWMenu.cxx
# End Source File
# Begin Source File

SOURCE=.\vtkKWMenuTcl.cxx
# End Source File
# Begin Source File

SOURCE=.\vtkKWMessageDialog.cxx
# End Source File
# Begin Source File

SOURCE=.\vtkKWMessageDialogTcl.cxx
# End Source File
# Begin Source File

SOURCE=.\vtkKWNotebook.cxx
# End Source File
# Begin Source File

SOURCE=.\vtkKWNotebookTcl.cxx
# End Source File
# Begin Source File

SOURCE=.\vtkKWObject.cxx
# End Source File
# Begin Source File

SOURCE=.\vtkKWObjectTcl.cxx
# End Source File
# Begin Source File

SOURCE=.\vtkKWOptionMenu.cxx
# End Source File
# Begin Source File

SOURCE=.\vtkKWOptionMenuTcl.cxx
# End Source File
# Begin Source File

SOURCE=.\vtkKWRadioButton.cxx
# End Source File
# Begin Source File

SOURCE=.\vtkKWRadioButtonTcl.cxx
# End Source File
# Begin Source File

SOURCE=.\vtkKWSaveImageDialog.cxx
# End Source File
# Begin Source File

SOURCE=.\vtkKWSaveImageDialogTcl.cxx
# End Source File
# Begin Source File

SOURCE=.\vtkKWScale.cxx
# End Source File
# Begin Source File

SOURCE=.\vtkKWScaleTcl.cxx
# End Source File
# Begin Source File

SOURCE=.\vtkKWSerializer.cxx
# End Source File
# Begin Source File

SOURCE=.\vtkKWSerializerTcl.cxx
# End Source File
# Begin Source File

SOURCE=.\vtkKWText.cxx
# End Source File
# Begin Source File

SOURCE=.\vtkKWTextTcl.cxx
# End Source File
# Begin Source File

SOURCE=.\vtkKWView.cxx
# End Source File
# Begin Source File

SOURCE=.\vtkKWViewCollection.cxx
# End Source File
# Begin Source File

SOURCE=.\vtkKWViewCollectionTcl.cxx
# End Source File
# Begin Source File

SOURCE=.\vtkKWViewTcl.cxx
# End Source File
# Begin Source File

SOURCE=.\vtkKWWidget.cxx
# End Source File
# Begin Source File

SOURCE=.\vtkKWWidgetCollection.cxx
# End Source File
# Begin Source File

SOURCE=.\vtkKWWidgetCollectionTcl.cxx
# End Source File
# Begin Source File

SOURCE=.\vtkKWWidgetTcl.cxx
# End Source File
# Begin Source File

SOURCE=.\vtkKWWindow.cxx
# End Source File
# Begin Source File

SOURCE=.\vtkKWWindowCollection.cxx
# End Source File
# Begin Source File

SOURCE=.\vtkKWWindowCollectionTcl.cxx
# End Source File
# Begin Source File

SOURCE=.\vtkKWWindowTcl.cxx
# End Source File
# End Group
# Begin Group "Header Files"

# PROP Default_Filter "h;hpp;hxx;hm;inl"
# Begin Source File

SOURCE=.\vtkKWApplication.h

!IF  "$(CFG)" == "Widgets - Win32 Release"

# Begin Custom Build
InputPath=.\vtkKWApplication.h
InputName=vtkKWApplication

"$(InputName)Tcl.cxx" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	..\..\vtk\pcmaker\vtkWrapTcl.exe $(InputName).h hints 1 > $(InputName)Tcl.cxx

# End Custom Build

!ELSEIF  "$(CFG)" == "Widgets - Win32 Debug"

# Begin Custom Build
InputPath=.\vtkKWApplication.h
InputName=vtkKWApplication

"$(InputName)Tcl.cxx" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	..\..\vtk\pcmaker\vtkWrapTcl.exe $(InputName).h hints 1 > $(InputName)Tcl.cxx

# End Custom Build

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\vtkKWCheckButton.h

!IF  "$(CFG)" == "Widgets - Win32 Release"

# Begin Custom Build
InputPath=.\vtkKWCheckButton.h
InputName=vtkKWCheckButton

"$(InputName)Tcl.cxx" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	..\..\vtk\pcmaker\vtkWrapTcl.exe $(InputName).h hints 1 > $(InputName)Tcl.cxx

# End Custom Build

!ELSEIF  "$(CFG)" == "Widgets - Win32 Debug"

# Begin Custom Build
InputPath=.\vtkKWCheckButton.h
InputName=vtkKWCheckButton

"$(InputName)Tcl.cxx" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	..\..\vtk\pcmaker\vtkWrapTcl.exe $(InputName).h hints 1 > $(InputName)Tcl.cxx

# End Custom Build

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\vtkKWComposite.h

!IF  "$(CFG)" == "Widgets - Win32 Release"

# Begin Custom Build
InputPath=.\vtkKWComposite.h
InputName=vtkKWComposite

"$(InputName)Tcl.cxx" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	..\..\vtk\pcmaker\vtkWrapTcl.exe $(InputName).h hints 0 > $(InputName)Tcl.cxx

# End Custom Build

!ELSEIF  "$(CFG)" == "Widgets - Win32 Debug"

# Begin Custom Build
InputPath=.\vtkKWComposite.h
InputName=vtkKWComposite

"$(InputName)Tcl.cxx" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	..\..\vtk\pcmaker\vtkWrapTcl.exe $(InputName).h hints 0 > $(InputName)Tcl.cxx

# End Custom Build

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\vtkKWCompositeCollection.h

!IF  "$(CFG)" == "Widgets - Win32 Release"

# Begin Custom Build
InputPath=.\vtkKWCompositeCollection.h
InputName=vtkKWCompositeCollection

"$(InputName)Tcl.cxx" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	..\..\vtk\pcmaker\vtkWrapTcl.exe $(InputName).h hints 1 > $(InputName)Tcl.cxx

# End Custom Build

!ELSEIF  "$(CFG)" == "Widgets - Win32 Debug"

# Begin Custom Build
InputPath=.\vtkKWCompositeCollection.h
InputName=vtkKWCompositeCollection

"$(InputName)Tcl.cxx" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	..\..\vtk\pcmaker\vtkWrapTcl.exe $(InputName).h hints 1 > $(InputName)Tcl.cxx

# End Custom Build

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\vtkKWDialog.h

!IF  "$(CFG)" == "Widgets - Win32 Release"

# Begin Custom Build
InputPath=.\vtkKWDialog.h
InputName=vtkKWDialog

"$(InputName)Tcl.cxx" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	..\..\vtk\pcmaker\vtkWrapTcl.exe $(InputName).h hints 1 > $(InputName)Tcl.cxx

# End Custom Build

!ELSEIF  "$(CFG)" == "Widgets - Win32 Debug"

# Begin Custom Build
InputPath=.\vtkKWDialog.h
InputName=vtkKWDialog

"$(InputName)Tcl.cxx" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	..\..\vtk\pcmaker\vtkWrapTcl.exe $(InputName).h hints 1 > $(InputName)Tcl.cxx

# End Custom Build

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\vtkKWEntry.h

!IF  "$(CFG)" == "Widgets - Win32 Release"

# Begin Custom Build
InputPath=.\vtkKWEntry.h
InputName=vtkKWEntry

"$(InputName)Tcl.cxx" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	..\..\vtk\pcmaker\vtkWrapTcl.exe $(InputName).h hints 1 > $(InputName)Tcl.cxx

# End Custom Build

!ELSEIF  "$(CFG)" == "Widgets - Win32 Debug"

# Begin Custom Build
InputPath=.\vtkKWEntry.h
InputName=vtkKWEntry

"$(InputName)Tcl.cxx" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	..\..\vtk\pcmaker\vtkWrapTcl.exe $(InputName).h hints 1 > $(InputName)Tcl.cxx

# End Custom Build

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\vtkKWExtent.h

!IF  "$(CFG)" == "Widgets - Win32 Release"

# Begin Custom Build
InputPath=.\vtkKWExtent.h
InputName=vtkKWExtent

"$(InputName)Tcl.cxx" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	..\..\vtk\pcmaker\vtkWrapTcl.exe $(InputName).h hints 1 > $(InputName)Tcl.cxx

# End Custom Build

!ELSEIF  "$(CFG)" == "Widgets - Win32 Debug"

# Begin Custom Build
InputPath=.\vtkKWExtent.h
InputName=vtkKWExtent

"$(InputName)Tcl.cxx" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	..\..\vtk\pcmaker\vtkWrapTcl.exe $(InputName).h hints 1 > $(InputName)Tcl.cxx

# End Custom Build

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\vtkKWGenericComposite.h

!IF  "$(CFG)" == "Widgets - Win32 Release"

# Begin Custom Build
InputPath=.\vtkKWGenericComposite.h
InputName=vtkKWGenericComposite

"$(InputName)Tcl.cxx" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	..\..\vtk\pcmaker\vtkWrapTcl.exe $(InputName).h hints 1 > $(InputName)Tcl.cxx

# End Custom Build

!ELSEIF  "$(CFG)" == "Widgets - Win32 Debug"

# Begin Custom Build
InputPath=.\vtkKWGenericComposite.h
InputName=vtkKWGenericComposite

"$(InputName)Tcl.cxx" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	..\..\vtk\pcmaker\vtkWrapTcl.exe $(InputName).h hints 1 > $(InputName)Tcl.cxx

# End Custom Build

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\vtkKWLabeledFrame.h

!IF  "$(CFG)" == "Widgets - Win32 Release"

# Begin Custom Build
InputPath=.\vtkKWLabeledFrame.h
InputName=vtkKWLabeledFrame

"$(InputName)Tcl.cxx" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	..\..\vtk\pcmaker\vtkWrapTcl.exe $(InputName).h hints 1 > $(InputName)Tcl.cxx

# End Custom Build

!ELSEIF  "$(CFG)" == "Widgets - Win32 Debug"

# Begin Custom Build
InputPath=.\vtkKWLabeledFrame.h
InputName=vtkKWLabeledFrame

"$(InputName)Tcl.cxx" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	..\..\vtk\pcmaker\vtkWrapTcl.exe $(InputName).h hints 1 > $(InputName)Tcl.cxx

# End Custom Build

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\vtkKWMenu.h

!IF  "$(CFG)" == "Widgets - Win32 Release"

!ELSEIF  "$(CFG)" == "Widgets - Win32 Debug"

# Begin Custom Build
InputPath=.\vtkKWMenu.h
InputName=vtkKWMenu

"$(InputName)Tcl.cxx" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	..\..\vtk\pcmaker\vtkWrapTcl.exe $(InputName).h hints 1 > $(InputName)Tcl.cxx

# End Custom Build

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\vtkKWMessageDialog.h

!IF  "$(CFG)" == "Widgets - Win32 Release"

!ELSEIF  "$(CFG)" == "Widgets - Win32 Debug"

# Begin Custom Build
InputPath=.\vtkKWMessageDialog.h
InputName=vtkKWMessageDialog

"$(InputName)Tcl.cxx" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	..\..\vtk\pcmaker\vtkWrapTcl.exe $(InputName).h hints 1 > $(InputName)Tcl.cxx

# End Custom Build

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\vtkKWNotebook.h

!IF  "$(CFG)" == "Widgets - Win32 Release"

# Begin Custom Build
InputPath=.\vtkKWNotebook.h
InputName=vtkKWNotebook

"$(InputName)Tcl.cxx" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	..\..\vtk\pcmaker\vtkWrapTcl.exe $(InputName).h hints 1 > $(InputName)Tcl.cxx

# End Custom Build

!ELSEIF  "$(CFG)" == "Widgets - Win32 Debug"

# Begin Custom Build
InputPath=.\vtkKWNotebook.h
InputName=vtkKWNotebook

"$(InputName)Tcl.cxx" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	..\..\vtk\pcmaker\vtkWrapTcl.exe $(InputName).h hints 1 > $(InputName)Tcl.cxx

# End Custom Build

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\vtkKWObject.h

!IF  "$(CFG)" == "Widgets - Win32 Release"

# Begin Custom Build
InputPath=.\vtkKWObject.h
InputName=vtkKWObject

"$(InputName)Tcl.cxx" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	..\..\vtk\pcmaker\vtkWrapTcl.exe $(InputName).h hints 1 > $(InputName)Tcl.cxx

# End Custom Build

!ELSEIF  "$(CFG)" == "Widgets - Win32 Debug"

# Begin Custom Build
InputPath=.\vtkKWObject.h
InputName=vtkKWObject

"$(InputName)Tcl.cxx" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	..\..\vtk\pcmaker\vtkWrapTcl.exe $(InputName).h hints 1 > $(InputName)Tcl.cxx

# End Custom Build

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\vtkKWOptionMenu.h

!IF  "$(CFG)" == "Widgets - Win32 Release"

# Begin Custom Build
InputPath=.\vtkKWOptionMenu.h
InputName=vtkKWOptionMenu

"$(InputName)Tcl.cxx" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	..\..\vtk\pcmaker\vtkWrapTcl.exe $(InputName).h hints 1 > $(InputName)Tcl.cxx

# End Custom Build

!ELSEIF  "$(CFG)" == "Widgets - Win32 Debug"

# Begin Custom Build
InputPath=.\vtkKWOptionMenu.h
InputName=vtkKWOptionMenu

"$(InputName)Tcl.cxx" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	..\..\vtk\pcmaker\vtkWrapTcl.exe $(InputName).h hints 1 > $(InputName)Tcl.cxx

# End Custom Build

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\vtkKWRadioButton.h

!IF  "$(CFG)" == "Widgets - Win32 Release"

# Begin Custom Build
InputPath=.\vtkKWRadioButton.h
InputName=vtkKWRadioButton

"$(InputName)Tcl.cxx" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	..\..\vtk\pcmaker\vtkWrapTcl.exe $(InputName).h hints 1 > $(InputName)Tcl.cxx

# End Custom Build

!ELSEIF  "$(CFG)" == "Widgets - Win32 Debug"

# Begin Custom Build
InputPath=.\vtkKWRadioButton.h
InputName=vtkKWRadioButton

"$(InputName)Tcl.cxx" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	..\..\vtk\pcmaker\vtkWrapTcl.exe $(InputName).h hints 1 > $(InputName)Tcl.cxx

# End Custom Build

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\vtkKWSaveImageDialog.h

!IF  "$(CFG)" == "Widgets - Win32 Release"

# Begin Custom Build
InputPath=.\vtkKWSaveImageDialog.h
InputName=vtkKWSaveImageDialog

"$(InputName)Tcl.cxx" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	..\..\vtk\pcmaker\vtkWrapTcl.exe $(InputName).h hints 1 > $(InputName)Tcl.cxx

# End Custom Build

!ELSEIF  "$(CFG)" == "Widgets - Win32 Debug"

# Begin Custom Build
InputPath=.\vtkKWSaveImageDialog.h
InputName=vtkKWSaveImageDialog

"$(InputName)Tcl.cxx" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	..\..\vtk\pcmaker\vtkWrapTcl.exe $(InputName).h hints 1 > $(InputName)Tcl.cxx

# End Custom Build

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\vtkKWScale.h

!IF  "$(CFG)" == "Widgets - Win32 Release"

# Begin Custom Build
InputPath=.\vtkKWScale.h
InputName=vtkKWScale

"$(InputName)Tcl.cxx" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	..\..\vtk\pcmaker\vtkWrapTcl.exe $(InputName).h hints 1 > $(InputName)Tcl.cxx

# End Custom Build

!ELSEIF  "$(CFG)" == "Widgets - Win32 Debug"

# Begin Custom Build
InputPath=.\vtkKWScale.h
InputName=vtkKWScale

"$(InputName)Tcl.cxx" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	..\..\vtk\pcmaker\vtkWrapTcl.exe $(InputName).h hints 1 > $(InputName)Tcl.cxx

# End Custom Build

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\vtkKWSerializer.h

!IF  "$(CFG)" == "Widgets - Win32 Release"

# Begin Custom Build
InputPath=.\vtkKWSerializer.h
InputName=vtkKWSerializer

"$(InputName)Tcl.cxx" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	..\..\vtk\pcmaker\vtkWrapTcl.exe $(InputName).h hints 1 > $(InputName)Tcl.cxx

# End Custom Build

!ELSEIF  "$(CFG)" == "Widgets - Win32 Debug"

# Begin Custom Build
InputPath=.\vtkKWSerializer.h
InputName=vtkKWSerializer

"$(InputName)Tcl.cxx" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	..\..\vtk\pcmaker\vtkWrapTcl.exe $(InputName).h hints 1 > $(InputName)Tcl.cxx

# End Custom Build

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\vtkKWText.h

!IF  "$(CFG)" == "Widgets - Win32 Release"

!ELSEIF  "$(CFG)" == "Widgets - Win32 Debug"

# Begin Custom Build
InputPath=.\vtkKWText.h
InputName=vtkKWText

"$(InputName)Tcl.cxx" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	..\..\vtk\pcmaker\vtkWrapTcl.exe $(InputName).h hints 1 > $(InputName)Tcl.cxx

# End Custom Build

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\vtkKWView.h

!IF  "$(CFG)" == "Widgets - Win32 Release"

# Begin Custom Build
InputPath=.\vtkKWView.h
InputName=vtkKWView

"$(InputName)Tcl.cxx" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	..\..\vtk\pcmaker\vtkWrapTcl.exe $(InputName).h hints 0 > $(InputName)Tcl.cxx

# End Custom Build

!ELSEIF  "$(CFG)" == "Widgets - Win32 Debug"

# Begin Custom Build
InputPath=.\vtkKWView.h
InputName=vtkKWView

"$(InputName)Tcl.cxx" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	..\..\vtk\pcmaker\vtkWrapTcl.exe $(InputName).h hints 0 > $(InputName)Tcl.cxx

# End Custom Build

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\vtkKWViewCollection.h

!IF  "$(CFG)" == "Widgets - Win32 Release"

# Begin Custom Build
InputPath=.\vtkKWViewCollection.h
InputName=vtkKWViewCollection

"$(InputName)Tcl.cxx" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	..\..\vtk\pcmaker\vtkWrapTcl.exe $(InputName).h hints 1 > $(InputName)Tcl.cxx

# End Custom Build

!ELSEIF  "$(CFG)" == "Widgets - Win32 Debug"

# Begin Custom Build
InputPath=.\vtkKWViewCollection.h
InputName=vtkKWViewCollection

"$(InputName)Tcl.cxx" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	..\..\vtk\pcmaker\vtkWrapTcl.exe $(InputName).h hints 1 > $(InputName)Tcl.cxx

# End Custom Build

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\vtkKWWidget.h

!IF  "$(CFG)" == "Widgets - Win32 Release"

# Begin Custom Build
InputPath=.\vtkKWWidget.h
InputName=vtkKWWidget

"$(InputName)Tcl.cxx" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	..\..\vtk\pcmaker\vtkWrapTcl.exe $(InputName).h hints 1 > $(InputName)Tcl.cxx

# End Custom Build

!ELSEIF  "$(CFG)" == "Widgets - Win32 Debug"

# Begin Custom Build
InputPath=.\vtkKWWidget.h
InputName=vtkKWWidget

"$(InputName)Tcl.cxx" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	..\..\vtk\pcmaker\vtkWrapTcl.exe $(InputName).h hints 1 > $(InputName)Tcl.cxx

# End Custom Build

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\vtkKWWidgetCollection.h

!IF  "$(CFG)" == "Widgets - Win32 Release"

# Begin Custom Build
InputPath=.\vtkKWWidgetCollection.h
InputName=vtkKWWidgetCollection

"$(InputName)Tcl.cxx" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	..\..\vtk\pcmaker\vtkWrapTcl.exe $(InputName).h hints 1 > $(InputName)Tcl.cxx

# End Custom Build

!ELSEIF  "$(CFG)" == "Widgets - Win32 Debug"

# Begin Custom Build
InputPath=.\vtkKWWidgetCollection.h
InputName=vtkKWWidgetCollection

"$(InputName)Tcl.cxx" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	..\..\vtk\pcmaker\vtkWrapTcl.exe $(InputName).h hints 1 > $(InputName)Tcl.cxx

# End Custom Build

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\vtkKWWindow.h

!IF  "$(CFG)" == "Widgets - Win32 Release"

# Begin Custom Build
InputPath=.\vtkKWWindow.h
InputName=vtkKWWindow

"$(InputName)Tcl.cxx" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	..\..\vtk\pcmaker\vtkWrapTcl.exe $(InputName).h hints 1 > $(InputName)Tcl.cxx

# End Custom Build

!ELSEIF  "$(CFG)" == "Widgets - Win32 Debug"

# Begin Custom Build
InputPath=.\vtkKWWindow.h
InputName=vtkKWWindow

"$(InputName)Tcl.cxx" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	..\..\vtk\pcmaker\vtkWrapTcl.exe $(InputName).h hints 1 > $(InputName)Tcl.cxx

# End Custom Build

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\vtkKWWindowCollection.h

!IF  "$(CFG)" == "Widgets - Win32 Release"

# Begin Custom Build
InputPath=.\vtkKWWindowCollection.h
InputName=vtkKWWindowCollection

"$(InputName)Tcl.cxx" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	..\..\vtk\pcmaker\vtkWrapTcl.exe $(InputName).h hints 1 > $(InputName)Tcl.cxx

# End Custom Build

!ELSEIF  "$(CFG)" == "Widgets - Win32 Debug"

# Begin Custom Build
InputPath=.\vtkKWWindowCollection.h
InputName=vtkKWWindowCollection

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
