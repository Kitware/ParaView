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
# ADD CPP /nologo /W3 /GX /O2 /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "_MBCS" /YX /FD /c
# ADD BASE MTL /nologo /D "NDEBUG" /mktyplib203 /win32
# ADD MTL /nologo /D "NDEBUG" /mktyplib203 /win32
# ADD BASE RSC /l 0x409 /d "NDEBUG"
# ADD RSC /l 0x409 /d "NDEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:windows /machine:I386
# ADD LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:windows /machine:I386

!ELSEIF  "$(CFG)" == "ParaView - Win32 Debug"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir "ParaView___Win32_Debug"
# PROP BASE Intermediate_Dir "ParaView___Win32_Debug"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 1
# PROP Output_Dir "Debug"
# PROP Intermediate_Dir "Debug"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
# ADD BASE CPP /nologo /W3 /Gm /GX /ZI /Od /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /D "_MBCS" /YX /FD /GZ /c
# ADD CPP /nologo /MDd /W3 /Gm /GX /ZI /Od /I "..\Widgets" /I "C:\program files\html help workshop\include" /I "..\..\vtkbin" /I "..\..\vtk\common" /I "..\..\vtk\graphics" /I "..\..\vtk\imaging" /I "..\..\vtk\contrib" /I "..\..\vtk\patented" /I "..\..\vtk\parallel" /I "..\..\vtk\pcmaker\xlib" /D "_DEBUG" /D "WIN32" /D "_WINDOWS" /D "_MBCS" /D "VTKDLL" /Fp"Debug/SciView.pch" /YX /FD /GZ /c
# ADD BASE MTL /nologo /D "_DEBUG" /mktyplib203 /win32
# ADD MTL /nologo /D "_DEBUG" /mktyplib203 /win32
# ADD BASE RSC /l 0x409 /d "_DEBUG"
# ADD RSC /l 0x409 /d "_DEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:windows /debug /machine:I386 /pdbtype:sept
# ADD LINK32 ..\..\vtkbin\Debug\lib\vtktcl.lib ..\Widgets\Debug\vtkKWWidgetsTcl.lib ..\..\vtkbin\Debug\lib\vtkPatented.lib opengl32.lib ..\..\vtk\pcmaker\tk82.lib ..\..\vtk\pcmaker\tcl82.lib vtktcl.lib vtkCommon.lib vtkImaging.lib vtkGraphics0.lib vtkGraphics1.lib vtkGraphics2.lib vtkGraphics3.lib vtkGraphics4.lib vtkGraphics5.lib vtkContrib.lib vtkParallel.lib htmlhelp.lib kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /dll /pdb:"../Debug/vtkKWParaViewTcl.pdb" /debug /machine:I386 /out:"Debug/vtkKWParaViewTcl.dll" /pdbtype:sept /libpath:"..\..\vtkbin\debug\lib" /libpath:"C:\program files\html help workshop\lib"
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

SOURCE=.\vtkCameraInteractor.cxx
# End Source File
# Begin Source File

SOURCE=.\vtkCameraInteractorTcl.cxx
# End Source File
# Begin Source File

SOURCE=.\vtkClipPlane.cxx
# End Source File
# Begin Source File

SOURCE=.\vtkClipPlaneTcl.cxx
# End Source File
# Begin Source File

SOURCE=.\vtkColorByProcess.cxx
# End Source File
# Begin Source File

SOURCE=.\vtkColorByProcessTcl.cxx
# End Source File
# Begin Source File

SOURCE=.\vtkCutPlane.cxx
# End Source File
# Begin Source File

SOURCE=.\vtkCutPlaneTcl.cxx
# End Source File
# Begin Source File

SOURCE=.\vtkDummyRenderWindowInteractor.cxx
# End Source File
# Begin Source File

SOURCE=.\vtkDummyRenderWindowInteractorTcl.cxx
# End Source File
# Begin Source File

SOURCE=.\vtkFastGeometryFilter.cxx
# End Source File
# Begin Source File

SOURCE=.\vtkFastGeometryFilterTcl.cxx
# End Source File
# Begin Source File

SOURCE=.\vtkImageOutlineFilter.cxx
# End Source File
# Begin Source File

SOURCE=.\vtkImageOutlineFilterTcl.cxx
# End Source File
# Begin Source File

SOURCE=.\vtkInteractor.cxx
# End Source File
# Begin Source File

SOURCE=.\vtkInteractorStyleCamera.cxx
# End Source File
# Begin Source File

SOURCE=.\vtkInteractorStyleCameraTcl.cxx
# End Source File
# Begin Source File

