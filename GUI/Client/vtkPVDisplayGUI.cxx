/*=========================================================================

  Program:   ParaView
  Module:    vtkPVDisplayGUI.cxx

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkPVDisplayGUI.h"

#include "vtkPVColorMap.h"
#include "vtkCellData.h"
#include "vtkCollection.h"
#include "vtkDataSetAttributes.h"
#include "vtkDataSetSurfaceFilter.h"
#include "vtkSMDisplayProxy.h"
#include "vtkSMLODDisplayProxy.h"
#include "vtkSMIntVectorProperty.h"
#include "vtkSMDoubleVectorProperty.h"
#include "vtkSMStringVectorProperty.h"
#include "vtkPVColorSelectionWidget.h"
#include "vtkCollection.h"
#include "vtkColorTransferFunction.h"
#include "vtkPVDataInformation.h"
#include "vtkPVDataSetAttributesInformation.h"
#include "vtkPVArrayInformation.h"
#include "vtkImageData.h"
#include "vtkKWBoundsDisplay.h"
#include "vtkKWChangeColorButton.h"
#include "vtkKWCheckButton.h"
#include "vtkKWEntry.h"
#include "vtkKWFrame.h"
#include "vtkKWLabel.h"
#include "vtkKWMenuButton.h"
#include "vtkKWNotebook.h"
#include "vtkKWOptionMenu.h"
#include "vtkKWPushButton.h"
#include "vtkKWScale.h"
#include "vtkKWThumbWheel.h"
#include "vtkKWTkUtilities.h"
#include "vtkKWView.h"
#include "vtkKWWidget.h"
#include "vtkKWMenu.h"
#include "vtkMultiProcessController.h"
#include "vtkObjectFactory.h"
#include "vtkPiecewiseFunction.h"
#include "vtkPVApplication.h"
#include "vtkPVNumberOfOutputsInformation.h"
#include "vtkPVProcessModule.h"
#include "vtkPVSource.h"
#include "vtkPVVolumeAppearanceEditor.h"
#include "vtkPVWindow.h"
#include "vtkPointData.h"
#include "vtkPolyData.h"
#include "vtkPolyDataMapper.h"
#include "vtkProp3D.h"
#include "vtkProperty.h"
#include "vtkProperty2D.h"
#include "vtkRectilinearGrid.h"
#include "vtkRenderer.h"
#include "vtkStructuredGrid.h"
#include "vtkTexture.h"
#include "vtkTimerLog.h"
#include "vtkToolkits.h"
#include "vtkPVGenericRenderWindowInteractor.h"
#include "vtkPVRenderView.h"
#include "vtkPVRenderModule.h"
#include "vtkPVArrayInformation.h"
#include "vtkPVRenderModuleUI.h"
#include "vtkVolumeProperty.h"
#include "vtkPVOptions.h"
#include "vtkStdString.h"


#define VTK_PV_OUTLINE_LABEL "Outline"
#define VTK_PV_SURFACE_LABEL "Surface"
#define VTK_PV_WIREFRAME_LABEL "Wireframe of Surface"
#define VTK_PV_POINTS_LABEL "Points of Surface"
#define VTK_PV_VOLUME_LABEL "Volume Render"

//----------------------------------------------------------------------------
vtkStandardNewMacro(vtkPVDisplayGUI);
vtkCxxRevisionMacro(vtkPVDisplayGUI, "1.27.2.2");

int vtkPVDisplayGUICommand(ClientData cd, Tcl_Interp *interp,
                     int argc, char *argv[]);

//----------------------------------------------------------------------------
vtkPVDisplayGUI::vtkPVDisplayGUI()
{
  this->CommandFunction = vtkPVDisplayGUICommand;

  this->PVSource = 0;
  
  this->EditColorMapButtonVisible = 1;
  this->MapScalarsCheckVisible = 0;
  this->ColorButtonVisible = 1;
  this->ScalarBarCheckVisible = 1;
  this->InterpolateColorsCheckVisible = 1;

  this->ColorFrame = vtkKWFrameLabeled::New();
  this->VolumeAppearanceFrame = vtkKWFrameLabeled::New();
  this->DisplayStyleFrame = vtkKWFrameLabeled::New();
  this->ViewFrame = vtkKWFrameLabeled::New();
  
  this->ColorMenuLabel = vtkKWLabel::New();
  this->ColorSelectionMenu = vtkPVColorSelectionWidget::New();

  this->MapScalarsCheck = vtkKWCheckButton::New();
  this->InterpolateColorsCheck = vtkKWCheckButton::New();
  this->EditColorMapButtonFrame = vtkKWWidget::New();
  this->EditColorMapButton = vtkKWPushButton::New();
  this->DataColorRangeButton = vtkKWPushButton::New();
  
  this->ColorButton = vtkKWChangeColorButton::New();

  this->VolumeScalarsMenuLabel = vtkKWLabel::New();
  this->VolumeScalarSelectionWidget = vtkPVColorSelectionWidget::New();
  
  this->EditVolumeAppearanceButton = vtkKWPushButton::New();

  this->RepresentationMenuLabel = vtkKWLabel::New();
  this->RepresentationMenu = vtkKWOptionMenu::New();
  
  this->InterpolationMenuLabel = vtkKWLabel::New();
  this->InterpolationMenu = vtkKWOptionMenu::New();
  
  this->PointSizeLabel = vtkKWLabel::New();
  this->PointSizeThumbWheel = vtkKWThumbWheel::New();
  this->LineWidthLabel = vtkKWLabel::New();
  this->LineWidthThumbWheel = vtkKWThumbWheel::New();
  
  this->ScalarBarCheck = vtkKWCheckButton::New();
  this->CubeAxesCheck = vtkKWCheckButton::New();
  this->PointLabelCheck = vtkKWCheckButton::New();
  this->VisibilityCheck = vtkKWCheckButton::New();

  this->ResetCameraButton = vtkKWPushButton::New();

  this->ActorControlFrame = vtkKWFrameLabeled::New();
  this->TranslateLabel = vtkKWLabel::New();
  this->ScaleLabel = vtkKWLabel::New();
  this->OrientationLabel = vtkKWLabel::New();
  this->OriginLabel = vtkKWLabel::New();

  int cc;
  for ( cc = 0; cc < 3; cc ++ )
    {
    this->TranslateThumbWheel[cc] = vtkKWThumbWheel::New();
    this->ScaleThumbWheel[cc] = vtkKWThumbWheel::New();
    this->OrientationScale[cc] = vtkKWScale::New();
    this->OriginThumbWheel[cc] = vtkKWThumbWheel::New();
    }

  this->OpacityLabel = vtkKWLabel::New();
  this->OpacityScale = vtkKWScale::New();
  
  this->ActorColor[0] = this->ActorColor[1] = this->ActorColor[2] = 1.0;

  this->ColorSetByUser = 0;
  this->ArraySetByUser = 0;
    
  this->VolumeRenderMode = 0;
  
  this->VolumeAppearanceEditor = NULL;

  this->ShouldReinitialize = 0;

}

//----------------------------------------------------------------------------
vtkPVDisplayGUI::~vtkPVDisplayGUI()
{  
  if ( this->VolumeAppearanceEditor )
    {
    this->VolumeAppearanceEditor->UnRegister(this);
    this->VolumeAppearanceEditor = NULL;
    }
  
  this->SetPVSource(NULL);
    
  this->ColorMenuLabel->Delete();
  this->ColorMenuLabel = NULL;
  
  this->ColorSelectionMenu->Delete();
  this->ColorSelectionMenu = NULL;

  this->EditColorMapButtonFrame->Delete();
  this->EditColorMapButtonFrame = NULL;
  this->EditColorMapButton->Delete();
  this->EditColorMapButton = NULL;
  this->DataColorRangeButton->Delete();
  this->DataColorRangeButton = NULL;

  this->MapScalarsCheck->Delete();
  this->MapScalarsCheck = NULL;  
    
  this->InterpolateColorsCheck->Delete();
  this->InterpolateColorsCheck = NULL;  
    
  this->ColorButton->Delete();
  this->ColorButton = NULL;
  
  this->VolumeScalarsMenuLabel->Delete();
  this->VolumeScalarsMenuLabel = NULL;
  this->VolumeScalarSelectionWidget->Delete();
  this->VolumeScalarSelectionWidget = 0;

  this->EditVolumeAppearanceButton->Delete();
  this->EditVolumeAppearanceButton = NULL;
  
  this->RepresentationMenuLabel->Delete();
  this->RepresentationMenuLabel = NULL;  
  this->RepresentationMenu->Delete();
  this->RepresentationMenu = NULL;
  
  this->InterpolationMenuLabel->Delete();
  this->InterpolationMenuLabel = NULL;
  this->InterpolationMenu->Delete();
  this->InterpolationMenu = NULL;
  
  this->PointSizeLabel->Delete();
  this->PointSizeLabel = NULL;
  this->PointSizeThumbWheel->Delete();
  this->PointSizeThumbWheel = NULL;
  this->LineWidthLabel->Delete();
  this->LineWidthLabel = NULL;
  this->LineWidthThumbWheel->Delete();
  this->LineWidthThumbWheel = NULL;

  this->ActorControlFrame->Delete();
  this->TranslateLabel->Delete();
  this->ScaleLabel->Delete();
  this->OrientationLabel->Delete();
  this->OriginLabel->Delete();

  int cc;
  for ( cc = 0; cc < 3; cc ++ )
    {
    this->TranslateThumbWheel[cc]->Delete();
    this->ScaleThumbWheel[cc]->Delete();
    this->OrientationScale[cc]->Delete();
    this->OriginThumbWheel[cc]->Delete();
    }

  this->OpacityLabel->Delete();
  this->OpacityScale->Delete();
 
  this->ScalarBarCheck->Delete();
  this->ScalarBarCheck = NULL;  

  this->CubeAxesCheck->Delete();
  this->CubeAxesCheck = NULL;

  this->PointLabelCheck->Delete();
  this->PointLabelCheck = NULL;

  this->VisibilityCheck->Delete();
  this->VisibilityCheck = NULL;
  
  this->ColorFrame->Delete();
  this->ColorFrame = NULL;
  this->VolumeAppearanceFrame->Delete();
  this->VolumeAppearanceFrame = NULL;
  this->DisplayStyleFrame->Delete();
  this->DisplayStyleFrame = NULL;
  this->ViewFrame->Delete();
  this->ViewFrame = NULL;
  
  this->ResetCameraButton->Delete();
  this->ResetCameraButton = NULL;
}


//----------------------------------------------------------------------------
// Legacy for old scripts.
void vtkPVDisplayGUI::SetVisibility(int v)
{
  this->PVSource->SetVisibility(v);
}
void vtkPVDisplayGUI::SetCubeAxesVisibility(int v)
{
  this->PVSource->SetCubeAxesVisibility(v);
}
void vtkPVDisplayGUI::SetPointLabelVisibility(int v)
{
  this->PVSource->SetPointLabelVisibility(v);
}
void vtkPVDisplayGUI::SetScalarBarVisibility(int v)
{
  if (this->PVSource && this->PVSource->GetPVColorMap())
    {
    this->PVSource->GetPVColorMap()->SetScalarBarVisibility(v);
    }
}
vtkPVColorMap* vtkPVDisplayGUI::GetPVColorMap()
{
  if (this->PVSource)
    {
    return this->PVSource->GetPVColorMap();
    }
  return 0;
}

//----------------------------------------------------------------------------
void vtkPVDisplayGUI::Close()
{
  if (this->VolumeAppearanceEditor)
    {
    this->VolumeAppearanceEditor->Close();
    }
}

//----------------------------------------------------------------------------
void vtkPVDisplayGUI::SetVolumeAppearanceEditor(vtkPVVolumeAppearanceEditor *appearanceEditor)
{
  if ( this->VolumeAppearanceEditor == appearanceEditor )
    {
    return;
    }
  
  if ( this->VolumeAppearanceEditor )
    {
    this->VolumeAppearanceEditor->UnRegister(this);
    this->VolumeAppearanceEditor = NULL;
    }
  
  if ( appearanceEditor )
    {
    this->VolumeAppearanceEditor = appearanceEditor;
    this->VolumeAppearanceEditor->Register(this);
    }
}


//----------------------------------------------------------------------------
void vtkPVDisplayGUI::DeleteCallback()
{
  if (this->PVSource)
    {
    this->PVSource->SetCubeAxesVisibility(0);
    }
  if (this->PVSource)
    {
    this->PVSource->SetPointLabelVisibility(0);
    }
}



//----------------------------------------------------------------------------
// WE DO NOT REFERENCE COUNT HERE BECAUSE OF CIRCULAR REFERENCES.
// THE SOURCE OWNS THE DATA.
void vtkPVDisplayGUI::SetPVSource(vtkPVSource *source)
{
  if (this->PVSource == source)
    {
    return;
    }
  this->Modified();

  this->PVSource = source;

  this->SetTraceReferenceObject(source);
  this->SetTraceReferenceCommand("GetPVOutput");
}






// ============= Use to be in vtkPVActorComposite ===================

//----------------------------------------------------------------------------
void vtkPVDisplayGUI::Create(vtkKWApplication* app, const char* options)
{
  if (this->GetApplication())
    {
    vtkErrorMacro("Widget already created.");
    return;
    }
  this->Superclass::Create(app, options);

  // We are going to 'grid' most of it, so let's define some const

  int col_1_padx = 2;
  int button_pady = 1;
  int col_0_weight = 0;
  int col_1_weight = 1;
  float col_0_factor = 1.5;
  float col_1_factor = 1.0;

  // View frame

  this->ViewFrame->SetParent(this->GetFrame());
  this->ViewFrame->ShowHideFrameOn();
  this->ViewFrame->Create(this->GetApplication(), 0);
  this->ViewFrame->SetLabelText("View");
 
  this->VisibilityCheck->SetParent(this->ViewFrame->GetFrame());
  this->VisibilityCheck->Create(this->GetApplication(), "-text Data");
  this->GetApplication()->Script(
    "%s configure -command {%s VisibilityCheckCallback}",
    this->VisibilityCheck->GetWidgetName(),
    this->GetTclName());
  this->VisibilityCheck->SetState(1);
  this->VisibilityCheck->SetBalloonHelpString(
    "Toggle the visibility of this dataset's geometry.");

  this->ResetCameraButton->SetParent(this->ViewFrame->GetFrame());
  this->ResetCameraButton->Create(this->GetApplication(), "");
  this->ResetCameraButton->SetText("Set View to Data");
  this->ResetCameraButton->SetCommand(this, "CenterCamera");
  this->ResetCameraButton->SetBalloonHelpString(
    "Change the camera location to best fit the dataset in the view window.");

  this->ScalarBarCheck->SetParent(this->ViewFrame->GetFrame());
  this->ScalarBarCheck->Create(this->GetApplication(), "-text {Scalar bar}");
  this->ScalarBarCheck->SetBalloonHelpString(
    "Toggle the visibility of the scalar bar for this data.");
  this->GetApplication()->Script(
    "%s configure -command {%s ScalarBarCheckCallback}",
    this->ScalarBarCheck->GetWidgetName(),
    this->GetTclName());

  this->CubeAxesCheck->SetParent(this->ViewFrame->GetFrame());
  this->CubeAxesCheck->Create(this->GetApplication(), "-text {Cube Axes}");
  this->CubeAxesCheck->SetCommand(this, "CubeAxesCheckCallback");
  this->CubeAxesCheck->SetBalloonHelpString(
    "Toggle the visibility of X,Y,Z scales for this dataset.");

  this->PointLabelCheck->SetParent(this->ViewFrame->GetFrame());
  this->PointLabelCheck->Create(this->GetApplication(), "-text {Label Point Ids}");
  this->PointLabelCheck->SetCommand(this, "PointLabelCheckCallback");
  this->PointLabelCheck->SetBalloonHelpString(
    "Toggle the visibility of point id labels for this dataset.");
  
  this->Script("grid %s %s -sticky wns",
               this->VisibilityCheck->GetWidgetName(),
               this->ResetCameraButton->GetWidgetName());

  this->Script("grid %s -sticky news -padx %d -pady %d",
               this->ResetCameraButton->GetWidgetName(),
               col_1_padx, button_pady);

  this->Script("grid %s -sticky wns",
               this->ScalarBarCheck->GetWidgetName());
  
  this->Script("grid %s -sticky wns",
               this->CubeAxesCheck->GetWidgetName());

  if ((this->GetPVApplication()->GetProcessModule()->GetNumberOfPartitions() == 1) &&
      (!this->GetPVApplication()->GetOptions()->GetClientMode()))
    {
    this->Script("grid %s -sticky wns",
                 this->PointLabelCheck->GetWidgetName());
    }

  // Color
  this->ColorFrame->SetParent(this->GetFrame());
  this->ColorFrame->ShowHideFrameOn();
  this->ColorFrame->Create(this->GetApplication(), 0);
  this->ColorFrame->SetLabelText("Color");

  this->ColorMenuLabel->SetParent(this->ColorFrame->GetFrame());
  this->ColorMenuLabel->Create(this->GetApplication(), "");
  this->ColorMenuLabel->SetText("Color by:");
  this->ColorMenuLabel->SetBalloonHelpString(
    "Select method for coloring dataset geometry.");
  
  this->ColorSelectionMenu->SetParent(this->ColorFrame->GetFrame());
  this->ColorSelectionMenu->Create(this->GetApplication(), "");   
  this->ColorSelectionMenu->SetColorSelectionCommand("ColorByArray");
  this->ColorSelectionMenu->SetTarget(this);
  this->ColorSelectionMenu->SetBalloonHelpString(
    "Select method for coloring dataset geometry.");

  this->ColorButton->SetParent(this->ColorFrame->GetFrame());
  this->ColorButton->GetLabel()->SetText("Actor Color");
  this->ColorButton->Create(this->GetApplication(), "");
  this->ColorButton->SetCommand(this, "ChangeActorColor");
  this->ColorButton->SetBalloonHelpString(
    "Edit the constant color for the geometry.");

  this->MapScalarsCheck->SetParent(this->ColorFrame->GetFrame());
  this->MapScalarsCheck->Create(this->GetApplication(), "-text {Map Scalars}");
  this->MapScalarsCheck->SetState(0);
  this->MapScalarsCheck->SetBalloonHelpString(
    "Pass attriubte through color map or use unsigned char values as color.");
  this->GetApplication()->Script(
    "%s configure -command {%s MapScalarsCheckCallback}",
    this->MapScalarsCheck->GetWidgetName(),
    this->GetTclName());
    
  this->InterpolateColorsCheck->SetParent(this->ColorFrame->GetFrame());
  this->InterpolateColorsCheck->Create(this->GetApplication(), "-text {Interpolate Colors}");
  this->InterpolateColorsCheck->SetState(0);
  this->InterpolateColorsCheck->SetBalloonHelpString(
    "Interpolate colors after mapping.");
  this->GetApplication()->Script(
    "%s configure -command {%s InterpolateColorsCheckCallback}",
    this->InterpolateColorsCheck->GetWidgetName(),
    this->GetTclName());
    
  // Group these two buttons in the place of one.
  this->EditColorMapButtonFrame->SetParent(this->ColorFrame->GetFrame());
  this->EditColorMapButtonFrame->Create(this->GetApplication(), "frame", "");
  // --
  this->EditColorMapButton->SetParent(this->EditColorMapButtonFrame);
  this->EditColorMapButton->Create(this->GetApplication(), "");
  this->EditColorMapButton->SetText("Edit Color Map");
  this->EditColorMapButton->SetCommand(this,"EditColorMapCallback");
  this->EditColorMapButton->SetBalloonHelpString(
    "Edit the table used to map data attributes to pseudo colors.");
  // --
  this->DataColorRangeButton->SetParent(this->EditColorMapButtonFrame);
  this->DataColorRangeButton->Create(this->GetApplication(), "");
  this->DataColorRangeButton->SetText("Reset Range");
  this->DataColorRangeButton->SetCommand(this,"DataColorRangeCallback");
  this->DataColorRangeButton->SetBalloonHelpString(
    "Sets the range of the color map to this module's scalar range.");
  // --
  this->Script("pack %s %s -side left -fill x -expand t -pady 2", 
               this->EditColorMapButton->GetWidgetName(),
               this->DataColorRangeButton->GetWidgetName());

  this->Script("grid %s %s -sticky wns",
               this->ColorMenuLabel->GetWidgetName(),
               this->ColorSelectionMenu->GetWidgetName());
  this->Script("grid %s -sticky news -padx %d -pady %d",
               this->ColorSelectionMenu->GetWidgetName(),
               col_1_padx, button_pady);

  this->Script("grid %s %s -sticky wns",
               this->MapScalarsCheck->GetWidgetName(),
               this->ColorButton->GetWidgetName());
  this->Script("grid %s -column 1 -sticky news -padx %d -pady %d",
               this->ColorButton->GetWidgetName(),
               col_1_padx, button_pady);
  this->ColorButtonVisible = 0;
  this->InterpolateColorsCheckVisible = 0;

  this->Script("grid %s %s -sticky wns",
               this->InterpolateColorsCheck->GetWidgetName(),
               this->EditColorMapButtonFrame->GetWidgetName());
  this->Script("grid %s -sticky news -padx %d -pady %d",
               this->EditColorMapButtonFrame->GetWidgetName(),
               col_1_padx, button_pady);

  // Volume Appearance
  this->SetVolumeAppearanceEditor(this->GetPVApplication()->GetMainWindow()->
                                  GetVolumeAppearanceEditor());
  this->VolumeAppearanceFrame->SetParent(this->GetFrame());
  this->VolumeAppearanceFrame->ShowHideFrameOn();
  this->VolumeAppearanceFrame->Create(this->GetApplication(), 0);
  this->VolumeAppearanceFrame->SetLabelText("Volume Appearance");

  this->VolumeScalarsMenuLabel->
    SetParent(this->VolumeAppearanceFrame->GetFrame());
  this->VolumeScalarsMenuLabel->Create(this->GetApplication(), "");
  this->VolumeScalarsMenuLabel->SetText("View Scalars:");
  this->VolumeScalarsMenuLabel->SetBalloonHelpString(
    "Select scalars to view with volume rendering.");

  this->VolumeScalarSelectionWidget->SetParent(this->VolumeAppearanceFrame->GetFrame());
  this->VolumeScalarSelectionWidget->Create(this->GetApplication(), "");
  this->VolumeScalarSelectionWidget->SetColorSelectionCommand("VolumeRenderByArray");
  this->VolumeScalarSelectionWidget->SetTarget(this);
  this->VolumeScalarSelectionWidget->SetBalloonHelpString(
    "Select scalars to view with volume rendering.");

  this->EditVolumeAppearanceButton->
    SetParent(this->VolumeAppearanceFrame->GetFrame());
  this->EditVolumeAppearanceButton->Create(this->GetApplication(), "");
  this->EditVolumeAppearanceButton->SetText("Edit Volume Appearance...");
  this->EditVolumeAppearanceButton->
    SetCommand(this,"EditVolumeAppearanceCallback");
  this->EditVolumeAppearanceButton->SetBalloonHelpString(
    "Edit the color and opacity functions for the volume.");

  
  this->Script("grid %s %s -sticky wns",
               this->VolumeScalarsMenuLabel->GetWidgetName(),
               this->VolumeScalarSelectionWidget->GetWidgetName());

  this->Script("grid %s -sticky news -padx %d -pady %d",
               this->VolumeScalarSelectionWidget->GetWidgetName(),
               col_1_padx, button_pady);

  this->Script("grid %s -column 1 -sticky news -padx %d -pady %d",
               this->EditVolumeAppearanceButton->GetWidgetName(),
               col_1_padx, button_pady);


  // Display style
  this->DisplayStyleFrame->SetParent(this->GetFrame());
  this->DisplayStyleFrame->ShowHideFrameOn();
  this->DisplayStyleFrame->Create(this->GetApplication(), 0);
  this->DisplayStyleFrame->SetLabelText("Display Style");
  
  this->RepresentationMenuLabel->SetParent(
    this->DisplayStyleFrame->GetFrame());
  this->RepresentationMenuLabel->Create(this->GetApplication(), "");
  this->RepresentationMenuLabel->SetText("Representation:");

  this->RepresentationMenu->SetParent(this->DisplayStyleFrame->GetFrame());
  this->RepresentationMenu->Create(this->GetApplication(), "");
  this->RepresentationMenu->AddEntryWithCommand(VTK_PV_OUTLINE_LABEL, this,
                                                "DrawOutline");
  this->RepresentationMenu->AddEntryWithCommand(VTK_PV_SURFACE_LABEL, this,
                                                "DrawSurface");
  this->RepresentationMenu->AddEntryWithCommand(VTK_PV_WIREFRAME_LABEL, this,
                                                "DrawWireframe");
  this->RepresentationMenu->AddEntryWithCommand(VTK_PV_POINTS_LABEL, this,
                                                "DrawPoints");

  this->RepresentationMenu->SetBalloonHelpString(
    "Choose what geometry should be used to represent the dataset.");

  this->InterpolationMenuLabel->SetParent(this->DisplayStyleFrame->GetFrame());
  this->InterpolationMenuLabel->Create(this->GetApplication(), "");
  this->InterpolationMenuLabel->SetText("Interpolation:");

  this->InterpolationMenu->SetParent(this->DisplayStyleFrame->GetFrame());
  this->InterpolationMenu->Create(this->GetApplication(), "");
  this->InterpolationMenu->AddEntryWithCommand("Flat", this,
                                               "SetInterpolationToFlat");
  this->InterpolationMenu->AddEntryWithCommand("Gouraud", this,
                                               "SetInterpolationToGouraud");
  this->InterpolationMenu->SetValue("Gouraud");
  this->InterpolationMenu->SetBalloonHelpString(
    "Choose the method used to shade the geometry and interpolate point attributes.");

  this->PointSizeLabel->SetParent(this->DisplayStyleFrame->GetFrame());
  this->PointSizeLabel->Create(this->GetApplication(), "");
  this->PointSizeLabel->SetText("Point size:");
  this->PointSizeLabel->SetBalloonHelpString(
    "If your dataset contains points/verticies, "
    "this scale adjusts the diameter of the rendered points.");

  this->PointSizeThumbWheel->SetParent(this->DisplayStyleFrame->GetFrame());
  this->PointSizeThumbWheel->PopupModeOn();
  this->PointSizeThumbWheel->SetValue(1.0);
  this->PointSizeThumbWheel->SetResolution(1.0);
  this->PointSizeThumbWheel->SetMinimumValue(1.0);
  this->PointSizeThumbWheel->ClampMinimumValueOn();
  this->PointSizeThumbWheel->Create(this->GetApplication(), "");
  this->PointSizeThumbWheel->DisplayEntryOn();
  this->PointSizeThumbWheel->DisplayEntryAndLabelOnTopOff();
  this->PointSizeThumbWheel->SetBalloonHelpString("Set the point size.");
  this->PointSizeThumbWheel->GetEntry()->SetWidth(5);
  this->PointSizeThumbWheel->SetCommand(this, "ChangePointSize");
  this->PointSizeThumbWheel->SetEndCommand(this, "ChangePointSizeEndCallback");
  this->PointSizeThumbWheel->SetEntryCommand(this, "ChangePointSizeEndCallback");
  this->PointSizeThumbWheel->SetBalloonHelpString(
    "If your dataset contains points/verticies, "
    "this scale adjusts the diameter of the rendered points.");

  this->LineWidthLabel->SetParent(this->DisplayStyleFrame->GetFrame());
  this->LineWidthLabel->Create(this->GetApplication(), "");
  this->LineWidthLabel->SetText("Line width:");
  this->LineWidthLabel->SetBalloonHelpString(
    "If your dataset containes lines/edges, "
    "this scale adjusts the width of the rendered lines.");
  
  this->LineWidthThumbWheel->SetParent(this->DisplayStyleFrame->GetFrame());
  this->LineWidthThumbWheel->PopupModeOn();
  this->LineWidthThumbWheel->SetValue(1.0);
  this->LineWidthThumbWheel->SetResolution(1.0);
  this->LineWidthThumbWheel->SetMinimumValue(1.0);
  this->LineWidthThumbWheel->ClampMinimumValueOn();
  this->LineWidthThumbWheel->Create(this->GetApplication(), "");
  this->LineWidthThumbWheel->DisplayEntryOn();
  this->LineWidthThumbWheel->DisplayEntryAndLabelOnTopOff();
  this->LineWidthThumbWheel->SetBalloonHelpString("Set the line width.");
  this->LineWidthThumbWheel->GetEntry()->SetWidth(5);
  this->LineWidthThumbWheel->SetCommand(this, "ChangeLineWidth");
  this->LineWidthThumbWheel->SetEndCommand(this, "ChangeLineWidthEndCallback");
  this->LineWidthThumbWheel->SetEntryCommand(this, "ChangeLineWidthEndCallback");
  this->LineWidthThumbWheel->SetBalloonHelpString(
    "If your dataset containes lines/edges, "
    "this scale adjusts the width of the rendered lines.");

  this->Script("grid %s %s -sticky wns",
               this->RepresentationMenuLabel->GetWidgetName(),
               this->RepresentationMenu->GetWidgetName());

  this->Script("grid %s -sticky news -padx %d -pady %d",
               this->RepresentationMenu->GetWidgetName(), 
               col_1_padx, button_pady);

  this->Script("grid %s %s -sticky wns",
               this->InterpolationMenuLabel->GetWidgetName(),
               this->InterpolationMenu->GetWidgetName());

  this->Script("grid %s -sticky news -padx %d -pady %d",
               this->InterpolationMenu->GetWidgetName(),
               col_1_padx, button_pady);
  
  this->Script("grid %s %s -sticky wns",
               this->PointSizeLabel->GetWidgetName(),
               this->PointSizeThumbWheel->GetWidgetName());

  this->Script("grid %s -sticky news -padx %d -pady %d",
               this->PointSizeThumbWheel->GetWidgetName(), 
               col_1_padx, button_pady);

  this->Script("grid %s %s -sticky wns",
               this->LineWidthLabel->GetWidgetName(),
               this->LineWidthThumbWheel->GetWidgetName());

  this->Script("grid %s -sticky news -padx %d -pady %d",
               this->LineWidthThumbWheel->GetWidgetName(),
               col_1_padx, button_pady);

  // Now synchronize all those grids to have them aligned

  const char *widgets[4];
  widgets[0] = this->ViewFrame->GetFrame()->GetWidgetName();
  widgets[1] = this->ColorFrame->GetFrame()->GetWidgetName();
  widgets[2] = this->VolumeAppearanceFrame->GetFrame()->GetWidgetName();
  widgets[3] = this->DisplayStyleFrame->GetFrame()->GetWidgetName();

  int weights[2];
  weights[0] = col_0_weight;
  weights[1] = col_1_weight;

  float factors[2];
  factors[0] = col_0_factor;
  factors[1] = col_1_factor;

  vtkKWTkUtilities::SynchroniseGridsColumnMinimumSize(
    this->GetPVApplication()->GetMainInterp(), 4, widgets, factors, weights);
  
  // Actor Control

  this->ActorControlFrame->SetParent(this->GetFrame());
  this->ActorControlFrame->ShowHideFrameOn();
  this->ActorControlFrame->Create(this->GetApplication(), 0);
  this->ActorControlFrame->SetLabelText("Actor Control");

  this->TranslateLabel->SetParent(this->ActorControlFrame->GetFrame());
  this->TranslateLabel->Create(this->GetApplication(), 0);
  this->TranslateLabel->SetText("Translate:");
  this->TranslateLabel->SetBalloonHelpString(
    "Translate the geometry relative to the dataset location.");

  this->ScaleLabel->SetParent(this->ActorControlFrame->GetFrame());
  this->ScaleLabel->Create(this->GetApplication(), 0);
  this->ScaleLabel->SetText("Scale:");
  this->ScaleLabel->SetBalloonHelpString(
    "Scale the geometry relative to the size of the dataset.");

  this->OrientationLabel->SetParent(this->ActorControlFrame->GetFrame());
  this->OrientationLabel->Create(this->GetApplication(), 0);
  this->OrientationLabel->SetText("Orientation:");
  this->OrientationLabel->SetBalloonHelpString(
    "Orient the geometry relative to the dataset origin.");

  this->OriginLabel->SetParent(this->ActorControlFrame->GetFrame());
  this->OriginLabel->Create(this->GetApplication(), 0);
  this->OriginLabel->SetText("Origin:");
  this->OriginLabel->SetBalloonHelpString(
    "Set the origin point about which rotations take place.");

  int cc;
  for ( cc = 0; cc < 3; cc ++ )
    {
    this->TranslateThumbWheel[cc]->SetParent(this->ActorControlFrame->GetFrame());
    this->TranslateThumbWheel[cc]->PopupModeOn();
    this->TranslateThumbWheel[cc]->SetValue(0.0);
    this->TranslateThumbWheel[cc]->Create(this->GetApplication(), 0);
    this->TranslateThumbWheel[cc]->DisplayEntryOn();
    this->TranslateThumbWheel[cc]->DisplayEntryAndLabelOnTopOff();
    this->TranslateThumbWheel[cc]->ExpandEntryOn();
    this->TranslateThumbWheel[cc]->GetEntry()->SetWidth(5);
    this->TranslateThumbWheel[cc]->SetCommand(this, "ActorTranslateCallback");
    this->TranslateThumbWheel[cc]->SetEndCommand(this, 
                                                 "ActorTranslateEndCallback");
    this->TranslateThumbWheel[cc]->SetEntryCommand(this,
                                                   "ActorTranslateEndCallback");
    this->TranslateThumbWheel[cc]->SetBalloonHelpString(
      "Translate the geometry relative to the dataset location.");

    this->ScaleThumbWheel[cc]->SetParent(this->ActorControlFrame->GetFrame());
    this->ScaleThumbWheel[cc]->PopupModeOn();
    this->ScaleThumbWheel[cc]->SetValue(1.0);
    this->ScaleThumbWheel[cc]->SetMinimumValue(0.0);
    this->ScaleThumbWheel[cc]->ClampMinimumValueOn();
    this->ScaleThumbWheel[cc]->SetResolution(0.05);
    this->ScaleThumbWheel[cc]->Create(this->GetApplication(), 0);
    this->ScaleThumbWheel[cc]->DisplayEntryOn();
    this->ScaleThumbWheel[cc]->DisplayEntryAndLabelOnTopOff();
    this->ScaleThumbWheel[cc]->ExpandEntryOn();
    this->ScaleThumbWheel[cc]->GetEntry()->SetWidth(5);
    this->ScaleThumbWheel[cc]->SetCommand(this, "ActorScaleCallback");
    this->ScaleThumbWheel[cc]->SetEndCommand(this, "ActorScaleEndCallback");
    this->ScaleThumbWheel[cc]->SetEntryCommand(this, "ActorScaleEndCallback");
    this->ScaleThumbWheel[cc]->SetBalloonHelpString(
      "Scale the geometry relative to the size of the dataset.");

    this->OrientationScale[cc]->SetParent(this->ActorControlFrame->GetFrame());
    this->OrientationScale[cc]->PopupScaleOn();
    this->OrientationScale[cc]->Create(this->GetApplication(), 0);
    this->OrientationScale[cc]->SetRange(0, 360);
    this->OrientationScale[cc]->SetResolution(1);
    this->OrientationScale[cc]->SetValue(0);
    this->OrientationScale[cc]->DisplayEntry();
    this->OrientationScale[cc]->DisplayEntryAndLabelOnTopOff();
    this->OrientationScale[cc]->ExpandEntryOn();
    this->OrientationScale[cc]->GetEntry()->SetWidth(5);
    this->OrientationScale[cc]->SetCommand(this, "ActorOrientationCallback");
    this->OrientationScale[cc]->SetEndCommand(this, 
                                              "ActorOrientationEndCallback");
    this->OrientationScale[cc]->SetEntryCommand(this, 
                                                "ActorOrientationEndCallback");
    this->OrientationScale[cc]->SetBalloonHelpString(
      "Orient the geometry relative to the dataset origin.");

    this->OriginThumbWheel[cc]->SetParent(this->ActorControlFrame->GetFrame());
    this->OriginThumbWheel[cc]->PopupModeOn();
    this->OriginThumbWheel[cc]->SetValue(0.0);
    this->OriginThumbWheel[cc]->Create(this->GetApplication(), 0);
    this->OriginThumbWheel[cc]->DisplayEntryOn();
    this->OriginThumbWheel[cc]->DisplayEntryAndLabelOnTopOff();
    this->OriginThumbWheel[cc]->ExpandEntryOn();
    this->OriginThumbWheel[cc]->GetEntry()->SetWidth(5);
    this->OriginThumbWheel[cc]->SetCommand(this, "ActorOriginCallback");
    this->OriginThumbWheel[cc]->SetEndCommand(this, "ActorOriginEndCallback");
    this->OriginThumbWheel[cc]->SetEntryCommand(this,"ActorOriginEndCallback");
    this->OriginThumbWheel[cc]->SetBalloonHelpString(
      "Orient the geometry relative to the dataset origin.");
    }

  this->OpacityLabel->SetParent(this->ActorControlFrame->GetFrame());
  this->OpacityLabel->Create(this->GetApplication(), 0);
  this->OpacityLabel->SetText("Opacity:");
  this->OpacityLabel->SetBalloonHelpString(
    "Set the opacity of the dataset's geometry.  "
    "Artifacts may appear in translucent geomtry "
    "because primatives are not sorted.");

  this->OpacityScale->SetParent(this->ActorControlFrame->GetFrame());
  this->OpacityScale->PopupScaleOn();
  this->OpacityScale->Create(this->GetApplication(), 0);
  this->OpacityScale->SetRange(0, 1);
  this->OpacityScale->SetResolution(0.1);
  this->OpacityScale->SetValue(1);
  this->OpacityScale->DisplayEntry();
  this->OpacityScale->DisplayEntryAndLabelOnTopOff();
  this->OpacityScale->ExpandEntryOn();
  this->OpacityScale->GetEntry()->SetWidth(5);
  this->OpacityScale->SetCommand(this, "OpacityChangedCallback");
  this->OpacityScale->SetEndCommand(this, "OpacityChangedEndCallback");
  this->OpacityScale->SetEntryCommand(this, "OpacityChangedEndCallback");
  this->OpacityScale->SetBalloonHelpString(
    "Set the opacity of the dataset's geometry.  "
    "Artifacts may appear in translucent geomtry "
    "because primatives are not sorted.");

  this->Script("grid %s %s %s %s -sticky news -pady %d",
               this->TranslateLabel->GetWidgetName(),
               this->TranslateThumbWheel[0]->GetWidgetName(),
               this->TranslateThumbWheel[1]->GetWidgetName(),
               this->TranslateThumbWheel[2]->GetWidgetName(),
               button_pady);

  this->Script("grid %s -sticky nws",
               this->TranslateLabel->GetWidgetName());

  this->Script("grid %s %s %s %s -sticky news -pady %d",
               this->ScaleLabel->GetWidgetName(),
               this->ScaleThumbWheel[0]->GetWidgetName(),
               this->ScaleThumbWheel[1]->GetWidgetName(),
               this->ScaleThumbWheel[2]->GetWidgetName(),
               button_pady);

  this->Script("grid %s -sticky nws",
               this->ScaleLabel->GetWidgetName());

  this->Script("grid %s %s %s %s -sticky news -pady %d",
               this->OrientationLabel->GetWidgetName(),
               this->OrientationScale[0]->GetWidgetName(),
               this->OrientationScale[1]->GetWidgetName(),
               this->OrientationScale[2]->GetWidgetName(),
               button_pady);

  this->Script("grid %s -sticky nws",
               this->OrientationLabel->GetWidgetName());

  this->Script("grid %s %s %s %s -sticky news -pady %d",
               this->OriginLabel->GetWidgetName(),
               this->OriginThumbWheel[0]->GetWidgetName(),
               this->OriginThumbWheel[1]->GetWidgetName(),
               this->OriginThumbWheel[2]->GetWidgetName(),
               button_pady);

  this->Script("grid %s -sticky nws",
               this->OriginLabel->GetWidgetName());

  this->Script("grid %s %s -sticky news -pady %d",
               this->OpacityLabel->GetWidgetName(),
               this->OpacityScale->GetWidgetName(),
               button_pady);

  this->Script("grid %s -sticky nws",
               this->OpacityLabel->GetWidgetName());

  this->Script("grid columnconfigure %s 0 -weight 0", 
               this->ActorControlFrame->GetFrame()->GetWidgetName());

  this->Script("grid columnconfigure %s 1 -weight 2", 
               this->ActorControlFrame->GetFrame()->GetWidgetName());

  this->Script("grid columnconfigure %s 2 -weight 2", 
               this->ActorControlFrame->GetFrame()->GetWidgetName());

  this->Script("grid columnconfigure %s 3 -weight 2", 
               this->ActorControlFrame->GetFrame()->GetWidgetName());

  // Pack

  this->Script("pack %s %s %s %s -fill x -expand t -pady 2", 
               this->ViewFrame->GetWidgetName(),
               this->ColorFrame->GetWidgetName(),
               this->DisplayStyleFrame->GetWidgetName(),
               this->ActorControlFrame->GetWidgetName());

  // Information page

  this->UpdateEnableState();
}

//----------------------------------------------------------------------------
void vtkPVDisplayGUI::EditColorMapCallback()
{
  if (this->PVSource == 0 || this->PVSource->GetPVColorMap() == 0)
    {
    // We could get the color map from the window,
    // but it must already be set for this button to be visible.
    vtkErrorMacro("Expecting a color map.");
    return;
    }
  this->Script("pack forget [pack slaves %s]",
          this->GetPVRenderView()->GetPropertiesParent()->GetWidgetName());
          
  this->Script("pack %s -side top -fill both -expand t",
          this->PVSource->GetPVColorMap()->GetWidgetName());
}

//----------------------------------------------------------------------------
void vtkPVDisplayGUI::DataColorRangeCallback()
{
  this->AddTraceEntry("$kw(%s) DataColorRangeCallback", this->GetTclName());
  if (this->PVSource)
    {
    vtkPVColorMap* colorMap = this->PVSource->GetPVColorMap();
    if (colorMap)
      {
      colorMap->ResetScalarRangeInternal(this->PVSource);
      }
    }
}

//----------------------------------------------------------------------------
void vtkPVDisplayGUI::EditVolumeAppearanceCallback()
{
  if (this->VolumeAppearanceEditor == NULL)
    {
    vtkErrorMacro("Expecting a volume appearance editor");
    return;
    }
  
  this->AddTraceEntry("$kw(%s) ShowVolumeAppearanceEditor",
                      this->GetTclName());

  this->ShowVolumeAppearanceEditor();
}

//----------------------------------------------------------------------------
void vtkPVDisplayGUI::ShowVolumeAppearanceEditor()
{
  if (this->VolumeAppearanceEditor == NULL)
    {
    vtkErrorMacro("Expecting a volume appearance editor");
    return;
    }
  
  this->Script("pack forget [pack slaves %s]",
          this->GetPVRenderView()->GetPropertiesParent()->GetWidgetName());
  this->Script("pack %s -side top -fill both -expand t",
          this->VolumeAppearanceEditor->GetWidgetName());
  
  vtkPVSource* source = this->GetPVSource();

  if (!source)
    {
    return;
    }

  const char* arrayname = source->GetDisplayProxy()->cmGetScalarArray();
  int colorField = source->GetDisplayProxy()->cmGetScalarMode();
  
  if (arrayname)
    {
    vtkPVDataInformation* dataInfo = source->GetDataInformation();
    vtkPVArrayInformation *arrayInfo;
    vtkPVDataSetAttributesInformation *attrInfo;
    
    if (colorField == vtkSMDisplayProxy::POINT_FIELD_DATA)
      {
      attrInfo = dataInfo->GetPointDataInformation();
      }
    else
      {
      attrInfo = dataInfo->GetCellDataInformation();
      }
    arrayInfo = attrInfo->GetArrayInformation(arrayname);
    this->VolumeAppearanceEditor->SetPVSourceAndArrayInfo( source, arrayInfo );
    }
  else
    {
    this->VolumeAppearanceEditor->SetPVSourceAndArrayInfo( NULL, NULL );
    }                                              
}


//----------------------------------------------------------------------------
void vtkPVDisplayGUI::Update()
{
  if (this->PVSource == 0 || this->PVSource->GetDisplayProxy() == 0)
    {
    this->SetEnabled(0);
    this->UpdateEnableState();
    return;
    }
  this->SetEnabled(1);
  this->UpdateEnableState();

  // This call makes sure the information is up to date.
  // If not, it gathers information and updates the properties (internal).
  this->GetPVSource()->GetDataInformation();
  this->UpdateInternal();
}

//----------------------------------------------------------------------------
void vtkPVDisplayGUI::UpdateInternal()
{
  vtkPVSource* source = this->GetPVSource();
  vtkSMDisplayProxy* pDisp = source->GetDisplayProxy();
  
  // First reset all the values of the widgets.
  // Active states, and menu items will ge generated later.  
  // This could be done with a mechanism similar to reset
  // of parameters page.  
  // Parameters pages could just use/share the prototype UI instead of cloning.
  
  //law int fixmeEventually; // Use proper SM properties with reset.
  
  // Visibility check
  this->VisibilityCheck->SetState(this->PVSource->GetVisibility());
  // Cube axis visibility
  this->UpdateCubeAxesVisibilityCheck();
  // Point label visibility
  this->UpdatePointLabelVisibilityCheck();
  // Colors
  this->UpdateColorGUI();
    
  // Representation menu.
  switch(pDisp->cmGetRepresentation())
    {
  case vtkSMDisplayProxy::OUTLINE:
    this->RepresentationMenu->SetValue(VTK_PV_OUTLINE_LABEL);
    break;
  case vtkSMDisplayProxy::SURFACE:
    this->RepresentationMenu->SetValue(VTK_PV_SURFACE_LABEL);
    break;
  case vtkSMDisplayProxy::WIREFRAME:
    this->RepresentationMenu->SetValue(VTK_PV_WIREFRAME_LABEL);
    break;
  case vtkSMDisplayProxy::POINTS:
    this->RepresentationMenu->SetValue(VTK_PV_POINTS_LABEL);
    break;
  case vtkSMDisplayProxy::VOLUME:
    this->RepresentationMenu->SetValue(VTK_PV_VOLUME_LABEL);
    break;
  default:
    vtkErrorMacro("Unknown representation.");
    }

  // Interpolation menu.
  switch (pDisp->cmGetInterpolation())
    {
  case vtkSMDisplayProxy::FLAT:
    this->InterpolationMenu->SetValue("Flat");
    break;
  case vtkSMDisplayProxy::GOURAND:
    this->InterpolationMenu->SetValue("Gouraud");
    break;
  default:
    vtkErrorMacro("Unknown representation.");
    }
  this->PointSizeThumbWheel->SetValue(pDisp->cmGetPointSize());
  this->PointSizeThumbWheel->SetValue(pDisp->cmGetLineWidth());
  this->OpacityScale->SetValue(pDisp->cmGetOpacity());

  // Update actor control resolutions
  this->UpdateActorControl();
  this->UpdateActorControlResolutions();

  this->UpdateVolumeGUI();
}


//----------------------------------------------------------------------------
void vtkPVDisplayGUI::UpdateVisibilityCheck()
{
  int v = 0;
  if (this->PVSource)
    {
    v = this->PVSource->GetVisibility();
    }
  if (this->VisibilityCheck->GetApplication())
    {
    this->VisibilityCheck->SetState(v);
    }
}

//----------------------------------------------------------------------------
void vtkPVDisplayGUI::UpdateCubeAxesVisibilityCheck()
{
  if (this->PVSource && this->VisibilityCheck->GetApplication())
    {
    this->CubeAxesCheck->SetState(this->PVSource->GetCubeAxesVisibility());
    }
}

//----------------------------------------------------------------------------
void vtkPVDisplayGUI::UpdatePointLabelVisibilityCheck()
{
  if (this->PVSource && this->VisibilityCheck->GetApplication())
    {
    this->PointLabelCheck->SetState(this->PVSource->GetPointLabelVisibility());
    }
}

//----------------------------------------------------------------------------
void vtkPVDisplayGUI::UpdateColorGUI()
{
  this->UpdateColorMenu();       // Computed value used in later methods.
  this->UpdateMapScalarsCheck(); // Computed value used in later methods.
  this->UpdateColorButton();
  this->UpdateEditColorMapButton();
  this->UpdateInterpolateColorsCheck();
  this->UpdateScalarBarVisibilityCheck();
}

//----------------------------------------------------------------------------
void vtkPVDisplayGUI::UpdateScalarBarVisibilityCheck()
{
  // Set enabled.
  if (this->PVSource->GetPVColorMap() == 0)
    {
    this->ScalarBarCheckVisible = 0;
    }
  else if (this->MapScalarsCheckVisible && 
    !this->PVSource->GetDisplayProxy()->cmGetColorMode())
    {
    this->ScalarBarCheckVisible = 0;
    }
  else
    {
    this->ScalarBarCheckVisible = 1;
    }

  // Set check on or off.
  if (this->ScalarBarCheckVisible)
    {
    this->ScalarBarCheck->SetState(
          this->PVSource->GetPVColorMap()->GetScalarBarVisibility());
    }
  else
    {
    this->ScalarBarCheck->SetState(0);
    }

  this->UpdateEnableState();
}


//----------------------------------------------------------------------------
void vtkPVDisplayGUI::UpdateColorMenu()
{  
  // Variables that hold the current color state.
  vtkPVDataInformation *dataInfo;
  vtkPVDataSetAttributesInformation *attrInfo;
  vtkPVArrayInformation *arrayInfo;
  vtkPVColorMap* colorMap = this->PVSource->GetPVColorMap();
  int colorField = -1;

  if (colorMap)
    {
    colorField = this->PVSource->GetDisplayProxy()->cmGetScalarMode();
    }
  dataInfo = this->PVSource->GetDataInformation();
    
  // See if the current selection still exists.
  // If not, set a new default.
  if (colorMap)
    {
    if (colorField == vtkSMDisplayProxy::POINT_FIELD_DATA)
      {
      attrInfo = dataInfo->GetPointDataInformation();
      }
    else
      {
      attrInfo = dataInfo->GetCellDataInformation();
      }
    arrayInfo = attrInfo->GetArrayInformation(colorMap->GetArrayName());
    if (arrayInfo == 0)
      {
      this->PVSource->SetDefaultColorParameters();
      colorMap = this->PVSource->GetPVColorMap();
      if (colorMap)
        {
        colorField = this->PVSource->GetDisplayProxy()->cmGetScalarMode();
        }
      else
        {
        colorField = -1;
        }
      }
    }
      
  // Populate menus
  this->ColorSelectionMenu->DeleteAllEntries();
  this->ColorSelectionMenu->AddEntryWithCommand("Property", 
    this, "ColorByProperty");
  this->ColorSelectionMenu->SetPVSource(this->PVSource);

  this->ColorSelectionMenu->Update(0);
  if (colorMap)
    {
    // Verify that the old colorby array has not disappeared.
    attrInfo = (colorField == vtkSMDisplayProxy::POINT_FIELD_DATA)?
      dataInfo->GetPointDataInformation() : dataInfo->GetCellDataInformation();
    arrayInfo = attrInfo->GetArrayInformation(colorMap->GetArrayName());
    if (!attrInfo)
      {
      vtkErrorMacro("Could not find previous color setting.");
      this->ColorSelectionMenu->SetValue("Property");
      }
    else
      {
      this->ColorSelectionMenu->SetValue(colorMap->GetArrayName(),
        colorField);
      }
    }
  else
    {
    this->ColorSelectionMenu->SetValue("Property");
    }
}

//----------------------------------------------------------------------------
void vtkPVDisplayGUI::UpdateColorButton()
{
  double rgb[3];
  this->PVSource->GetDisplayProxy()->cmGetColor(rgb);
  this->ColorButton->SetColor(rgb[0], rgb[1], rgb[2]);
  
  // We could look at the color menu's value too.
  this->ColorButtonVisible = 1;
  if (this->PVSource && this->PVSource->GetPVColorMap())
    {
    this->ColorButtonVisible = 0;
    }
  this->UpdateEnableState();
}  

//----------------------------------------------------------------------------
void vtkPVDisplayGUI::UpdateEditColorMapButton()
{
  if (this->PVSource->GetPVColorMap() == 0)
    {
    this->EditColorMapButtonVisible = 0;
    }
  else if (this->MapScalarsCheckVisible && 
    !this->PVSource->GetDisplayProxy()->cmGetColorMode())
    {
    this->EditColorMapButtonVisible = 0;
    }
  else
    {
    this->EditColorMapButtonVisible = 1;
    }
  this->UpdateEnableState();
}  

//----------------------------------------------------------------------------
void vtkPVDisplayGUI::UpdateInterpolateColorsCheck()
{
  if (this->PVSource->GetPVColorMap() == 0 ||
    (!this->PVSource->GetDisplayProxy()->cmGetInterpolateScalarsBeforeMapping() && 
     this->MapScalarsCheckVisible) ||
    this->PVSource->GetDisplayProxy()->cmGetScalarMode() 
    == vtkDataSet::CELL_DATA_FIELD)
    {
    this->InterpolateColorsCheckVisible = 0;
    this->InterpolateColorsCheck->SetState(0);
    }
  else
    {
    this->InterpolateColorsCheckVisible = 1;
    this->InterpolateColorsCheck->SetState(
      !this->PVSource->GetDisplayProxy()->cmGetInterpolateScalarsBeforeMapping());
    }
  this->UpdateEnableState();
}

//-----------------------------------------------------------------------------
void vtkPVDisplayGUI::UpdateVolumeGUI()
{
  vtkSMDisplayProxy* pDisp = this->PVSource->GetDisplayProxy();

  // Determine if this is unstructured grid data and add the 
  // volume rendering option
  if ( this->RepresentationMenu->HasEntry( VTK_PV_VOLUME_LABEL ) )
    {
      this->RepresentationMenu->DeleteEntry( VTK_PV_VOLUME_LABEL );
    }
  
  if (!vtkSMSimpleDisplayProxy::SafeDownCast(pDisp)->GetHasVolumePipeline())
    {
    return;
    }
  this->RepresentationMenu->AddEntryWithCommand(VTK_PV_VOLUME_LABEL, this,
    "DrawVolume");

  // Update the transfer functions    
  // I wonder if this needs to be done here
/*
  vtkSMProperty* p = pDisp->GetProperty("ResetTransferFunctions");
  if (!p)
    {
    vtkErrorMacro("Failed to find property ResetTransferFunctions on DisplayProxy.");
    return;
    }
  p->Modified();
*/
  this->VolumeRenderMode = 
    (pDisp->cmGetRepresentation() == vtkSMDisplayProxy::VOLUME)? 1 : 0;
  this->VolumeScalarSelectionWidget->SetPVSource(this->PVSource);
  this->VolumeScalarSelectionWidget->SetColorSelectionCommand(
    "VolumeRenderByArray");
  this->VolumeScalarSelectionWidget->Update();
}
//----------------------------------------------------------------------------
void vtkPVDisplayGUI::SetActorColor(double r, double g, double b)
{
  this->ActorColor[0] = r;
  this->ActorColor[1] = g;
  this->ActorColor[2] = b;
  this->PVSource->GetDisplayProxy()->cmSetColor(this->ActorColor);
}  

