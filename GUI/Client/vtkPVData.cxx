/*=========================================================================

  Program:   ParaView
  Module:    vtkPVData.cxx

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkPVData.h"

#include "vtkCellData.h"
#include "vtkCollection.h"
#include "vtkCubeAxesActor2D.h"
#include "vtkDataSetAttributes.h"
#include "vtkDataSetSurfaceFilter.h"
#include "vtkPVPart.h"
#include "vtkPVPartDisplay.h"
#include "vtkCollection.h"
#include "vtkPVDataInformation.h"
#include "vtkPVDataSetAttributesInformation.h"
#include "vtkPVArrayInformation.h"
#include "vtkPVLODPartDisplay.h"
#include "vtkImageData.h"
#include "vtkKWBoundsDisplay.h"
#include "vtkKWChangeColorButton.h"
#include "vtkKWCheckButton.h"
#include "vtkKWEntry.h"
#include "vtkKWFrame.h"
#include "vtkKWLabel.h"
#include "vtkKWLabeledEntry.h"
#include "vtkKWMenuButton.h"
#include "vtkKWNotebook.h"
#include "vtkKWOptionMenu.h"
#include "vtkKWPushButton.h"
#include "vtkKWScale.h"
#include "vtkKWThumbWheel.h"
#include "vtkKWTkUtilities.h"
#include "vtkKWView.h"
#include "vtkKWWidget.h"
#include "vtkMultiProcessController.h"
#include "vtkObjectFactory.h"
#include "vtkPVApplication.h"
#include "vtkPVColorMap.h"
#include "vtkPVConfig.h"
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
#include "vtkString.h"
#include "vtkStructuredGrid.h"
#include "vtkTexture.h"
#include "vtkTimerLog.h"
#include "vtkToolkits.h"
#include "vtkPVGenericRenderWindowInteractor.h"
#include "vtkPVRenderView.h"
#include "vtkPVRenderModule.h"
#include "vtkPVArrayInformation.h"
#include "vtkPVRenderModuleUI.h"

// Just for the definition of VTK_POINT_DATA_FIELD ...
#include "vtkFieldDataToAttributeDataFilter.h"

#define VTK_PV_OUTLINE_LABEL "Outline"
#define VTK_PV_SURFACE_LABEL "Surface"
#define VTK_PV_WIREFRAME_LABEL "Wireframe of Surface"
#define VTK_PV_POINTS_LABEL "Points of Surface"
#define VTK_PV_VOLUME_LABEL "Volume Render"

//----------------------------------------------------------------------------
vtkStandardNewMacro(vtkPVData);
vtkCxxRevisionMacro(vtkPVData, "1.292.2.1");

int vtkPVDataCommand(ClientData cd, Tcl_Interp *interp,
                     int argc, char *argv[]);

//----------------------------------------------------------------------------
vtkPVData::vtkPVData()
{
  static int instanceCount = 0;

  this->CommandFunction = vtkPVDataCommand;

  this->PVSource = NULL;
  
  this->PropertiesCreated = 0;
  
  this->PropertiesParent = NULL; 

  this->CubeAxes = NULL;

  this->EditColorMapButtonVisible = 1;
  this->MapScalarsCheckVisible = 1;
  this->ColorButtonVisible = 1;
  this->ScalarBarCheckVisible = 1;
  this->InterpolateColorsCheckVisible = 1;
  this->CurrentColorAttributeIsFromCellData = 0;

  // Create a unique id for creating tcl names.
  ++instanceCount;
  this->InstanceCount = instanceCount;
    
  this->Properties = vtkKWFrame::New();
  this->InformationFrame = vtkKWFrame::New();

  this->ColorFrame = vtkKWLabeledFrame::New();
  this->VolumeAppearanceFrame = vtkKWLabeledFrame::New();
  this->DisplayStyleFrame = vtkKWLabeledFrame::New();
  this->StatsFrame = vtkKWLabeledFrame::New();
  this->ViewFrame = vtkKWLabeledFrame::New();
  
  this->TypeLabel = vtkKWLabel::New();
  this->NumCellsLabel = vtkKWLabel::New();
  this->NumPointsLabel = vtkKWLabel::New();
  this->MemorySizeLabel = vtkKWLabel::New();
  
  this->BoundsDisplay = vtkKWBoundsDisplay::New();
  this->BoundsDisplay->ShowHideFrameOn();
  
  this->ExtentDisplay = vtkKWBoundsDisplay::New();
  this->ExtentDisplay->ShowHideFrameOn();
  
  this->AmbientScale = vtkKWScale::New();

  this->ColorMenuLabel = vtkKWLabel::New();
  this->ColorMenu = vtkKWOptionMenu::New();

  this->MapScalarsCheck = vtkKWCheckButton::New();
  this->InterpolateColorsCheck = vtkKWCheckButton::New();
  this->EditColorMapButton = vtkKWPushButton::New();
  
  this->ColorButton = vtkKWChangeColorButton::New();

  this->VolumeScalarsMenuLabel = vtkKWLabel::New();
  this->VolumeScalarsMenu = vtkKWOptionMenu::New();
  
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
  this->VisibilityCheck = vtkKWCheckButton::New();
  this->Visibility = 1;

  this->ResetCameraButton = vtkKWPushButton::New();

  this->ActorControlFrame = vtkKWLabeledFrame::New();
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
  
  this->PreviousAmbient = 0.0;
  this->PreviousSpecular = 0.1;
  this->PreviousDiffuse = 1.0;
  this->PreviousWasSolid = 1;

  this->PVColorMap = NULL;

  this->ColorSetByUser = 0;
  this->ArraySetByUser = 0;
  
  this->PointLabelCheck = vtkKWCheckButton::New();
  
  this->VolumeRenderMode = 0;
  
  this->VolumeAppearanceEditor = NULL;
}

//----------------------------------------------------------------------------
vtkPVData::~vtkPVData()
{  
  if (this->PVColorMap)
    {
    // Use count to manage color map visibility.
    if (this->Visibility)
      {
      this->PVColorMap->DecrementUseCount();
      }
    this->PVColorMap->UnRegister(this);
    this->PVColorMap = 0;
    }

  if ( this->VolumeAppearanceEditor )
    {
    this->VolumeAppearanceEditor->UnRegister(this);
    this->VolumeAppearanceEditor = NULL;
    }
  
  this->SetPVSource(NULL);

  if (this->PropertiesParent)
    {
    this->PropertiesParent->UnRegister(this);
    this->PropertiesParent = NULL;
    }
    
  this->TypeLabel->Delete();
  this->TypeLabel = NULL;
  
  this->NumCellsLabel->Delete();
  this->NumCellsLabel = NULL;
  
  this->NumPointsLabel->Delete();
  this->NumPointsLabel = NULL;
  
  this->MemorySizeLabel->Delete();
  this->MemorySizeLabel = NULL;
  
  this->BoundsDisplay->Delete();
  this->BoundsDisplay = NULL;
  
  this->ExtentDisplay->Delete();
  this->ExtentDisplay = NULL;
  
  this->AmbientScale->Delete();
  this->AmbientScale = NULL;
  
  this->ColorMenuLabel->Delete();
  this->ColorMenuLabel = NULL;
  
  this->ColorMenu->Delete();
  this->ColorMenu = NULL;

  this->EditColorMapButton->Delete();
  this->EditColorMapButton = NULL;

  this->MapScalarsCheck->Delete();
  this->MapScalarsCheck = NULL;  
    
  this->InterpolateColorsCheck->Delete();
  this->InterpolateColorsCheck = NULL;  
    
  this->ColorButton->Delete();
  this->ColorButton = NULL;
  
  this->VolumeScalarsMenuLabel->Delete();
  this->VolumeScalarsMenuLabel = NULL;

  this->VolumeScalarsMenu->Delete();
  this->VolumeScalarsMenu = NULL;

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
 
  if (this->CubeAxes)
    {
    this->CubeAxes->Delete();
    }

  this->ScalarBarCheck->Delete();
  this->ScalarBarCheck = NULL;  

  this->CubeAxesCheck->Delete();
  this->CubeAxesCheck = NULL;

  this->VisibilityCheck->Delete();
  this->VisibilityCheck = NULL;
  
  this->ColorFrame->Delete();
  this->ColorFrame = NULL;
  this->VolumeAppearanceFrame->Delete();
  this->VolumeAppearanceFrame = NULL;
  this->DisplayStyleFrame->Delete();
  this->DisplayStyleFrame = NULL;
  this->StatsFrame->Delete();
  this->StatsFrame = NULL;
  this->ViewFrame->Delete();
  this->ViewFrame = NULL;
  
  this->ResetCameraButton->Delete();
  this->ResetCameraButton = NULL;

  this->Properties->Delete();
  this->Properties = NULL;

  this->InformationFrame->Delete();
  this->InformationFrame = NULL;
  
  this->PointLabelCheck->Delete();
  this->PointLabelCheck = NULL;
}



//----------------------------------------------------------------------------
void vtkPVData::SetVisibilityCheckState(int v)
{
  if (this->Visibility != v)
    {
    this->Visibility = v;
    // Use count to manage color map visibility.
    if (this->PVColorMap)
      {
      if (v)
        {
        this->PVColorMap->IncrementUseCount();
        }
      else
        {
        this->PVColorMap->DecrementUseCount();
        }
      }
    }

  if (this->VisibilityCheck->GetApplication())
    {
    if (this->VisibilityCheck->GetState() != v)
      {
      this->VisibilityCheck->SetState(v);
      }
    }
}


//----------------------------------------------------------------------------
void vtkPVData::SetVolumeAppearanceEditor(vtkPVVolumeAppearanceEditor *appearanceEditor)
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
void vtkPVData::SetPVColorMap(vtkPVColorMap *colorMap)
{
  if (this->PVColorMap == colorMap)
    {
    return;
    }

  if (this->PVColorMap)
    {
    // Use count to manage color map visibility.
    if (this->Visibility)
      {
      this->PVColorMap->DecrementUseCount();
      }
    this->PVColorMap->UnRegister(this);
    this->PVColorMap = NULL;
    }

  this->PVColorMap = colorMap;
  if (this->PVColorMap)
    {
    if (this->Visibility)
      {
      this->PVColorMap->IncrementUseCount();
      }
    this->PVColorMap->Register(this);
    
    if (this->ScalarBarCheck->IsCreated())
      {
      // Let's make those 2 chekbuttons use the same variable name
      this->ScalarBarCheck->SetVariableName(
        this->PVColorMap->GetScalarBarCheck()->GetVariableName());
      }
    }
}

//----------------------------------------------------------------------------
void vtkPVData::CreateParallelTclObjects(vtkPVApplication *pvApp)
{
  this->vtkKWObject::SetApplication(pvApp);
  
  // Nothing to do here since we created vtkPVPart.  
  // I did not remove method in case we need to create Cube Axes in parallel.
  // I am having issues rendering on client.  I do not want to send
  // the zbuffer.
}

//----------------------------------------------------------------------------
void vtkPVData::DeleteCallback()
{
  this->SetCubeAxesVisibility(0);
  this->SetPointLabelVisibility(0);
}

//----------------------------------------------------------------------------
void vtkPVData::SetPVApplication(vtkPVApplication *pvApp)
{
  this->CreateParallelTclObjects(pvApp);
  this->vtkKWObject::SetApplication(pvApp);
}




//----------------------------------------------------------------------------
// WE DO NOT REFERENCE COUNT HERE BECAUSE OF CIRCULAR REFERENCES.
// THE SOURCE OWNS THE DATA.
void vtkPVData::SetPVSource(vtkPVSource *source)
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
void vtkPVData::CreateProperties()
{
  if (this->PropertiesCreated)
    {
    return;
    }

  // Properties (aka Display page)

  if (this->GetPVSource()->GetHideDisplayPage())
    {
    // We use the parameters frame as a bogus parent.
    // This is not a problem since we never pack the
    // properties frame in this case.
    this->Properties->SetParent(
      this->GetPVSource()->GetParametersParent());
    }
  else
    {
    this->Properties->SetParent(
      this->GetPVSource()->GetNotebook()->GetFrame("Display"));
    }
  this->Properties->ScrollableOn();
  this->Properties->Create(this->GetApplication(),0);

  // We are going to 'grid' most of it, so let's define some const

  int col_1_padx = 2;
  int button_pady = 1;
  int col_0_weight = 0;
  int col_1_weight = 1;
  float col_0_factor = 1.5;
  float col_1_factor = 1.0;

  // View frame

  this->ViewFrame->SetParent(this->Properties->GetFrame());
  this->ViewFrame->ShowHideFrameOn();
  this->ViewFrame->Create(this->GetApplication(), 0);
  this->ViewFrame->SetLabel("View");
 
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
  this->ResetCameraButton->SetLabel("Set View to Data");
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
      (!this->GetPVApplication()->GetClientMode()))
    {
    this->Script("grid %s -sticky wns",
                 this->PointLabelCheck->GetWidgetName());
    }

  // Color
  this->ColorFrame->SetParent(this->Properties->GetFrame());
  this->ColorFrame->ShowHideFrameOn();
  this->ColorFrame->Create(this->GetApplication(), 0);
  this->ColorFrame->SetLabel("Color");

  this->ColorMenuLabel->SetParent(this->ColorFrame->GetFrame());
  this->ColorMenuLabel->Create(this->GetApplication(), "");
  this->ColorMenuLabel->SetLabel("Color by:");
  this->ColorMenuLabel->SetBalloonHelpString(
    "Select method for coloring dataset geometry.");
  
  this->ColorMenu->SetParent(this->ColorFrame->GetFrame());
  this->ColorMenu->Create(this->GetApplication(), "");   
  this->ColorMenu->SetBalloonHelpString(
    "Select method for coloring dataset geometry.");

  this->ColorButton->SetParent(this->ColorFrame->GetFrame());
  this->ColorButton->SetText("Actor Color");
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

  this->EditColorMapButton->SetParent(this->ColorFrame->GetFrame());
  this->EditColorMapButton->Create(this->GetApplication(), "");
  this->EditColorMapButton->SetLabel("Edit Color Map...");
  this->EditColorMapButton->SetCommand(this,"EditColorMapCallback");
  this->EditColorMapButton->SetBalloonHelpString(
    "Edit the table used to map data attributes to pseudo colors.");

  
  this->Script("grid %s %s -sticky wns",
               this->ColorMenuLabel->GetWidgetName(),
               this->ColorMenu->GetWidgetName());
  this->Script("grid %s -sticky news -padx %d -pady %d",
               this->ColorMenu->GetWidgetName(),
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
               this->EditColorMapButton->GetWidgetName());
  this->Script("grid %s -sticky news -padx %d -pady %d",
               this->EditColorMapButton->GetWidgetName(),
               col_1_padx, button_pady);
  this->MapScalarsCheckVisible = 0;
  this->EditColorMapButtonVisible = 0;

  // Volume Appearance
  this->SetVolumeAppearanceEditor(this->GetPVApplication()->GetMainWindow()->
                                  GetVolumeAppearanceEditor());

  this->VolumeAppearanceFrame->SetParent(this->Properties->GetFrame());
  this->VolumeAppearanceFrame->ShowHideFrameOn();
  this->VolumeAppearanceFrame->Create(this->GetApplication(), 0);
  this->VolumeAppearanceFrame->SetLabel("Volume Appearance");

  this->VolumeScalarsMenuLabel->
    SetParent(this->VolumeAppearanceFrame->GetFrame());
  this->VolumeScalarsMenuLabel->Create(this->GetApplication(), "");
  this->VolumeScalarsMenuLabel->SetLabel("View Scalars:");
  this->VolumeScalarsMenuLabel->SetBalloonHelpString(
    "Select scalars to view with volume rendering.");
  
  this->VolumeScalarsMenu->SetParent(this->VolumeAppearanceFrame->GetFrame());
  this->VolumeScalarsMenu->Create(this->GetApplication(), "");   
  this->VolumeScalarsMenu->SetBalloonHelpString(
    "Select scalars to view with volume rendering.");

  this->EditVolumeAppearanceButton->
    SetParent(this->VolumeAppearanceFrame->GetFrame());
  this->EditVolumeAppearanceButton->Create(this->GetApplication(), "");
  this->EditVolumeAppearanceButton->SetLabel("Edit Volume Appearance...");
  this->EditVolumeAppearanceButton->
    SetCommand(this,"EditVolumeAppearanceCallback");
  this->EditVolumeAppearanceButton->SetBalloonHelpString(
    "Edit the color and opacity functions for the volume.");

  
  this->Script("grid %s %s -sticky wns",
               this->VolumeScalarsMenuLabel->GetWidgetName(),
               this->VolumeScalarsMenu->GetWidgetName());

  this->Script("grid %s -sticky news -padx %d -pady %d",
               this->VolumeScalarsMenu->GetWidgetName(),
               col_1_padx, button_pady);

  this->Script("grid %s -column 1 -sticky news -padx %d -pady %d",
               this->EditVolumeAppearanceButton->GetWidgetName(),
               col_1_padx, button_pady);


  // Display style
  this->DisplayStyleFrame->SetParent(this->Properties->GetFrame());
  this->DisplayStyleFrame->ShowHideFrameOn();
  this->DisplayStyleFrame->Create(this->GetApplication(), 0);
  this->DisplayStyleFrame->SetLabel("Display Style");

  this->AmbientScale->SetParent(this->Properties->GetFrame());
  this->AmbientScale->Create(this->GetApplication(), "-showvalue 1");
  this->AmbientScale->DisplayLabel("Ambient Light");
  this->AmbientScale->SetRange(0.0, 1.0);
  this->AmbientScale->SetResolution(0.1);
  this->AmbientScale->SetCommand(this, "AmbientChanged");
  
  this->RepresentationMenuLabel->SetParent(
    this->DisplayStyleFrame->GetFrame());
  this->RepresentationMenuLabel->Create(this->GetApplication(), "");
  this->RepresentationMenuLabel->SetLabel("Representation:");

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
  this->InterpolationMenuLabel->SetLabel("Interpolation:");

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
  this->PointSizeLabel->SetLabel("Point size:");
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
  this->LineWidthLabel->SetLabel("Line width:");
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

  this->ActorControlFrame->SetParent(this->Properties->GetFrame());
  this->ActorControlFrame->ShowHideFrameOn();
  this->ActorControlFrame->Create(this->GetApplication(), 0);
  this->ActorControlFrame->SetLabel("Actor Control");

  this->TranslateLabel->SetParent(this->ActorControlFrame->GetFrame());
  this->TranslateLabel->Create(this->GetApplication(), 0);
  this->TranslateLabel->SetLabel("Translate:");
  this->TranslateLabel->SetBalloonHelpString(
    "Translate the geometry relative to the dataset location.");

  this->ScaleLabel->SetParent(this->ActorControlFrame->GetFrame());
  this->ScaleLabel->Create(this->GetApplication(), 0);
  this->ScaleLabel->SetLabel("Scale:");
  this->ScaleLabel->SetBalloonHelpString(
    "Scale the geometry relative to the size of the dataset.");

  this->OrientationLabel->SetParent(this->ActorControlFrame->GetFrame());
  this->OrientationLabel->Create(this->GetApplication(), 0);
  this->OrientationLabel->SetLabel("Orientation:");
  this->OrientationLabel->SetBalloonHelpString(
    "Orient the geometry relative to the dataset origin.");

  this->OriginLabel->SetParent(this->ActorControlFrame->GetFrame());
  this->OriginLabel->Create(this->GetApplication(), 0);
  this->OriginLabel->SetLabel("Origin:");
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
  this->OpacityLabel->SetLabel("Opacity:");
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

  if (!this->GetPVSource()->GetHideDisplayPage())
    {
    this->Script("pack %s -fill both -expand yes -side top",
                 this->Properties->GetWidgetName());
    }

  // Pack

  this->Script("pack %s %s %s %s -fill x -expand t -pady 2", 
               this->ViewFrame->GetWidgetName(),
               this->ColorFrame->GetWidgetName(),
               this->DisplayStyleFrame->GetWidgetName(),
               this->ActorControlFrame->GetWidgetName());

  // Information page

  if (this->GetPVSource()->GetHideInformationPage())
    {
    // We use the parameters frame as a bogus parent.
    // This is not a problem since we never pack the
    // properties frame in this case.
    this->InformationFrame->SetParent(
      this->GetPVSource()->GetParametersParent());
    }
  else
    {
    this->InformationFrame->SetParent(
      this->GetPVSource()->GetNotebook()->GetFrame("Information"));
    }
  this->InformationFrame->ScrollableOn();
  this->InformationFrame->Create(this->GetApplication(),0);

  this->StatsFrame->SetParent(this->InformationFrame->GetFrame());
  this->StatsFrame->ShowHideFrameOn();
  this->StatsFrame->Create(this->GetApplication(), 0);
  this->StatsFrame->SetLabel("Statistics");

  this->TypeLabel->SetParent(this->StatsFrame->GetFrame());
  this->TypeLabel->Create(this->GetApplication(), "");

  this->NumCellsLabel->SetParent(this->StatsFrame->GetFrame());
  this->NumCellsLabel->Create(this->GetApplication(), "");

  this->NumPointsLabel->SetParent(this->StatsFrame->GetFrame());
  this->NumPointsLabel->Create(this->GetApplication(), "");
  
  this->MemorySizeLabel->SetParent(this->StatsFrame->GetFrame());
  this->MemorySizeLabel->Create(this->GetApplication(), "");

  this->BoundsDisplay->SetParent(this->InformationFrame->GetFrame());
  this->BoundsDisplay->Create(this->GetApplication(), "");
  
  this->ExtentDisplay->SetParent(this->InformationFrame->GetFrame());
  this->ExtentDisplay->Create(this->GetApplication(), "");
  this->ExtentDisplay->SetLabel("Extents");
  
  this->Script("pack %s %s %s %s -side top -anchor nw",
               this->TypeLabel->GetWidgetName(),
               this->NumCellsLabel->GetWidgetName(),
               this->NumPointsLabel->GetWidgetName(),
               this->MemorySizeLabel->GetWidgetName());

  this->Script("pack %s %s -fill x -expand t -pady 2", 
               this->StatsFrame->GetWidgetName(),
               this->BoundsDisplay->GetWidgetName());

  if (!this->GetPVSource()->GetHideInformationPage())
    {
    this->Script("pack %s -fill both -expand yes -side top",
                 this->InformationFrame->GetWidgetName());
    }

  // OK, all done

  this->PropertiesCreated = 1;
  this->UpdateEnableState();
}

//----------------------------------------------------------------------------
void vtkPVData::EditColorMapCallback()
{
  if (this->PVColorMap == NULL)
    {
    // We could get the color map from the window,
    // but it must already be set for this button to be visible.
    vtkErrorMacro("Expecting a color map.");
    return;
    }
  this->Script("pack forget [pack slaves %s]",
          this->GetPVRenderView()->GetPropertiesParent()->GetWidgetName());
  this->Script("pack %s -side top -fill both -expand t",
          this->PVColorMap->GetWidgetName());
}

//----------------------------------------------------------------------------
void vtkPVData::EditVolumeAppearanceCallback()
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

  const char *menuValue = this->VolumeScalarsMenu->GetValue();

  if ( menuValue && strlen(menuValue) > 6 )
    {
    vtkPVDataInformation* dataInfo = source->GetDataInformation();
    vtkPVDataSetAttributesInformation *attrInfo = dataInfo->GetPointDataInformation();
    vtkPVArrayInformation *arrayInfo = attrInfo->GetArrayInformation(menuValue+6);
    this->VolumeAppearanceEditor->SetPVSourceAndArrayInfo( source, arrayInfo );
    }
  else
    {
    this->VolumeAppearanceEditor->SetPVSourceAndArrayInfo( NULL, NULL );
    }                                              
}


//----------------------------------------------------------------------------
void vtkPVData::UpdateProperties()
{
  // This call makes sure the information is up to date.
  // If not, it gathers information and updates the properties (internal).
  this->GetPVSource()->GetDataInformation();
  this->UpdatePropertiesInternal();
}

//----------------------------------------------------------------------------
void vtkPVData::UpdatePropertiesInternal()
{
  vtkPVSource* source = this->GetPVSource();

  if (!source)
    {
    return;
    }

  vtkPVDataInformation* dataInfo = source->GetDataInformation();
  char tmp[350], cmd[1024], volCmd[1024], defCmd[350];
  double bounds[6];
  int i, numArrays, numComps;
  vtkPVDataSetAttributesInformation *attrInfo;
  vtkPVArrayInformation *arrayInfo;
  const char *currentColorBy, *inputColorBy = 0;
  int currentColorByFound = 0, inputColorByFound = 0;
  int defPoint = 0, inputPoint = 0;
  vtkPVArrayInformation *defArray, *inputArray, *volRenArray;
  int inputColorSetByUser = 0, inputArraySetByUser = 0;
  
  // Default is the scalars to use when current color is not found.
  // This is sort of a mess, and should be handled by a color selection widget.
  defCmd[0] = '\0'; 
  defArray = NULL;
  inputArray = NULL;
  volRenArray = NULL;
  
  if (this->PVColorMap)
    {
    if (this->PVColorMap->GetScalarBarVisibility())
      {
      this->ScalarBarCheck->SetState(1);
      }
    else
      {
      this->ScalarBarCheck->SetState(0);
      }
    }

  if (source->GetHideDisplayPage())
    {
    return;
    }

  // Update actor control resolutions

  this->UpdateActorControlResolutions();
  
  ostrstream type;
  type << "Type: ";

  // Put the data type as the label of the top frame.
  int dataType = dataInfo->GetDataSetType();
  if (dataType == VTK_POLY_DATA)
    {
    type << "Polygonal";
    this->Script("pack forget %s", 
                 this->ExtentDisplay->GetWidgetName());
    }
  else if (dataType == VTK_UNSTRUCTURED_GRID)
    {
    type << "Unstructured Grid";
    this->Script("pack forget %s", 
                 this->ExtentDisplay->GetWidgetName());
    }
  else if (dataType == VTK_STRUCTURED_GRID)
    {
    type << "Curvilinear";
    this->ExtentDisplay->SetExtent(dataInfo->GetExtent());
    this->Script("pack %s -fill x -expand t -pady 2", 
                 this->ExtentDisplay->GetWidgetName());
    }
  else if (dataType == VTK_RECTILINEAR_GRID)
    {
    type << "Nonuniform Rectilinear";
    this->ExtentDisplay->SetExtent(dataInfo->GetExtent());
    this->Script("pack %s -fill x -expand t -pady 2", 
                 this->ExtentDisplay->GetWidgetName());
    }
  else if (dataType == VTK_IMAGE_DATA)
    {
    int *ext = dataInfo->GetExtent();
    if (ext[0] == ext[1] || ext[2] == ext[3] || ext[4] == ext[5])
      {
      type << "Image (Uniform Rectilinear)";
      }
    else
      {
      type << "Volume (Uniform Rectilinear)";
      }
    this->ExtentDisplay->SetExtent(ext);
    this->Script("pack %s -fill x -expand t -pady 2", 
                 this->ExtentDisplay->GetWidgetName());
    }
  else if (dataType == VTK_MULTI_BLOCK_DATA_SET)
    {
    type << "Multi-block composite";
    this->Script("pack forget %s", 
                 this->ExtentDisplay->GetWidgetName());
    }
  else if (dataType == VTK_HIERARCHICAL_BOX_DATA_SET)
    {
    type << "Hierarchical Uniform AMR";
    this->Script("pack forget %s", 
                 this->ExtentDisplay->GetWidgetName());
    }
  else
    {
    type << "Unknown";
    }
  type << ends;
  this->TypeLabel->SetLabel(type.str());
  type.rdbuf()->freeze(0);
  
  ostrstream numcells;
  numcells << "Number of cells: " << dataInfo->GetNumberOfCells() << ends;
  this->NumCellsLabel->SetLabel(numcells.str());
  numcells.rdbuf()->freeze(0);

  ostrstream numpts;
  numpts << "Number of points: " << dataInfo->GetNumberOfPoints() << ends;
  this->NumPointsLabel->SetLabel(numpts.str());
  numpts.rdbuf()->freeze(0);
  
  ostrstream memsize;
  memsize << "Memory: " << ((float)(dataInfo->GetMemorySize())/1000.0) << " MBytes" << ends;
  this->MemorySizeLabel->SetLabel(memsize.str());
  memsize.rdbuf()->freeze(0);

  dataInfo->GetBounds(bounds);
  this->BoundsDisplay->SetBounds(bounds);
  if (this->CubeAxes)
    {
    this->CubeAxes->SetBounds(bounds[0], bounds[1], bounds[2],
                              bounds[3], bounds[4], bounds[5]);
    }

  currentColorBy = this->ColorMenu->GetValue();
  // Initially, get the "color by" value from input (if it exists)
  vtkPVSource *input = this->GetPVSource()->GetPVInput(0);
  if (strlen(currentColorBy) == 0 && input)
    {
    inputColorBy = input->GetPVOutput()->GetColorMenu()->GetValue();
    inputColorSetByUser = input->GetPVOutput()->GetColorSetByUser();
    inputArraySetByUser = input->GetPVOutput()->GetArraySetByUser();
    }
  else
    {
    inputColorBy = "\0";
    }
  
  this->ColorMenu->ClearEntries();
  this->ColorMenu->AddEntryWithCommand("Property",
                                       this, "ColorByProperty");

  const char *currentVolumeField = this->VolumeScalarsMenu->GetValue();
  this->VolumeScalarsMenu->ClearEntries();
  
  attrInfo = dataInfo->GetPointDataInformation();
  numArrays = attrInfo->GetNumberOfArrays();
  int firstField = 1;
  for (i = 0; i < numArrays; i++)
    {
    arrayInfo = attrInfo->GetArrayInformation(i);
    numComps = arrayInfo->GetNumberOfComponents();
    sprintf(cmd, "ColorByPointField {%s} %d", 
            arrayInfo->GetName(), numComps);
    sprintf(volCmd, "VolumeRenderPointField {%s}", arrayInfo->GetName());
    if (numComps > 1)
      {
      sprintf(tmp, "Point %s (%d)", arrayInfo->GetName(), numComps);
      }
    else
      {
      sprintf(tmp, "Point %s", arrayInfo->GetName());
      this->VolumeScalarsMenu->AddEntryWithCommand(tmp, this, volCmd);
      if ( (firstField && !strlen(currentVolumeField)) ||
           !strcmp( tmp, currentVolumeField ) )
        {
        this->VolumeScalarsMenu->SetValue( tmp );
        volRenArray = arrayInfo;
        firstField = 0;
        }
      }
    this->ColorMenu->AddEntryWithCommand(tmp, this, cmd);
    if (strcmp(tmp, currentColorBy) == 0)
      {
      currentColorByFound = 1;
      }
    if (strcmp(tmp, inputColorBy) == 0)
      {
      inputColorByFound = 1;
      inputPoint = 1;
      inputArray = arrayInfo;
      }
    if (attrInfo->IsArrayAnAttribute(i) == vtkDataSetAttributes::SCALARS)
      {
      strcpy(defCmd, tmp);
      defPoint = 1;
      defArray = arrayInfo;
      if ( !strlen(currentVolumeField) )
        {
        volRenArray = arrayInfo;
        this->VolumeScalarsMenu->SetValue( tmp );
        }
      }
    }

  attrInfo = dataInfo->GetCellDataInformation();
  numArrays = attrInfo->GetNumberOfArrays();
  for (i = 0; i < numArrays; i++)
    {
    arrayInfo = attrInfo->GetArrayInformation(i);
    sprintf(cmd, "ColorByCellField {%s} %d", 
            arrayInfo->GetName(), arrayInfo->GetNumberOfComponents());
    if (arrayInfo->GetNumberOfComponents() > 1)
      {
      sprintf(tmp, "Cell %s (%d)", arrayInfo->GetName(),
              arrayInfo->GetNumberOfComponents());
      }
    else
      {
      sprintf(tmp, "Cell %s", arrayInfo->GetName());
      }
    this->ColorMenu->AddEntryWithCommand(tmp, this, cmd);
    if (strcmp(tmp, currentColorBy) == 0)
      {
      currentColorByFound = 1;
      }
    if (strcmp(tmp, inputColorBy) == 0 && !inputArray)
      {
      inputColorByFound = 1;
      inputPoint = 0;
      inputArray = arrayInfo;
      }
    if (defArray == NULL && attrInfo->IsArrayAnAttribute(i) == vtkDataSetAttributes::SCALARS)
      {
      strcpy(defCmd, tmp);
      defPoint = 0;
      defArray = arrayInfo;
      }
    }

  // Current color by will never be NULL ....
  if (currentColorBy != NULL && strcmp(currentColorBy, "Property") == 0 && this->ColorSetByUser)
    {
    return;
    }

  // If the current array we are coloring by has disappeared,
  // then default back to the property.
  if ( ! currentColorByFound)
    {
    this->ColorSetByUser = 0;
    this->ArraySetByUser = 0;
    if (defArray != NULL && !inputArraySetByUser && !inputColorSetByUser)
      {
      this->ColorMenu->SetValue(defCmd);
      if (defPoint)
        {
        this->ColorByPointFieldInternal(defArray->GetName(), 
                                        defArray->GetNumberOfComponents());
        }
      else
        {
        this->ColorByCellFieldInternal(defArray->GetName(), 
                                       defArray->GetNumberOfComponents());
        }
      }
    else if (inputColorByFound && inputArraySetByUser)
      {
      this->ColorMenu->SetValue(inputColorBy);
      if (inputPoint)
        {
        this->ColorByPointFieldInternal(inputArray->GetName(),
                                        inputArray->GetNumberOfComponents());
        }
      else
        {
        this->ColorByCellFieldInternal(inputArray->GetName(),
                                       inputArray->GetNumberOfComponents());
        }
      }
    else if (inputColorSetByUser)
      {
      this->ColorButton->SetColor(input->GetPVOutput()->GetActorColor());
      this->ColorMenu->SetValue("Property");
      this->ColorByPropertyInternal();
      this->ColorSetByUser = 1;
      }
    else
      {
      this->ColorMenu->SetValue("Property");
      this->ColorByPropertyInternal();
      }
    }

  // Determine if this is unstructured grid data and add the 
  // volume rendering option
  if ( this->RepresentationMenu->HasEntry( VTK_PV_VOLUME_LABEL ) )
    {
      this->RepresentationMenu->DeleteEntry( VTK_PV_VOLUME_LABEL );
    }
  
  if (dataType == VTK_UNSTRUCTURED_GRID && volRenArray)
    {
    this->RepresentationMenu->AddEntryWithCommand(VTK_PV_VOLUME_LABEL, this,
                                                  "DrawVolume");
      
    // Update the transfer functions
    vtkPVPart *part;
    int idx, num;
    
    num = this->GetPVSource()->GetNumberOfParts();
    for (idx = 0; idx < num; ++idx)
      {
      part = this->GetPVSource()->GetPart(idx);
      part->GetPartDisplay()->InitializeTransferFunctions(volRenArray, dataInfo);
      }
    }
}

//----------------------------------------------------------------------------
void vtkPVData::SetActorColor(double r, double g, double b)
{
  vtkPVPart *part;
  int idx, num;

  num = this->GetPVSource()->GetNumberOfParts();
  for (idx = 0; idx < num; ++idx)
    {
    part = this->GetPVSource()->GetPart(idx);
    part->GetPartDisplay()->SetColor(r, g, b);
    }
}  

//----------------------------------------------------------------------------
void vtkPVData::ChangeActorColor(double r, double g, double b)
{
  this->AddTraceEntry("$kw(%s) ChangeActorColor %f %f %f",
                      this->GetTclName(), r, g, b);

  this->SetActorColor(r, g, b);
  this->ColorButton->SetColor(r, g, b);

  if ( this->GetPVRenderView() )
    {
    this->GetPVRenderView()->EventuallyRender();
    }
  
  if (strcmp(this->ColorMenu->GetValue(), "Property") == 0)
    {
    this->ColorSetByUser = 1;
    }
}

//----------------------------------------------------------------------------
double* vtkPVData::GetActorColor()
{
  return this->GetPVSource()->GetPart(0)->GetPartDisplay()->GetProperty()->GetColor();
}

//----------------------------------------------------------------------------
void vtkPVData::SetColorRange(double min, double max)
{
  if (this->PVColorMap == NULL)
    {
    vtkErrorMacro("Color map is missing.");
    return;
    }

  this->PVColorMap->SetScalarRange(min, max);
}

//----------------------------------------------------------------------------
// Hack for now.
void vtkPVData::SetColorRangeInternal(double min, double max)
{
  if (this->PVColorMap == NULL)
    {
    vtkErrorMacro("Color map is missing.");
    return;
    }

  this->PVColorMap->SetScalarRangeInternal(min, max);
}

//----------------------------------------------------------------------------
void vtkPVData::ResetColorRange()
{
  if (this->PVColorMap == NULL)
    {
    vtkErrorMacro("Color map is missing.");
    return;
    }

  this->PVColorMap->ResetScalarRange();
  if ( this->GetPVRenderView() )
    {
    this->GetPVRenderView()->EventuallyRender();
    }
}

//----------------------------------------------------------------------------
void vtkPVData::ColorByProperty()
{
  this->ColorSetByUser = 1;
  this->AddTraceEntry("$kw(%s) ColorByProperty", this->GetTclName());
  this->ColorMenu->SetValue("Property");
  this->ColorByPropertyInternal();
}

//----------------------------------------------------------------------------
void vtkPVData::ColorByPropertyInternal()
{
  vtkPVPart *part;
  int idx, num;

  num = this->GetPVSource()->GetNumberOfParts();
  for (idx = 0; idx < num; ++idx)
    {
    part = this->GetPVSource()->GetPart(idx);
    part->GetPartDisplay()->SetScalarVisibility(0);
    }

  double *color = this->ColorButton->GetColor();
  this->SetActorColor(color[0], color[1], color[2]);

  this->SetPVColorMap(NULL);

  this->MapScalarsCheckVisible = 0;
  this->EditColorMapButtonVisible = 0;
  this->ScalarBarCheckVisible = 0;
  this->InterpolateColorsCheckVisible = 0;
  this->CurrentColorAttributeIsFromCellData = 0;
  this->ColorButtonVisible = 1;
  this->UpdateEnableState();


  if (this->GetPVRenderView())
    {
    this->GetPVRenderView()->EventuallyRender();
    }
}


//----------------------------------------------------------------------------
// Select which point field to use for volume rendering
//
void vtkPVData::VolumeRenderPointField(const char *name)
{
  if (name == NULL)
    {
    return;
    }

  this->AddTraceEntry("$kw(%s) VolumeRenderPointField {%s}", 
                      this->GetTclName(), name);

  char *str;
  str = new char [strlen(name) + 16];
  sprintf(str, "Point %s", name);
  
  if ( !strcmp(this->VolumeScalarsMenu->GetValue(),str) )
    {
    // Update the transfer functions
    vtkPVPart *part;
    int idx, num;
    
    vtkPVDataInformation* dataInfo = this->GetPVSource()->GetDataInformation();
    vtkPVDataSetAttributesInformation *attrInfo = dataInfo->GetPointDataInformation();
    vtkPVArrayInformation *arrayInfo = attrInfo->GetArrayInformation(name);
    
    num = this->GetPVSource()->GetNumberOfParts();
    for (idx = 0; idx < num; ++idx)
      {
      part = this->GetPVSource()->GetPart(idx);
      part->GetPartDisplay()->ResetTransferFunctions(arrayInfo, dataInfo);
      }

    this->VolumeScalarsMenu->SetValue(str);
    this->VolumeRenderPointFieldInternal(name);
    }
  
  delete [] str;
}

//----------------------------------------------------------------------------
void vtkPVData::VolumeRenderPointFieldInternal(const char *name)
{
  int num, idx;
  vtkPVPart *part;

  num = this->GetPVSource()->GetNumberOfParts();
  for (idx = 0; idx < num; ++idx)
    {
    part = this->GetPVSource()->GetPart(idx);
    part->GetPartDisplay()->VolumeRenderPointField( name );
    } 

  if ( this->GetPVRenderView() )
    {
    this->GetPVRenderView()->EventuallyRender();
    }

}

//----------------------------------------------------------------------------
void vtkPVData::ColorByPointField(const char *name, int numComps)
{
  if (name == NULL)
    {
    return;
    }

  this->AddTraceEntry("$kw(%s) ColorByPointField {%s} %d", 
                      this->GetTclName(), name, numComps);

  this->ArraySetByUser = 1;
  
  char *str;
  str = new char [strlen(name) + 16];
  if (numComps == 1)
    {
    sprintf(str, "Point %s", name);
    }
  else
    {
    sprintf(str, "Point %s (%d)", name, numComps);
    }
  this->ColorMenu->SetValue(str);
  delete [] str;

  this->ColorByPointFieldInternal(name, numComps);
}

//----------------------------------------------------------------------------
void vtkPVData::ColorByPointFieldInternal(const char *name, int numComps)
{
  vtkPVApplication *pvApp = this->GetPVApplication();
  int num, idx;
  vtkPVPart *part;

  this->SetPVColorMap(pvApp->GetMainWindow()->GetPVColorMap(name, numComps));
  if (this->PVColorMap == NULL)
    {
    vtkErrorMacro("Could not get the color map.");
    return;
    }

  num = this->GetPVSource()->GetNumberOfParts();
  for (idx = 0; idx < num; ++idx)
    {
    part = this->GetPVSource()->GetPart(idx);
    part->GetPartDisplay()->ColorByArray(this->PVColorMap, VTK_POINT_DATA_FIELD);
    } 
  this->ColorButtonVisible = 0;
  this->InterpolateColorsCheckVisible = 1;
  this->CurrentColorAttributeIsFromCellData = 0;
  this->UpdateMapScalarsCheck(
           this->PVSource->GetDataInformation()->GetPointDataInformation(),
           name);
  this->UpdateEnableState();


  if ( this->GetPVRenderView() )
    {
    this->GetPVRenderView()->EventuallyRender();
    }

}

//----------------------------------------------------------------------------
void vtkPVData::ColorByCellField(const char *name, int numComps)
{
  if (name == NULL)
    {
    return;
    }

  this->AddTraceEntry("$kw(%s) ColorByCellField {%s} %d", 
                      this->GetTclName(), name, numComps);
  
  this->ArraySetByUser = 1;
  
  // Set the menu value.
  char *str;
  str = new char [strlen(name) + 16];
  if (numComps == 1)
    {
    sprintf(str, "Cell %s", name);
    }
  else
    {
    sprintf(str, "Cell %s (%d)", name, numComps);
    }
  this->ColorMenu->SetValue(str);
  delete [] str;

  this->ColorByCellFieldInternal(name, numComps);
}

//----------------------------------------------------------------------------
void vtkPVData::ColorByCellFieldInternal(const char *name, int numComps)
{
  vtkPVApplication *pvApp = this->GetPVApplication();
  vtkPVPart *part;
  int idx, num;

  this->SetPVColorMap(pvApp->GetMainWindow()->GetPVColorMap(name, numComps));
  if (this->PVColorMap == NULL)
    {
    vtkErrorMacro("Could not get the color map.");
    return;
    }

  num = this->GetPVSource()->GetNumberOfParts();
  for (idx = 0; idx < num; ++idx)
    {
    part = this->GetPVSource()->GetPart(idx);
    part->GetPartDisplay()->ColorByArray(this->PVColorMap, VTK_CELL_DATA_FIELD);
    }

  this->ColorButtonVisible = 0;
  this->InterpolateColorsCheckVisible = 0;
  // We have to remember that the current color attribute is from cell data.
  // InterpolateColors is not applicable for cell attriubtes we have
  // to know a cell attribute is current so that we do not enable the
  // interpolate button when map scalars is turned on.
  this->CurrentColorAttributeIsFromCellData = 1;


  this->UpdateMapScalarsCheck(
           this->PVSource->GetDataInformation()->GetCellDataInformation(),
           name);
  this->UpdateEnableState();

  if ( this->GetPVRenderView() )
    {
    this->GetPVRenderView()->EventuallyRender();
    }
}


//----------------------------------------------------------------------------
void vtkPVData::UpdateMapScalarsCheck(vtkPVDataSetAttributesInformation* info,
                                       const char* name)
{
  vtkPVArrayInformation* arrayInfo;
  
  arrayInfo = info->GetArrayInformation(name);
  
  // First set of conditions.
  if (arrayInfo == NULL || 
      arrayInfo->GetDataType() != VTK_UNSIGNED_CHAR)
    { // Direct mapping not an option.
    this->MapScalarsCheck->SetState(1);
    this->MapScalarsCheckVisible = 0;
    this->ScalarBarCheckVisible = 1;
    this->EditColorMapButtonVisible = 1;
    this->UpdateEnableState();

    // Tell all of the part displays to map scalars.
    int num, idx;
    num = this->PVSource->GetNumberOfParts();
    for (idx = 0; idx < num; ++idx)
      {
      this->PVSource->GetPart(idx)->GetPartDisplay()->SetDirectColorFlag(0);
      }
    return;
    }

  // Number of component restriction.
  if (arrayInfo->GetNumberOfComponents() != 3)
    { // I would like to have two as an option also ...
    // One component causes more trouble than it is worth.
    this->MapScalarsCheck->SetState(1);
    this->MapScalarsCheckVisible = 0;
    this->ScalarBarCheckVisible = 1;
    this->EditColorMapButtonVisible = 1;
    this->UpdateEnableState();
    // Tell all of the part displays to map scalars.
    int num, idx;
    num = this->PVSource->GetNumberOfParts();
    for (idx = 0; idx < num; ++idx)
      {
      this->PVSource->GetPart(idx)->GetPartDisplay()->SetDirectColorFlag(0);
      }
  
    return;
    }

  // Direct color map is an option.
  this->MapScalarsCheckVisible = 1;

  // I might like to store the default in another variable.
  if (this->MapScalarsCheck->GetState())
    {
    this->ScalarBarCheckVisible = 1;
    this->EditColorMapButtonVisible = 1;
    }
  else
    {
    this->ScalarBarCheckVisible = 0;
    this->EditColorMapButtonVisible = 0;
    this->InterpolateColorsCheckVisible = 0;
    }
  this->UpdateEnableState();    
}

//----------------------------------------------------------------------------
void vtkPVData::SetRepresentation(const char* repr)
{
  if ( vtkString::Equals(repr, VTK_PV_WIREFRAME_LABEL) )
    {
    this->DrawWireframe();
    }
  else if ( vtkString::Equals(repr, VTK_PV_SURFACE_LABEL) )
    {
    this->DrawSurface();
    }
  else if ( vtkString::Equals(repr, VTK_PV_POINTS_LABEL) )
    {
    this->DrawPoints();
    }
  else if ( vtkString::Equals(repr, VTK_PV_OUTLINE_LABEL) )
    {
    this->DrawOutline();
    }
  else if ( vtkString::Equals(repr, VTK_PV_VOLUME_LABEL) )
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
void vtkPVData::DrawWireframe()
{
  vtkPVApplication *pvApp = this->GetPVApplication();
  vtkPVProcessModule* pm = pvApp->GetProcessModule();
  vtkPVPart *part;
  int idx, num;
  
  if (this->GetPVSource()->GetInitialized())
    {
    this->AddTraceEntry("$kw(%s) DrawWireframe", this->GetTclName());
    }
  this->RepresentationMenu->SetValue(VTK_PV_WIREFRAME_LABEL);
  this->VolumeRenderModeOff();
  
  // !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
  // We could move the property into vtkPVData so parts would share one object.
  // Well, this would complicate initialization.  We would have to pass
  // the property into the part for the mappers.
  // !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
  num = this->GetPVSource()->GetNumberOfParts();
  for (idx = 0; idx < num; ++idx)
    {
    part = this->GetPVSource()->GetPart(idx);
    part->GetPartDisplay()->VolumeRenderModeOff();

    if (part->GetPartDisplay()->GetPropertyID().ID != 0)
      {
      if (this->PreviousWasSolid)
        {
        this->PreviousAmbient = 
          part->GetPartDisplay()->GetProperty()->GetAmbient();
        this->PreviousDiffuse = 
          part->GetPartDisplay()->GetProperty()->GetDiffuse();
        this->PreviousSpecular = 
          part->GetPartDisplay()->GetProperty()->GetSpecular();
        }
      this->PreviousWasSolid = 0;
      pm->GetStream() 
        << vtkClientServerStream::Invoke 
        << part->GetPartDisplay()->GetPropertyID()
        << "SetAmbient" << 1 << vtkClientServerStream::End;
      pm->GetStream() 
        << vtkClientServerStream::Invoke 
        << part->GetPartDisplay()->GetPropertyID()
        << "SetDiffuse" << 0 << vtkClientServerStream::End;
      pm->GetStream() 
        << vtkClientServerStream::Invoke 
        << part->GetPartDisplay()->GetPropertyID()
        << "SetSpecular" << 0 << vtkClientServerStream::End;
      pm->GetStream() 
        << vtkClientServerStream::Invoke 
        << part->GetPartDisplay()->GetPropertyID()
        << "SetRepresentationToWireframe" << vtkClientServerStream::End;
      pm->SendStream(vtkProcessModule::CLIENT|vtkProcessModule::RENDER_SERVER);
      }
    if (part->GetPartDisplay()->GetGeometryID().ID != 0)
      {
       pm->GetStream() 
         << vtkClientServerStream::Invoke 
         << part->GetPartDisplay()->GetGeometryID()
         << "GetUseOutline" << vtkClientServerStream::End;
       pm->SendStream(vtkProcessModule::CLIENT);
       int useOutline;
       pm->GetLastClientResult().GetArgument(0, 0, &useOutline);
       if (useOutline)
         {
         pm->GetStream() 
           << vtkClientServerStream::Invoke 
           << part->GetPartDisplay()->GetGeometryID()
           << "SetUseOutline" << 0 << vtkClientServerStream::End;
         pm->SendStream(vtkProcessModule::CLIENT|vtkProcessModule::DATA_SERVER);
         part->GetPartDisplay()->InvalidateGeometry();
         }
      }
    }

  if ( this->GetPVRenderView() )
    {
    this->GetPVRenderView()->EventuallyRender();
    }
}

//----------------------------------------------------------------------------
void vtkPVData::DrawPoints()
{
  vtkPVApplication *pvApp = this->GetPVApplication();
  vtkPVProcessModule* pm = pvApp->GetProcessModule();
  vtkPVPart *part;
  int idx, num;

  if (this->GetPVSource()->GetInitialized())
    {
    this->AddTraceEntry("$kw(%s) DrawPoints", this->GetTclName());
    }
  this->RepresentationMenu->SetValue(VTK_PV_POINTS_LABEL);
  this->VolumeRenderModeOff();
  
  num = this->GetPVSource()->GetNumberOfParts();
  for (idx = 0; idx < num; ++idx)
    {
    part = this->GetPVSource()->GetPart(idx);
    part->GetPartDisplay()->VolumeRenderModeOff();

    if (part->GetPartDisplay()->GetProperty())
      {
      if (this->PreviousWasSolid)
        {
        this->PreviousAmbient = 
          part->GetPartDisplay()->GetProperty()->GetAmbient();
        this->PreviousDiffuse = 
          part->GetPartDisplay()->GetProperty()->GetDiffuse();
        this->PreviousSpecular = 
          part->GetPartDisplay()->GetProperty()->GetSpecular();
        }
      this->PreviousWasSolid = 0;
      pm->GetStream() 
        << vtkClientServerStream::Invoke
        << part->GetPartDisplay()->GetPropertyID()
        << "SetAmbient" << 1 << vtkClientServerStream::End;
      pm->GetStream() 
        << vtkClientServerStream::Invoke 
        << part->GetPartDisplay()->GetPropertyID()
        << "SetDiffuse" << 0 << vtkClientServerStream::End;
      pm->GetStream() 
        << vtkClientServerStream::Invoke 
        << part->GetPartDisplay()->GetPropertyID()
        << "SetSpecular" << 0 << vtkClientServerStream::End;
      pm->GetStream() 
        << vtkClientServerStream::Invoke 
        << part->GetPartDisplay()->GetPropertyID()
        << "SetRepresentationToPoints" << vtkClientServerStream::End;
      pm->SendStream(vtkProcessModule::CLIENT|vtkProcessModule::RENDER_SERVER);
      }
    if (part->GetPartDisplay()->GetGeometryID().ID != 0)
      {
      pm->GetStream() 
        << vtkClientServerStream::Invoke 
        << part->GetPartDisplay()->GetGeometryID()
        << "GetUseOutline" << vtkClientServerStream::End;
      pm->SendStream(vtkProcessModule::CLIENT);
      int useOutline;
      pm->GetLastClientResult().GetArgument(0, 0, &useOutline);
      if (useOutline)
        { 
        pm->GetStream() 
          << vtkClientServerStream::Invoke 
          << part->GetPartDisplay()->GetGeometryID()
          << "SetUseOutline" << 0 << vtkClientServerStream::End;
        pm->SendStream(vtkProcessModule::CLIENT|vtkProcessModule::DATA_SERVER);
        part->GetPartDisplay()->InvalidateGeometry();
        }
      }
    }
  
  if ( this->GetPVRenderView() )
    {
    this->GetPVRenderView()->EventuallyRender();
    }
}

//----------------------------------------------------------------------------
void vtkPVData::DrawVolume()
{
  vtkPVPart *part;
  int idx, num;

  if (this->GetPVSource()->GetInitialized())
    {
    this->AddTraceEntry("$kw(%s) DrawVolume", this->GetTclName());
    }
  this->RepresentationMenu->SetValue(VTK_PV_VOLUME_LABEL);
  this->VolumeRenderModeOn();
  
  num = this->GetPVSource()->GetNumberOfParts();
  for (idx = 0; idx < num; ++idx)
    {
    part = this->GetPVSource()->GetPart(idx);
    part->GetPartDisplay()->VolumeRenderModeOn();
    }
  
  if ( this->GetPVRenderView() )
    {
    this->GetPVRenderView()->EventuallyRender();
    }
}

//----------------------------------------------------------------------------
void vtkPVData::DrawSurface()
{
  vtkPVApplication *pvApp = this->GetPVApplication();
  vtkPVProcessModule* pm = pvApp->GetProcessModule();

  vtkPVPart *part;
  int num, idx;
  
  if (this->GetPVSource()->GetInitialized())
    {
    this->AddTraceEntry("$kw(%s) DrawSurface", this->GetTclName());
    }
  this->RepresentationMenu->SetValue(VTK_PV_SURFACE_LABEL);
  this->VolumeRenderModeOff();
  
  // fixme
  // It would be better to loop over part displays from the render module.

  num = this->GetPVSource()->GetNumberOfParts();
  for (idx = 0; idx < num; ++idx)
    {
    part = this->GetPVSource()->GetPart(idx);
    part->GetPartDisplay()->VolumeRenderModeOff();

    if (part->GetPartDisplay()->GetProperty())
      {
      if (!this->PreviousWasSolid)
        {
        pm->GetStream() 
          << vtkClientServerStream::Invoke 
          << part->GetPartDisplay()->GetPropertyID()
          << "SetAmbient" << this->PreviousAmbient 
          << vtkClientServerStream::End;
        pm->GetStream() 
          << vtkClientServerStream::Invoke 
          << part->GetPartDisplay()->GetPropertyID()
          << "SetDiffuse" << this->PreviousDiffuse 
          << vtkClientServerStream::End;
        pm->GetStream() 
          << vtkClientServerStream::Invoke
          << part->GetPartDisplay()->GetPropertyID()
          << "SetSpecular" << this->PreviousSpecular 
          << vtkClientServerStream::End;
        pm->GetStream() 
          << vtkClientServerStream::Invoke 
          << part->GetPartDisplay()->GetPropertyID()
          << "SetRepresentationToSurface" 
          << vtkClientServerStream::End;
        pm->SendStream(vtkProcessModule::CLIENT|vtkProcessModule::RENDER_SERVER);
        }
      }
    if (part->GetPartDisplay()->GetGeometryID().ID != 0)
      {
      pm->GetStream() 
        << vtkClientServerStream::Invoke 
        << part->GetPartDisplay()->GetGeometryID()
        << "GetUseOutline" << vtkClientServerStream::End;
      pm->SendStream(vtkProcessModule::CLIENT);
      int useOutline;
      pm->GetLastClientResult().GetArgument(0, 0, &useOutline);
      if (useOutline)
        { 
        pm->GetStream() 
          << vtkClientServerStream::Invoke
          << part->GetPartDisplay()->GetGeometryID()
          << "SetUseOutline" << 0 << vtkClientServerStream::End;
        pm->SendStream(vtkProcessModule::CLIENT|vtkProcessModule::DATA_SERVER);
        part->GetPartDisplay()->InvalidateGeometry();
        }
      }
    }
  this->PreviousWasSolid = 1;

  if ( this->GetPVRenderView() )
    {
    this->GetPVRenderView()->EventuallyRender();
    }
}

//----------------------------------------------------------------------------
void vtkPVData::DrawOutline()
{
  vtkPVApplication *pvApp = this->GetPVApplication();
  vtkPVProcessModule* pm = pvApp->GetProcessModule();

  vtkPVPart *part;
  int num, idx;
  
  if (this->GetPVSource()->GetInitialized())
    {
    this->AddTraceEntry("$kw(%s) DrawOutline", this->GetTclName());
    }
  this->RepresentationMenu->SetValue(VTK_PV_OUTLINE_LABEL);
  this->VolumeRenderModeOff();
  
  num = this->GetPVSource()->GetNumberOfParts();
  for (idx = 0; idx < num; ++idx)
    {
    part = this->GetPVSource()->GetPart(idx);
    part->GetPartDisplay()->VolumeRenderModeOff();

    if (part->GetPartDisplay()->GetProperty())
      {
      if (this->PreviousWasSolid)
        {
        this->PreviousAmbient = 
          part->GetPartDisplay()->GetProperty()->GetAmbient();
        this->PreviousDiffuse = 
          part->GetPartDisplay()->GetProperty()->GetDiffuse();
        this->PreviousSpecular = 
          part->GetPartDisplay()->GetProperty()->GetSpecular();
        }
      this->PreviousWasSolid = 0; 
      pm->GetStream() 
        << vtkClientServerStream::Invoke 
        << part->GetPartDisplay()->GetPropertyID()
        << "SetAmbient" << 1 << vtkClientServerStream::End;
      pm->GetStream() 
        << vtkClientServerStream::Invoke 
        << part->GetPartDisplay()->GetPropertyID()
        << "SetDiffuse" << 0 << vtkClientServerStream::End;
      pm->GetStream() 
        << vtkClientServerStream::Invoke 
        << part->GetPartDisplay()->GetPropertyID()
        << "SetSpecular" << 0 << vtkClientServerStream::End;
      pm->GetStream() 
        << vtkClientServerStream::Invoke 
        << part->GetPartDisplay()->GetPropertyID()
        << "SetRepresentationToSurface" << vtkClientServerStream::End;
      pm->SendStream(vtkProcessModule::CLIENT|vtkProcessModule::RENDER_SERVER);
      }
    if (part->GetPartDisplay()->GetGeometryID().ID != 0)
      { 
      pm->GetStream() 
        << vtkClientServerStream::Invoke 
        <<  part->GetPartDisplay()->GetGeometryID()
        << "SetUseOutline" << 1 << vtkClientServerStream::End;
      pm->SendStream(vtkProcessModule::CLIENT|vtkProcessModule::DATA_SERVER);
      part->GetPartDisplay()->InvalidateGeometry();
      }
    }

  if ( this->GetPVRenderView() )
    {
    this->GetPVRenderView()->EventuallyRender();
    }
}


//----------------------------------------------------------------------------
// Change visibility / enabled state of widgets for actor properties
//
void vtkPVData::VolumeRenderModeOff()
{
  this->Script("pack forget %s", 
               this->VolumeAppearanceFrame->GetWidgetName() );
  this->Script("pack forget %s", 
               this->ColorFrame->GetWidgetName() );

  this->Script("pack %s -after %s -fill x -expand t -pady 2", 
               this->ColorFrame->GetWidgetName(),
               this->ViewFrame->GetWidgetName() );
  
  this->VolumeRenderMode = 0;
  this->UpdateEnableState();
}

//----------------------------------------------------------------------------
// Change visibility / enabled state of widgets for volume properties
//
void vtkPVData::VolumeRenderModeOn()
{
  this->Script("pack forget %s", 
               this->VolumeAppearanceFrame->GetWidgetName() );
  this->Script("pack forget %s", 
               this->ColorFrame->GetWidgetName() );
  
  this->Script("pack %s -after %s -fill x -expand t -pady 2", 
               this->VolumeAppearanceFrame->GetWidgetName(),
               this->ViewFrame->GetWidgetName() );
  
  this->VolumeRenderMode = 1;
  this->UpdateEnableState();
}

  
//----------------------------------------------------------------------------
void vtkPVData::SetInterpolation(const char* repr)
{
  if ( vtkString::Equals(repr, "Flat") )
    {
    this->SetInterpolationToFlat();
    }
  else if ( vtkString::Equals(repr, "Gouraud") )
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
void vtkPVData::SetInterpolationToFlat()
{
  vtkPVApplication *pvApp = this->GetPVApplication();
  vtkPVProcessModule* pm = pvApp->GetProcessModule();

  vtkPVPart *part;
  int idx, num;
  
  this->AddTraceEntry("$kw(%s) SetInterpolationToFlat", 
                      this->GetTclName());
  this->InterpolationMenu->SetValue("Flat");

  num = this->GetPVSource()->GetNumberOfParts();
  for (idx = 0; idx < num; ++idx)
    {
    part = this->GetPVSource()->GetPart(idx);
    if (part->GetPartDisplay()->GetPropertyID().ID != 0)
      { 
      pm->GetStream() 
        << vtkClientServerStream::Invoke 
        << part->GetPartDisplay()->GetPropertyID()
        << "SetInterpolationToFlat" << vtkClientServerStream::End;
      pm->SendStream(vtkProcessModule::CLIENT|vtkProcessModule::RENDER_SERVER);
      }
    if ( this->GetPVRenderView() )
      {
      this->GetPVRenderView()->EventuallyRender();
      }
    }
}


//----------------------------------------------------------------------------
void vtkPVData::SetInterpolationToGouraud()
{
  vtkPVApplication *pvApp = this->GetPVApplication();
  vtkPVProcessModule* pm = pvApp->GetProcessModule();
  vtkPVPart *part;
  int num, idx;
  
  this->AddTraceEntry("$kw(%s) SetInterpolationToGouraud", 
                      this->GetTclName());
  this->InterpolationMenu->SetValue("Gouraud");

  num = this->GetPVSource()->GetNumberOfParts();
  for (idx = 0; idx < num; ++idx)
    {
    part = this->GetPVSource()->GetPart(idx);
      if (part->GetPartDisplay()->GetPropertyID().ID != 0)
      { 
      pm->GetStream() 
        << vtkClientServerStream::Invoke 
        << part->GetPartDisplay()->GetPropertyID()
        << "SetInterpolationToGouraud" << vtkClientServerStream::End;
      pm->SendStream(vtkProcessModule::CLIENT|vtkProcessModule::RENDER_SERVER);
      }
    }
  
  if ( this->GetPVRenderView() )
    {
    this->GetPVRenderView()->EventuallyRender();
    }
}



//----------------------------------------------------------------------------
void vtkPVData::AmbientChanged()
{
  // This doesn't currently need to do anything since we aren't actually
  //packing the ambient scale.
/*  vtkPVApplication *pvApp = this->GetPVApplication();
  float ambient = this->AmbientScale->GetValue();
  
  //pvApp->BroadcastScript("%s SetAmbient %f", this->GetTclName(), ambient);

  
  this->SetAmbient(ambient);
  if ( this->GetPVRenderView() )
    {
    this->GetPVRenderView()->EventuallyRender();
    }
*/
}