SOURCE=.\vtkInteractorStyleExtent.cxx
# End Source File
# Begin Source File

SOURCE=.\vtkInteractorStyleExtentTcl.cxx
# End Source File
# Begin Source File

SOURCE=.\vtkInteractorStyleGridExtent.cxx
# End Source File
# Begin Source File

SOURCE=.\vtkInteractorStyleGridExtentTcl.cxx
# End Source File
# Begin Source File

SOURCE=.\vtkInteractorStyleImageExtent.cxx
# End Source File
# Begin Source File

SOURCE=.\vtkInteractorStyleImageExtentTcl.cxx
# End Source File
# Begin Source File

SOURCE=.\vtkInteractorStylePlane.cxx
# End Source File
# Begin Source File

SOURCE=.\vtkInteractorStylePlaneSource.cxx
# End Source File
# Begin Source File

SOURCE=.\vtkInteractorStylePlaneSourceTcl.cxx
# End Source File
# Begin Source File

SOURCE=.\vtkInteractorStylePlaneTcl.cxx
# End Source File
# Begin Source File

SOURCE=.\vtkInteractorStyleSphere.cxx
# End Source File
# Begin Source File

SOURCE=.\vtkInteractorStyleSphereTcl.cxx
# End Source File
# Begin Source File

SOURCE=.\vtkInteractorTcl.cxx
# End Source File
# Begin Source File

SOURCE=.\vtkKWCenterOfRotation.cxx
# End Source File
# Begin Source File

SOURCE=.\vtkKWCenterOfRotationTcl.cxx
# End Source File
# Begin Source File

SOURCE=.\vtkKWFlyInteractor.cxx
# End Source File
# Begin Source File

SOURCE=.\vtkKWFlyInteractorTcl.cxx
# End Source File
# Begin Source File

SOURCE=.\vtkKWInteractor.cxx
# End Source File
# Begin Source File

SOURCE=.\vtkKWInteractorTcl.cxx
# End Source File
# Begin Source File

SOURCE=.\vtkKWRenderView.cxx
# End Source File
# Begin Source File

SOURCE=.\vtkKWRenderViewTcl.cxx
# End Source File
# Begin Source File

SOURCE=.\vtkKWRotateCameraInteractor.cxx
# End Source File
# Begin Source File

SOURCE=.\vtkKWRotateCameraInteractorTcl.cxx
# End Source File
# Begin Source File

SOURCE=.\vtkKWTranslateCameraInteractor.cxx
# End Source File
# Begin Source File

SOURCE=.\vtkKWTranslateCameraInteractorTcl.cxx
# End Source File
# Begin Source File

SOURCE=.\vtkParallelDecimate.cxx
# End Source File
# Begin Source File

SOURCE=.\vtkParallelDecimateTcl.cxx
# End Source File
# Begin Source File

SOURCE=.\vtkPOPReader.cxx
# End Source File
# Begin Source File

SOURCE=.\vtkPOPReaderTcl.cxx
# End Source File
# Begin Source File

SOURCE=.\vtkPVActorComposite.cxx
# End Source File
# Begin Source File

SOURCE=.\vtkPVActorCompositeTcl.cxx
# End Source File
# Begin Source File

SOURCE=.\vtkPVAnimation.cxx
# End Source File
# Begin Source File

SOURCE=.\vtkPVAnimationTcl.cxx
# End Source File
# Begin Source File

SOURCE=.\vtkPVApplication.cxx
# End Source File
# Begin Source File

SOURCE=.\vtkPVApplicationTcl.cxx
# End Source File
# Begin Source File

SOURCE=.\vtkPVArrayCalculator.cxx
# End Source File
# Begin Source File

SOURCE=.\vtkPVArrayCalculatorTcl.cxx
# End Source File
# Begin Source File

SOURCE=.\vtkPVContour.cxx
# End Source File
# Begin Source File

SOURCE=.\vtkPVContourTcl.cxx
# End Source File
# Begin Source File

SOURCE=.\vtkPVData.cxx
# End Source File
# Begin Source File

SOURCE=.\vtkPVDataSetReaderInterface.cxx
# End Source File
# Begin Source File

SOURCE=.\vtkPVDataSetReaderInterfaceTcl.cxx
# End Source File
# Begin Source File

SOURCE=.\vtkPVDataTcl.cxx
# End Source File
# Begin Source File

SOURCE=.\vtkPVEnSightReaderInterface.cxx
# End Source File
# Begin Source File