//----------------------------------------------------------------------------
void vtkPVDisplayGUI::ChangeActorColor(double r, double g, double b)
{
  this->AddTraceEntry("$kw(%s) ChangeActorColor %f %f %f",
                      this->GetTclName(), r, g, b);

  this->SetActorColor(r, g, b);
  this->ColorButton->SetColor(r, g, b);

  if ( this->GetPVRenderView() )
    {
    this->GetPVRenderView()->EventuallyRender();
    }
  
  if (strcmp(this->ColorSelectionMenu->GetValue(), "Property") == 0)
    {
    this->ColorSetByUser = 1;
    }
}

//----------------------------------------------------------------------------
void vtkPVDisplayGUI::ColorByArray(const char* array, int field)
{
  this->AddTraceEntry("$kw(%s) ColorByArray %s %d", this->GetTclName(),
    array, field);
  
  this->PVSource->ColorByArray(array, field);
  this->ColorSelectionMenu->SetValue(array, field);
  
  this->UpdateColorGUI(); // why?

  if ( this->GetPVRenderView() )
    {
    this->GetPVRenderView()->EventuallyRender();
    }
}

//----------------------------------------------------------------------------
void vtkPVDisplayGUI::ColorByProperty()
{
  this->ColorSetByUser = 1;
  this->AddTraceEntry("$kw(%s) ColorByProperty", this->GetTclName());
  this->ColorSelectionMenu->SetValue("Property");
  this->ColorByPropertyInternal();
}