//----------------------------------------------------------------------------
void vtkPVData::SetAmbient(double ambient)
{
  vtkPVPart *part;
  int num, idx;

  num = this->GetPVSource()->GetNumberOfParts();
  for (idx = 0; idx < num; ++idx)
    {
    part = this->GetPVSource()->GetPart(idx);
    part->GetPartDisplay()->GetProperty()->SetAmbient(ambient);
    }
}


//----------------------------------------------------------------------------
// Set up the default UI values.  Data information is valid by this time.
void vtkPVData::Initialize()
{
  vtkPVApplication *pvApp = this->GetPVApplication();
  double bounds[6];

  vtkDebugMacro( << "Initialize --------")
  
  this->GetPVSource()->GetDataInformation()->GetBounds(bounds);

  vtkRenderer* ren = pvApp->GetRenderModule()->GetRenderer();
  this->CubeAxes = vtkCubeAxesActor2D::New();
  this->CubeAxes->SetFlyModeToOuterEdges();
  this->CubeAxes->GetProperty()->SetColor(1,1,1);
  this->CubeAxes->SetBounds(bounds[0], bounds[1], bounds[2], 
                            bounds[3], bounds[4], bounds[5]);
  this->CubeAxes->SetCamera(ren->GetActiveCamera());
  this->CubeAxes->SetInertia(20);

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
    }
}