SOURCE=.\vtkPVEnSightReaderInterfaceTcl.cxx
# End Source File
# Begin Source File

SOURCE=.\vtkPVGlyph3D.cxx
# End Source File
# Begin Source File

SOURCE=.\vtkPVGlyph3DTcl.cxx
# End Source File
# Begin Source File

SOURCE=.\vtkPVInputMenu.cxx
# End Source File
# Begin Source File

SOURCE=.\vtkPVInputMenuTcl.cxx
# End Source File
# Begin Source File

SOURCE=.\vtkPVMethodInterface.cxx
# End Source File
# Begin Source File

SOURCE=.\vtkPVMethodInterfaceTcl.cxx
# End Source File
# Begin Source File

SOURCE=.\vtkPVRenderView.cxx
# End Source File
# Begin Source File

SOURCE=.\vtkPVRenderViewTcl.cxx
# End Source File
# Begin Source File

SOURCE=.\vtkPVSelectionList.cxx
# End Source File
# Begin Source File

SOURCE=.\vtkPVSelectionListTcl.cxx
# End Source File
# Begin Source File

SOURCE=.\vtkPVSource.cxx
# End Source File
# Begin Source File

SOURCE=.\vtkPVSourceCollection.cxx
# End Source File
# Begin Source File

SOURCE=.\vtkPVSourceCollectionTcl.cxx
# End Source File
# Begin Source File

SOURCE=.\vtkPVSourceInterface.cxx
# End Source File
# Begin Source File

SOURCE=.\vtkPVSourceInterfaceTcl.cxx
# End Source File
# Begin Source File

SOURCE=.\vtkPVSourceList.cxx
# End Source File
# Begin Source File

SOURCE=.\vtkPVSourceListTcl.cxx
# End Source File
# Begin Source File

SOURCE=.\vtkPVSourceTcl.cxx
# End Source File
# Begin Source File

SOURCE=.\vtkPVThreshold.cxx
# End Source File
# Begin Source File

SOURCE=.\vtkPVThresholdTcl.cxx
# End Source File
# Begin Source File

SOURCE=.\vtkPVWindow.cxx
# End Source File
# Begin Source File

SOURCE=.\vtkPVWindowTcl.cxx
# End Source File
# Begin Source File

SOURCE=.\vtkSimpleFieldDataToAttributeDataFilter.cxx
# End Source File
# Begin Source File

SOURCE=.\vtkSimpleFieldDataToAttributeDataFilterTcl.cxx
# End Source File
# Begin Source File

SOURCE=.\vtkSingleContourFilter.cxx
# End Source File
# Begin Source File

SOURCE=.\vtkSingleContourFilterTcl.cxx
# End Source File
# Begin Source File

SOURCE=.\vtkStringList.cxx
# End Source File
# Begin Source File

SOURCE=.\vtkStringListTcl.cxx
# End Source File
# End Group
# Begin Group "Header Files"

# PROP Default_Filter "h;hpp;hxx;hm;inl"
# Begin Source File

SOURCE=.\vtkCameraInteractor.h

!IF  "$(CFG)" == "ParaView - Win32 Release"

!ELSEIF  "$(CFG)" == "ParaView - Win32 Debug"

# Begin Custom Build
InputPath=.\vtkCameraInteractor.h
InputName=vtkCameraInteractor

"$(InputName)Tcl.cxx" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	..\..\vtk\pcmaker\vtkWrapTcl.exe $(InputName).h hints 1 > $(InputName)Tcl.cxx

# End Custom Build

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\vtkClipPlane.h

!IF  "$(CFG)" == "ParaView - Win32 Release"

!ELSEIF  "$(CFG)" == "ParaView - Win32 Debug"

# Begin Custom Build
InputPath=.\vtkClipPlane.h
InputName=vtkClipPlane

"$(InputName)Tcl.cxx" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	..\..\vtk\pcmaker\vtkWrapTcl.exe $(InputName).h hints 1 > $(InputName)Tcl.cxx

# End Custom Build

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\vtkColorByProcess.h

!IF  "$(CFG)" == "ParaView - Win32 Release"

!ELSEIF  "$(CFG)" == "ParaView - Win32 Debug"

# Begin Custom Build
InputPath=.\vtkColorByProcess.h
InputName=vtkColorByProcess

"$(InputName)Tcl.cxx" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	..\..\vtk\pcmaker\vtkWrapTcl.exe $(InputName).h hints 1 > $(InputName)Tcl.cxx

# End Custom Build

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\vtkCutPlane.h

!IF  "$(CFG)" == "ParaView - Win32 Release"