//----------------------------------------------------------------------------
void vtkPVDisplayGUI::ColorByPropertyInternal()
{
  this->PVSource->GetDisplayProxy()->cmSetScalarVisibility(0);
  double *color = this->ColorButton->GetColor();
  this->SetActorColor(color[0], color[1], color[2]);

  this->PVSource->SetPVColorMap(0);

  this->UpdateColorGUI();
  if (this->GetPVRenderView())
    {
    this->GetPVRenderView()->EventuallyRender();
    }
}


//----------------------------------------------------------------------------
void vtkPVDisplayGUI::VolumeRenderByArray(const char* name, int field)
{
  this->AddTraceEntry("$kw(%s) VolumeRenderByArray %s %d",
    this->GetTclName(), name, field);
  this->VolumeScalarSelectionWidget->SetValue(name , field);
  this->PVSource->VolumeRenderByArray(name, field);

  this->PVSource->ColorByArray(name, field); // So the LOD Mapper remains 
        //synchronized with the Volume mapper.
        //Note that this call also invalidates the "name" pointer.
  if (this->GetPVRenderView())
    {
    this->GetPVRenderView()->EventuallyRender();
    } 
}

//----------------------------------------------------------------------------
void vtkPVDisplayGUI::UpdateMapScalarsCheck()
{
  vtkPVColorMap* colorMap = this->PVSource->GetPVColorMap();

  this->MapScalarsCheckVisible = 0;  
  this->MapScalarsCheck->SetState(0);  
  if (colorMap)
    {
    this->MapScalarsCheck->SetState(1);
    // See if the array satisfies conditions necessary for direct coloring.  
    vtkPVDataInformation* dataInfo = this->PVSource->GetDataInformation();
    vtkPVDataSetAttributesInformation* attrInfo;
    if (this->PVSource->GetDisplayProxy()->cmGetScalarMode() == vtkSMDisplayProxy::POINT_FIELD_DATA)
      {
      attrInfo = dataInfo->GetPointDataInformation();
      }
    else
      {
      attrInfo = dataInfo->GetCellDataInformation();
      }
    vtkPVArrayInformation* arrayInfo = attrInfo->GetArrayInformation(colorMap->GetArrayName());      
    // First set of conditions.
    if (arrayInfo && arrayInfo->GetDataType() == VTK_UNSIGNED_CHAR)
      {
      // Number of component restriction.
      if (arrayInfo->GetNumberOfComponents() == 3)
        { // I would like to have two as an option also ...
        // One component causes more trouble than it is worth.
        this->MapScalarsCheckVisible = 1;
        this->MapScalarsCheck->SetState(this->PVSource->GetDisplayProxy()->cmGetColorMode());
        }
      else
        { // Keep VTK from directly coloring single component arrays.
        this->PVSource->GetDisplayProxy()->cmSetColorMode(1);
        }
      }
    }
    this->UpdateEnableState();    
}