//----------------------------------------------------------------------------
void vtkPVData::CenterCamera()
{
  vtkPVApplication* pvApp = this->GetPVApplication();
  vtkRenderer* ren = pvApp->GetRenderModule()->GetRenderer();

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
void vtkPVData::VisibilityCheckCallback()
{
  this->GetPVSource()->SetVisibility(this->VisibilityCheck->GetState());
  if ( this->GetPVRenderView() )
    {
    this->GetPVRenderView()->UpdateNavigationWindow(this->GetPVSource(), 0);
    this->GetPVRenderView()->EventuallyRender();
    }
}

//----------------------------------------------------------------------------
void vtkPVData::SetVisibility(int v)
{
  if ( !this->GetPVSource() )
    {
    return;
    }
  this->AddTraceEntry("$kw(%s) SetVisibility %d", this->GetTclName(), v);
  this->GetPVSource()->SetVisibilityInternal(v);
  this->CubeAxes->SetVisibility(v);
  this->SetPointLabelVisibility(v, 0);
}

//----------------------------------------------------------------------------
vtkPVRenderView* vtkPVData::GetPVRenderView()
{
  if ( !this->GetPVSource() )
    {
    return 0;
    }
  return this->GetPVSource()->GetPVRenderView();
}



//----------------------------------------------------------------------------
vtkPVApplication* vtkPVData::GetPVApplication()
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
void vtkPVData::SetScalarBarVisibility(int val)
{
  if (this->PVColorMap)
    {
    this->PVColorMap->SetScalarBarVisibility(val);
    }
  
  if (this->ScalarBarCheck->GetState() != val)
    {
    this->ScalarBarCheck->SetState(val);
    }
}


// This should really be a part of vtkPVPartDisplay object.
//----------------------------------------------------------------------------
void vtkPVData::SetCubeAxesVisibility(int val)
{
  
  if (!this->GetPVRenderView())
    {
    return;
    }
  
  vtkRenderer* ren = this->GetPVRenderView()->GetRenderer();
  
  if (ren == NULL)
    {
    return;
    }
  
  if (this->CubeAxesCheck->GetState() != val)
    {
    this->AddTraceEntry("$kw(%s) SetCubeAxesVisibility %d", this->GetTclName(), val);
    this->CubeAxesCheck->SetState(val);
    }

  // I am going to add and remove it from the renderer instead of using visibility.
  // Composites should really have multiple props.
  
  if (ren)
    {
    if (val)
      {
      ren->AddProp(this->CubeAxes);
      }
    else
      {
      ren->RemoveProp(this->CubeAxes);
      }
    }
}

//----------------------------------------------------------------------------
void vtkPVData::SetPointLabelVisibility(int val, int changeButtonState)
{
  int state = this->PointLabelCheck->GetState();
  
  if (state != val && changeButtonState)
    {
    this->AddTraceEntry("$kw(%s) SetPointLabelVisibility %d %d",
                        this->GetTclName(), val, changeButtonState);
    this->PointLabelCheck->SetState(val);
    state = val;
    }
  
  if ((val && !state) || (val && !this->Visibility))
    {
    return;
    }
  
  int numParts = this->GetPVSource()->GetNumberOfParts();
  int i;
  vtkPVPart *part;
  vtkPVLODPartDisplay *display;
  
  for (i = 0; i < numParts; i++)
    {
    part =  this->GetPVSource()->GetPart(i);
    display = vtkPVLODPartDisplay::SafeDownCast(part->GetPartDisplay());
    if (display)
      {
      display->SetPointLabelVisibility(val);
      }
    }
  
  if (this->GetPVRenderView())
    {
    this->GetPVRenderView()->EventuallyRender();
    }
}

//----------------------------------------------------------------------------
void vtkPVData::ScalarBarCheckCallback()
{
  this->SetScalarBarVisibility(this->ScalarBarCheck->GetState());
  if ( this->GetPVRenderView() )
    {
    this->GetPVRenderView()->EventuallyRender();
    }
}

//----------------------------------------------------------------------------
void vtkPVData::CubeAxesCheckCallback()
{
  this->AddTraceEntry("$kw(%s) SetCubeAxesVisibility %d", this->GetTclName(),
                      this->CubeAxesCheck->GetState());
  this->SetCubeAxesVisibility(this->CubeAxesCheck->GetState());
  if ( this->GetPVRenderView() )
    {
    this->GetPVRenderView()->EventuallyRender();
    }
}

//----------------------------------------------------------------------------
void vtkPVData::PointLabelCheckCallback()
{
  this->AddTraceEntry("$kw(%s) SetPointLabelVisibility %d 1",
                      this->GetTclName(),
                      this->PointLabelCheck->GetState());
  this->SetPointLabelVisibility(this->PointLabelCheck->GetState());
  if ( this->GetPVRenderView() )
    {
    this->GetPVRenderView()->EventuallyRender();
    }
}

//----------------------------------------------------------------------------
void vtkPVData::MapScalarsCheckCallback()
{
  this->SetMapScalarsFlag(this->MapScalarsCheck->GetState());
  if ( this->GetPVRenderView() )
    {
    this->GetPVRenderView()->EventuallyRender();
    }
}

//----------------------------------------------------------------------------
void vtkPVData::SetMapScalarsFlag(int val)
{
  this->AddTraceEntry("$kw(%s) SetMapScalarsFlag %d", this->GetTclName(), val);
  if (this->MapScalarsCheck->GetState() != val)
    {
    this->MapScalarsCheck->SetState(val);
    }

  if (val)
    {
    this->EditColorMapButtonVisible = 1;
    this->ScalarBarCheckVisible = 1;
    this->InterpolateColorsCheckVisible = 1;
    if (this->CurrentColorAttributeIsFromCellData)
      {
      this->InterpolateColorsCheckVisible = 0;
      }
    }
  else
    {
    this->EditColorMapButtonVisible = 0;
    this->ScalarBarCheckVisible = 0;
    this->InterpolateColorsCheckVisible = 0;
    }
  this->UpdateEnableState();

  int num, idx;
  num = this->PVSource->GetNumberOfParts();
  for (idx = 0; idx < num; ++idx)
    {
    this->PVSource->GetPart(idx)->GetPartDisplay()->SetDirectColorFlag(!val);
    }
}

//----------------------------------------------------------------------------
void vtkPVData::InterpolateColorsCheckCallback()
{
  this->SetInterpolateColorsFlag(this->InterpolateColorsCheck->GetState());
  if ( this->GetPVRenderView() )
    {
    this->GetPVRenderView()->EventuallyRender();
    }
}

//----------------------------------------------------------------------------
void vtkPVData::SetInterpolateColorsFlag(int val)
{
  this->AddTraceEntry("$kw(%s) SetInterpolateColorsFlag %d", this->GetTclName(), val);
  if (this->InterpolateColorsCheck->GetState() != val)
    {
    this->InterpolateColorsCheck->SetState(val);
    }

  int num, idx;
  num = this->PVSource->GetNumberOfParts();
  for (idx = 0; idx < num; ++idx)
    {
    this->PVSource->GetPart(idx)->GetPartDisplay()->SetInterpolateColorsFlag(val);
    }
}

//----------------------------------------------------------------------------
void vtkPVData::SetPointSize(int size)
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
void vtkPVData::ChangePointSize()
{
  vtkPVApplication *pvApp = this->GetPVApplication();
  vtkPVProcessModule* pm = pvApp->GetProcessModule();
  vtkPVPart *part;
  int idx, num;
  
  num = this->GetPVSource()->GetNumberOfParts();
  for (idx = 0; idx < num; ++idx)
    {
    part = this->GetPVSource()->GetPart(idx);
     if (part->GetPartDisplay()->GetPropertyID().ID != 0)
      { 
      pm->GetStream() 
        << vtkClientServerStream::Invoke 
        << part->GetPartDisplay()->GetPropertyID()
        << "SetPointSize" << this->PointSizeThumbWheel->GetValue()
        << vtkClientServerStream::End;
      pm->SendStream(vtkProcessModule::CLIENT|vtkProcessModule::RENDER_SERVER);
      }
    }
 
  if ( this->GetPVRenderView() )
    {
    this->GetPVRenderView()->EventuallyRender();
    }
} 

//----------------------------------------------------------------------------
void vtkPVData::ChangePointSizeEndCallback()
{
  this->ChangePointSize();
  this->AddTraceEntry("$kw(%s) SetPointSize %d", this->GetTclName(),
                      (int)(this->PointSizeThumbWheel->GetValue()));
} 

//----------------------------------------------------------------------------
void vtkPVData::SetLineWidth(int width)
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
void vtkPVData::ChangeLineWidth()
{
  vtkPVApplication *pvApp = this->GetPVApplication();
  vtkPVProcessModule* pm = pvApp->GetProcessModule();
  vtkPVPart *part;
  int idx, num;
  
  num = this->GetPVSource()->GetNumberOfParts();
  for (idx = 0; idx < num; ++idx)
    {
    part = this->GetPVSource()->GetPart(idx);
      if (part->GetPartDisplay()->GetPropertyID().ID != 0)
      { 
      pm->GetStream() 
        << vtkClientServerStream::Invoke 
        << part->GetPartDisplay()->GetPropertyID()
        << "SetLineWidth" << this->LineWidthThumbWheel->GetValue()
        << vtkClientServerStream::End;
      pm->SendStream(vtkProcessModule::CLIENT|vtkProcessModule::RENDER_SERVER);
      }
    }

  if ( this->GetPVRenderView() )
    {
    this->GetPVRenderView()->EventuallyRender();
    }
}

//----------------------------------------------------------------------------
void vtkPVData::ChangeLineWidthEndCallback()
{
  this->ChangeLineWidth();
  this->AddTraceEntry("$kw(%s) SetLineWidth %d", this->GetTclName(),
                      (int)(this->LineWidthThumbWheel->GetValue()));
}

//----------------------------------------------------------------------------
void vtkPVData::SetPropertiesParent(vtkKWWidget *parent)
{
  if (this->PropertiesParent == parent)
    {
    return;
    }
  if (this->PropertiesParent)
    {
    vtkErrorMacro("Cannot reparent properties.");
    return;
    }
  this->PropertiesParent = parent;
  parent->Register(this);
}


//----------------------------------------------------------------------------
void vtkPVData::SaveInBatchScript(ofstream *file)
{
  if (this->GetVisibility())
    {
    if (this->PVColorMap)
      {
      this->PVColorMap->SaveInBatchScript(file);
      }
    vtkPVPart *part = this->GetPVSource()->GetPart(0);
    vtkPVPartDisplay *partD = part->GetPartDisplay();
    *file << endl;
    *file << "set pvTemp" <<  partD->GetGeometryID()
          << " [$proxyManager NewProxy rendering DefaultDisplayer]"
          << endl;
    *file << "  $proxyManager RegisterProxy rendering pvTemp"
          << partD->GetGeometryID() << " $pvTemp" << partD->GetGeometryID() 
          << endl;
    *file << "  $pvTemp" << partD->GetGeometryID() << " UnRegister {}" << endl;

    *file << "  [$pvTemp" <<  partD->GetGeometryID() << " GetProperty Input] "
          << " AddProxy $pvTemp" << this->GetPVSource()->GetVTKSourceID(0)
          << endl;

    *file << "  [$Ren1 GetProperty Displayers] AddProxy $pvTemp" << partD->GetGeometryID() << endl;
    
    if ( vtkString::Equals(this->RepresentationMenu->GetValue(),
                           VTK_PV_OUTLINE_LABEL) )
      {
      *file << "  [$pvTemp" << partD->GetGeometryID() 
            << " GetProperty DisplayAsOutline] SetElements1 1"  << endl;
      }
    else
      {
      *file << "  [$pvTemp" << partD->GetGeometryID() 
            << " GetProperty DisplayAsOutline] SetElements1 0"  << endl;
      }
    
    *file << "  [$pvTemp" << partD->GetGeometryID() 
          << " GetProperty ImmediateModeRendering] SetElements1 "  
          << part->GetPartDisplay()->GetMapper()->GetImmediateModeRendering()
          << endl;
    
    *file << "  [$pvTemp" << partD->GetGeometryID() 
          << " GetProperty ScalarVisibility] SetElements1 "  
          << part->GetPartDisplay()->GetMapper()->GetScalarVisibility()
          << endl;

    *file << "  [$pvTemp" << partD->GetGeometryID() 
          << " GetProperty ColorMode] SetElements1 "  
          << part->GetPartDisplay()->GetMapper()->GetColorMode()
          << endl;

    *file << "  [$pvTemp" << partD->GetGeometryID() 
          << " GetProperty InterpolateColorsBeforeMapping] SetElements1 "  
          << part->GetPartDisplay()->GetMapper()->GetInterpolateScalarsBeforeMapping()
          << endl;
    
    *file << "  [$pvTemp" << partD->GetGeometryID() 
          << " GetProperty Representation] SetElements1 "  
          << part->GetPartDisplay()->GetProperty()->GetRepresentation()
          << endl;
    
    *file << "  [$pvTemp" << partD->GetGeometryID() 
          << " GetProperty Interpolation] SetElements1 "  
          << part->GetPartDisplay()->GetProperty()->GetInterpolation()
          << endl;
    
    *file << "  [$pvTemp" << partD->GetGeometryID() 
          << " GetProperty LineWidth] SetElements1 "  
          << part->GetPartDisplay()->GetProperty()->GetLineWidth()
          << endl;
    
    *file << "  [$pvTemp" << partD->GetGeometryID() 
          << " GetProperty PointSize] SetElements1 "  
          << part->GetPartDisplay()->GetProperty()->GetPointSize()
          << endl;

    *file << "  [$pvTemp" << partD->GetGeometryID() 
          << " GetProperty Position] SetElements3 "  
          << this->TranslateThumbWheel[0]->GetValue() << " "
          << this->TranslateThumbWheel[1]->GetValue() << " "
          << this->TranslateThumbWheel[2]->GetValue() << " "
          << endl;

    *file << "  [$pvTemp" << partD->GetGeometryID() 
          << " GetProperty Scale] SetElements3 "  
          << this->ScaleThumbWheel[0]->GetValue() << " "
          << this->ScaleThumbWheel[1]->GetValue() << " "
          << this->ScaleThumbWheel[2]->GetValue() << " "
          << endl;

    *file << "  [$pvTemp" << partD->GetGeometryID() 
          << " GetProperty Orientation] SetElements3 "  
          << this->OrientationScale[0]->GetValue() << " "
          << this->OrientationScale[1]->GetValue() << " "
          << this->OrientationScale[2]->GetValue() << " "
          << endl;

    *file << "  [$pvTemp" << partD->GetGeometryID() 
          << " GetProperty Origin] SetElements3 "  
          << this->OriginThumbWheel[0]->GetValue() << " "
          << this->OriginThumbWheel[1]->GetValue() << " "
          << this->OriginThumbWheel[2]->GetValue() << " "
          << endl;

    *file << "  [$pvTemp" << partD->GetGeometryID() 
          << " GetProperty Opacity] SetElements1 "  
          << this->OpacityScale->GetValue() << endl;
    
    double propColor[3];
    part->GetPartDisplay()->GetProperty()->GetDiffuseColor(propColor);
    *file << "  [$pvTemp" << partD->GetGeometryID() 
          << " GetProperty Color] SetElements3 "  
          << propColor[0] << " " << propColor[1] << " " << propColor[2]
          << endl;

    if (this->PVColorMap)
      {
      *file << "  [$pvTemp" << partD->GetGeometryID() 
            << " GetProperty LookupTable] AddProxy $pvTemp" 
            << this->PVColorMap->GetLookupTableID() << endl;
      }
    *file << "  [$pvTemp" << partD->GetGeometryID() 
          << " GetProperty ScalarMode] SetElement 0 " 
          << part->GetPartDisplay()->GetMapper()->GetScalarMode()
          << endl;

    *file << "  [$pvTemp" << partD->GetGeometryID() 
          << " GetProperty ColorArray] SetElement 0 {" 
          << part->GetPartDisplay()->GetMapper()->GetArrayName()
          << "}" << endl;

    *file << "  $pvTemp" << partD->GetGeometryID() << " UpdateVTKObjects" 
          << endl;

    }

  // TODO implement this

//       part->GetPartDisplay()->GetMapper()->GetScalarRange(range);
//       *file << "pvTemp" << part->GetPartDisplay()->GetMapperID() << " SetScalarRange "
//             << range[0] << " " << range[1] << "\n\t";
//       *file << "pvTemp" << part->GetPartDisplay()->GetMapperID() << " UseLookupTableScalarRangeOn\n\t";
//             << "pvTemp" << part->GetPartDisplay()->GetMapperID() << " SetScalarModeTo";
//       scalarMode = part->GetPartDisplay()->GetMapper()->GetScalarModeAsString();
//       *file << scalarMode << "\n";
//       if (strcmp(scalarMode, "UsePointFieldData") == 0 ||
//           strcmp(scalarMode, "UseCellFieldData") == 0)
//         {
//         *file << "\t" << "pvTemp" << part->GetPartDisplay()->GetMapperID() << " SelectColorArray {"
//               << part->GetPartDisplay()->GetMapper()->GetArrayName() << "}\n";
//         }
//   if (this->CubeAxesCheck->GetState())
//     {
//     char cubeAxesBatchName[128];
//     double bounds[6];
//     this->CubeAxes->GetBounds(bounds);
//     sprintf(cubeAxesBatchName, "cubeAxes%d", this->InstanceCount);
//     *file << "vtkCubeAxesActor2D " << cubeAxesBatchName << "\n\t"
//           << cubeAxesBatchName << " SetFlyModeToOuterEdges\n\t"
//           << "[" << cubeAxesBatchName << " GetProperty] SetColor 1 1 1\n\t"
//           << cubeAxesBatchName << " SetBounds "
//           << bounds[0] << " " << bounds[1] << " " << bounds[2] << " "
//           << bounds[3] << " " << bounds[4] << " " << bounds[5] << "\n\t";
//     *file << cubeAxesBatchName << " SetCamera [";
//     *file << "pvTemp" << renID.ID << " GetActiveCamera]\n\t"
//           << cubeAxesBatchName << " SetInertia 20\n";
//     *file << "pvTemp" << renID.ID << " AddProp " << cubeAxesBatchName << "\n";
//     }
  
//   if (this->PointLabelCheck->GetState())
//     {
//     }
}

//----------------------------------------------------------------------------
void vtkPVData::SaveState(ofstream *file)
{
  double *pf;
  float f1, f2, f3;
  int   i1;

  *file << "set kw(" << this->GetTclName() << ") [$kw(" 
        << this->PVSource->GetTclName() << ") GetPVOutput]\n";

  pf = this->ColorButton->GetColor();
  if (pf[0] != 1.0 || pf[1] != 1.0 || pf[2] != 1.0)
    {
    *file << "$kw(" << this->GetTclName() << ") ChangeActorColor " << pf[0]
          << " " << pf[1] << " " << pf[2] << endl;
    }

  if (strcmp (this->ColorMenu->GetValue(),"Property") != 0)
    {
    const char* name = this->PVColorMap->GetArrayName();
    int comps = this->PVColorMap->GetNumberOfVectorComponents();
    if (strncmp (this->ColorMenu->GetValue(),"Point",5) == 0)
      {
      *file << "$kw(" << this->GetTclName() << ") ColorByPointField {" 
            << name << "} " << comps << endl;
      }
    if (strncmp (this->ColorMenu->GetValue(),"Cell",4) == 0)
      {
      *file << "$kw(" << this->GetTclName() << ") ColorByCellField {" 
            << name << "} " << comps << endl;
      }
    }

  *file << "$kw(" << this->GetTclName() << ") SetMapScalarsFlag " 
        << this->MapScalarsCheck->GetState() << endl;
  *file << "$kw(" << this->GetTclName() << ") SetInterpolateColorsFlag " 
        << this->InterpolateColorsCheck->GetState() << endl;

  if (strcmp(this->RepresentationMenu->GetValue(), VTK_PV_OUTLINE_LABEL) == 0)
    {
    *file << "$kw(" << this->GetTclName() << ") DrawOutline" << endl;
    }
  if (strcmp(this->RepresentationMenu->GetValue(), VTK_PV_SURFACE_LABEL) == 0)
    {
    *file << "$kw(" << this->GetTclName() << ") DrawSurface" << endl;
    }
  if (strcmp(this->RepresentationMenu->GetValue(),VTK_PV_WIREFRAME_LABEL) == 0)
    {
    *file << "$kw(" << this->GetTclName() << ") DrawWireframe\n";
    }
  if (strcmp(this->RepresentationMenu->GetValue(), VTK_PV_POINTS_LABEL) == 0)
    {
    *file << "$kw(" << this->GetTclName() << ") DrawPoints\n";
    }
  if (strcmp(this->RepresentationMenu->GetValue(), VTK_PV_VOLUME_LABEL) == 0)
    {
    *file << "$kw(" << this->GetTclName() << ") DrawVolume\n";
    }

  if (strcmp(this->InterpolationMenu->GetValue(),"Flat") == 0)
    {
    *file << "$kw(" << this->GetTclName() << ") SetInterpolationToFlat\n";
    }


  if (this->CubeAxesCheck->GetState())
    {
    *file << "$kw(" << this->GetTclName() << ") SetCubeAxesVisibility 1\n";
    }

  if (this->PointLabelCheck->GetState())
    {
    *file << "$kw(" << this->GetTclName() << ") SetPointLabelVisibility 1 1\n";
    }
  
  i1 = (int)(this->PointSizeThumbWheel->GetValue());
  if (i1 != 1)
    {
    *file << "$kw(" << this->GetTclName() << ") SetPointSize " << i1 << endl;
    }
    
  i1 = (int)(this->LineWidthThumbWheel->GetValue());
  if (i1 != 1)
    {
    *file << "$kw(" << this->GetTclName() << ") SetLineWidth " << i1 << endl;
    }

  f1 = this->OpacityScale->GetValue();
  if (f1 != 1.0)
    {
    *file << "$kw(" << this->GetTclName() << ") SetOpacity " << f1 << endl;
    }

  f1 = this->TranslateThumbWheel[0]->GetValue();
  f2 = this->TranslateThumbWheel[1]->GetValue();
  f3 = this->TranslateThumbWheel[2]->GetValue();
  if (f1 != 0.0 || f2 != 0.0 || f3 != 0.0)
    {
    *file << "$kw(" << this->GetTclName() << ") SetActorTranslate "
          << f1 << " " << f2 << " " << f3 << endl;
    }

  f1 = this->ScaleThumbWheel[0]->GetValue();
  f2 = this->ScaleThumbWheel[1]->GetValue();
  f3 = this->ScaleThumbWheel[2]->GetValue();
  if (f1 != 1.0 || f2 != 1.0 || f3 != 1.0)
    {
    *file << "$kw(" << this->GetTclName() << ") SetActorScale " 
          << f1 << " " << f2 << " " << f3 << endl;
    }

  f1 = this->OrientationScale[0]->GetValue();
  f2 = this->OrientationScale[1]->GetValue();
  f3 = this->OrientationScale[2]->GetValue();
  if (f1 != 0.0 || f2 != 0.0 || f3 != 0.0)
    {
    *file << "$kw(" << this->GetTclName() << ") SetActorOrientation "
          << f1 << " " << f2 << " " << f3 << endl;
    }


  f1 = this->OriginThumbWheel[0]->GetValue();
  f2 = this->OriginThumbWheel[1]->GetValue();
  f3 = this->OriginThumbWheel[2]->GetValue();
  if (f1 != 0.0 || f2 != 0.0 || f3 != 0.0)
    {
    *file << "$kw(" << this->GetTclName() << ") SetActorOrigin "
          << f1 << " " << f2 << " " << f3 << endl;
    }
  
  *file << "$kw(" << this->GetTclName() << ") SetVisibility "
        << this->GetVisibility() << endl;  
}

//----------------------------------------------------------------------------
void vtkPVData::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);
  os << indent << "ColorMap: " << this->PVColorMap << endl;
  os << indent << "ColorMenu: " << this->ColorMenu << endl;
  os << indent << "VolumeScalarsMenu: " << this->VolumeScalarsMenu << endl;
  os << indent << "ResetCameraButton: " << this->ResetCameraButton << endl;
  os << indent << "EditColorMapButton: " << this->EditColorMapButton << endl;
  if(this->CubeAxes)
    {
    os << indent << "CubeAxes: " << this->CubeAxes << endl;
    }
  else
    {
    os << indent << "CubeAxes: none" << endl;
    }
  os << indent << "PVSource: " << this->GetPVSource() << endl;
  os << indent << "PropertiesParent: " << this->GetPropertiesParent() << endl;
  if (this->PVColorMap)
    {
    os << indent << "PVColorMap: " << this->PVColorMap->GetScalarBarTitle() << endl;
    }
  else
    {
    os << indent << "PVColorMap: NULL\n";
    }
  os << indent << "PropertiesCreated: " << this->PropertiesCreated << endl;
  os << indent << "CubeAxesCheck: " << this->CubeAxesCheck << endl;
  os << indent << "ScalarBarCheck: " << this->ScalarBarCheck << endl;
  os << indent << "RepresentationMenu: " << this->RepresentationMenu << endl;
  os << indent << "InterpolationMenu: " << this->InterpolationMenu << endl;
  os << indent << "ActorControlFrame: " << this->ActorControlFrame << endl;
  os << indent << "Visibility: " << this->Visibility << endl;
  os << indent << "ArraySetByUser: " << this->ArraySetByUser << endl;
  os << indent << "ColorSetByUser: " << this->ColorSetByUser << endl;
}