!ELSEIF  "$(CFG)" == "ParaView - Win32 Debug"

# Begin Custom Build
InputPath=.\vtkCutPlane.h
InputName=vtkCutPlane

"$(InputName)Tcl.cxx" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	..\..\vtk\pcmaker\vtkWrapTcl.exe $(InputName).h hints 1 > $(InputName)Tcl.cxx

# End Custom Build

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\vtkDummyRenderWindowInteractor.h

!IF  "$(CFG)" == "ParaView - Win32 Release"

# Begin Custom Build
InputPath=.\vtkDummyRenderWindowInteractor.h
InputName=vtkDummyRenderWindowInteractor

"$(InputName)Tcl.cxx" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	..\..\vtk\pcmaker\vtkWrapTcl.exe $(InputName).h hints 1 > $(InputName)Tcl.cxx

# End Custom Build

!ELSEIF  "$(CFG)" == "ParaView - Win32 Debug"

# Begin Custom Build
InputPath=.\vtkDummyRenderWindowInteractor.h
InputName=vtkDummyRenderWindowInteractor

"$(InputName)Tcl.cxx" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	..\..\vtk\pcmaker\vtkWrapTcl.exe $(InputName).h hints 1 > $(InputName)Tcl.cxx

# End Custom Build

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\vtkFastGeometryFilter.h

!IF  "$(CFG)" == "ParaView - Win32 Release"

!ELSEIF  "$(CFG)" == "ParaView - Win32 Debug"

# Begin Custom Build
InputPath=.\vtkFastGeometryFilter.h
InputName=vtkFastGeometryFilter

"$(InputName)Tcl.cxx" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	..\..\vtk\pcmaker\vtkWrapTcl.exe $(InputName).h hints 1 > $(InputName)Tcl.cxx

# End Custom Build

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\vtkImageOutlineFilter.h

!IF  "$(CFG)" == "ParaView - Win32 Release"

# Begin Custom Build
InputPath=.\vtkImageOutlineFilter.h
InputName=vtkImageOutlineFilter

"$(InputName)Tcl.cxx" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	..\..\vtk\pcmaker\vtkWrapTcl.exe $(InputName).h hints 1 > $(InputName)Tcl.cxx

# End Custom Build

!ELSEIF  "$(CFG)" == "ParaView - Win32 Debug"

# Begin Custom Build
InputPath=.\vtkImageOutlineFilter.h
InputName=vtkImageOutlineFilter

"$(InputName)Tcl.cxx" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	..\..\vtk\pcmaker\vtkWrapTcl.exe $(InputName).h hints 1 > $(InputName)Tcl.cxx

# End Custom Build

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\vtkInteractor.h

!IF  "$(CFG)" == "ParaView - Win32 Release"

!ELSEIF  "$(CFG)" == "ParaView - Win32 Debug"

# Begin Custom Build
InputPath=.\vtkInteractor.h
InputName=vtkInteractor

"$(InputName)Tcl.cxx" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	..\..\vtk\pcmaker\vtkWrapTcl.exe $(InputName).h hints 1 > $(InputName)Tcl.cxx

# End Custom Build

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\vtkInteractorStyleCamera.h

!IF  "$(CFG)" == "ParaView - Win32 Release"

# Begin Custom Build
InputPath=.\vtkInteractorStyleCamera.h
InputName=vtkInteractorStyleCamera

"$(InputName)Tcl.cxx" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	..\..\vtk\pcmaker\vtkWrapTcl.exe $(InputName).h hints 1 > $(InputName)Tcl.cxx

# End Custom Build

!ELSEIF  "$(CFG)" == "ParaView - Win32 Debug"

# Begin Custom Build
InputPath=.\vtkInteractorStyleCamera.h
InputName=vtkInteractorStyleCamera

"$(InputName)Tcl.cxx" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	..\..\vtk\pcmaker\vtkWrapTcl.exe $(InputName).h hints 1 > $(InputName)Tcl.cxx

# End Custom Build

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\vtkInteractorStyleExtent.h

!IF  "$(CFG)" == "ParaView - Win32 Release"

!ELSEIF  "$(CFG)" == "ParaView - Win32 Debug"

# Begin Custom Build
InputPath=.\vtkInteractorStyleExtent.h
InputName=vtkInteractorStyleExtent

"$(InputName)Tcl.cxx" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	..\..\vtk\pcmaker\vtkWrapTcl.exe $(InputName).h hints 1 > $(InputName)Tcl.cxx

# End Custom Build

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\vtkInteractorStyleGridExtent.h