//----------------------------------------------------------------------------
void vtkPVDisplayGUI::SetRepresentation(const char* repr)
{
  if (!repr)
    {
    return;
    }
  if (!strcmp(repr, VTK_PV_WIREFRAME_LABEL) )
    {
    this->DrawWireframe();
    }
  else if (!strcmp(repr, VTK_PV_SURFACE_LABEL) )
    {
    this->DrawSurface();
    }
  else if (!strcmp(repr, VTK_PV_POINTS_LABEL) )
    {
    this->DrawPoints();
    }
  else if (!strcmp(repr, VTK_PV_OUTLINE_LABEL) )
    {
    this->DrawOutline();
    }
  else if (!strcmp(repr, VTK_PV_VOLUME_LABEL) )
    {
    this->DrawVolume();
    }
  else
    {
    vtkErrorMacro("Don't know the representation: " << repr);
    this->DrawSurface();
    }
}

//----------------------------------------------------------------------------
void vtkPVDisplayGUI::DrawWireframe()
{
  if (this->GetPVSource()->GetInitialized())
    {
    this->AddTraceEntry("$kw(%s) DrawWireframe", this->GetTclName());
    }
  this->RepresentationMenu->SetValue(VTK_PV_WIREFRAME_LABEL);
  this->VolumeRenderModeOff();
  this->PVSource->GetDisplayProxy()->cmSetRepresentation(vtkSMDisplayProxy::WIREFRAME);

  if ( this->GetPVRenderView() )
    {
    this->GetPVRenderView()->EventuallyRender();
    }
}

