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

#include "vtkCellData.h"
#include "vtkCollection.h"
#include "vtkColorTransferFunction.h"
#include "vtkCommand.h"
#include "vtkDataSetAttributes.h"
#include "vtkDataSetSurfaceFilter.h"
#include "vtkImageData.h"
#include "vtkKWBoundsDisplay.h"
#include "vtkKWChangeColorButton.h"
#include "vtkKWCheckButton.h"
#include "vtkKWEntry.h"
#include "vtkKWFrame.h"
#include "vtkKWFrameWithScrollbar.h"
#include "vtkKWLabel.h"
#include "vtkKWMenu.h"
#include "vtkKWMenuButton.h"
#include "vtkKWNotebook.h"
#include "vtkKWPushButton.h"
#include "vtkKWScaleWithEntry.h"
#include "vtkKWThumbWheel.h"
#include "vtkKWTkUtilities.h"
#include "vtkKWView.h"
#include "vtkKWWidget.h"
#include "vtkMaterialLibrary.h"
#include "vtkMultiProcessController.h"
#include "vtkObjectFactory.h"
#include "vtkProcessModule.h"
#include "vtkPVApplication.h"
#include "vtkPVArrayInformation.h"
#include "vtkPVColorMap.h"
#include "vtkPVColorMapUI.h"
#include "vtkPVColorSelectionWidget.h"
#include "vtkPVDataInformation.h"
#include "vtkPVDataSetAttributesInformation.h"
#include "vtkPVGenericRenderWindowInteractor.h"
#include "vtkPVGeometryInformation.h"
#include "vtkPVNumberOfOutputsInformation.h"
#include "vtkPVOptions.h"
#include "vtkPVRenderModuleUI.h"
#include "vtkPVRenderView.h"
#include "vtkPVServerFileDialog.h"
#include "vtkPVSource.h"
#include "vtkPVTraceHelper.h"
#include "vtkPVVolumeAppearanceEditor.h"
#include "vtkPVWindow.h"
#include "vtkPiecewiseFunction.h"
#include "vtkPointData.h"
#include "vtkPolyData.h"
#include "vtkPolyDataMapper.h"
#include "vtkProp3D.h"
#include "vtkProperty.h"
#include "vtkProperty2D.h"
#include "vtkRectilinearGrid.h"
#include "vtkRenderWindow.h"
#include "vtkRenderer.h"
#include "vtkSMDataObjectDisplayProxy.h"
#include "vtkSMDisplayProxy.h"
#include "vtkSMDoubleVectorProperty.h"
#include "vtkSMIntVectorProperty.h"
#include "vtkSMLODDisplayProxy.h"
#include "vtkSMPointLabelDisplayProxy.h"
#include "vtkSMRenderModuleProxy.h"
#include "vtkSMStringVectorProperty.h"
#include "vtkStdString.h"
#include "vtkStructuredGrid.h"
#include "vtkTexture.h"
#include "vtkTimerLog.h"
#include "vtkToolkits.h"
#include "vtkVolumeProperty.h"

#include <vtkstd/map>
#include <vtksys/ios/sstream>
#include <vtksys/SystemTools.hxx>

#define VTK_PV_OUTLINE_LABEL              "Outline"
#define VTK_PV_SURFACE_LABEL              "Surface"
#define VTK_PV_WIREFRAME_LABEL            "Wireframe of Surface"
#define VTK_PV_POINTS_LABEL               "Points of Surface"
#define VTK_PV_VOLUME_LABEL               "Volume Render"
#define VTK_PV_VOLUME_PT_METHOD_LABEL     "Projection"
#define VTK_PV_VOLUME_HAVS_METHOD_LABEL   "HAVS"
#define VTK_PV_VOLUME_ZSWEEP_METHOD_LABEL "ZSweep"
#define VTK_PV_VOLUME_BUNYK_METHOD_LABEL  "Bunyk Ray Cast"
#define VTK_PV_MATERIAL_NONE_LABEL        "None"
#define VTK_PV_MATERIAL_BROWSE_LABEL      "Browse..."
#define VTK_PV_MATERIAL_BROWSE_HELP       "Browse Material description XML"

//----------------------------------------------------------------------------
vtkStandardNewMacro(vtkPVDisplayGUI);
vtkCxxRevisionMacro(vtkPVDisplayGUI, "1.68");

//----------------------------------------------------------------------------

class vtkPVDisplayGUIVRObserver : public vtkCommand
{
public:
  static vtkPVDisplayGUIVRObserver *New()
    { return new vtkPVDisplayGUIVRObserver; }

  void SetDisplayGUI( vtkPVDisplayGUI *gui )
    { this->DisplayGUI = gui; }

  virtual void Execute( vtkObject *vtkNotUsed(w), 
                        unsigned long vtkNotUsed(event), 
                        void *vtkNotUsed(data) )
    { 
      if (   !this->DisplayGUI
          || !this->DisplayGUI->VolumeRenderMode
          || !(this->DisplayGUI->PVSource && this->DisplayGUI->PVSource->GetDisplayProxy()))
        {
        return;
        }
      if ( this->DisplayGUI->GetPVRenderView()->GetRenderWindow()->GetDesiredUpdateRate() >= 1 )
        {
        if ( this->DisplayGUI->VolumeRenderMethodMenu->GetMenu()->HasItem(
               VTK_PV_VOLUME_HAVS_METHOD_LABEL) &&
             strncmp(this->DisplayGUI->VolumeScalarSelectionWidget->GetValue(),
                     "Cell", 4) != 0 )
          {
          this->DisplayGUI->DrawVolumeHAVSInternal();
          }
        else
          {
          this->DisplayGUI->DrawVolumePTInternal();
          }
        }
      else
        {
        if ( !strcmp( this->DisplayGUI->VolumeRenderMethodMenu->GetValue(),
                     VTK_PV_VOLUME_PT_METHOD_LABEL ) )
          {
          this->DisplayGUI->DrawVolumePTInternal();
          }
        else if ( !strcmp( this->DisplayGUI->VolumeRenderMethodMenu->GetValue(),
                          VTK_PV_VOLUME_HAVS_METHOD_LABEL ) )
          {
          this->DisplayGUI->DrawVolumeHAVSInternal();
          }
        else if ( !strcmp( this->DisplayGUI->VolumeRenderMethodMenu->GetValue(),
                          VTK_PV_VOLUME_ZSWEEP_METHOD_LABEL ) )
          {
          this->DisplayGUI->DrawVolumeZSweepInternal();
          }
        else if ( !strcmp( this->DisplayGUI->VolumeRenderMethodMenu->GetValue(),
                          VTK_PV_VOLUME_BUNYK_METHOD_LABEL ) )
          {
          this->DisplayGUI->DrawVolumeBunykInternal();
          }
        }
    }
  
  
protected:
  
  vtkPVDisplayGUIVRObserver()
    {
      this->DisplayGUI = NULL;
    }
  
  vtkPVDisplayGUI *DisplayGUI;
};



//----------------------------------------------------------------------------
struct vtkPVDisplayGUIInternal
{
  typedef vtkstd::map<vtkStdString, vtkStdString> MapOfStringToString;
  // Map with key== material filename, value== label.
  MapOfStringToString MaterialsMap;
};

//----------------------------------------------------------------------------
vtkPVDisplayGUI::vtkPVDisplayGUI()
{
  this->PVSource = 0;
  this->Internal = new vtkPVDisplayGUIInternal;
  
  this->EditColorMapButtonVisible = 1;
  this->MapScalarsCheckVisible = 0;
  this->ColorButtonVisible = 1;
  this->ScalarBarCheckVisible = 1;
  this->InterpolateColorsCheckVisible = 1;

  this->MainFrame = vtkKWFrameWithScrollbar::New();
  this->ColorFrame = vtkKWFrameWithLabel::New();
  this->VolumeAppearanceFrame = vtkKWFrameWithLabel::New();
  this->DisplayStyleFrame = vtkKWFrameWithLabel::New();
  this->ViewFrame = vtkKWFrameWithLabel::New();
  
  this->ColorMenuLabel = vtkKWLabel::New();
  this->ColorSelectionMenu = vtkPVColorSelectionWidget::New();

  this->MapScalarsCheck = vtkKWCheckButton::New();
  this->InterpolateColorsCheck = vtkKWCheckButton::New();
  this->EditColorMapButtonFrame = vtkKWFrame::New();
  this->EditColorMapButton = vtkKWPushButton::New();
  this->DataColorRangeButton = vtkKWPushButton::New();
  
  this->ColorButton = vtkKWChangeColorButton::New();

  this->VolumeScalarsMenuLabel = vtkKWLabel::New();
  this->VolumeScalarSelectionWidget = vtkPVColorSelectionWidget::New();
  
  this->VolumeRenderMethodMenuLabel = vtkKWLabel::New();
  this->VolumeRenderMethodMenu = vtkKWMenuButton::New();
  
  this->EditVolumeAppearanceButton = vtkKWPushButton::New();

  this->RepresentationMenuLabel = vtkKWLabel::New();
  this->RepresentationMenu = vtkKWMenuButton::New();
  
  this->InterpolationMenuLabel = vtkKWLabel::New();
  this->InterpolationMenu = vtkKWMenuButton::New();

  this->MaterialMenuLabel = 0;
  this->MaterialMenu = 0;
  
  this->PointSizeLabel = vtkKWLabel::New();
  this->PointSizeThumbWheel = vtkKWThumbWheel::New();
  this->LineWidthLabel = vtkKWLabel::New();
  this->LineWidthThumbWheel = vtkKWThumbWheel::New();
  this->PointLabelFontSizeLabel = vtkKWLabel::New();
  this->PointLabelFontSizeThumbWheel = vtkKWThumbWheel::New();
  
  this->ScalarBarCheck = vtkKWCheckButton::New();
  this->CubeAxesCheck = vtkKWCheckButton::New();
  this->PointLabelCheck = vtkKWCheckButton::New();
  this->VisibilityCheck = vtkKWCheckButton::New();

  this->ResetCameraButton = vtkKWPushButton::New();

  this->ActorControlFrame = vtkKWFrameWithLabel::New();
  this->TranslateLabel = vtkKWLabel::New();
  this->ScaleLabel = vtkKWLabel::New();
  this->OrientationLabel = vtkKWLabel::New();
  this->OriginLabel = vtkKWLabel::New();

  int cc;
  for ( cc = 0; cc < 3; cc ++ )
    {
    this->TranslateThumbWheel[cc] = vtkKWThumbWheel::New();
    this->ScaleThumbWheel[cc] = vtkKWThumbWheel::New();
    this->OrientationScale[cc] = vtkKWScaleWithEntry::New();
    this->OriginThumbWheel[cc] = vtkKWThumbWheel::New();
    }

  this->OpacityLabel = vtkKWLabel::New();
  this->OpacityScale = vtkKWScaleWithEntry::New();
  
  this->ActorColor[0] = this->ActorColor[1] = this->ActorColor[2] = 1.0;

  // I do not this these are used because this is a shared GUI object.
  //this->ColorSetByUser = 0;
  //this->ArraySetByUser = 0;
    
  this->VolumeRenderMode = 0;
  
  this->VolumeAppearanceEditor = NULL;

  this->ShouldReinitialize = 0;
  
  this->VRObserver = vtkPVDisplayGUIVRObserver::New();
  this->VRObserver->SetDisplayGUI( this );
}