//----------------------------------------------------------------------------
void vtkPVData::GetColorRange(double *range)
{
  double *tmp;
  range[0] = 0.0;
  range[1] = 1.0;
  
  if (this->PVColorMap == NULL)
    {
    vtkErrorMacro("Color map missing.");
    return;
    }
  tmp = this->PVColorMap->GetScalarRange();
  range[0] = tmp[0];
  range[1] = tmp[1];
}

//----------------------------------------------------------------------------
void vtkPVData::SetOpacity(float val)
{ 
  this->OpacityScale->SetValue(val);
}

//----------------------------------------------------------------------------
void vtkPVData::OpacityChangedCallback()
{
  vtkPVProcessModule* pm = this->GetPVApplication()->GetProcessModule();

  vtkPVPart *part;
  int idx, num;

  num = this->GetPVSource()->GetNumberOfParts();
  for (idx = 0; idx < num; ++idx)
    {
    part = this->GetPVSource()->GetPart(idx);
    pm->GetStream() 
      << vtkClientServerStream::Invoke 
      << part->GetPartDisplay()->GetPropID()
      << "GetProperty"
      << vtkClientServerStream::End
      << vtkClientServerStream::Invoke
      << vtkClientServerStream::LastResult 
      << "SetOpacity" << this->OpacityScale->GetValue()
      << vtkClientServerStream::End;
    pm->SendStream(vtkProcessModule::CLIENT|vtkProcessModule::RENDER_SERVER);
    }

  if ( this->GetPVRenderView() )
    {
    this->GetPVRenderView()->EventuallyRender();
    }
}