//----------------------------------------------------------------------------
void vtkPVDisplayGUI::DrawPoints()
{
  if (this->GetPVSource()->GetInitialized())
    {
    this->AddTraceEntry("$kw(%s) DrawPoints", this->GetTclName());
    }
  this->RepresentationMenu->SetValue(VTK_PV_POINTS_LABEL);
  this->VolumeRenderModeOff();
  this->PVSource->GetDisplayProxy()->cmSetRepresentation(vtkSMDisplayProxy::POINTS);
  
  if ( this->GetPVRenderView() )
    {
    this->GetPVRenderView()->EventuallyRender();
    }
}

//----------------------------------------------------------------------------
void vtkPVDisplayGUI::DrawVolume()
{
  if (this->PVSource->GetDataInformation()->GetNumberOfCells() > 500000)
    {
    vtkWarningMacro("Sorry.  Unstructured grids with more than 500,000 "
                    "cells cannot currently be rendered with ParaView.  "
                    "Consider thresholding cells you are not interested in.");
    return;
    }
  if (this->GetPVSource()->GetInitialized())
    {
    this->AddTraceEntry("$kw(%s) DrawVolume", this->GetTclName());
    }
  this->RepresentationMenu->SetValue(VTK_PV_VOLUME_LABEL);
  this->VolumeRenderModeOn();
  this->PVSource->GetDisplayProxy()->cmSetRepresentation(vtkSMDisplayProxy::VOLUME);

  if ( this->GetPVRenderView() )
    {
    this->GetPVRenderView()->EventuallyRender();
    }
}

//----------------------------------------------------------------------------
void vtkPVDisplayGUI::DrawSurface()
{
  if (this->GetPVSource()->GetInitialized())
    {
    this->AddTraceEntry("$kw(%s) DrawSurface", this->GetTclName());
    }
  this->RepresentationMenu->SetValue(VTK_PV_SURFACE_LABEL);
  this->VolumeRenderModeOff();
  
  // fixme
  // It would be better to loop over part displays from the render module.
  this->PVSource->GetDisplayProxy()->cmSetRepresentation(vtkSMDisplayProxy::SURFACE);

  if ( this->GetPVRenderView() )
    {
    this->GetPVRenderView()->EventuallyRender();
    }
}

//----------------------------------------------------------------------------
void vtkPVDisplayGUI::DrawOutline()
{
  if (this->GetPVSource()->GetInitialized())
    {
    this->AddTraceEntry("$kw(%s) DrawOutline", this->GetTclName());
    }
  this->RepresentationMenu->SetValue(VTK_PV_OUTLINE_LABEL);
  this->VolumeRenderModeOff();
  this->PVSource->GetDisplayProxy()->cmSetRepresentation(vtkSMDisplayProxy::OUTLINE);

  if ( this->GetPVRenderView() )
    {
    this->GetPVRenderView()->EventuallyRender();
    }
}


//----------------------------------------------------------------------------
// Change visibility / enabled state of widgets for actor properties
//
void vtkPVDisplayGUI::VolumeRenderModeOff()
{
  this->Script("pack forget %s", 
               this->VolumeAppearanceFrame->GetWidgetName() );
  this->Script("pack forget %s", 
               this->ColorFrame->GetWidgetName() );

  this->Script("pack %s -after %s -fill x -expand t -pady 2", 
               this->ColorFrame->GetWidgetName(),
               this->ViewFrame->GetWidgetName() );

  // Make the color selection the same as the scalars we were just volume
  // rendering.
  if (this->VolumeRenderMode)
    {
    vtkSMDisplayProxy* pDisp = this->PVSource->GetDisplayProxy();
    vtkSMStringVectorProperty* svp = vtkSMStringVectorProperty::SafeDownCast(
      pDisp->GetProperty("SelectScalarArray"));
    if (svp)
      {
      this->ColorByArray(svp->GetElement(0), pDisp->cmGetScalarMode());
      }
    else
      {
      vtkErrorMacro("Failed to find property ScalarMode on DisplayProxy.");
      }
    }
  this->VolumeRenderMode = 0;
  this->UpdateEnableState();
}

