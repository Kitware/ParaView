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
# ADD CPP /nologo /MDd /W3 /Gm /GX /ZI /Od /I "..\Widgets" /I "C:\program files\html help workshop\include" /I "..\..\vtk\common" /I "..\..\vtk\graphics" /I "..\..\vtk\imaging" /I "..\..\vtk\contrib" /I "..\..\vtk\patented" /I "..\..\vtk\pcmaker\xlib" /D "_DEBUG" /D "WIN32" /D "_WINDOWS" /D "_MBCS" /D "VTKDLL" /Fp"Debug/SciView.pch" /YX /FD /GZ /c
# ADD BASE MTL /nologo /D "_DEBUG" /mktyplib203 /win32
# ADD MTL /nologo /D "_DEBUG" /mktyplib203 /win32
# ADD BASE RSC /l 0x409 /d "_DEBUG"
# ADD RSC /l 0x409 /d "_DEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:windows /debug /machine:I386 /pdbtype:sept
# ADD LINK32 ..\..\vtkbin\Debug\lib\vtktcl.lib ..\Widgets\Debug\vtkKWWidgetsTcl.lib ..\..\vtkbin\Debug\lib\vtkPatented.lib opengl32.lib ..\..\vtk\pcmaker\tk82.lib ..\..\vtk\pcmaker\tcl82.lib vtktcl.lib vtkCommon.lib vtkImaging.lib vtkGraphics0.lib vtkGraphics1.lib vtkGraphics2.lib vtkGraphics3.lib vtkGraphics4.lib vtkContrib.lib htmlhelp.lib kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /dll /pdb:"../Debug/vtkKWParaViewTcl.pdb" /debug /machine:I386 /out:"Debug/vtkKWParaViewTcl.dll" /pdbtype:sept /libpath:"..\..\vtkbin\debug\lib" /libpath:"C:\program files\html help workshop\lib"
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

SOURCE=.\vtkColorByProcess.cxx
# End Source File
# Begin Source File

SOURCE=.\vtkColorByProcessTcl.cxx
# End Source File
# Begin Source File

SOURCE=.\vtkDummyRenderWindowInteractor.cxx
# End Source File
# Begin Source File

SOURCE=.\vtkDummyRenderWindowInteractorTcl.cxx
# End Source File
# Begin Source File

SOURCE=.\vtkImageOutlineFilter.cxx
# End Source File
# Begin Source File

SOURCE=.\vtkImageOutlineFilterTcl.cxx
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

SOURCE=.\vtkKWRenderView.cxx
# End Source File
# Begin Source File

SOURCE=.\vtkKWRenderViewTcl.cxx
# End Source File
# Begin Source File

SOURCE=.\vtkPVActorComposite.cxx
# End Source File
# Begin Source File

SOURCE=.\vtkPVActorCompositeTcl.cxx
# End Source File
# Begin Source File

SOURCE=.\vtkPVApplication.cxx
# End Source File
# Begin Source File

SOURCE=.\vtkPVApplicationTcl.cxx
# End Source File
# Begin Source File

SOURCE=.\vtkPVAssignment.cxx
# End Source File
# Begin Source File

SOURCE=.\vtkPVAssignmentTcl.cxx
# End Source File
# Begin Source File

SOURCE=.\vtkPVColorByProcess.cxx
# End Source File
# Begin Source File

SOURCE=.\vtkPVColorByProcessTcl.cxx
# End Source File
# Begin Source File

SOURCE=.\vtkPVConeSource.cxx
# End Source File
# Begin Source File

SOURCE=.\vtkPVConeSourceTcl.cxx
# End Source File
# Begin Source File

SOURCE=.\vtkPVContourFilter.cxx
# End Source File
# Begin Source File

SOURCE=.\vtkPVContourFilterTcl.cxx
# End Source File
# Begin Source File

SOURCE=.\vtkPVData.cxx
# End Source File
# Begin Source File

SOURCE=.\vtkPVDataSetToDataSetFilter.cxx
# End Source File
# Begin Source File

SOURCE=.\vtkPVDataSetToDataSetFilterTcl.cxx
# End Source File
# Begin Source File