//----------------------------------------------------------------------------
void vtkPVData::OpacityChangedEndCallback()
{
  this->OpacityChangedCallback();
  this->AddTraceEntry("$kw(%s) SetOpacity %f", 
                      this->GetTclName(), this->OpacityScale->GetValue());
}

//----------------------------------------------------------------------------
void vtkPVData::GetActorTranslate(double* point)
{
  vtkPVPart *part;

  part = this->GetPVSource()->GetPart(0);  
  vtkProp3D *prop = vtkProp3D::SafeDownCast(part->GetPartDisplay()->GetProp());
  if (prop)
    {
    prop->GetPosition(point);
    }
  else
    {
    point[0] = this->TranslateThumbWheel[0]->GetValue();
    point[1] = this->TranslateThumbWheel[1]->GetValue();
    point[2] = this->TranslateThumbWheel[2]->GetValue();
    }
}

//----------------------------------------------------------------------------
void vtkPVData::SetActorTranslateNoTrace(double x, double y, double z)
{
  vtkPVPart *part;
  int idx, num;

  this->TranslateThumbWheel[0]->SetValue(x);
  this->TranslateThumbWheel[1]->SetValue(y);
  this->TranslateThumbWheel[2]->SetValue(z);

  num = this->GetPVSource()->GetNumberOfParts();
  vtkPVProcessModule* pm = this->GetPVApplication()->GetProcessModule();
  for (idx = 0; idx < num; ++idx)
    {
    part = this->GetPVSource()->GetPart(idx);
    pm->GetStream() 
      << vtkClientServerStream::Invoke 
      << part->GetPartDisplay()->GetPropID()
      << "SetPosition" << x << y << z
      << vtkClientServerStream::End;
    pm->GetStream() 
      << vtkClientServerStream::Invoke 
      << part->GetPartDisplay()->GetVolumeID()
      << "SetPosition" << x << y << z
      << vtkClientServerStream::End;
    pm->SendStream(vtkProcessModule::CLIENT|vtkProcessModule::RENDER_SERVER);
    }

  // Do not render here (do it in the callback, since it could be either
  // Render or EventuallyRender depending on the interaction)
}