//----------------------------------------------------------------------------
// Change visibility / enabled state of widgets for volume properties
//
void vtkPVDisplayGUI::VolumeRenderModeOn()
{
  this->Script("pack forget %s", 
               this->VolumeAppearanceFrame->GetWidgetName() );
  this->Script("pack forget %s", 
               this->ColorFrame->GetWidgetName() );
  
  this->Script("pack %s -after %s -fill x -expand t -pady 2", 
               this->VolumeAppearanceFrame->GetWidgetName(),
               this->ViewFrame->GetWidgetName() );

  // Make scalar selection be the same for colors.
  if (!this->VolumeRenderMode)
    {
    const char *colorSelection = this->ColorSelectionMenu->GetValue();
    if (strcmp(colorSelection, "Property") != 0)
      {
      vtkSMDisplayProxy* pDisp = this->PVSource->GetDisplayProxy();
      vtkSMStringVectorProperty* svp = vtkSMStringVectorProperty::SafeDownCast(
        pDisp->GetProperty("ColorArray"));
      if (svp)
        {
        this->VolumeRenderByArray(svp->GetElement(0), pDisp->cmGetScalarMode());
        }
      else
        {
        vtkErrorMacro("Failed to find property ScalarMode on DisplayProxy.");
        }
      }
    }
  
  this->VolumeRenderMode = 1;
  this->UpdateEnableState();
}

  
//----------------------------------------------------------------------------
void vtkPVDisplayGUI::SetInterpolation(const char* repr)
{
  if (!repr)
    {
    return;
    }
  if (!strcmp(repr, "Flat") )
    {
    this->SetInterpolationToFlat();
    }
  else if (!strcmp(repr, "Gouraud") )
    {
    this->SetInterpolationToGouraud();
    }
  else
    {
    vtkErrorMacro("Don't know the interpolation: " << repr);
    this->DrawSurface();
    }
}

//----------------------------------------------------------------------------
void vtkPVDisplayGUI::SetInterpolationToFlat()
{
  this->AddTraceEntry("$kw(%s) SetInterpolationToFlat", 
                      this->GetTclName());
  this->InterpolationMenu->SetValue("Flat");
  this->PVSource->GetDisplayProxy()->cmSetInterpolation(vtkSMDisplayProxy::FLAT);

  if ( this->GetPVRenderView() )
    {
    this->GetPVRenderView()->EventuallyRender();
    }
}


//----------------------------------------------------------------------------
void vtkPVDisplayGUI::SetInterpolationToGouraud()
{
  this->AddTraceEntry("$kw(%s) SetInterpolationToGouraud", 
                      this->GetTclName());
  this->InterpolationMenu->SetValue("Gouraud");

  this->PVSource->GetDisplayProxy()->cmSetInterpolation(vtkSMDisplayProxy::GOURAND);
  
  if ( this->GetPVRenderView() )
    {
    this->GetPVRenderView()->EventuallyRender();
    }
}

//----------------------------------------------------------------------------
// Set up the default UI values.  Data information is valid by this time.
void vtkPVDisplayGUI::Initialize()
{
  if (this->PVSource == 0)
    {
    return;
    }
  double bounds[6];

  vtkDebugMacro( << "Initialize --------")
  
  this->GetPVSource()->GetDataInformation()->GetBounds(bounds);

  // Choose the representation based on the data.
  // Polydata is always surface.
  // Structured data is surface when 2d, outline when 3d.
  int dataSetType = this->GetPVSource()->GetDataInformation()->GetDataSetType();
  if (dataSetType == VTK_POLY_DATA)
    {
    this->SetRepresentation(VTK_PV_SURFACE_LABEL);
    }
  else if (dataSetType == VTK_STRUCTURED_GRID || 
           dataSetType == VTK_RECTILINEAR_GRID ||
           dataSetType == VTK_IMAGE_DATA)
    {
    int* ext = this->GetPVSource()->GetDataInformation()->GetExtent();
    if (ext[0] == ext[1] || ext[2] == ext[3] || ext[4] == ext[5])
      {
      this->SetRepresentation(VTK_PV_SURFACE_LABEL);
      }
    else
      {
      this->SetRepresentation(VTK_PV_OUTLINE_LABEL);
      }
    }
  else if (dataSetType == VTK_UNSTRUCTURED_GRID)
    {
    if (this->GetPVSource()->GetDataInformation()->GetNumberOfCells() 
        < this->GetPVRenderView()->GetRenderModuleUI()->GetOutlineThreshold())
      {
      this->SetRepresentation(VTK_PV_SURFACE_LABEL);
      }
    else
      {
      this->GetPVApplication()->GetMainWindow()->SetStatusText("Using outline for large unstructured grid.");
      this->SetRepresentation(VTK_PV_OUTLINE_LABEL);
      }
    }
  else
    {
    this->SetRepresentation(VTK_PV_OUTLINE_LABEL);
    this->ShouldReinitialize = 1;
    return;
    }

  this->ShouldReinitialize = 0;
}

//----------------------------------------------------------------------------
void vtkPVDisplayGUI::CenterCamera()
{
  vtkPVApplication* pvApp = this->GetPVApplication();
  vtkRenderer* ren = pvApp->GetProcessModule()->GetRenderModule()->GetRenderer();

  double bounds[6];
  this->GetPVSource()->GetDataInformation()->GetBounds(bounds);
  if (bounds[0]<=bounds[1] && bounds[2]<=bounds[3] && bounds[4]<=bounds[5])
    {
    vtkPVWindow* window = this->GetPVSource()->GetPVWindow();
    window->SetCenterOfRotation(0.5*(bounds[0]+bounds[1]), 
                                0.5*(bounds[2]+bounds[3]),
                                0.5*(bounds[4]+bounds[5]));
    window->ResetCenterCallback();

    ren->ResetCamera(bounds[0], bounds[1], bounds[2], 
                     bounds[3], bounds[4], bounds[5]);
    ren->ResetCameraClippingRange();
        
    if ( this->GetPVRenderView() )
      {
      this->GetPVRenderView()->EventuallyRender();
      }
    }
  
  this->AddTraceEntry("$kw(%s) CenterCamera", this->GetTclName());
}

//----------------------------------------------------------------------------
void vtkPVDisplayGUI::VisibilityCheckCallback()
{
  this->GetPVSource()->SetVisibility(this->VisibilityCheck->GetState());
}

//----------------------------------------------------------------------------
vtkPVRenderView* vtkPVDisplayGUI::GetPVRenderView()
{
  if ( !this->GetPVSource() )
    {
    return 0;
    }
  return this->GetPVSource()->GetPVRenderView();
}

//----------------------------------------------------------------------------
vtkPVApplication* vtkPVDisplayGUI::GetPVApplication()
{
  if (this->GetApplication() == NULL)
    {
    return NULL;
    }
  
  if (this->GetApplication()->IsA("vtkPVApplication"))
    {  
    return (vtkPVApplication*)(this->GetApplication());
    }
  else
    {
    vtkErrorMacro("Bad typecast");
    return NULL;
    } 
}

//----------------------------------------------------------------------------
void vtkPVDisplayGUI::ScalarBarCheckCallback()
{
  if (this->PVSource == 0 || this->PVSource->GetPVColorMap() == 0)
    {
    vtkErrorMacro("Cannot find the color map.");
    return;
    }
  this->PVSource->GetPVColorMap()->SetScalarBarVisibility(
          this->ScalarBarCheck->GetState());
  if ( this->GetPVRenderView() )
    {
    this->GetPVRenderView()->EventuallyRender();
    }
}

//----------------------------------------------------------------------------
void vtkPVDisplayGUI::CubeAxesCheckCallback()
{
  //law int fixme;  // Loading the trace will not trace the visibility.
  // Move the tracing into vtkPVSource.
  this->AddTraceEntry("$kw(%s) SetCubeAxesVisibility %d", 
                      this->PVSource->GetTclName(),
                      this->CubeAxesCheck->GetState());
  this->PVSource->SetCubeAxesVisibility(this->CubeAxesCheck->GetState());
  if ( this->GetPVRenderView() )
    {
    this->GetPVRenderView()->EventuallyRender();
    }
}

//----------------------------------------------------------------------------
void vtkPVDisplayGUI::PointLabelCheckCallback()
{
  //law int fixme;  // Loading the trace will not trace the visibility.
  // Move the tracing into vtkPVSource.
  this->AddTraceEntry("$kw(%s) SetPointLabelVisibility %d", 
                      this->PVSource->GetTclName(),
                      this->PointLabelCheck->GetState());
  this->PVSource->SetPointLabelVisibility(this->PointLabelCheck->GetState());
  if ( this->GetPVRenderView() )
    {
    this->GetPVRenderView()->EventuallyRender();
    }
}

//----------------------------------------------------------------------------
void vtkPVDisplayGUI::MapScalarsCheckCallback()
{
  this->SetMapScalarsFlag(this->MapScalarsCheck->GetState());
  if ( this->GetPVRenderView() )
    {
    this->GetPVRenderView()->EventuallyRender();
    }
}

//----------------------------------------------------------------------------
void vtkPVDisplayGUI::SetMapScalarsFlag(int val)
{
  this->AddTraceEntry("$kw(%s) SetMapScalarsFlag %d", this->GetTclName(), val);
  if (this->MapScalarsCheck->GetState() != val)
    {
    this->MapScalarsCheck->SetState(val);
    }

  this->UpdateEnableState();

  this->PVSource->GetDisplayProxy()->cmSetColorMode(val);
  this->UpdateColorGUI();
}

//----------------------------------------------------------------------------
void vtkPVDisplayGUI::InterpolateColorsCheckCallback()
{
  this->SetInterpolateColorsFlag(this->InterpolateColorsCheck->GetState());
  if ( this->GetPVRenderView() )
    {
    this->GetPVRenderView()->EventuallyRender();
    }
}

//----------------------------------------------------------------------------
void vtkPVDisplayGUI::SetInterpolateColorsFlag(int val)
{
  this->AddTraceEntry("$kw(%s) SetInterpolateColorsFlag %d", this->GetTclName(), val);
  if (this->InterpolateColorsCheck->GetState() != val)
    {
    this->InterpolateColorsCheck->SetState(val);
    }

  this->PVSource->GetDisplayProxy()->cmSetInterpolateScalarsBeforeMapping(!val);
}

//----------------------------------------------------------------------------
void vtkPVDisplayGUI::SetPointSize(int size)
{
  if ( this->PointSizeThumbWheel->GetValue() == size )
    {
    return;
    }
  // The following call with trigger the ChangePointSize callback (below)
  // but won't add a trace entry. Let's do it. A trace entry is also
  // added by the ChangePointSizeEndCallback but this callback is only
  // called when the interaction on the scale is stopped.
  this->PointSizeThumbWheel->SetValue(size);
  this->AddTraceEntry("$kw(%s) SetPointSize %d", this->GetTclName(),
                      (int)(this->PointSizeThumbWheel->GetValue()));
}

//----------------------------------------------------------------------------
void vtkPVDisplayGUI::ChangePointSize()
{
  this->PVSource->GetDisplayProxy()->cmSetPointSize(this->PointSizeThumbWheel->GetValue());
 
  if ( this->GetPVRenderView() )
    {
    this->GetPVRenderView()->EventuallyRender();
    }
} 

//----------------------------------------------------------------------------
void vtkPVDisplayGUI::ChangePointSizeEndCallback()
{
  this->ChangePointSize();
  this->AddTraceEntry("$kw(%s) SetPointSize %d", this->GetTclName(),
                      (int)(this->PointSizeThumbWheel->GetValue()));
} 

//----------------------------------------------------------------------------
void vtkPVDisplayGUI::SetLineWidth(int width)
{
  if ( this->LineWidthThumbWheel->GetValue() == width )
    {
    return;
    }
  // The following call with trigger the ChangeLineWidth callback (below)
  // but won't add a trace entry. Let's do it. A trace entry is also
  // added by the ChangeLineWidthEndCallback but this callback is only
  // called when the interaction on the scale is stopped.
  this->LineWidthThumbWheel->SetValue(width);
  this->AddTraceEntry("$kw(%s) SetLineWidth %d", this->GetTclName(),
                      (int)(this->LineWidthThumbWheel->GetValue()));
}

//----------------------------------------------------------------------------
void vtkPVDisplayGUI::ChangeLineWidth()
{
  this->PVSource->GetDisplayProxy()->cmSetLineWidth(this->LineWidthThumbWheel->GetValue());

  if ( this->GetPVRenderView() )
    {
    this->GetPVRenderView()->EventuallyRender();
    }
}

//----------------------------------------------------------------------------
void vtkPVDisplayGUI::ChangeLineWidthEndCallback()
{
  this->ChangeLineWidth();
  this->AddTraceEntry("$kw(%s) SetLineWidth %d", this->GetTclName(),
                      (int)(this->LineWidthThumbWheel->GetValue()));
}