SOURCE=.\vtkPVDataTcl.cxx
# End Source File
# Begin Source File

SOURCE=.\vtkPVElevationFilter.cxx
# End Source File
# Begin Source File

SOURCE=.\vtkPVElevationFilterTcl.cxx
# End Source File
# Begin Source File

SOURCE=.\vtkPVExtentTranslator.cxx
# End Source File
# Begin Source File

SOURCE=.\vtkPVExtentTranslatorTcl.cxx
# End Source File
# Begin Source File

SOURCE=.\vtkPVGlyph3D.cxx
# End Source File
# Begin Source File

SOURCE=.\vtkPVGlyph3DTcl.cxx
# End Source File
# Begin Source File

SOURCE=.\vtkPVImage.cxx
# End Source File
# Begin Source File

SOURCE=.\vtkPVImageClip.cxx
# End Source File
# Begin Source File

SOURCE=.\vtkPVImageClipTcl.cxx
# End Source File
# Begin Source File

SOURCE=.\vtkPVImageReader.cxx
# End Source File
# Begin Source File

SOURCE=.\vtkPVImageReaderTcl.cxx
# End Source File
# Begin Source File

SOURCE=.\vtkPVImageSlice.cxx
# End Source File
# Begin Source File

SOURCE=.\vtkPVImageSliceTcl.cxx
# End Source File
# Begin Source File

SOURCE=.\vtkPVImageTcl.cxx
# End Source File
# Begin Source File

SOURCE=.\vtkPVImageTextureFilter.cxx
# End Source File
# Begin Source File

SOURCE=.\vtkPVImageTextureFilterTcl.cxx
# End Source File
# Begin Source File

SOURCE=.\vtkPVMenuButton.cxx
# End Source File
# Begin Source File

SOURCE=.\vtkPVMenuButtonTcl.cxx
# End Source File
# Begin Source File

SOURCE=.\vtkPVPolyData.cxx
# End Source File
# Begin Source File

SOURCE=.\vtkPVPolyDataSource.cxx
# End Source File
# Begin Source File

SOURCE=.\vtkPVPolyDataSourceTcl.cxx
# End Source File
# Begin Source File

SOURCE=.\vtkPVPolyDataTcl.cxx
# End Source File
# Begin Source File

SOURCE=.\vtkPVPolyDataToPolyDataFilter.cxx
# End Source File
# Begin Source File

SOURCE=.\vtkPVPolyDataToPolyDataFilterTcl.cxx
# End Source File
# Begin Source File

SOURCE=.\vtkPVRenderView.cxx
# End Source File
# Begin Source File

SOURCE=.\vtkPVRenderViewTcl.cxx
# End Source File
# Begin Source File

SOURCE=.\vtkPVShrinkPolyData.cxx
# End Source File
# Begin Source File

SOURCE=.\vtkPVShrinkPolyDataTcl.cxx
# End Source File
# Begin Source File

SOURCE=.\vtkPVSource.cxx
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

SOURCE=.\vtkPVSphereSource.cxx
# End Source File
# Begin Source File

SOURCE=.\vtkPVWindow.cxx
# End Source File
# Begin Source File

SOURCE=.\vtkPVWindowTcl.cxx
# End Source File
# End Group
# Begin Group "Header Files"

# PROP Default_Filter "h;hpp;hxx;hm;inl"
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

SOURCE=.\vtkPVAssignment.h

!IF  "$(CFG)" == "ParaView - Win32 Release"

# Begin Custom Build
InputPath=.\vtkPVAssignment.h
InputName=vtkPVAssignment

"$(InputName)Tcl.cxx" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	..\..\vtk\pcmaker\vtkWrapTcl.exe $(InputName).h hints 1 > $(InputName)Tcl.cxx

# End Custom Build

!ELSEIF  "$(CFG)" == "ParaView - Win32 Debug"

# Begin Custom Build
InputPath=.\vtkPVAssignment.h
InputName=vtkPVAssignment

"$(InputName)Tcl.cxx" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	..\..\vtk\pcmaker\vtkWrapTcl.exe $(InputName).h hints 1 > $(InputName)Tcl.cxx