//----------------------------------------------------------------------------
void vtkPVData::SetActorTranslate(double x, double y, double z)
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
void vtkPVData::SetActorTranslate(double* point)
{
  this->SetActorTranslate(point[0], point[1], point[2]);
}

//----------------------------------------------------------------------------
void vtkPVData::ActorTranslateCallback()
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
void vtkPVData::ActorTranslateEndCallback()
{
  double point[3];
  point[0] = this->TranslateThumbWheel[0]->GetValue();
  point[1] = this->TranslateThumbWheel[1]->GetValue();
  point[2] = this->TranslateThumbWheel[2]->GetValue();
  this->SetActorTranslate(point);
}

//----------------------------------------------------------------------------
void vtkPVData::GetActorScale(double* point)
{
  vtkPVPart *part;

  part = this->GetPVSource()->GetPart(0);
  vtkProp3D *prop = vtkProp3D::SafeDownCast(part->GetPartDisplay()->GetProp());
  if (prop)
    {
    prop->GetScale(point);
    }
  else
    {
    point[0] = this->ScaleThumbWheel[0]->GetValue();
    point[1] = this->ScaleThumbWheel[1]->GetValue();
    point[2] = this->ScaleThumbWheel[2]->GetValue();
    }
}

//----------------------------------------------------------------------------
void vtkPVData::SetActorScaleNoTrace(double x, double y, double z)
{
  vtkPVPart *part;
  int idx, num;

  this->ScaleThumbWheel[0]->SetValue(x);
  this->ScaleThumbWheel[1]->SetValue(y);
  this->ScaleThumbWheel[2]->SetValue(z);

  num = this->GetPVSource()->GetNumberOfParts();
  vtkPVProcessModule* pm = this->GetPVApplication()->GetProcessModule();
  for (idx = 0; idx < num; ++idx)
    {
    part = this->GetPVSource()->GetPart(idx); 
    pm->GetStream() 
        << vtkClientServerStream::Invoke 
        << part->GetPartDisplay()->GetPropID()
        << "SetScale" << x << y << z 
        << vtkClientServerStream::End;
    pm->GetStream() 
        << vtkClientServerStream::Invoke 
        << part->GetPartDisplay()->GetVolumeID()
        << "SetScale" << x << y << z 
        << vtkClientServerStream::End;
    pm->SendStream(vtkProcessModule::CLIENT|vtkProcessModule::RENDER_SERVER);
    }

  // Do not render here (do it in the callback, since it could be either
  // Render or EventuallyRender depending on the interaction)
}