!IF  "$(CFG)" == "ParaView - Win32 Release"

!ELSEIF  "$(CFG)" == "ParaView - Win32 Debug"

# Begin Custom Build
InputPath=.\vtkInteractorStyleGridExtent.h
InputName=vtkInteractorStyleGridExtent

"$(InputName)Tcl.cxx" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	..\..\vtk\pcmaker\vtkWrapTcl.exe $(InputName).h hints 1 > $(InputName)Tcl.cxx

# End Custom Build

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\vtkInteractorStyleImageExtent.h

!IF  "$(CFG)" == "ParaView - Win32 Release"

!ELSEIF  "$(CFG)" == "ParaView - Win32 Debug"

# Begin Custom Build
InputPath=.\vtkInteractorStyleImageExtent.h
InputName=vtkInteractorStyleImageExtent

"$(InputName)Tcl.cxx" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	..\..\vtk\pcmaker\vtkWrapTcl.exe $(InputName).h hints 1 > $(InputName)Tcl.cxx

# End Custom Build

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\vtkInteractorStylePlane.h

!IF  "$(CFG)" == "ParaView - Win32 Release"

!ELSEIF  "$(CFG)" == "ParaView - Win32 Debug"

# Begin Custom Build
InputPath=.\vtkInteractorStylePlane.h
InputName=vtkInteractorStylePlane

"$(InputName)Tcl.cxx" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	..\..\vtk\pcmaker\vtkWrapTcl.exe $(InputName).h hints 1 > $(InputName)Tcl.cxx

# End Custom Build

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\vtkInteractorStylePlaneSource.h

!IF  "$(CFG)" == "ParaView - Win32 Release"

!ELSEIF  "$(CFG)" == "ParaView - Win32 Debug"

# Begin Custom Build
InputPath=.\vtkInteractorStylePlaneSource.h
InputName=vtkInteractorStylePlaneSource

"$(InputName)Tcl.cxx" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	..\..\vtk\pcmaker\vtkWrapTcl.exe $(InputName).h hints 1 > $(InputName)Tcl.cxx

# End Custom Build

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\vtkInteractorStyleSphere.h

!IF  "$(CFG)" == "ParaView - Win32 Release"

!ELSEIF  "$(CFG)" == "ParaView - Win32 Debug"

# Begin Custom Build
InputPath=.\vtkInteractorStyleSphere.h
InputName=vtkInteractorStyleSphere

"$(InputName)Tcl.cxx" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	..\..\vtk\pcmaker\vtkWrapTcl.exe $(InputName).h hints 1 > $(InputName)Tcl.cxx

# End Custom Build

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\vtkKWCenterOfRotation.h

!IF  "$(CFG)" == "ParaView - Win32 Release"

!ELSEIF  "$(CFG)" == "ParaView - Win32 Debug"

# Begin Custom Build
InputPath=.\vtkKWCenterOfRotation.h
InputName=vtkKWCenterOfRotation

"$(InputName)Tcl.cxx" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	..\..\vtk\pcmaker\vtkWrapTcl.exe $(InputName).h hints 1 > $(InputName)Tcl.cxx

# End Custom Build

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\vtkKWFlyInteractor.h

!IF  "$(CFG)" == "ParaView - Win32 Release"

!ELSEIF  "$(CFG)" == "ParaView - Win32 Debug"

# Begin Custom Build
InputPath=.\vtkKWFlyInteractor.h
InputName=vtkKWFlyInteractor

"$(InputName)Tcl.cxx" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	..\..\vtk\pcmaker\vtkWrapTcl.exe $(InputName).h hints 1 > $(InputName)Tcl.cxx

# End Custom Build

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\vtkKWInteractor.h

!IF  "$(CFG)" == "ParaView - Win32 Release"

!ELSEIF  "$(CFG)" == "ParaView - Win32 Debug"

# Begin Custom Build
InputPath=.\vtkKWInteractor.h
InputName=vtkKWInteractor

"$(InputName)Tcl.cxx" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	..\..\vtk\pcmaker\vtkWrapTcl.exe $(InputName).h hints 1 > $(InputName)Tcl.cxx

# End Custom Build

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\vtkKWRenderView.h

!IF  "$(CFG)" == "ParaView - Win32 Release"

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

SOURCE=.\vtkKWRotateCameraInteractor.h

!IF  "$(CFG)" == "ParaView - Win32 Release"

!ELSEIF  "$(CFG)" == "ParaView - Win32 Debug"