//----------------------------------------------------------------------------
void vtkPVDisplayGUI::SetVolumeOpacityUnitDistance( double d )
{
  vtkSMDisplayProxy* pDisp = this->PVSource->GetDisplayProxy();
  vtkSMDoubleVectorProperty* dvp = vtkSMDoubleVectorProperty::SafeDownCast(
    pDisp->GetProperty("ScalarOpacityUnitDistance"));
  if (!dvp)
    {
    vtkErrorMacro("Failed to find property ScalarOpacityUnitDistance on DisplayProxy.");
    return;
    }
  dvp->SetElement(0, d);
  pDisp->UpdateVTKObjects();
}

//----------------------------------------------------------------------------
void vtkPVDisplayGUI::ClearVolumeOpacity()
{
#if !defined(PARAVIEW_USE_SERVERMANAGER_RENDERING)
  vtkPVApplication *pvApp = this->GetPVApplication();
  vtkPVProcessModule* pm = pvApp->GetProcessModule();
  vtkClientServerStream stream;

  vtkSMProxy *volume = this->PVSource->GetPartDisplay()->GetVolumeOpacityProxy();
  stream 
    << vtkClientServerStream::Invoke 
    << volume->GetID(0) << "RemoveAllPoints" 
    << vtkClientServerStream::End;
  pm->SendStream(
    vtkProcessModule::CLIENT|vtkProcessModule::RENDER_SERVER, stream);
#endif
}

//----------------------------------------------------------------------------
void vtkPVDisplayGUI::AddVolumeOpacity( double scalar, double opacity )
{
#if !defined(PARAVIEW_USE_SERVERMANAGER_RENDERING)
  vtkPVApplication *pvApp = this->GetPVApplication();
  vtkPVProcessModule* pm = pvApp->GetProcessModule();
  vtkClientServerStream stream;

  vtkSMProxy *volume = this->PVSource->GetPartDisplay()->GetVolumeOpacityProxy();
  stream 
    << vtkClientServerStream::Invoke 
    << volume->GetID(0) << "AddPoint" << scalar << opacity 
    << vtkClientServerStream::End;
  pm->SendStream(
    vtkProcessModule::CLIENT|vtkProcessModule::RENDER_SERVER, stream);
#endif
}

//----------------------------------------------------------------------------
void vtkPVDisplayGUI::ClearVolumeColor()
{
#if !defined(PARAVIEW_USE_SERVERMANAGER_RENDERING)
  vtkPVApplication *pvApp = this->GetPVApplication();
  vtkPVProcessModule* pm = pvApp->GetProcessModule();
  vtkClientServerStream stream;

  vtkSMProxy *volume = this->PVSource->GetPartDisplay()->GetVolumeColorProxy();
  stream 
    << vtkClientServerStream::Invoke 
    << volume->GetID(0) << "RemoveAllPoints" 
    << vtkClientServerStream::End;
  pm->SendStream(
    vtkProcessModule::CLIENT|vtkProcessModule::RENDER_SERVER, stream);
#endif
}

//----------------------------------------------------------------------------
void vtkPVDisplayGUI::AddVolumeColor( double scalar, double r, double g, double b )
{
#if !defined(PARAVIEW_USE_SERVERMANAGER_RENDERING)
  vtkPVApplication *pvApp = this->GetPVApplication();
  vtkPVProcessModule* pm = pvApp->GetProcessModule();
  vtkClientServerStream stream;

  vtkSMProxy *volume = this->PVSource->GetPartDisplay()->GetVolumeColorProxy();
  stream 
    << vtkClientServerStream::Invoke 
    << volume->GetID(0) << "AddRGBPoint" << scalar << r << g << b 
    << vtkClientServerStream::End;
  pm->SendStream(
    vtkProcessModule::CLIENT|vtkProcessModule::RENDER_SERVER, stream);
#endif
}

//----------------------------------------------------------------------------
void vtkPVDisplayGUI::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);
  os << indent << "ColorSelectionMenu: " <<
    this->ColorSelectionMenu << endl;
  os << indent << "VolumeScalarSelectionWidget: " << 
    this->VolumeScalarSelectionWidget << endl;
  os << indent << "ResetCameraButton: " << this->ResetCameraButton << endl;
  os << indent << "EditColorMapButton: " << this->EditColorMapButton << endl;
  os << indent << "PVSource: " << this->GetPVSource() << endl;
  os << indent << "CubeAxesCheck: " << this->CubeAxesCheck << endl;
  os << indent << "PointLabelCheck: " << this->PointLabelCheck << endl;
  os << indent << "ScalarBarCheck: " << this->ScalarBarCheck << endl;
  os << indent << "RepresentationMenu: " << this->RepresentationMenu << endl;
  os << indent << "InterpolationMenu: " << this->InterpolationMenu << endl;
  os << indent << "ActorControlFrame: " << this->ActorControlFrame << endl;
  os << indent << "ArraySetByUser: " << this->ArraySetByUser << endl;
  os << indent << "ActorColor: " << this->ActorColor[0] << ", " << this->ActorColor[1]
               << ", " << this->ActorColor[2] << endl;
  os << indent << "ColorSetByUser: " << this->ColorSetByUser << endl;
  os << indent << "ShouldReinitialize: " << this->ShouldReinitialize << endl;
}

//----------------------------------------------------------------------------
void vtkPVDisplayGUI::SetOpacity(float val)
{ 
  this->OpacityScale->SetValue(val);
}

//----------------------------------------------------------------------------
void vtkPVDisplayGUI::OpacityChangedCallback()
{
#if defined(PARAVIEW_USE_SERVERMANAGER_RENDERING)
  this->PVSource->GetDisplayProxy()->cmSetOpacity(this->OpacityScale->GetValue());
#else
  this->PVSource->GetPartDisplay()->SetOpacity(this->OpacityScale->GetValue());
#endif

  if ( this->GetPVRenderView() )
    {
    this->GetPVRenderView()->EventuallyRender();
    }
}

//----------------------------------------------------------------------------
void vtkPVDisplayGUI::OpacityChangedEndCallback()
{
  this->OpacityChangedCallback();
  this->AddTraceEntry("$kw(%s) SetOpacity %f", 
                      this->GetTclName(), this->OpacityScale->GetValue());
}

//----------------------------------------------------------------------------
void vtkPVDisplayGUI::GetActorTranslate(double* point)
{
#if defined(PARAVIEW_USE_SERVERMANAGER_RENDERING)
  vtkSMDisplayProxy* pDisp = this->PVSource->GetDisplayProxy();
  if (pDisp)
    {
    pDisp->cmGetPosition(point);
    }
#else
  vtkSMPartDisplay* pDisp = this->PVSource->GetPartDisplay();
  if (pDisp)
    {
    pDisp->GetTranslate(point);
    }
#endif
  else
    {
    point[0] = this->TranslateThumbWheel[0]->GetValue();
    point[1] = this->TranslateThumbWheel[1]->GetValue();
    point[2] = this->TranslateThumbWheel[2]->GetValue();
    }
}

//----------------------------------------------------------------------------
void vtkPVDisplayGUI::SetActorTranslateNoTrace(double x, double y, double z)
{
  this->TranslateThumbWheel[0]->SetValue(x);
  this->TranslateThumbWheel[1]->SetValue(y);
  this->TranslateThumbWheel[2]->SetValue(z);
#if defined(PARAVIEW_USE_SERVERMANAGER_RENDERING)
  double pos[3];
  pos[0] = x; pos[1] = y; pos[2] = z;
  this->PVSource->GetDisplayProxy()->cmSetPosition(pos);
#else
  this->PVSource->GetPartDisplay()->SetTranslate(x, y, z);
#endif
  // Do not render here (do it in the callback, since it could be either
  // Render or EventuallyRender depending on the interaction)
}

//----------------------------------------------------------------------------
void vtkPVDisplayGUI::SetActorTranslate(double x, double y, double z)
{
  this->SetActorTranslateNoTrace(x, y, z);
  if ( this->GetPVRenderView() )
    {
    this->GetPVRenderView()->EventuallyRender();
    }

  this->AddTraceEntry("$kw(%s) SetActorTranslate %f %f %f",
                      this->GetTclName(), x, y, z);  
}

//----------------------------------------------------------------------------
void vtkPVDisplayGUI::SetActorTranslate(double* point)
{
  this->SetActorTranslate(point[0], point[1], point[2]);
}

//----------------------------------------------------------------------------
void vtkPVDisplayGUI::ActorTranslateCallback()
{
  double point[3];
  point[0] = this->TranslateThumbWheel[0]->GetValue();
  point[1] = this->TranslateThumbWheel[1]->GetValue();
  point[2] = this->TranslateThumbWheel[2]->GetValue();
  this->SetActorTranslateNoTrace(point[0], point[1], point[2]);
  if ( this->GetPVRenderView() )
    {
    this->GetPVRenderView()->Render();
    }
}

//----------------------------------------------------------------------------
void vtkPVDisplayGUI::ActorTranslateEndCallback()
{
  double point[3];
  point[0] = this->TranslateThumbWheel[0]->GetValue();
  point[1] = this->TranslateThumbWheel[1]->GetValue();
  point[2] = this->TranslateThumbWheel[2]->GetValue();
  this->SetActorTranslate(point);
}

//----------------------------------------------------------------------------
void vtkPVDisplayGUI::GetActorScale(double* point)
{
#if defined(PARAVIEW_USE_SERVERMANAGER_RENDERING)
  vtkSMDisplayProxy* pDisp = this->PVSource->GetDisplayProxy();
#else
  vtkSMPartDisplay* pDisp = this->PVSource->GetPartDisplay();
#endif
  if (pDisp)
    {
    pDisp->cmGetScale(point);
    }
  else
    {
    point[0] = this->ScaleThumbWheel[0]->GetValue();
    point[1] = this->ScaleThumbWheel[1]->GetValue();
    point[2] = this->ScaleThumbWheel[2]->GetValue();
    }
}

//----------------------------------------------------------------------------
void vtkPVDisplayGUI::SetActorScaleNoTrace(double x, double y, double z)
{
  this->ScaleThumbWheel[0]->SetValue(x);
  this->ScaleThumbWheel[1]->SetValue(y);
  this->ScaleThumbWheel[2]->SetValue(z);
#if defined(PARAVIEW_USE_SERVERMANAGER_RENDERING)
  double scale[3];
  scale[0] = x; scale[1] = y; scale[2] = z;
  this->PVSource->GetDisplayProxy()->cmSetScale(scale);
#else
  this->PVSource->GetPartDisplay()->SetScale(x, y, z);
#endif

  // Do not render here (do it in the callback, since it could be either
  // Render or EventuallyRender depending on the interaction)
}

//----------------------------------------------------------------------------
void vtkPVDisplayGUI::SetActorScale(double x, double y, double z)
{
  this->SetActorScaleNoTrace(x, y, z);
  if ( this->GetPVRenderView() )
    {
    this->GetPVRenderView()->EventuallyRender();
    }

  this->AddTraceEntry("$kw(%s) SetActorScale %f %f %f",
                      this->GetTclName(), x, y, z);  
}

//----------------------------------------------------------------------------
void vtkPVDisplayGUI::SetActorScale(double* point)
{
  this->SetActorScale(point[0], point[1], point[2]);
}

//----------------------------------------------------------------------------
void vtkPVDisplayGUI::ActorScaleCallback()
{
  double point[3];
  point[0] = this->ScaleThumbWheel[0]->GetValue();
  point[1] = this->ScaleThumbWheel[1]->GetValue();
  point[2] = this->ScaleThumbWheel[2]->GetValue();
  this->SetActorScaleNoTrace(point[0], point[1], point[2]);
  if ( this->GetPVRenderView() )
    {
    this->GetPVRenderView()->Render();
    }
}

//----------------------------------------------------------------------------
void vtkPVDisplayGUI::ActorScaleEndCallback()
{
  double point[3];
  point[0] = this->ScaleThumbWheel[0]->GetValue();
  point[1] = this->ScaleThumbWheel[1]->GetValue();
  point[2] = this->ScaleThumbWheel[2]->GetValue();
  this->SetActorScale(point);
}

//----------------------------------------------------------------------------
void vtkPVDisplayGUI::GetActorOrientation(double* point)
{
#if defined(PARAVIEW_USE_SERVERMANAGER_RENDERING)
  vtkSMDisplayProxy* pDisp = this->PVSource->GetDisplayProxy();
#else
  vtkSMPartDisplay* pDisp = this->PVSource->GetPartDisplay();
#endif
  if (pDisp)
    {
    pDisp->cmGetOrientation(point);
    }
  else
    {
    point[0] = this->OrientationScale[0]->GetValue();
    point[1] = this->OrientationScale[1]->GetValue();
    point[2] = this->OrientationScale[2]->GetValue();
    }
}

//----------------------------------------------------------------------------
void vtkPVDisplayGUI::SetActorOrientationNoTrace(double x, double y, double z)
{
  this->OrientationScale[0]->SetValue(x);
  this->OrientationScale[1]->SetValue(y);
  this->OrientationScale[2]->SetValue(z);
#if defined(PARAVIEW_USE_SERVERMANAGER_RENDERING)
  double orient[3];
  orient[0] = x; orient[1] = y; orient[2] = z;
  this->PVSource->GetDisplayProxy()->cmSetOrientation(orient);
#else
  this->PVSource->GetPartDisplay()->SetOrientation(x, y, z);
#endif

  // Do not render here (do it in the callback, since it could be either
  // Render or EventuallyRender depending on the interaction)
}