//----------------------------------------------------------------------------
void vtkPVData::SetActorScale(double x, double y, double z)
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
void vtkPVData::SetActorScale(double* point)
{
  this->SetActorScale(point[0], point[1], point[2]);
}

//----------------------------------------------------------------------------
void vtkPVData::ActorScaleCallback()
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
void vtkPVData::ActorScaleEndCallback()
{
  double point[3];
  point[0] = this->ScaleThumbWheel[0]->GetValue();
  point[1] = this->ScaleThumbWheel[1]->GetValue();
  point[2] = this->ScaleThumbWheel[2]->GetValue();
  this->SetActorScale(point);
}

//----------------------------------------------------------------------------
void vtkPVData::GetActorOrientation(double* point)
{
  vtkPVPart *part;

  part = this->GetPVSource()->GetPart(0);

  vtkProp3D *prop = vtkProp3D::SafeDownCast(part->GetPartDisplay()->GetProp());
  if (prop)
    {
    prop->GetOrientation(point);
    }
  else
    {
    point[0] = this->OrientationScale[0]->GetValue();
    point[1] = this->OrientationScale[1]->GetValue();
    point[2] = this->OrientationScale[2]->GetValue();
    }
}

//----------------------------------------------------------------------------
void vtkPVData::SetActorOrientationNoTrace(double x, double y, double z)
{
  vtkPVPart *part;
  int idx, num;

  this->OrientationScale[0]->SetValue(x);
  this->OrientationScale[1]->SetValue(y);
  this->OrientationScale[2]->SetValue(z);

  num = this->GetPVSource()->GetNumberOfParts();
  vtkPVProcessModule* pm = this->GetPVApplication()->GetProcessModule();
  for (idx = 0; idx < num; ++idx)
    {
    part = this->GetPVSource()->GetPart(idx);  
    pm->GetStream() 
        << vtkClientServerStream::Invoke 
        << part->GetPartDisplay()->GetPropID()
        << "SetOrientation" << x << y << z 
        << vtkClientServerStream::End;
    pm->GetStream() 
        << vtkClientServerStream::Invoke 
        << part->GetPartDisplay()->GetVolumeID()
        << "SetOrientation" << x << y << z 
        << vtkClientServerStream::End;
    pm->SendStream(vtkProcessModule::CLIENT|vtkProcessModule::RENDER_SERVER);
    }

  // Do not render here (do it in the callback, since it could be either
  // Render or EventuallyRender depending on the interaction)
}