# Begin Custom Build
InputPath=.\vtkKWRotateCameraInteractor.h
InputName=vtkKWRotateCameraInteractor

"$(InputName)Tcl.cxx" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	..\..\vtk\pcmaker\vtkWrapTcl.exe $(InputName).h hints 1 > $(InputName)Tcl.cxx

# End Custom Build

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\vtkKWTranslateCameraInteractor.h

!IF  "$(CFG)" == "ParaView - Win32 Release"

!ELSEIF  "$(CFG)" == "ParaView - Win32 Debug"

# Begin Custom Build
InputPath=.\vtkKWTranslateCameraInteractor.h
InputName=vtkKWTranslateCameraInteractor

"$(InputName)Tcl.cxx" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	..\..\vtk\pcmaker\vtkWrapTcl.exe $(InputName).h hints 1 > $(InputName)Tcl.cxx

# End Custom Build

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\vtkParallelDecimate.h

!IF  "$(CFG)" == "ParaView - Win32 Release"

!ELSEIF  "$(CFG)" == "ParaView - Win32 Debug"

# Begin Custom Build
InputPath=.\vtkParallelDecimate.h
InputName=vtkParallelDecimate

"$(InputName)Tcl.cxx" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	..\..\vtk\pcmaker\vtkWrapTcl.exe $(InputName).h hints 1 > $(InputName)Tcl.cxx

# End Custom Build

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\vtkPOPReader.h

!IF  "$(CFG)" == "ParaView - Win32 Release"

!ELSEIF  "$(CFG)" == "ParaView - Win32 Debug"

# Begin Custom Build
InputPath=.\vtkPOPReader.h
InputName=vtkPOPReader

"$(InputName)Tcl.cxx" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	..\..\vtk\pcmaker\vtkWrapTcl.exe $(InputName).h hints 1 > $(InputName)Tcl.cxx

# End Custom Build

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\vtkPVActorComposite.h

!IF  "$(CFG)" == "ParaView - Win32 Release"

!ELSEIF  "$(CFG)" == "ParaView - Win32 Debug"

# Begin Custom Build
InputPath=.\vtkPVActorComposite.h
InputName=vtkPVActorComposite

"$(InputName)Tcl.cxx" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	..\..\vtk\pcmaker\vtkWrapTcl.exe $(InputName).h hints 1 > $(InputName)Tcl.cxx

# End Custom Build

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\vtkPVAnimation.h

!IF  "$(CFG)" == "ParaView - Win32 Release"

!ELSEIF  "$(CFG)" == "ParaView - Win32 Debug"

# Begin Custom Build
InputPath=.\vtkPVAnimation.h
InputName=vtkPVAnimation

"$(InputName)Tcl.cxx" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	..\..\vtk\pcmaker\vtkWrapTcl.exe $(InputName).h hints 1 > $(InputName)Tcl.cxx

# End Custom Build

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\vtkPVApplication.h

!IF  "$(CFG)" == "ParaView - Win32 Release"

# Begin Custom Build
InputPath=.\vtkPVApplication.h
InputName=vtkPVApplication

"$(InputName)Tcl.cxx" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	..\..\vtk\pcmaker\vtkWrapTcl.exe $(InputName).h hints 1 > $(InputName)Tcl.cxx

# End Custom Build

!ELSEIF  "$(CFG)" == "ParaView - Win32 Debug"

# Begin Custom Build
InputPath=.\vtkPVApplication.h
InputName=vtkPVApplication

"$(InputName)Tcl.cxx" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	..\..\vtk\pcmaker\vtkWrapTcl.exe $(InputName).h hints 1 > $(InputName)Tcl.cxx

# End Custom Build

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\vtkPVArrayCalculator.h

!IF  "$(CFG)" == "ParaView - Win32 Release"

!ELSEIF  "$(CFG)" == "ParaView - Win32 Debug"

# Begin Custom Build
InputPath=.\vtkPVArrayCalculator.h
InputName=vtkPVArrayCalculator

"$(InputName)Tcl.cxx" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	..\..\vtk\pcmaker\vtkWrapTcl.exe $(InputName).h hints 1 > $(InputName)Tcl.cxx

# End Custom Build

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\vtkPVContour.h

!IF  "$(CFG)" == "ParaView - Win32 Release"

!ELSEIF  "$(CFG)" == "ParaView - Win32 Debug"

# Begin Custom Build
InputPath=.\vtkPVContour.h
InputName=vtkPVContour

"$(InputName)Tcl.cxx" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	..\..\vtk\pcmaker\vtkWrapTcl.exe $(InputName).h hints 1 > $(InputName)Tcl.cxx