//----------------------------------------------------------------------------
void vtkPVDisplayGUI::SetActorOrientation(double x, double y, double z)
{
  this->SetActorOrientationNoTrace(x, y, z);
  if ( this->GetPVRenderView() )
    {
    this->GetPVRenderView()->EventuallyRender();
    }

  this->AddTraceEntry("$kw(%s) SetActorOrientation %f %f %f",
                      this->GetTclName(), x, y, z);  
}

//----------------------------------------------------------------------------
void vtkPVDisplayGUI::SetActorOrientation(double* point)
{
  this->SetActorOrientation(point[0], point[1], point[2]);
}

//----------------------------------------------------------------------------
void vtkPVDisplayGUI::ActorOrientationCallback()
{
  double point[3];
  point[0] = this->OrientationScale[0]->GetValue();
  point[1] = this->OrientationScale[1]->GetValue();
  point[2] = this->OrientationScale[2]->GetValue();
  this->SetActorOrientationNoTrace(point[0], point[1], point[2]);
  if ( this->GetPVRenderView() )
    {
    this->GetPVRenderView()->Render();
    }
}

//----------------------------------------------------------------------------
void vtkPVDisplayGUI::ActorOrientationEndCallback()
{
  double point[3];
  point[0] = this->OrientationScale[0]->GetValue();
  point[1] = this->OrientationScale[1]->GetValue();
  point[2] = this->OrientationScale[2]->GetValue();
  this->SetActorOrientation(point);
}

//----------------------------------------------------------------------------
void vtkPVDisplayGUI::GetActorOrigin(double* point)
{
#if defined(PARAVIEW_USE_SERVERMANAGER_RENDERING)
  if (this->PVSource->GetDisplayProxy())
    {
    this->PVSource->GetDisplayProxy()->cmGetOrigin(point);
    }
#else
  if (this->PVSource->GetPartDisplay())
    {
    this->PVSource->GetPartDisplay()->GetOrigin(point);
    }
#endif
  else
    {
    point[0] = this->OriginThumbWheel[0]->GetValue();
    point[1] = this->OriginThumbWheel[1]->GetValue();
    point[2] = this->OriginThumbWheel[2]->GetValue();
    }
}

//----------------------------------------------------------------------------
void vtkPVDisplayGUI::SetActorOriginNoTrace(double x, double y, double z)
{
  this->OriginThumbWheel[0]->SetValue(x);
  this->OriginThumbWheel[1]->SetValue(y);
  this->OriginThumbWheel[2]->SetValue(z);
#if defined(PARAVIEW_USE_SERVERMANAGER_RENDERING)
  double origin[3];
  origin[0] = x; origin[1] = y; origin[2] = z;
  this->PVSource->GetDisplayProxy()->cmSetOrigin(origin);
#else
  this->PVSource->GetPartDisplay()->SetOrigin(x, y, z);
#endif

  // Do not render here (do it in the callback, since it could be either
  // Render or EventuallyRender depending on the interaction)
}

//----------------------------------------------------------------------------
void vtkPVDisplayGUI::SetActorOrigin(double x, double y, double z)
{
  this->SetActorOriginNoTrace(x, y, z);
  if ( this->GetPVRenderView() )
    {
    this->GetPVRenderView()->EventuallyRender();
    }

  this->AddTraceEntry("$kw(%s) SetActorOrigin %f %f %f",
                      this->GetTclName(), x, y, z);  
}

//----------------------------------------------------------------------------
void vtkPVDisplayGUI::SetActorOrigin(double* point)
{
  this->SetActorOrigin(point[0], point[1], point[2]);
}

//----------------------------------------------------------------------------
void vtkPVDisplayGUI::ActorOriginCallback()
{
  double point[3];
  point[0] = this->OriginThumbWheel[0]->GetValue();
  point[1] = this->OriginThumbWheel[1]->GetValue();
  point[2] = this->OriginThumbWheel[2]->GetValue();
  this->SetActorOriginNoTrace(point[0], point[1], point[2]);
  if ( this->GetPVRenderView() )
    {
    this->GetPVRenderView()->Render();
    }
}

//----------------------------------------------------------------------------
void vtkPVDisplayGUI::ActorOriginEndCallback()
{
  double point[3];
  point[0] = this->OriginThumbWheel[0]->GetValue();
  point[1] = this->OriginThumbWheel[1]->GetValue();
  point[2] = this->OriginThumbWheel[2]->GetValue();
  this->SetActorOrigin(point);
}

//----------------------------------------------------------------------------
void vtkPVDisplayGUI::UpdateActorControl()
{
  int i;
  double translate[3];
  double scale[3];
  double origin[3];
  double orientation[3];
#if defined(PARAVIEW_USE_SERVERMANAGER_RENDERING)
  vtkSMDisplayProxy* pDisp = this->PVSource->GetDisplayProxy();
  pDisp->cmGetPosition(translate);
  pDisp->cmGetScale(scale);
  pDisp->cmGetOrientation(orientation);
  pDisp->cmGetOrigin(origin);
#else
  this->PVSource->GetPartDisplay()->GetTranslate(translate);
  this->PVSource->GetPartDisplay()->GetScale(scale);
  this->PVSource->GetPartDisplay()->GetOrientation(orientation);
  this->PVSource->GetPartDisplay()->GetOrigin(origin);
#endif
  for (i = 0; i < 3; i++)
    {    
    this->TranslateThumbWheel[i]->SetValue(translate[i]);
    this->ScaleThumbWheel[i]->SetValue(scale[i]);
    this->OrientationScale[i]->SetValue(orientation[i]);
    this->OriginThumbWheel[i]->SetValue(origin[i]);
    }
}

//----------------------------------------------------------------------------
void vtkPVDisplayGUI::UpdateActorControlResolutions()
{
  double bounds[6];
  this->GetPVSource()->GetDataInformation()->GetBounds(bounds);

  double res, oneh, half;

  // Update the resolution according to the bounds
  // Set res to 1/20 of the range, rounding to nearest .1 or .5 form.

  int i;
  for (i = 0; i < 3; i++)
    {
    double delta = bounds[i * 2 + 1] - bounds[i * 2];
    if (delta <= 0)
      {
      res = 0.1;
      }
    else
      {
      oneh = log10(delta * 0.051234);
      half = 0.5 * pow(10.0, ceil(oneh));
      res = (oneh > log10(half) ? half : pow(10.0, floor(oneh)));
      // cout << "up i: " << i << ", delta: " << delta << ", oneh: " << oneh << ", half: " << half << ", res: " << res << endl;
      }
    this->TranslateThumbWheel[i]->SetResolution(res);
    this->OriginThumbWheel[i]->SetResolution(res);
    }
}

void vtkPVDisplayGUI::UpdateEnableState()
{
  this->Superclass::UpdateEnableState();

  this->PropagateEnableState(this->ColorFrame);
  this->PropagateEnableState(this->VolumeAppearanceFrame);
  this->PropagateEnableState(this->DisplayStyleFrame);
  this->PropagateEnableState(this->ViewFrame);
  this->PropagateEnableState(this->ColorMenuLabel);
  this->PropagateEnableState(this->VolumeScalarsMenuLabel);
  this->PropagateEnableState(this->EditVolumeAppearanceButton);
  this->PropagateEnableState(this->ColorSelectionMenu);
  this->PropagateEnableState(this->VolumeScalarSelectionWidget);
  if ( this->EditColorMapButtonVisible )
    {
    this->PropagateEnableState(this->EditColorMapButton);
    this->PropagateEnableState(this->DataColorRangeButton);
    }
  else
    {
    this->EditColorMapButton->SetEnabled(0);
    this->DataColorRangeButton->SetEnabled(0);
    }
  if ( this->MapScalarsCheckVisible)
    {
    this->PropagateEnableState(this->MapScalarsCheck);
    }
  else
    {
    this->MapScalarsCheck->SetEnabled(0);
    }
  if ( this->InterpolateColorsCheckVisible)
    {
    this->PropagateEnableState(this->InterpolateColorsCheck);
    }
  else
    {
    this->InterpolateColorsCheck->SetEnabled(0);
    }
  if (this->ColorButtonVisible)
    {
    this->PropagateEnableState(this->ColorButton);
    }
  else
    {
    this->ColorButton->SetEnabled(0);
    }
  this->PropagateEnableState(this->RepresentationMenuLabel);
  this->PropagateEnableState(this->RepresentationMenu);
  this->PropagateEnableState(this->VisibilityCheck);
  if ( this->ScalarBarCheckVisible )
    {
    this->PropagateEnableState(this->ScalarBarCheck);
    }
  else
    {
    this->ScalarBarCheck->SetEnabled(0);
    }
  this->PropagateEnableState(this->ActorControlFrame);
  this->PropagateEnableState(this->TranslateLabel);
  this->PropagateEnableState(this->ScaleLabel);
  this->PropagateEnableState(this->OrientationLabel);
  this->PropagateEnableState(this->OriginLabel);
  int cc;
  for ( cc = 0; cc < 3; cc ++ )
    {
    this->PropagateEnableState(this->TranslateThumbWheel[cc]);
    this->PropagateEnableState(this->ScaleThumbWheel[cc]);
    this->PropagateEnableState(this->OrientationScale[cc]);
    this->PropagateEnableState(this->OriginThumbWheel[cc]);
    }
  this->PropagateEnableState(this->CubeAxesCheck);
  this->PropagateEnableState(this->PointLabelCheck);
  this->PropagateEnableState(this->ResetCameraButton);
  
  if ( this->VolumeRenderMode )
    {
    this->InterpolationMenuLabel->SetEnabled(0);
    this->InterpolationMenu->SetEnabled(0);
    this->LineWidthLabel->SetEnabled(0);
    this->LineWidthThumbWheel->SetEnabled(0);
    this->PointSizeLabel->SetEnabled(0);
    this->PointSizeThumbWheel->SetEnabled(0);
    this->OpacityLabel->SetEnabled(0);
    this->OpacityScale->SetEnabled(0);
    }
  else
    {
    this->PropagateEnableState(this->InterpolationMenuLabel);
    this->PropagateEnableState(this->InterpolationMenu);
    this->PropagateEnableState(this->PointSizeLabel);
    this->PropagateEnableState(this->PointSizeThumbWheel);
    this->PropagateEnableState(this->LineWidthLabel);
    this->PropagateEnableState(this->LineWidthThumbWheel);
    this->PropagateEnableState(this->OpacityLabel);
    this->PropagateEnableState(this->OpacityScale);
    }
  
}

//----------------------------------------------------------------------------
void vtkPVDisplayGUI::SaveVolumeRenderStateDisplay(ofstream *file)
{
#if !defined(PARAVIEW_USE_SERVERMANAGER_RENDERING)
  if(this->VolumeRenderMode)
    {
    *file << "[$kw(" << this->GetPVSource()->GetTclName()
          << ") GetPVOutput] DrawVolume" << endl;
//    *file << "[$kw(" << this->GetPVSource()->GetTclName()
//          << ") GetPVOutput] ShowVolumeAppearanceEditor" << endl;

    vtkStdString command(this->VolumeScalarsMenu->GetValue());

    // The form of the command is of the form
    // vtkTemp??? ColorBy???Field {Field Name} NumComponents
    // The field name is between the first and last braces, and
    // the number of components is at the end of the string.
    vtkStdString::size_type firstspace = command.find_first_of(' ');
    vtkStdString::size_type lastspace = command.find_last_of(' ');
    vtkStdString name = command.substr(firstspace+1, lastspace-firstspace-1);
  //  vtkStdString numCompsStr = command.substr(command.find_last_of(' '));

    // have to visit this panel in order to initialize vars
//    this->ShowVolumeAppearanceEditor();

    if ( command.c_str() && strlen(command.c_str()) > 6 )
      {
      vtkPVDataInformation* dataInfo = this->PVSource->GetDataInformation();
      vtkPVArrayInformation *arrayInfo;
      int colorField = this->PVSource->GetPartDisplay()->GetColorField();
      if (colorField == vtkDataSet::POINT_DATA_FIELD)
        {
        vtkPVDataSetAttributesInformation *attrInfo
          = dataInfo->GetPointDataInformation();
        arrayInfo = attrInfo->GetArrayInformation(name.c_str());
        if(arrayInfo)
          *file << "[$kw(" << this->GetPVSource()->GetTclName() << ") GetPVOutput] VolumeRenderPointField {" << arrayInfo->GetName() << "} " << arrayInfo->GetNumberOfComponents() << endl;
        }
      else
        {
        vtkPVDataSetAttributesInformation *attrInfo
          = dataInfo->GetCellDataInformation();
        arrayInfo = attrInfo->GetArrayInformation(name.c_str());
        if(arrayInfo)
          *file << "[$kw(" << this->GetPVSource()->GetTclName() << ") GetPVOutput] VolumeRenderCellField {" << arrayInfo->GetName() << "} " << arrayInfo->GetNumberOfComponents() << endl;
        }
      }
    }
#endif
}