//----------------------------------------------------------------------------
void vtkPVData::SetActorOrientation(double x, double y, double z)
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
void vtkPVData::SetActorOrientation(double* point)
{
  this->SetActorOrientation(point[0], point[1], point[2]);
}

//----------------------------------------------------------------------------
void vtkPVData::ActorOrientationCallback()
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
void vtkPVData::ActorOrientationEndCallback()
{
  double point[3];
  point[0] = this->OrientationScale[0]->GetValue();
  point[1] = this->OrientationScale[1]->GetValue();
  point[2] = this->OrientationScale[2]->GetValue();
  this->SetActorOrientation(point);
}

//----------------------------------------------------------------------------
void vtkPVData::GetActorOrigin(double* point)
{
  vtkPVPart *part;

  part = this->GetPVSource()->GetPart(0);
  vtkProp3D *prop = vtkProp3D::SafeDownCast(part->GetPartDisplay()->GetProp());
  if (prop)
    {
    prop->GetOrigin(point);
    }
  else
    {
    point[0] = this->OriginThumbWheel[0]->GetValue();
    point[1] = this->OriginThumbWheel[1]->GetValue();
    point[2] = this->OriginThumbWheel[2]->GetValue();
    }
}

//----------------------------------------------------------------------------
void vtkPVData::SetActorOriginNoTrace(double x, double y, double z)
{
  vtkPVPart *part;
  int idx, num;

  this->OriginThumbWheel[0]->SetValue(x);
  this->OriginThumbWheel[1]->SetValue(y);
  this->OriginThumbWheel[2]->SetValue(z);

  num = this->GetPVSource()->GetNumberOfParts();
  vtkPVProcessModule* pm = this->GetPVApplication()->GetProcessModule();
  for (idx = 0; idx < num; ++idx)
    {
    part = this->GetPVSource()->GetPart(idx);  
    pm->GetStream() 
        << vtkClientServerStream::Invoke 
        << part->GetPartDisplay()->GetPropID()
        << "SetOrigin" << x << y << z 
        << vtkClientServerStream::End;
    pm->GetStream() 
        << vtkClientServerStream::Invoke 
        << part->GetPartDisplay()->GetVolumeID()
        << "SetOrigin" << x << y << z 
        << vtkClientServerStream::End;
    pm->SendStream(vtkProcessModule::CLIENT|vtkProcessModule::RENDER_SERVER);
    }

  // Do not render here (do it in the callback, since it could be either
  // Render or EventuallyRender depending on the interaction)
}

//----------------------------------------------------------------------------
void vtkPVData::SetActorOrigin(double x, double y, double z)
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
void vtkPVData::SetActorOrigin(double* point)
{
  this->SetActorOrigin(point[0], point[1], point[2]);
}

//----------------------------------------------------------------------------
void vtkPVData::ActorOriginCallback()
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
void vtkPVData::ActorOriginEndCallback()
{
  double point[3];
  point[0] = this->OriginThumbWheel[0]->GetValue();
  point[1] = this->OriginThumbWheel[1]->GetValue();
  point[2] = this->OriginThumbWheel[2]->GetValue();
  this->SetActorOrigin(point);
}

//----------------------------------------------------------------------------
void vtkPVData::UpdateActorControlResolutions()
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

void vtkPVData::UpdateEnableState()
{
  this->Superclass::UpdateEnableState();

  this->PropagateEnableState(this->Properties);
  this->PropagateEnableState(this->InformationFrame);
  this->PropagateEnableState(this->TypeLabel);
  this->PropagateEnableState(this->NumCellsLabel);
  this->PropagateEnableState(this->NumPointsLabel);
  this->PropagateEnableState(this->MemorySizeLabel);
  this->PropagateEnableState(this->BoundsDisplay);
  this->PropagateEnableState(this->ExtentDisplay);
  this->PropagateEnableState(this->AmbientScale);
  this->PropagateEnableState(this->ColorFrame);
  this->PropagateEnableState(this->VolumeAppearanceFrame);
  this->PropagateEnableState(this->DisplayStyleFrame);
  this->PropagateEnableState(this->StatsFrame);
  this->PropagateEnableState(this->ViewFrame);
  this->PropagateEnableState(this->ColorMenuLabel);
  this->PropagateEnableState(this->ColorMenu);
  this->PropagateEnableState(this->VolumeScalarsMenuLabel);
  this->PropagateEnableState(this->VolumeScalarsMenu);
  if ( this->EditColorMapButtonVisible )
    {
    this->PropagateEnableState(this->EditColorMapButton);
    }
  else
    {
    this->EditColorMapButton->SetEnabled(0);
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
  if ( this->PVColorMap || !this->ColorButtonVisible )
    {
    this->ColorButton->SetEnabled(0);
    }
  else
    {
    this->PropagateEnableState(this->ColorButton);
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
  this->PropagateEnableState(this->ResetCameraButton);
  this->PropagateEnableState(this->PVColorMap);
  this->PropagateEnableState(this->PointLabelCheck);
  
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