# End Custom Build

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\vtkPVData.h

!IF  "$(CFG)" == "ParaView - Win32 Release"

!ELSEIF  "$(CFG)" == "ParaView - Win32 Debug"

# Begin Custom Build
InputPath=.\vtkPVData.h
InputName=vtkPVData

"$(InputName)Tcl.cxx" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	..\..\vtk\pcmaker\vtkWrapTcl.exe $(InputName).h hints 1 > $(InputName)Tcl.cxx

# End Custom Build

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\vtkPVDataSetReaderInterface.h

!IF  "$(CFG)" == "ParaView - Win32 Release"

!ELSEIF  "$(CFG)" == "ParaView - Win32 Debug"

# Begin Custom Build
InputPath=.\vtkPVDataSetReaderInterface.h
InputName=vtkPVDataSetReaderInterface

"$(InputName)Tcl.cxx" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	..\..\vtk\pcmaker\vtkWrapTcl.exe $(InputName).h hints 1 > $(InputName)Tcl.cxx

# End Custom Build

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\vtkPVEnSightReaderInterface.h

!IF  "$(CFG)" == "ParaView - Win32 Release"

!ELSEIF  "$(CFG)" == "ParaView - Win32 Debug"

# Begin Custom Build
InputPath=.\vtkPVEnSightReaderInterface.h
InputName=vtkPVEnSightReaderInterface

"$(InputName)Tcl.cxx" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	..\..\vtk\pcmaker\vtkWrapTcl.exe $(InputName).h hints 1 > $(InputName)Tcl.cxx

# End Custom Build

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\vtkPVGlyph3D.h

!IF  "$(CFG)" == "ParaView - Win32 Release"

!ELSEIF  "$(CFG)" == "ParaView - Win32 Debug"

# Begin Custom Build
InputPath=.\vtkPVGlyph3D.h
InputName=vtkPVGlyph3D

"$(InputName)Tcl.cxx" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	..\..\vtk\pcmaker\vtkWrapTcl.exe $(InputName).h hints 1 > $(InputName)Tcl.cxx

# End Custom Build

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\vtkPVInputMenu.h

!IF  "$(CFG)" == "ParaView - Win32 Release"

!ELSEIF  "$(CFG)" == "ParaView - Win32 Debug"

# Begin Custom Build
InputPath=.\vtkPVInputMenu.h
InputName=vtkPVInputMenu

"$(InputName)Tcl.cxx" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	..\..\vtk\pcmaker\vtkWrapTcl.exe $(InputName).h hints 1 > $(InputName)Tcl.cxx

# End Custom Build

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\vtkPVMethodInterface.h

!IF  "$(CFG)" == "ParaView - Win32 Release"

!ELSEIF  "$(CFG)" == "ParaView - Win32 Debug"

# Begin Custom Build
InputPath=.\vtkPVMethodInterface.h
InputName=vtkPVMethodInterface

"$(InputName)Tcl.cxx" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	..\..\vtk\pcmaker\vtkWrapTcl.exe $(InputName).h hints 1 > $(InputName)Tcl.cxx

# End Custom Build

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\vtkPVRenderView.h

!IF  "$(CFG)" == "ParaView - Win32 Release"

!ELSEIF  "$(CFG)" == "ParaView - Win32 Debug"

# Begin Custom Build
InputPath=.\vtkPVRenderView.h
InputName=vtkPVRenderView

"$(InputName)Tcl.cxx" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	..\..\vtk\pcmaker\vtkWrapTcl.exe $(InputName).h hints 1 > $(InputName)Tcl.cxx

# End Custom Build

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\vtkPVSelectionList.h

!IF  "$(CFG)" == "ParaView - Win32 Release"

!ELSEIF  "$(CFG)" == "ParaView - Win32 Debug"

# Begin Custom Build
InputPath=.\vtkPVSelectionList.h
InputName=vtkPVSelectionList

"$(InputName)Tcl.cxx" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	..\..\vtk\pcmaker\vtkWrapTcl.exe $(InputName).h hints 1 > $(InputName)Tcl.cxx

# End Custom Build

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\vtkPVSource.h

!IF  "$(CFG)" == "ParaView - Win32 Release"

!ELSEIF  "$(CFG)" == "ParaView - Win32 Debug"

# Begin Custom Build
InputPath=.\vtkPVSource.h
InputName=vtkPVSource

"$(InputName)Tcl.cxx" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	..\..\vtk\pcmaker\vtkWrapTcl.exe $(InputName).h hints 1 > $(InputName)Tcl.cxx