//----------------------------------------------------------------------------
vtkPVDisplayGUI::~vtkPVDisplayGUI()
{  
  this->VRObserver->SetDisplayGUI( NULL );
  delete this->Internal;
  this->Internal = 0;

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

  this->VolumeRenderMethodMenuLabel->Delete();
  this->VolumeRenderMethodMenuLabel = NULL;
  this->VolumeRenderMethodMenu->Delete();
  this->VolumeRenderMethodMenu = NULL;
  
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

  if (this->MaterialMenuLabel)
    {
    this->MaterialMenuLabel->Delete();
    this->MaterialMenuLabel = NULL;
    }
  if (this->MaterialMenu)
    {
    this->MaterialMenu->Delete();
    this->MaterialMenu = NULL;
    }
  
  this->PointSizeLabel->Delete();
  this->PointSizeLabel = NULL;
  this->PointSizeThumbWheel->Delete();
  this->PointSizeThumbWheel = NULL;
  this->LineWidthLabel->Delete();
  this->LineWidthLabel = NULL;
  this->LineWidthThumbWheel->Delete();
  this->LineWidthThumbWheel = NULL;
  this->PointLabelFontSizeLabel->Delete();
  this->PointLabelFontSizeLabel = NULL;
  this->PointLabelFontSizeThumbWheel->Delete();
  this->PointLabelFontSizeThumbWheel = NULL;

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
  
  this->MainFrame->Delete();
  this->MainFrame = NULL;
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
  
  this->VRObserver->Delete();
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
void vtkPVDisplayGUI::Close()
{
  if (this->VolumeAppearanceEditor)
    {
    this->VolumeAppearanceEditor->Close();
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

  this->GetTraceHelper()->SetReferenceHelper(
    source ? source->GetTraceHelper() : NULL);
  this->GetTraceHelper()->SetReferenceCommand("GetPVOutput");
}






// ============= Use to be in vtkPVActorComposite ===================

//----------------------------------------------------------------------------
void vtkPVDisplayGUI::CreateWidget()
{
  // Check if already created

  if (this->IsCreated())
    {
    vtkErrorMacro(<< this->GetClassName() << " already created");
    return;
    }

  // Call the superclass to create the whole widget

  this->Superclass::CreateWidget();
  int cc;

  // We are going to 'grid' most of it, so let's define some const

  int col_1_padx = 2;
  int button_pady = 1;
  int col_0_weight = 0;
  int col_1_weight = 1;
  float col_0_factor = 1.5;
  float col_1_factor = 1.0;

  // Main frame

  this->MainFrame->SetParent(this);
  this->MainFrame->Create();
  this->Script("pack %s -fill both -expand t -pady 0 -padx 0", 
               this->MainFrame->GetWidgetName());

  // View frame

  this->ViewFrame->SetParent(this->MainFrame->GetFrame());
  this->ViewFrame->Create();
  this->ViewFrame->SetLabelText("View");
 
  this->VisibilityCheck->SetParent(this->ViewFrame->GetFrame());
  this->VisibilityCheck->Create();
  this->VisibilityCheck->SetText("Data");
  this->GetApplication()->Script(
    "%s configure -command {%s VisibilityCheckCallback}",
    this->VisibilityCheck->GetWidgetName(),
    this->GetTclName());
  this->VisibilityCheck->SetSelectedState(1);
  this->VisibilityCheck->SetBalloonHelpString(
    "Toggle the visibility of this dataset's geometry.");

  this->ResetCameraButton->SetParent(this->ViewFrame->GetFrame());
  this->ResetCameraButton->Create();
  this->ResetCameraButton->SetText("Set View to Data");
  this->ResetCameraButton->SetCommand(this, "CenterCamera");
  this->ResetCameraButton->SetBalloonHelpString(
    "Change the camera location to best fit the dataset in the view window.");

  this->ScalarBarCheck->SetParent(this->ViewFrame->GetFrame());
  this->ScalarBarCheck->Create();
  this->ScalarBarCheck->SetText("Scalar bar");
  this->ScalarBarCheck->SetBalloonHelpString(
    "Toggle the visibility of the scalar bar for this data.");
  this->GetApplication()->Script(
    "%s configure -command {%s ScalarBarCheckCallback}",
    this->ScalarBarCheck->GetWidgetName(),
    this->GetTclName());

  this->CubeAxesCheck->SetParent(this->ViewFrame->GetFrame());
  this->CubeAxesCheck->Create();
  this->CubeAxesCheck->SetText("Cube Axes");
  this->CubeAxesCheck->SetCommand(this, "CubeAxesCheckCallback");
  this->CubeAxesCheck->SetBalloonHelpString(
    "Toggle the visibility of X,Y,Z scales for this dataset.");

  this->PointLabelCheck->SetParent(this->ViewFrame->GetFrame());
  this->PointLabelCheck->Create();
  this->PointLabelCheck->SetText("Label Point Ids");
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
  this->ColorFrame->SetParent(this->MainFrame->GetFrame());
  this->ColorFrame->Create();
  this->ColorFrame->SetLabelText("Color");

  this->ColorMenuLabel->SetParent(this->ColorFrame->GetFrame());
  this->ColorMenuLabel->Create();
  this->ColorMenuLabel->SetText("Color by:");
  this->ColorMenuLabel->SetBalloonHelpString(
    "Select method for coloring dataset geometry.");
  
  this->ColorSelectionMenu->SetParent(this->ColorFrame->GetFrame());
  this->ColorSelectionMenu->Create();   
  this->ColorSelectionMenu->SetColorSelectionCommand("ColorByArray");
  this->ColorSelectionMenu->SetTarget(this);
  this->ColorSelectionMenu->SetBalloonHelpString(
    "Select method for coloring dataset geometry.");

  this->ColorButton->SetParent(this->ColorFrame->GetFrame());
  this->ColorButton->GetLabel()->SetText("Actor Color");
  this->ColorButton->Create();
  this->ColorButton->SetCommand(this, "ChangeActorColor");
  this->ColorButton->SetBalloonHelpString(
    "Edit the constant color for the geometry.");

  this->MapScalarsCheck->SetParent(this->ColorFrame->GetFrame());
  this->MapScalarsCheck->Create();
  this->MapScalarsCheck->SetText("Map Scalars");
  this->MapScalarsCheck->SetSelectedState(0);
  this->MapScalarsCheck->SetBalloonHelpString(
    "Pass attriubte through color map or use unsigned char values as color.");
  this->GetApplication()->Script(
    "%s configure -command {%s MapScalarsCheckCallback}",
    this->MapScalarsCheck->GetWidgetName(),
    this->GetTclName());
    
  this->InterpolateColorsCheck->SetParent(this->ColorFrame->GetFrame());
  this->InterpolateColorsCheck->Create();
  this->InterpolateColorsCheck->SetText("Interpolate Colors");
  this->InterpolateColorsCheck->SetSelectedState(0);
  this->InterpolateColorsCheck->SetBalloonHelpString(
    "Interpolate colors after mapping.");
  this->GetApplication()->Script(
    "%s configure -command {%s InterpolateColorsCheckCallback}",
    this->InterpolateColorsCheck->GetWidgetName(),
    this->GetTclName());
    
  // Group these two buttons in the place of one.
  this->EditColorMapButtonFrame->SetParent(this->ColorFrame->GetFrame());
  this->EditColorMapButtonFrame->Create();
  // --
  this->EditColorMapButton->SetParent(this->EditColorMapButtonFrame);
  this->EditColorMapButton->Create();
  this->EditColorMapButton->SetText("Edit Color Map");
  this->EditColorMapButton->SetCommand(this,"EditColorMapCallback");
  this->EditColorMapButton->SetBalloonHelpString(
    "Edit the table used to map data attributes to pseudo colors.");
  // --
  this->DataColorRangeButton->SetParent(this->EditColorMapButtonFrame);
  this->DataColorRangeButton->Create();
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

  this->VolumeAppearanceFrame->SetParent(this->MainFrame->GetFrame());
  this->VolumeAppearanceFrame->Create();
  this->VolumeAppearanceFrame->SetLabelText("Volume Appearance");

  this->VolumeScalarsMenuLabel->
    SetParent(this->VolumeAppearanceFrame->GetFrame());
  this->VolumeScalarsMenuLabel->Create();
  this->VolumeScalarsMenuLabel->SetText("View Scalars:");
  this->VolumeScalarsMenuLabel->SetBalloonHelpString(
    "Select scalars to view with volume rendering.");

  this->VolumeScalarSelectionWidget->SetParent(this->VolumeAppearanceFrame->GetFrame());
  this->VolumeScalarSelectionWidget->Create();
  this->VolumeScalarSelectionWidget->SetColorSelectionCommand("VolumeRenderByArray");
  this->VolumeScalarSelectionWidget->SetTarget(this);
  this->VolumeScalarSelectionWidget->SetBalloonHelpString(
    "Select scalars to view with volume rendering.");

  this->VolumeRenderMethodMenuLabel->SetParent(
    this->VolumeAppearanceFrame->GetFrame());
  this->VolumeRenderMethodMenuLabel->Create();
  this->VolumeRenderMethodMenuLabel->SetText("Still Render Method:");
  this->VolumeRenderMethodMenuLabel->SetBalloonHelpString(
    "Select the render method to be used when not interacting "
    "(during interaction projection is always used). "
    "Projection is fast, ZSweep and Bunyk are much slower, but more accurate. "
    "HAVS (listed if supported) is also fast, but cannot render cell data; in "
    "this case, Projection will be automatically selected.");

  this->VolumeRenderMethodMenu->SetParent(
    this->VolumeAppearanceFrame->GetFrame());
  this->VolumeRenderMethodMenu->Create();
  this->VolumeRenderMethodMenu->SetBalloonHelpString(
    "Select the render method to be used when not interacting "
    "(during interaction projection is always used). "
    "Projection is fast, ZSweep and Bunyk are much slower, but more accurate.");
  
  this->EditVolumeAppearanceButton->
    SetParent(this->VolumeAppearanceFrame->GetFrame());
  this->EditVolumeAppearanceButton->Create();
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

  this->Script("grid %s %s -row 1 -sticky wns",
               this->VolumeRenderMethodMenuLabel->GetWidgetName(),
               this->VolumeRenderMethodMenu->GetWidgetName());

  this->Script("grid %s -row 1 -column 1 -sticky news -padx %d -pady %d",
               this->VolumeRenderMethodMenu->GetWidgetName(),
               col_1_padx, button_pady);

  this->Script("grid %s -row 2 -column 1 -sticky news -padx %d -pady %d",
               this->EditVolumeAppearanceButton->GetWidgetName(),
               col_1_padx, button_pady);


  // Display style
  this->DisplayStyleFrame->SetParent(this->MainFrame->GetFrame());
  this->DisplayStyleFrame->Create();
  this->DisplayStyleFrame->SetLabelText("Display Style");
  
  this->RepresentationMenuLabel->SetParent(
    this->DisplayStyleFrame->GetFrame());
  this->RepresentationMenuLabel->Create();
  this->RepresentationMenuLabel->SetText("Representation:");

  this->RepresentationMenu->SetParent(this->DisplayStyleFrame->GetFrame());
  this->RepresentationMenu->Create();
  this->RepresentationMenu->GetMenu()->AddRadioButton(
    VTK_PV_OUTLINE_LABEL, this, "DrawOutline");
  this->RepresentationMenu->GetMenu()->AddRadioButton(
    VTK_PV_SURFACE_LABEL, this, "DrawSurface");
  this->RepresentationMenu->GetMenu()->AddRadioButton(
    VTK_PV_WIREFRAME_LABEL, this, "DrawWireframe");
  this->RepresentationMenu->GetMenu()->AddRadioButton(
    VTK_PV_POINTS_LABEL, this, "DrawPoints");

  this->RepresentationMenu->SetBalloonHelpString(
    "Choose what geometry should be used to represent the dataset.");

  this->InterpolationMenuLabel->SetParent(this->DisplayStyleFrame->GetFrame());
  this->InterpolationMenuLabel->Create();
  this->InterpolationMenuLabel->SetText("Interpolation:");

  this->InterpolationMenu->SetParent(this->DisplayStyleFrame->GetFrame());
  this->InterpolationMenu->Create();
  this->InterpolationMenu->GetMenu()->AddRadioButton(
    "Flat", this, "SetInterpolationToFlat");
  this->InterpolationMenu->GetMenu()->AddRadioButton(
    "Gouraud", this, "SetInterpolationToGouraud");
  this->InterpolationMenu->SetValue("Gouraud");
  this->InterpolationMenu->SetBalloonHelpString(
    "Choose the method used to shade the geometry and interpolate point attributes.");

  if (vtkMaterialLibrary::GetNumberOfMaterials() > 0)
    {
    this->MaterialMenuLabel = vtkKWLabel::New();
    this->MaterialMenu = vtkKWMenuButton::New();

    this->MaterialMenuLabel->SetParent(this->DisplayStyleFrame->GetFrame());
    this->MaterialMenuLabel->Create();
    this->MaterialMenuLabel->SetText("Material:");

    this->MaterialMenu->SetParent(this->DisplayStyleFrame->GetFrame());
    this->MaterialMenu->Create();
    this->MaterialMenu->GetMenu()->AddRadioButton(
      VTK_PV_MATERIAL_NONE_LABEL, this, "SetMaterial {} {}");
    this->MaterialMenu->SetBalloonHelpString(
      "Choose the material to apply to the object.");

    // Populate the material list.
    const char** names = vtkMaterialLibrary::GetListOfMaterialNames();
    for(cc=0; names[cc];cc++)
      {
      vtksys_ios::ostringstream stream;
      stream << "SetMaterial {" << names[cc] << "} {" << names[cc] << "}";
      this->MaterialMenu->GetMenu()->AddRadioButton(
        names[cc], this, stream.str().c_str());
      }
    int index = this->MaterialMenu->GetMenu()->AddRadioButton(
      VTK_PV_MATERIAL_BROWSE_LABEL, this, "BrowseMaterial");
    this->MaterialMenu->GetMenu()->SetItemHelpString(
      index, VTK_PV_MATERIAL_BROWSE_HELP);
    }

  

  this->PointSizeLabel->SetParent(this->DisplayStyleFrame->GetFrame());
  this->PointSizeLabel->Create();
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
  this->PointSizeThumbWheel->Create();
  this->PointSizeThumbWheel->DisplayEntryOn();
  this->PointSizeThumbWheel->DisplayEntryAndLabelOnTopOff();
  this->PointSizeThumbWheel->SetBalloonHelpString("Set the point size.");
  this->PointSizeThumbWheel->GetEntry()->SetWidth(5);
  this->PointSizeThumbWheel->SetCommand(this, "ChangePointSizeCallback");
  this->PointSizeThumbWheel->SetEndCommand(this, "ChangePointSizeEndCallback");
  this->PointSizeThumbWheel->SetEntryCommand(this, "ChangePointSizeEndCallback");
  this->PointSizeThumbWheel->SetBalloonHelpString(
    "If your dataset contains points/verticies, "
    "this scale adjusts the diameter of the rendered points.");

  this->LineWidthLabel->SetParent(this->DisplayStyleFrame->GetFrame());
  this->LineWidthLabel->Create();
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
  this->LineWidthThumbWheel->Create();
  this->LineWidthThumbWheel->DisplayEntryOn();
  this->LineWidthThumbWheel->DisplayEntryAndLabelOnTopOff();
  this->LineWidthThumbWheel->SetBalloonHelpString("Set the line width.");
  this->LineWidthThumbWheel->GetEntry()->SetWidth(5);
  this->LineWidthThumbWheel->SetCommand(this, "ChangeLineWidthCallback");
  this->LineWidthThumbWheel->SetEndCommand(this, "ChangeLineWidthEndCallback");
  this->LineWidthThumbWheel->SetEntryCommand(this, "ChangeLineWidthEndCallback");
  this->LineWidthThumbWheel->SetBalloonHelpString(
    "If your dataset containes lines/edges, "
    "this scale adjusts the width of the rendered lines.");

  this->PointLabelFontSizeLabel->SetParent(this->DisplayStyleFrame->GetFrame());
  this->PointLabelFontSizeLabel->Create();
  this->PointLabelFontSizeLabel->SetText("Point Id size:");
  this->PointLabelFontSizeLabel->SetBalloonHelpString(
    "This scale adjusts the size of the point ID labels.");

  this->PointLabelFontSizeThumbWheel->SetParent(this->DisplayStyleFrame->GetFrame());
  this->PointLabelFontSizeThumbWheel->PopupModeOn();
  this->PointLabelFontSizeThumbWheel->SetResolution(1.0);
  this->PointLabelFontSizeThumbWheel->SetMinimumValue(4.0);
  this->PointLabelFontSizeThumbWheel->ClampMinimumValueOn();
  this->PointLabelFontSizeThumbWheel->Create();
  this->PointLabelFontSizeThumbWheel->DisplayEntryOn();
  this->PointLabelFontSizeThumbWheel->DisplayEntryAndLabelOnTopOff();
  this->PointLabelFontSizeThumbWheel->SetBalloonHelpString("Set the point ID label font size.");
  this->PointLabelFontSizeThumbWheel->GetEntry()->SetWidth(5);
  this->PointLabelFontSizeThumbWheel->SetCommand(this, "ChangePointLabelFontSizeCallback");
  this->PointLabelFontSizeThumbWheel->SetEndCommand(this, "ChangePointLabelFontSizeCallback");
  this->PointLabelFontSizeThumbWheel->SetEntryCommand(this, "ChangePointLabelFontSizeCallback");
  this->PointLabelFontSizeThumbWheel->SetBalloonHelpString(
    "This scale adjusts the font size of the point ID labels.");

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

  if (this->MaterialMenuLabel && this->MaterialMenu)
    {
    this->Script("grid %s %s -sticky wns",
      this->MaterialMenuLabel->GetWidgetName(),
      this->MaterialMenu->GetWidgetName());

    this->Script("grid %s -sticky news -padx %d -pady %d",
      this->MaterialMenu->GetWidgetName(),
      col_1_padx, button_pady);
    }
  
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

  if ((this->GetPVApplication()->GetProcessModule()->GetNumberOfPartitions() == 1) &&
      (!this->GetPVApplication()->GetOptions()->GetClientMode()))
    {

    this->Script("grid %s %s -sticky wns",
                 this->PointLabelFontSizeLabel->GetWidgetName(),
                 this->PointLabelFontSizeThumbWheel->GetWidgetName());
    
    this->Script("grid %s -sticky news -padx %d -pady %d",
                 this->PointLabelFontSizeThumbWheel->GetWidgetName(), 
                 col_1_padx, button_pady);
    }
  
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

  this->ActorControlFrame->SetParent(this->MainFrame->GetFrame());
  this->ActorControlFrame->Create();
  this->ActorControlFrame->SetLabelText("Actor Control");

  this->TranslateLabel->SetParent(this->ActorControlFrame->GetFrame());
  this->TranslateLabel->Create();
  this->TranslateLabel->SetText("Translate:");
  this->TranslateLabel->SetBalloonHelpString(
    "Translate the geometry relative to the dataset location.");

  this->ScaleLabel->SetParent(this->ActorControlFrame->GetFrame());
  this->ScaleLabel->Create();
  this->ScaleLabel->SetText("Scale:");
  this->ScaleLabel->SetBalloonHelpString(
    "Scale the geometry relative to the size of the dataset.");

  this->OrientationLabel->SetParent(this->ActorControlFrame->GetFrame());
  this->OrientationLabel->Create();
  this->OrientationLabel->SetText("Orientation:");
  this->OrientationLabel->SetBalloonHelpString(
    "Orient the geometry relative to the dataset origin.");

  this->OriginLabel->SetParent(this->ActorControlFrame->GetFrame());
  this->OriginLabel->Create();
  this->OriginLabel->SetText("Origin:");
  this->OriginLabel->SetBalloonHelpString(
    "Set the origin point about which rotations take place.");

  for ( cc = 0; cc < 3; cc ++ )
    {
    this->TranslateThumbWheel[cc]->SetParent(this->ActorControlFrame->GetFrame());
    this->TranslateThumbWheel[cc]->PopupModeOn();
    this->TranslateThumbWheel[cc]->SetValue(0.0);
    this->TranslateThumbWheel[cc]->Create();
    this->TranslateThumbWheel[cc]->DisplayEntryOn();
    this->TranslateThumbWheel[cc]->ExpandEntryOn();
    this->TranslateThumbWheel[cc]->DisplayEntryAndLabelOnTopOff();
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
    this->ScaleThumbWheel[cc]->Create();
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
    this->OrientationScale[cc]->PopupModeOn();
    this->OrientationScale[cc]->Create();
    this->OrientationScale[cc]->SetRange(0, 360);
    this->OrientationScale[cc]->SetResolution(1);
    this->OrientationScale[cc]->SetValue(0);
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
    this->OriginThumbWheel[cc]->Create();
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
  this->OpacityLabel->Create();
  this->OpacityLabel->SetText("Opacity:");
  this->OpacityLabel->SetBalloonHelpString(
    "Set the opacity of the dataset's geometry.  "
    "Artifacts may appear in translucent geometry "
    "because primatives are not sorted.");

  this->OpacityScale->SetParent(this->ActorControlFrame->GetFrame());
  this->OpacityScale->PopupModeOn();
  this->OpacityScale->Create();
  this->OpacityScale->SetRange(0, 1);
  this->OpacityScale->SetResolution(0.1);
  this->OpacityScale->SetValue(1);
  this->OpacityScale->ExpandEntryOn();
  this->OpacityScale->GetEntry()->SetWidth(5);
  this->OpacityScale->SetCommand(this, "OpacityChangedCallback");
  this->OpacityScale->SetEndCommand(this, "OpacityChangedEndCallback");
  this->OpacityScale->SetEntryCommand(this, "OpacityChangedEndCallback");
  this->OpacityScale->SetBalloonHelpString(
    "Set the opacity of the dataset's geometry.  "
    "Artifacts may appear in translucent geometry "
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
  vtkPVRenderView *rv = this->GetPVRenderView();
  rv->ShowViewProperties();
  rv->GetNotebook()->RaisePage("Annotate");
}

//----------------------------------------------------------------------------
void vtkPVDisplayGUI::DataColorRangeCallback()
{
  this->GetTraceHelper()->AddEntry("$kw(%s) DataColorRangeCallback", this->GetTclName());
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
  
  this->GetTraceHelper()->AddEntry("$kw(%s) ShowVolumeAppearanceEditor",
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

  const char* arrayname = source->GetDisplayProxy()->GetScalarArrayCM();
  int colorField = source->GetDisplayProxy()->GetScalarModeCM();
  
  if (arrayname)
    {
    vtkPVDataInformation* dataInfo = source->GetDataInformation();
    vtkPVArrayInformation *arrayInfo;
    vtkPVDataSetAttributesInformation *attrInfo;
    
    if (colorField == vtkSMDataObjectDisplayProxy::POINT_FIELD_DATA)
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
  vtkSMDataObjectDisplayProxy* pDisp = source->GetDisplayProxy();
  
  // First reset all the values of the widgets.
  // Active states, and menu items will ge generated later.  
  // This could be done with a mechanism similar to reset
  // of parameters page.  
  // Parameters pages could just use/share the prototype UI instead of cloning.
  
  //law int fixmeEventually; // Use proper SM properties with reset.
  
  // Visibility check
  this->VisibilityCheck->SetSelectedState(this->PVSource->GetVisibility());
  // Cube axis visibility
  this->UpdateCubeAxesVisibilityCheck();
  // Point label visibility
  this->UpdatePointLabelVisibilityCheck();
  // Colors
  this->UpdateColorGUI();
    
  // Representation menu.
  switch(pDisp->GetRepresentationCM())
    {
  case vtkSMDataObjectDisplayProxy::OUTLINE:
    this->RepresentationMenu->SetValue(VTK_PV_OUTLINE_LABEL);
    break;
  case vtkSMDataObjectDisplayProxy::SURFACE:
    this->RepresentationMenu->SetValue(VTK_PV_SURFACE_LABEL);
    break;
  case vtkSMDataObjectDisplayProxy::WIREFRAME:
    this->RepresentationMenu->SetValue(VTK_PV_WIREFRAME_LABEL);
    break;
  case vtkSMDataObjectDisplayProxy::POINTS:
    this->RepresentationMenu->SetValue(VTK_PV_POINTS_LABEL);
    break;
  case vtkSMDataObjectDisplayProxy::VOLUME:
    this->RepresentationMenu->SetValue(VTK_PV_VOLUME_LABEL);
    break;
  default:
    vtkErrorMacro("Unknown representation.");
    }

  // Interpolation menu.
  switch (pDisp->GetInterpolationCM())
    {
  case vtkSMDataObjectDisplayProxy::FLAT:
    this->InterpolationMenu->SetValue("Flat");
    break;
  case vtkSMDataObjectDisplayProxy::GOURAND:
    this->InterpolationMenu->SetValue("Gouraud");
    break;
  default:
    vtkErrorMacro("Unknown representation.");
    }

  // Material menu.
  if (this->MaterialMenu)
    {
    this->SetMaterialInternal(pDisp->GetMaterialCM(), 0); 
    }
  this->PointSizeThumbWheel->SetValue(pDisp->GetPointSizeCM());
  this->LineWidthThumbWheel->SetValue(pDisp->GetLineWidthCM());
  this->PointLabelFontSizeThumbWheel->SetValue(
    source->GetPointLabelDisplayProxy()->GetFontSizeCM());
  this->OpacityScale->SetValue(pDisp->GetOpacityCM());

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
    this->VisibilityCheck->SetSelectedState(v);
    }
}

//----------------------------------------------------------------------------
void vtkPVDisplayGUI::UpdateCubeAxesVisibilityCheck()
{
  if (this->PVSource && this->VisibilityCheck->GetApplication())
    {
    this->CubeAxesCheck->SetSelectedState(this->PVSource->GetCubeAxesVisibility());
    }
}

//----------------------------------------------------------------------------
void vtkPVDisplayGUI::UpdatePointLabelVisibilityCheck()
{
  if (this->PVSource && this->VisibilityCheck->GetApplication())
    {
    this->PointLabelCheck->SetSelectedState(this->PVSource->GetPointLabelVisibility());
    this->PointLabelFontSizeThumbWheel->SetValue(this->PVSource->GetPointLabelFontSize());    
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
    !this->PVSource->GetDisplayProxy()->GetColorModeCM())
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
    this->ScalarBarCheck->SetSelectedState(
          this->PVSource->GetPVColorMap()->GetScalarBarVisibility());
    }
  else
    {
    this->ScalarBarCheck->SetSelectedState(0);
    }

  this->UpdateEnableState();
}


//----------------------------------------------------------------------------
void vtkPVDisplayGUI::UpdateColorMenu()
{  
  // Variables that hold the current color state.
  vtkPVDataInformation *geomInfo;
  vtkPVDataSetAttributesInformation *attrInfo;
  vtkPVArrayInformation *arrayInfo;
  int colorField = this->PVSource->GetSavedColorArrayField();
  const char* colorArrayName = this->PVSource->GetSavedColorArrayName();

  // Detect the case where the color has never been set, 
  // and we are switching from outline to surface.
  // SavedColorField is initialized to -1 in PVSOurce contructor.
  // It is set to 0 when the user colors by property.
  if (colorField == -1)
    {
    this->PVSource->SetDefaultColorParameters();
    vtkPVColorMap* colorMap = this->PVSource->GetPVColorMap();
    if (colorMap)
      {
      colorField = this->PVSource->GetDisplayProxy()->GetScalarModeCM();
      colorArrayName = colorMap->GetArrayName();
      }
    }
        
  // See if the current selection still exists.
  // If not, set a new default.
  geomInfo = this->PVSource->GetDisplayProxy()->GetGeometryInformation();
  if (colorArrayName)
    {
    if (colorField == vtkSMDataObjectDisplayProxy::POINT_FIELD_DATA)
      {
      attrInfo = geomInfo->GetPointDataInformation();
      }
    else
      {
      attrInfo = geomInfo->GetCellDataInformation();
      }
    arrayInfo = attrInfo->GetArrayInformation(colorArrayName);
    if (arrayInfo == 0)
      {
      this->PVSource->SetDefaultColorParameters();
      vtkPVColorMap* colorMap = this->PVSource->GetPVColorMap();
      if (colorMap)
        {
        colorField = this->PVSource->GetDisplayProxy()->GetScalarModeCM();
        colorArrayName = colorMap->GetArrayName();
        }
      else
        {
        colorField = 0;
        colorArrayName = 0;
        }
      }
    }
      
  // Populate menus
  this->ColorSelectionMenu->GetMenu()->DeleteAllItems();
  this->ColorSelectionMenu->GetMenu()->AddRadioButton(
    "Property", this, "ColorByProperty");
  this->ColorSelectionMenu->SetPVSource(this->PVSource);

  this->ColorSelectionMenu->Update(0);
  if (colorArrayName)
    {
    // Verify that the old colorby array has not disappeared.
    attrInfo = (colorField == vtkSMDataObjectDisplayProxy::POINT_FIELD_DATA)?
      geomInfo->GetPointDataInformation() : geomInfo->GetCellDataInformation();
    arrayInfo = attrInfo->GetArrayInformation(colorArrayName);
    if (!attrInfo)
      {
      vtkErrorMacro("Could not find previous color setting.");
      this->ColorSelectionMenu->SetValue("Property");
      }
    else
      {
      this->ColorSelectionMenu->SetValue(colorArrayName,
                                         colorField);
      // If color map does not exist, 
      // we are switching from outline (which has no color arrays).
      vtkPVColorMap* colorMap = this->PVSource->GetPVColorMap();
      if (colorMap == 0)
        {
        this->PVSource->ColorByArray(colorArrayName, colorField); 
        }                                  
      }
    }
  else
    {
    this->ColorSelectionMenu->SetValue("Property");
    }

  this->UpdateColorMapUI();
}

//----------------------------------------------------------------------------
void vtkPVDisplayGUI::UpdateColorButton()
{
  double rgb[3];
  this->PVSource->GetDisplayProxy()->GetColorCM(rgb);
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
    !this->PVSource->GetDisplayProxy()->GetColorModeCM())
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
    (!this->PVSource->GetDisplayProxy()->GetInterpolateScalarsBeforeMappingCM() && 
     this->MapScalarsCheckVisible) ||
    this->PVSource->GetDisplayProxy()->GetScalarModeCM() 
    == vtkDataSet::CELL_DATA_FIELD)
    {
    this->InterpolateColorsCheckVisible = 0;
    this->InterpolateColorsCheck->SetSelectedState(0);
    }
  else
    {
    this->InterpolateColorsCheckVisible = 1;
    this->InterpolateColorsCheck->SetSelectedState(
      !this->PVSource->GetDisplayProxy()->GetInterpolateScalarsBeforeMappingCM());
    }
  this->UpdateEnableState();
}

//-----------------------------------------------------------------------------
void vtkPVDisplayGUI::UpdateVolumeGUI()
{
  vtkSMDataObjectDisplayProxy* pDisp = this->PVSource->GetDisplayProxy();

  // Determine if this is unstructured grid data and add the 
  // volume rendering option
  if ( this->RepresentationMenu->GetMenu()->HasItem( VTK_PV_VOLUME_LABEL ) )
    {
    this->RepresentationMenu->GetMenu()->DeleteItem(
      this->RepresentationMenu->GetMenu()->GetIndexOfItem(
        VTK_PV_VOLUME_LABEL));
    }
  
  if (!pDisp->GetHasVolumePipeline())
    {
    this->VolumeRenderMode = 0;
    return;
    }
  this->RepresentationMenu->GetMenu()->AddRadioButton(
    VTK_PV_VOLUME_LABEL, this, "DrawVolume");

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
    (pDisp->GetRepresentationCM() == vtkSMDataObjectDisplayProxy::VOLUME)? 1 : 0;
  this->VolumeScalarSelectionWidget->SetPVSource(this->PVSource);
  this->VolumeScalarSelectionWidget->SetColorSelectionCommand(
    "VolumeRenderByArray");
  this->VolumeScalarSelectionWidget->Update();
  
  this->VolumeRenderMethodMenu->GetMenu()->DeleteAllItems();
  
  if (pDisp->GetSupportsHAVSMapper() )
    {
    this->VolumeRenderMethodMenu->GetMenu()->AddRadioButton(
      VTK_PV_VOLUME_HAVS_METHOD_LABEL, this, "DrawVolumeHAVS" );
    }

  this->VolumeRenderMethodMenu->GetMenu()->AddRadioButton(
    VTK_PV_VOLUME_PT_METHOD_LABEL, this, "DrawVolumePT" );

  if (pDisp->GetSupportsZSweepMapper() )
    {
    this->VolumeRenderMethodMenu->GetMenu()->AddRadioButton(
      VTK_PV_VOLUME_ZSWEEP_METHOD_LABEL, this, "DrawVolumeZSweep" );
    }
  if (pDisp->GetSupportsBunykMapper() )
    {
    this->VolumeRenderMethodMenu->GetMenu()->AddRadioButton(
      VTK_PV_VOLUME_BUNYK_METHOD_LABEL, this, "DrawVolumeBunyk" );
    }
  
  switch (this->PVSource->GetDisplayProxy()->GetVolumeMapperTypeCM())
    {
    case vtkSMDataObjectDisplayProxy::PROJECTED_TETRA_VOLUME_MAPPER:
      this->VolumeRenderMethodMenu->SetValue
        (VTK_PV_VOLUME_PT_METHOD_LABEL);
      break;
    case vtkSMDataObjectDisplayProxy::HAVS_VOLUME_MAPPER:
      this->VolumeRenderMethodMenu->SetValue
        (VTK_PV_VOLUME_HAVS_METHOD_LABEL);
      break;
    case vtkSMDataObjectDisplayProxy::ZSWEEP_VOLUME_MAPPER:
      this->VolumeRenderMethodMenu->SetValue
        (VTK_PV_VOLUME_ZSWEEP_METHOD_LABEL);
      break;
    case vtkSMDataObjectDisplayProxy::BUNYK_RAY_CAST_VOLUME_MAPPER:
      this->VolumeRenderMethodMenu->SetValue
        (VTK_PV_VOLUME_BUNYK_METHOD_LABEL);
      break;
    }
}
//----------------------------------------------------------------------------
void vtkPVDisplayGUI::SetActorColor(double r, double g, double b)
{
  this->ActorColor[0] = r;
  this->ActorColor[1] = g;
  this->ActorColor[2] = b;
  this->PVSource->GetDisplayProxy()->SetColorCM(this->ActorColor);
}  

//----------------------------------------------------------------------------
void vtkPVDisplayGUI::ChangeActorColor(double r, double g, double b)
{
  this->GetTraceHelper()->AddEntry("$kw(%s) ChangeActorColor %f %f %f",
                      this->GetTclName(), r, g, b);

  this->SetActorColor(r, g, b);
  this->ColorButton->SetColor(r, g, b);

  if ( this->GetPVRenderView() )
    {
    this->GetPVRenderView()->EventuallyRender();
    }
  
  // I do not this these are used because this is a shared GUI object.
  //if (strcmp(this->ColorSelectionMenu->GetValue(), "Property") == 0)
  //  {
  //  this->ColorSetByUser = 1;
  //  }
}

//----------------------------------------------------------------------------
void vtkPVDisplayGUI::ColorByArray(const char* array, int field)
{
  this->GetTraceHelper()->AddEntry("$kw(%s) ColorByArray {%s} %d", 
                                   this->GetTclName(),
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
void vtkPVDisplayGUI::UpdateColorMapUI()
{
  vtkPVRenderView *rv = this->GetPVRenderView();
  if (this->PVSource)
    {
    vtkPVColorMap *colorMap = this->PVSource->GetPVColorMap();
    if (colorMap)
      {
      rv->GetColorMapUI()->UpdateColorMapUI(
        colorMap->GetArrayName(),
        colorMap->GetNumberOfVectorComponents(),
        this->PVSource->GetSavedColorArrayField());
      }
    }
}

//----------------------------------------------------------------------------
void vtkPVDisplayGUI::ColorByProperty()
{
  // I do not this these are used because this is a shared GUI object.
  //this->ColorSetByUser = 1;
  this->GetTraceHelper()->AddEntry("$kw(%s) ColorByProperty", 
                                   this->GetTclName());
  this->ColorSelectionMenu->SetValue("Property");
  this->ColorByPropertyInternal();
}

//----------------------------------------------------------------------------
void vtkPVDisplayGUI::ColorByPropertyInternal()
{
//  this->PVSource->GetDisplayProxy()->SetScalarVisibilityCM(0);
  // NOTE: don't ever directly set the Scalar Visibility on the part display.
  // Instead use PVSource. Since, we need to remove the LUT from the proxy 
  // property otherwise the batch may be incorrect.
  this->PVSource->ColorByArray((char*) 0, 0);

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
  this->GetTraceHelper()->AddEntry("$kw(%s) VolumeRenderByArray {%s} %d",
    this->GetTclName(), name, field);
  this->VolumeScalarSelectionWidget->SetValue(name , field);
  this->PVSource->VolumeRenderByArray(name, field);

  this->PVSource->ColorByArray(name, field); // So the LOD Mapper remains 
        //synchronized with the Volume mapper.
        //Note that this call also invalidates the "name" pointer.
  if (field == vtkSMDataObjectDisplayProxy::CELL_FIELD_DATA)
    {
    this->VolumeRenderMethodMenu->GetMenu()->SetItemStateToDisabled(
      VTK_PV_VOLUME_HAVS_METHOD_LABEL);
    }
  else
    {
    this->VolumeRenderMethodMenu->GetMenu()->SetItemStateToNormal(
      VTK_PV_VOLUME_HAVS_METHOD_LABEL);
    }

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
  this->MapScalarsCheck->SetSelectedState(0);  
  if (colorMap)
    {
    this->MapScalarsCheck->SetSelectedState(1);
    // See if the array satisfies conditions necessary for direct coloring.  
    vtkPVDataInformation* dataInfo = this->PVSource->GetDataInformation();
    vtkPVDataSetAttributesInformation* attrInfo;
    if (this->PVSource->GetDisplayProxy()->GetScalarModeCM() == 
        vtkSMDataObjectDisplayProxy::POINT_FIELD_DATA)
      {
      attrInfo = dataInfo->GetPointDataInformation();
      }
    else
      {
      attrInfo = dataInfo->GetCellDataInformation();
      }
    vtkPVArrayInformation* arrayInfo = attrInfo->GetArrayInformation(
      colorMap->GetArrayName());      
    // First set of conditions.
    if (arrayInfo && arrayInfo->GetDataType() == VTK_UNSIGNED_CHAR)
      {
      // Number of component restriction.
      if (arrayInfo->GetNumberOfComponents() == 3)
        { // I would like to have two as an option also ...
        // One component causes more trouble than it is worth.
        this->MapScalarsCheckVisible = 1;
        this->MapScalarsCheck->SetEnabled(1);
        this->MapScalarsCheck->SetSelectedState(
          this->PVSource->GetDisplayProxy()->GetColorModeCM());
        }
      else
        { // Keep VTK from directly coloring single component arrays.
        this->PVSource->GetDisplayProxy()->SetColorModeCM(1);
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
    this->GetTraceHelper()->AddEntry("$kw(%s) DrawWireframe", this->GetTclName());
    }
  this->RepresentationMenu->SetValue(VTK_PV_WIREFRAME_LABEL);
  this->VolumeRenderModeOff();
  this->PVSource->GetDisplayProxy()->SetRepresentationCM(
    vtkSMDataObjectDisplayProxy::WIREFRAME);

  if ( this->GetPVRenderView() )
    {
    this->GetPVRenderView()->EventuallyRender();
    }
  this->UpdateColorGUI(); 
}

//----------------------------------------------------------------------------
void vtkPVDisplayGUI::DrawPoints()
{
  if (this->GetPVSource()->GetInitialized())
    {
    this->GetTraceHelper()->AddEntry("$kw(%s) DrawPoints", this->GetTclName());
    }
  this->RepresentationMenu->SetValue(VTK_PV_POINTS_LABEL);
  this->VolumeRenderModeOff();
  this->PVSource->GetDisplayProxy()->SetRepresentationCM(
    vtkSMDataObjectDisplayProxy::POINTS);
  
  if ( this->GetPVRenderView() )
    {
    this->GetPVRenderView()->EventuallyRender();
    }
  this->UpdateColorGUI(); 
}

//----------------------------------------------------------------------------
void vtkPVDisplayGUI::DrawVolume()
{
  if (this->GetPVSource()->GetInitialized())
    {
    this->GetTraceHelper()->AddEntry("$kw(%s) DrawVolume", this->GetTclName());
    }
  this->RepresentationMenu->SetValue(VTK_PV_VOLUME_LABEL);
  this->VolumeRenderModeOn();
  this->PVSource->GetDisplayProxy()->SetRepresentationCM(
    vtkSMDataObjectDisplayProxy::VOLUME);

  this->GetPVRenderView()->GetRenderWindow()->AddObserver( 
    vtkCommand::StartEvent, this->VRObserver, 1.0f );
  
  if ( this->GetPVRenderView() )
    {
    this->GetPVRenderView()->EventuallyRender();
    }
  this->UpdateColorGUI(); 
}

//----------------------------------------------------------------------------
void vtkPVDisplayGUI::DrawVolumePT()
{
  this->DrawVolumePTInternal();
  if ( this->GetPVRenderView() )
    {
    this->GetPVRenderView()->EventuallyRender();
    }
}

//----------------------------------------------------------------------------
void vtkPVDisplayGUI::DrawVolumePTInternal()
{
  this->PVSource->GetDisplayProxy()->SetVolumeMapperToPTCM();
}

//----------------------------------------------------------------------------
void vtkPVDisplayGUI::DrawVolumeHAVS()
{
  this->DrawVolumeHAVSInternal();
  if ( this->GetPVRenderView() )
    {
    this->GetPVRenderView()->EventuallyRender();
    }
}
//----------------------------------------------------------------------------
void vtkPVDisplayGUI::DrawVolumeHAVSInternal()
{
  if (strncmp( this->VolumeScalarSelectionWidget->GetValue(), "Cell", 4 ) != 0)
    {
    this->PVSource->GetDisplayProxy()->SetVolumeMapperToHAVSCM();
    }
  else
    {
    this->VolumeRenderMethodMenu->SetValue(VTK_PV_VOLUME_PT_METHOD_LABEL);
    this->DrawVolumePTInternal();
    }
}

//----------------------------------------------------------------------------
void vtkPVDisplayGUI::DrawVolumeZSweep()
{
  this->DrawVolumeZSweepInternal();
  if ( this->GetPVRenderView() )
    {
    this->GetPVRenderView()->EventuallyRender();
    }
}
//----------------------------------------------------------------------------
void vtkPVDisplayGUI::DrawVolumeZSweepInternal()
{
  this->PVSource->GetDisplayProxy()->SetVolumeMapperToZSweepCM();
}

//----------------------------------------------------------------------------
void vtkPVDisplayGUI::DrawVolumeBunyk()
{
  this->DrawVolumeBunykInternal();
  if ( this->GetPVRenderView() )
    {
    this->GetPVRenderView()->EventuallyRender();
    }
}
//----------------------------------------------------------------------------
void vtkPVDisplayGUI::DrawVolumeBunykInternal()
{
  this->PVSource->GetDisplayProxy()->SetVolumeMapperToBunykCM();
}

//----------------------------------------------------------------------------
void vtkPVDisplayGUI::DrawSurface()
{
  if (this->GetPVSource()->GetInitialized())
    {
    this->GetTraceHelper()->AddEntry("$kw(%s) DrawSurface", this->GetTclName());
    }
  this->RepresentationMenu->SetValue(VTK_PV_SURFACE_LABEL);
  this->VolumeRenderModeOff();
  
  // fixme
  // It would be better to loop over part displays from the render module.
  this->PVSource->GetDisplayProxy()->SetRepresentationCM(
    vtkSMDataObjectDisplayProxy::SURFACE);

  if ( this->GetPVRenderView() )
    {
    this->GetPVRenderView()->EventuallyRender();
    }

  this->UpdateColorGUI();
}

//----------------------------------------------------------------------------
void vtkPVDisplayGUI::DrawOutline()
{
  if (this->GetPVSource()->GetInitialized())
    {
    this->GetTraceHelper()->AddEntry("$kw(%s) DrawOutline", this->GetTclName());
    }
  this->RepresentationMenu->SetValue(VTK_PV_OUTLINE_LABEL);
  this->VolumeRenderModeOff();
  this->PVSource->GetDisplayProxy()->SetRepresentationCM(
    vtkSMDataObjectDisplayProxy::OUTLINE);

  if ( this->GetPVRenderView() )
    {
    this->GetPVRenderView()->EventuallyRender();
    }

  this->UpdateColorGUI();
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
    vtkSMDataObjectDisplayProxy* pDisp = this->PVSource->GetDisplayProxy();
    vtkSMStringVectorProperty* svp = vtkSMStringVectorProperty::SafeDownCast(
      pDisp->GetProperty("SelectScalarArray"));
    if (svp)
      {
      this->ColorByArray(svp->GetElement(0), pDisp->GetScalarModeCM());
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
      vtkSMDataObjectDisplayProxy* pDisp = this->PVSource->GetDisplayProxy();
      vtkSMStringVectorProperty* svp = vtkSMStringVectorProperty::SafeDownCast(
        pDisp->GetProperty("ColorArray"));
      if (svp)
        {
        this->VolumeRenderByArray(svp->GetElement(0), pDisp->GetScalarModeCM());
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
  this->GetTraceHelper()->AddEntry("$kw(%s) SetInterpolationToFlat", 
                      this->GetTclName());
  this->InterpolationMenu->SetValue("Flat");
  this->PVSource->GetDisplayProxy()->SetInterpolationCM(vtkSMDataObjectDisplayProxy::FLAT);

  if ( this->GetPVRenderView() )
    {
    this->GetPVRenderView()->EventuallyRender();
    }
}


//----------------------------------------------------------------------------
void vtkPVDisplayGUI::SetInterpolationToGouraud()
{
  this->GetTraceHelper()->AddEntry("$kw(%s) SetInterpolationToGouraud", 
                      this->GetTclName());
  this->InterpolationMenu->SetValue("Gouraud");

  this->PVSource->GetDisplayProxy()->SetInterpolationCM(vtkSMDataObjectDisplayProxy::GOURAND);
  
  if ( this->GetPVRenderView() )
    {
    this->GetPVRenderView()->EventuallyRender();
    }
}

//----------------------------------------------------------------------------
void vtkPVDisplayGUI::BrowseMaterial()
{
  if (!this->MaterialMenu)
    {
    return;
    }
  
  // This is a client side browser. The material file is located 
  // on the client, and sent to the server(s) if required.
  vtkKWLoadSaveDialog* dialog = vtkKWLoadSaveDialog::New(); 
  dialog->SetApplication(this->GetPVApplication());
  dialog->RetrieveLastPathFromRegistry("BrowseMaterial");
  dialog->Create();
  dialog->SetParent(this);
  dialog->SetTitle("Open Material Xml");
  dialog->SetFileTypes("{{Material Description XML} {.xml}} {{All Files} {*}}");
  
  if (dialog->Invoke())
    {
    this->SetMaterial(dialog->GetFileName(), 0);
    dialog->SaveLastPathToRegistry("BrowseMaterial");
    }
  dialog->Delete();
}


//----------------------------------------------------------------------------
void vtkPVDisplayGUI::SetMaterial(const char* materialname, 
  const char* menulabel)
{
  this->SetMaterialInternal(materialname, menulabel);
  
  this->GetTraceHelper()->AddEntry("$kw(%s) SetMaterial {%s} {%s}", 
    this->GetTclName(), (materialname? materialname: ""), 
    (menulabel? menulabel: ""));
}

//----------------------------------------------------------------------------
void vtkPVDisplayGUI::SetMaterialInternal(const char* materialname, 
  const char* menulabel)
{
  if (!this->MaterialMenu)
    {
    return;
    }

  vtkSMDataObjectDisplayProxy* pDisp = this->PVSource->GetDisplayProxy();
  const char* old_material_name = pDisp->GetMaterialCM();
  if (!materialname || strlen(materialname) == 0)
    {
    if (old_material_name != 0)
      {
      pDisp->SetMaterialCM(0);
      if ( this->GetPVRenderView() )
        {
        this->GetPVRenderView()->EventuallyRender();
        }

      }
    this->MaterialMenu->SetValue(VTK_PV_MATERIAL_NONE_LABEL);
    return;
    }

  vtkStdString label = (menulabel)? menulabel: "";
  if (!menulabel || label == "")
    {
    // Detemine the label from the material name.
    vtkStdString filename = materialname; 
    // First decide if the material name is a library name or a file name.
    vtkStdString name = vtksys::SystemTools::GetFilenameWithoutLastExtension(
      filename); 
    if (name == filename)
      {
      // most certainly a library name --- in that case simply use the library 
      // name as lable.
      label = name;
      }
    else
      {
      // materialname is a xml file name.
      // Was the file already assigned a label?
      vtkPVDisplayGUIInternal::MapOfStringToString::iterator iter =
        this->Internal->MaterialsMap.find(filename);

      if (iter == this->Internal->MaterialsMap.end())
        {
        // This is a never-before used xml. Give it a nice label.
        name += " (xml)";
        label = name;
        int count = 1;
        // Add a new entry for this file.
        while (this->MaterialMenu->GetMenu()->HasItem(label.c_str()))
          {
          vtksys_ios::ostringstream stream;
          stream << name.c_str() << " " << count++;
          label = stream.str();
          }
        this->Internal->MaterialsMap[filename] = label;
        }
      else
        {
        // Use the previously assigned label for this file.
        label = iter->second;
        }
      }
    }

  if (!this->MaterialMenu->GetMenu()->HasItem(label.c_str()))
    {
    this->MaterialMenu->GetMenu()->DeleteItem(
      this->MaterialMenu->GetMenu()->GetIndexOfItem(
        VTK_PV_MATERIAL_BROWSE_LABEL));

    vtksys_ios::ostringstream stream;
    stream << "SetMaterial {" << materialname << "} {" << label.c_str() << "}";
    vtksys_ios::ostringstream help;
    help << "Load Material " << materialname;
    int index = this->MaterialMenu->GetMenu()->AddRadioButton(
      label.c_str(), this, stream.str().c_str());
    this->MaterialMenu->GetMenu()->SetItemHelpString(
      index, help.str().c_str());

    // Since browse button must be the last button.
    index = this->MaterialMenu->GetMenu()->AddRadioButton(
      VTK_PV_MATERIAL_BROWSE_LABEL, this, "BrowseMaterial");
    this->MaterialMenu->GetMenu()->SetItemHelpString(
      index , VTK_PV_MATERIAL_BROWSE_HELP);
    }
  this->MaterialMenu->SetValue(label.c_str());

  if (!old_material_name || strcmp(old_material_name, materialname) != 0)
    {
    pDisp->SetMaterialCM(materialname);
    if ( this->GetPVRenderView() )
      {
      this->GetPVRenderView()->EventuallyRender();
      }

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
  else if (dataSetType == VTK_HYPER_OCTREE)
    {
    this->SetRepresentation(VTK_PV_SURFACE_LABEL);
    }
  else if (dataSetType == VTK_GENERIC_DATA_SET)
    {
    this->SetRepresentation(VTK_PV_OUTLINE_LABEL);
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
  vtkSMRenderModuleProxy* renderModule = pvApp->GetRenderModuleProxy();

  double bounds[6];
  this->GetPVSource()->GetDataInformation()->GetBounds(bounds);
  if (bounds[0]<=bounds[1] && bounds[2]<=bounds[3] && bounds[4]<=bounds[5])
    {
    vtkPVWindow* window = this->GetPVSource()->GetPVWindow();
    window->SetCenterOfRotation(0.5*(bounds[0]+bounds[1]), 
                                0.5*(bounds[2]+bounds[3]),
                                0.5*(bounds[4]+bounds[5]));
    window->ResetCenterCallback();
    renderModule->ResetCamera(bounds);
    renderModule->ResetCameraClippingRange();
        
    if ( this->GetPVRenderView() )
      {
      this->GetPVRenderView()->EventuallyRender();
      }
    }
  
  this->GetTraceHelper()->AddEntry("$kw(%s) CenterCamera", this->GetTclName());
}

//----------------------------------------------------------------------------
void vtkPVDisplayGUI::VisibilityCheckCallback()
{
  this->GetPVSource()->SetVisibility(this->VisibilityCheck->GetSelectedState());
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
  int state = this->ScalarBarCheck->GetSelectedState();
  this->PVSource->GetPVColorMap()->SetScalarBarVisibility(state);
  if (this->GetPVRenderView()->GetColorMapUI()->GetCurrentColorMap() ==
      this->PVSource->GetPVColorMap())
    {
    this->GetPVRenderView()->GetColorMapUI()->VisibilityCheckCallback(state);
    }

  if ( this->GetPVRenderView() )
    {
    this->GetPVRenderView()->EventuallyRender();
    }
}

//----------------------------------------------------------------------------
void vtkPVDisplayGUI::CubeAxesCheckCallback(int state)
{
  //law int fixme;  // Loading the trace will not trace the visibility.
  // Move the tracing into vtkPVSource.
  this->GetTraceHelper()->AddEntry("$kw(%s) SetCubeAxesVisibility %d", 
                      this->PVSource->GetTclName(),
                      state);
  this->PVSource->SetCubeAxesVisibility(state);
  if ( this->GetPVRenderView() )
    {
    this->GetPVRenderView()->EventuallyRender();
    }
}

//----------------------------------------------------------------------------
void vtkPVDisplayGUI::PointLabelCheckCallback(int state)
{
  //PVSource does tracing for us
  this->PVSource->SetPointLabelVisibility(state);
}

//----------------------------------------------------------------------------
void vtkPVDisplayGUI::MapScalarsCheckCallback()
{
  this->SetMapScalarsFlag(this->MapScalarsCheck->GetSelectedState());
  if ( this->GetPVRenderView() )
    {
    this->GetPVRenderView()->EventuallyRender();
    }
}

//----------------------------------------------------------------------------
void vtkPVDisplayGUI::SetMapScalarsFlag(int val)
{
  this->GetTraceHelper()->AddEntry("$kw(%s) SetMapScalarsFlag %d", this->GetTclName(), val);
  if (this->MapScalarsCheck->GetSelectedState() != val)
    {
    this->MapScalarsCheck->SetSelectedState(val);
    }

  this->UpdateEnableState();

  this->PVSource->GetDisplayProxy()->SetColorModeCM(val);
  this->UpdateColorGUI();
}

//----------------------------------------------------------------------------
void vtkPVDisplayGUI::InterpolateColorsCheckCallback()
{
  this->SetInterpolateColorsFlag(this->InterpolateColorsCheck->GetSelectedState());
  if ( this->GetPVRenderView() )
    {
    this->GetPVRenderView()->EventuallyRender();
    }
}

//----------------------------------------------------------------------------
void vtkPVDisplayGUI::SetInterpolateColorsFlag(int val)
{
  this->GetTraceHelper()->AddEntry("$kw(%s) SetInterpolateColorsFlag %d", this->GetTclName(), val);
  if (this->InterpolateColorsCheck->GetSelectedState() != val)
    {
    this->InterpolateColorsCheck->SetSelectedState(val);
    }

  this->PVSource->GetDisplayProxy()->SetInterpolateScalarsBeforeMappingCM(!val);
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
  this->GetTraceHelper()->AddEntry("$kw(%s) SetPointSize %d", this->GetTclName(),
                      (int)(this->PointSizeThumbWheel->GetValue()));
}

//----------------------------------------------------------------------------
void vtkPVDisplayGUI::ChangePointSizeCallback(double value)
{
  this->PVSource->GetDisplayProxy()->SetPointSizeCM(value);
 
  if ( this->GetPVRenderView() )
    {
    this->GetPVRenderView()->EventuallyRender();
    }
} 

//----------------------------------------------------------------------------
void vtkPVDisplayGUI::ChangePointSizeEndCallback(double value)
{
  this->ChangePointSizeCallback(value);
  this->GetTraceHelper()->AddEntry("$kw(%s) SetPointSize %d", this->GetTclName(),
                      (int)(value));
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
  this->GetTraceHelper()->AddEntry("$kw(%s) SetLineWidth %d", this->GetTclName(),
                      (int)(this->LineWidthThumbWheel->GetValue()));
}

//----------------------------------------------------------------------------
void vtkPVDisplayGUI::ChangeLineWidthCallback(double value)
{
  this->PVSource->GetDisplayProxy()->SetLineWidthCM(value);

  if ( this->GetPVRenderView() )
    {
    this->GetPVRenderView()->EventuallyRender();
    }
}

//----------------------------------------------------------------------------
void vtkPVDisplayGUI::ChangeLineWidthEndCallback(double value)
{
  this->ChangeLineWidthCallback(value);
  this->GetTraceHelper()->AddEntry("$kw(%s) SetLineWidth %d", this->GetTclName(),
                      (int)(value));
}

//----------------------------------------------------------------------------
void vtkPVDisplayGUI::SetPointLabelFontSize(int size)
{
  if ( this->PointLabelFontSizeThumbWheel->GetValue() == size )
    {
    return;
    }
  this->PointLabelFontSizeThumbWheel->SetValue(size);
}

//----------------------------------------------------------------------------
void vtkPVDisplayGUI::ChangePointLabelFontSizeCallback(double value)
{
  this->PVSource->SetPointLabelFontSize((int)value);
 } 

//----------------------------------------------------------------------------
void vtkPVDisplayGUI::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);
  os << indent << "MainFrame: " << this->MainFrame << endl;
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
  // I do not this these are used because this is a shared GUI object.
  //os << indent << "ArraySetByUser: " << this->ArraySetByUser << endl;
  //os << indent << "ColorSetByUser: " << this->ColorSetByUser << endl;
  os << indent << "ActorColor: " << this->ActorColor[0] << ", " << this->ActorColor[1]
               << ", " << this->ActorColor[2] << endl;
  os << indent << "ShouldReinitialize: " << this->ShouldReinitialize << endl;
}

//----------------------------------------------------------------------------
void vtkPVDisplayGUI::SetOpacity(float val)
{ 
  this->OpacityScale->SetValue(val);
}

//----------------------------------------------------------------------------
void vtkPVDisplayGUI::OpacityChangedCallback(double value)
{
  this->PVSource->GetDisplayProxy()->SetOpacityCM(value);

  if ( this->GetPVRenderView() )
    {
    this->GetPVRenderView()->EventuallyRender();
    }
}

//----------------------------------------------------------------------------
void vtkPVDisplayGUI::OpacityChangedEndCallback(double value)
{
  this->OpacityChangedCallback(value);
  this->GetTraceHelper()->AddEntry("$kw(%s) SetOpacity %f", 
                      this->GetTclName(), value);
}

//----------------------------------------------------------------------------
void vtkPVDisplayGUI::GetActorTranslate(double* point)
{
  vtkSMDataObjectDisplayProxy* pDisp = this->PVSource->GetDisplayProxy();
  if (pDisp)
    {
    pDisp->GetPositionCM(point);
    }
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
  double pos[3];
  pos[0] = x; pos[1] = y; pos[2] = z;
  this->PVSource->GetDisplayProxy()->SetPositionCM(pos);
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

  this->GetTraceHelper()->AddEntry("$kw(%s) SetActorTranslate %f %f %f",
                      this->GetTclName(), x, y, z);  
}

//----------------------------------------------------------------------------
void vtkPVDisplayGUI::SetActorTranslate(double* point)
{
  this->SetActorTranslate(point[0], point[1], point[2]);
}

//----------------------------------------------------------------------------
void vtkPVDisplayGUI::ActorTranslateCallback(double)
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
void vtkPVDisplayGUI::ActorTranslateEndCallback(double)
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
  vtkSMDataObjectDisplayProxy* pDisp = this->PVSource->GetDisplayProxy();
  if (pDisp)
    {
    pDisp->GetScaleCM(point);
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
  double scale[3];
  scale[0] = x; scale[1] = y; scale[2] = z;
  this->PVSource->GetDisplayProxy()->SetScaleCM(scale);
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

  this->GetTraceHelper()->AddEntry("$kw(%s) SetActorScale %f %f %f",
                      this->GetTclName(), x, y, z);  
}

//----------------------------------------------------------------------------
void vtkPVDisplayGUI::SetActorScale(double* point)
{
  this->SetActorScale(point[0], point[1], point[2]);
}

//----------------------------------------------------------------------------
void vtkPVDisplayGUI::ActorScaleCallback(double)
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
void vtkPVDisplayGUI::ActorScaleEndCallback(double)
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
  vtkSMDataObjectDisplayProxy* pDisp = this->PVSource->GetDisplayProxy();
  if (pDisp)
    {
    pDisp->GetOrientationCM(point);
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
  double orient[3];
  orient[0] = x; orient[1] = y; orient[2] = z;
  this->PVSource->GetDisplayProxy()->SetOrientationCM(orient);
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

  this->GetTraceHelper()->AddEntry("$kw(%s) SetActorOrientation %f %f %f",
                      this->GetTclName(), x, y, z);  
}

//----------------------------------------------------------------------------
void vtkPVDisplayGUI::SetActorOrientation(double* point)
{
  this->SetActorOrientation(point[0], point[1], point[2]);
}

//----------------------------------------------------------------------------
void vtkPVDisplayGUI::ActorOrientationCallback(double)
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
void vtkPVDisplayGUI::ActorOrientationEndCallback(double)
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
  if (this->PVSource->GetDisplayProxy())
    {
    this->PVSource->GetDisplayProxy()->GetOriginCM(point);
    }
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
  double origin[3];
  origin[0] = x; origin[1] = y; origin[2] = z;
  this->PVSource->GetDisplayProxy()->SetOriginCM(origin);

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

  this->GetTraceHelper()->AddEntry("$kw(%s) SetActorOrigin %f %f %f",
                      this->GetTclName(), x, y, z);  
}

//----------------------------------------------------------------------------
void vtkPVDisplayGUI::SetActorOrigin(double* point)
{
  this->SetActorOrigin(point[0], point[1], point[2]);
}

//----------------------------------------------------------------------------
void vtkPVDisplayGUI::ActorOriginCallback(double)
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
void vtkPVDisplayGUI::ActorOriginEndCallback(double)
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
  vtkSMDataObjectDisplayProxy* pDisp = this->PVSource->GetDisplayProxy();
  pDisp->GetPositionCM(translate);
  pDisp->GetScaleCM(scale);
  pDisp->GetOrientationCM(orientation);
  pDisp->GetOriginCM(origin);
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

//----------------------------------------------------------------------------
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

#ifndef VTK_LEGACY_REMOVE
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
#endif