# End Custom Build

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\vtkPVColorByProcess.h

!IF  "$(CFG)" == "ParaView - Win32 Release"

!ELSEIF  "$(CFG)" == "ParaView - Win32 Debug"

# Begin Custom Build
InputPath=.\vtkPVColorByProcess.h
InputName=vtkPVColorByProcess

"$(InputName)Tcl.cxx" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	..\..\vtk\pcmaker\vtkWrapTcl.exe $(InputName).h hints 1 > $(InputName)Tcl.cxx

# End Custom Build

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\vtkPVConeSource.h

!IF  "$(CFG)" == "ParaView - Win32 Release"

!ELSEIF  "$(CFG)" == "ParaView - Win32 Debug"

# Begin Custom Build
InputPath=.\vtkPVConeSource.h
InputName=vtkPVConeSource

"$(InputName)Tcl.cxx" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	..\..\vtk\pcmaker\vtkWrapTcl.exe $(InputName).h hints 1 > $(InputName)Tcl.cxx

# End Custom Build

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\vtkPVContourFilter.h

!IF  "$(CFG)" == "ParaView - Win32 Release"

!ELSEIF  "$(CFG)" == "ParaView - Win32 Debug"

# Begin Custom Build
InputPath=.\vtkPVContourFilter.h
InputName=vtkPVContourFilter

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

SOURCE=.\vtkPVDataSetToDataSetFilter.h

!IF  "$(CFG)" == "ParaView - Win32 Release"

!ELSEIF  "$(CFG)" == "ParaView - Win32 Debug"

# Begin Custom Build
InputPath=.\vtkPVDataSetToDataSetFilter.h
InputName=vtkPVDataSetToDataSetFilter

"$(InputName)Tcl.cxx" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	..\..\vtk\pcmaker\vtkWrapTcl.exe $(InputName).h hints 1 > $(InputName)Tcl.cxx

# End Custom Build

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\vtkPVElevationFilter.h

!IF  "$(CFG)" == "ParaView - Win32 Release"

!ELSEIF  "$(CFG)" == "ParaView - Win32 Debug"

# Begin Custom Build
InputPath=.\vtkPVElevationFilter.h
InputName=vtkPVElevationFilter

"$(InputName)Tcl.cxx" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	..\..\vtk\pcmaker\vtkWrapTcl.exe $(InputName).h hints 1 > $(InputName)Tcl.cxx

# End Custom Build

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\vtkPVExtentTranslator.h

!IF  "$(CFG)" == "ParaView - Win32 Release"

!ELSEIF  "$(CFG)" == "ParaView - Win32 Debug"

# Begin Custom Build
InputPath=.\vtkPVExtentTranslator.h
InputName=vtkPVExtentTranslator

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

SOURCE=.\vtkPVImage.h

!IF  "$(CFG)" == "ParaView - Win32 Release"

!ELSEIF  "$(CFG)" == "ParaView - Win32 Debug"

# Begin Custom Build
InputPath=.\vtkPVImage.h
InputName=vtkPVImage

"$(InputName)Tcl.cxx" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	..\..\vtk\pcmaker\vtkWrapTcl.exe $(InputName).h hints 1 > $(InputName)Tcl.cxx

# End Custom Build

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\vtkPVImageClip.h

!IF  "$(CFG)" == "ParaView - Win32 Release"

!ELSEIF  "$(CFG)" == "ParaView - Win32 Debug"

# Begin Custom Build
InputPath=.\vtkPVImageClip.h
InputName=vtkPVImageClip

"$(InputName)Tcl.cxx" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	..\..\vtk\pcmaker\vtkWrapTcl.exe $(InputName).h hints 1 > $(InputName)Tcl.cxx

# End Custom Build

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\vtkPVImageReader.h

!IF  "$(CFG)" == "ParaView - Win32 Release"

!ELSEIF  "$(CFG)" == "ParaView - Win32 Debug"

# Begin Custom Build
InputPath=.\vtkPVImageReader.h
InputName=vtkPVImageReader

"$(InputName)Tcl.cxx" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	..\..\vtk\pcmaker\vtkWrapTcl.exe $(InputName).h hints 1 > $(InputName)Tcl.cxx