# End Custom Build

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\vtkPVSourceCollection.h

!IF  "$(CFG)" == "ParaView - Win32 Release"

!ELSEIF  "$(CFG)" == "ParaView - Win32 Debug"

# Begin Custom Build
InputPath=.\vtkPVSourceCollection.h
InputName=vtkPVSourceCollection

"$(InputName)Tcl.cxx" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	..\..\vtk\pcmaker\vtkWrapTcl.exe $(InputName).h hints 1 > $(InputName)Tcl.cxx

# End Custom Build

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\vtkPVSourceInterface.h

!IF  "$(CFG)" == "ParaView - Win32 Release"

!ELSEIF  "$(CFG)" == "ParaView - Win32 Debug"

# Begin Custom Build
InputPath=.\vtkPVSourceInterface.h
InputName=vtkPVSourceInterface

"$(InputName)Tcl.cxx" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	..\..\vtk\pcmaker\vtkWrapTcl.exe $(InputName).h hints 1 > $(InputName)Tcl.cxx

# End Custom Build

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\vtkPVSourceList.h

!IF  "$(CFG)" == "ParaView - Win32 Release"

# Begin Custom Build
InputPath=.\vtkPVSourceList.h
InputName=vtkPVSourceList

"$(InputName)Tcl.cxx" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	..\..\vtk\pcmaker\vtkWrapTcl.exe $(InputName).h hints 1 > $(InputName)Tcl.cxx

# End Custom Build

!ELSEIF  "$(CFG)" == "ParaView - Win32 Debug"

# Begin Custom Build
InputPath=.\vtkPVSourceList.h
InputName=vtkPVSourceList

"$(InputName)Tcl.cxx" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	..\..\vtk\pcmaker\vtkWrapTcl.exe $(InputName).h hints 1 > $(InputName)Tcl.cxx

# End Custom Build

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\vtkPVThreshold.h

!IF  "$(CFG)" == "ParaView - Win32 Release"

!ELSEIF  "$(CFG)" == "ParaView - Win32 Debug"

# Begin Custom Build
InputPath=.\vtkPVThreshold.h
InputName=vtkPVThreshold

"$(InputName)Tcl.cxx" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	..\..\vtk\pcmaker\vtkWrapTcl.exe $(InputName).h hints 1 > $(InputName)Tcl.cxx

# End Custom Build

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\vtkPVWindow.h

!IF  "$(CFG)" == "ParaView - Win32 Release"

# Begin Custom Build
InputPath=.\vtkPVWindow.h
InputName=vtkPVWindow

"$(InputName)Tcl.cxx" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	..\..\vtk\pcmaker\vtkWrapTcl.exe $(InputName).h hints 1 > $(InputName)Tcl.cxx

# End Custom Build

!ELSEIF  "$(CFG)" == "ParaView - Win32 Debug"

# Begin Custom Build
InputPath=.\vtkPVWindow.h
InputName=vtkPVWindow

"$(InputName)Tcl.cxx" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	..\..\vtk\pcmaker\vtkWrapTcl.exe $(InputName).h hints 1 > $(InputName)Tcl.cxx

# End Custom Build

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\vtkSimpleFieldDataToAttributeDataFilter.h

!IF  "$(CFG)" == "ParaView - Win32 Release"

!ELSEIF  "$(CFG)" == "ParaView - Win32 Debug"

# Begin Custom Build
InputPath=.\vtkSimpleFieldDataToAttributeDataFilter.h
InputName=vtkSimpleFieldDataToAttributeDataFilter

"$(InputName)Tcl.cxx" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	..\..\vtk\pcmaker\vtkWrapTcl.exe $(InputName).h hints 1 > $(InputName)Tcl.cxx

# End Custom Build

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\vtkSingleContourFilter.h

!IF  "$(CFG)" == "ParaView - Win32 Release"

!ELSEIF  "$(CFG)" == "ParaView - Win32 Debug"

# Begin Custom Build
InputPath=.\vtkSingleContourFilter.h
InputName=vtkSingleContourFilter

"$(InputName)Tcl.cxx" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	..\..\vtk\pcmaker\vtkWrapTcl.exe $(InputName).h hints 1 > $(InputName)Tcl.cxx

# End Custom Build

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\vtkStringList.h

!IF  "$(CFG)" == "ParaView - Win32 Release"

!ELSEIF  "$(CFG)" == "ParaView - Win32 Debug"

# Begin Custom Build
InputPath=.\vtkStringList.h
InputName=vtkStringList

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