# End Custom Build

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\vtkPVImageSlice.h

!IF  "$(CFG)" == "ParaView - Win32 Release"

!ELSEIF  "$(CFG)" == "ParaView - Win32 Debug"

# Begin Custom Build
InputPath=.\vtkPVImageSlice.h
InputName=vtkPVImageSlice

"$(InputName)Tcl.cxx" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	..\..\vtk\pcmaker\vtkWrapTcl.exe $(InputName).h hints 1 > $(InputName)Tcl.cxx

# End Custom Build

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\vtkPVImageTextureFilter.h

!IF  "$(CFG)" == "ParaView - Win32 Release"

!ELSEIF  "$(CFG)" == "ParaView - Win32 Debug"

# Begin Custom Build
InputPath=.\vtkPVImageTextureFilter.h
InputName=vtkPVImageTextureFilter

"$(InputName)Tcl.cxx" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	..\..\vtk\pcmaker\vtkWrapTcl.exe $(InputName).h hints 1 > $(InputName)Tcl.cxx

# End Custom Build

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\vtkPVMenuButton.h

!IF  "$(CFG)" == "ParaView - Win32 Release"

!ELSEIF  "$(CFG)" == "ParaView - Win32 Debug"

# Begin Custom Build
InputPath=.\vtkPVMenuButton.h
InputName=vtkPVMenuButton

"$(InputName)Tcl.cxx" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	..\..\vtk\pcmaker\vtkWrapTcl.exe $(InputName).h hints 1 > $(InputName)Tcl.cxx

# End Custom Build

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\vtkPVPolyData.h

!IF  "$(CFG)" == "ParaView - Win32 Release"

!ELSEIF  "$(CFG)" == "ParaView - Win32 Debug"

# Begin Custom Build
InputPath=.\vtkPVPolyData.h
InputName=vtkPVPolyData

"$(InputName)Tcl.cxx" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	..\..\vtk\pcmaker\vtkWrapTcl.exe $(InputName).h hints 1 > $(InputName)Tcl.cxx

# End Custom Build

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\vtkPVPolyDataSource.h

!IF  "$(CFG)" == "ParaView - Win32 Release"

!ELSEIF  "$(CFG)" == "ParaView - Win32 Debug"

# Begin Custom Build
InputPath=.\vtkPVPolyDataSource.h
InputName=vtkPVPolyDataSource

"$(InputName)Tcl.cxx" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	..\..\vtk\pcmaker\vtkWrapTcl.exe $(InputName).h hints 1 > $(InputName)Tcl.cxx

# End Custom Build

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\vtkPVPolyDataToPolyDataFilter.h

!IF  "$(CFG)" == "ParaView - Win32 Release"

!ELSEIF  "$(CFG)" == "ParaView - Win32 Debug"

# Begin Custom Build
InputPath=.\vtkPVPolyDataToPolyDataFilter.h
InputName=vtkPVPolyDataToPolyDataFilter

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

SOURCE=.\vtkPVShrinkPolyData.h

!IF  "$(CFG)" == "ParaView - Win32 Release"

!ELSEIF  "$(CFG)" == "ParaView - Win32 Debug"

# Begin Custom Build
InputPath=.\vtkPVShrinkPolyData.h
InputName=vtkPVShrinkPolyData

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

SOURCE=.\vtkPVSphereSource.h

!IF  "$(CFG)" == "ParaView - Win32 Release"

!ELSEIF  "$(CFG)" == "ParaView - Win32 Debug"

# Begin Custom Build
InputPath=.\vtkPVSphereSource.h
InputName=vtkPVSphereSource

"$(InputName)Tcl.cxx" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	..\..\vtk\pcmaker\vtkWrapTcl.exe $(InputName).h hints 1 > $(InputName)Tcl.cxx

# End Custom Build

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\vtkPVSphereSourceTcl.cxx
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
# End Group
# Begin Group "Resource Files"

# PROP Default_Filter "ico;cur;bmp;dlg;rc2;rct;bin;rgs;gif;jpg;jpeg;jpe"
# End Group
# End Target
# End Project
