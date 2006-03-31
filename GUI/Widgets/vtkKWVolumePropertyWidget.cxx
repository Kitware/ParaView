/*=========================================================================

  Module:    vtkKWVolumePropertyWidget.cxx

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkKWVolumePropertyWidget.h"

#include "vtkColorTransferFunction.h"
#include "vtkDataArray.h"
#include "vtkDataSet.h"
#include "vtkImageData.h"
#include "vtkKWCheckButton.h"
#include "vtkKWColorTransferFunctionEditor.h"
#include "vtkKWEntryWithLabel.h"
#include "vtkKWEvent.h"
#include "vtkKWFrame.h"
#include "vtkKWFrameWithLabel.h"
#include "vtkKWHSVColorSelector.h"
#include "vtkKWHistogramSet.h"
#include "vtkKWIcon.h"
#include "vtkKWLabel.h"
#include "vtkKWMenu.h"
#include "vtkKWMenuButton.h"
#include "vtkKWMenuButtonWithLabel.h"
#include "vtkKWPiecewiseFunctionEditor.h"
#include "vtkKWPopupButtonWithLabel.h"
#include "vtkKWScalarComponentSelectionWidget.h"
#include "vtkKWScaleWithEntry.h"
#include "vtkKWScaleWithEntrySet.h"
#include "vtkKWScaleWithEntrySetWithLabel.h"
#include "vtkKWVolumeMaterialPropertyWidget.h"
#include "vtkMath.h"
#include "vtkObjectFactory.h"
#include "vtkPiecewiseFunction.h"
#include "vtkPointData.h"
#include "vtkVolumeProperty.h"
#include "vtkKWTkUtilities.h"

#include <vtksys/stl/string>

#define VTK_KW_VPW_INTERPOLATION_LINEAR     "Linear"
#define VTK_KW_VPW_INTERPOLATION_NEAREST    "Nearest"

#define VTK_KW_VPW_TESTING 0

//----------------------------------------------------------------------------
vtkCxxRevisionMacro(vtkKWVolumePropertyWidget, "1.38");
vtkStandardNewMacro(vtkKWVolumePropertyWidget);

//----------------------------------------------------------------------------
vtkKWVolumePropertyWidget::vtkKWVolumePropertyWidget()
{
  int i;

  this->VolumeProperty                = NULL;
  this->DataSet                       = NULL;
  this->HistogramSet                  = NULL;

  this->SelectedComponent             = 0;
  this->DisableCommands               = 0;
  this->EnableShadingForAllComponents = 0;

  this->InteractiveApplyMode              = 0;
  this->InteractiveApplyButtonVisibility  = 1;
  this->ScalarOpacityUnitDistanceVisibility  = 1;
  this->HSVColorSelectorVisibility        = 1;
  this->ComponentSelectionVisibility      = 1;
  this->InterpolationTypeVisibility       = 1;
  this->MaterialPropertyVisibility        = 1;
  this->GradientOpacityFunctionVisibility = 1;
  this->ComponentWeightsVisibility        = 1;
  this->MaterialPropertyPosition   = vtkKWVolumePropertyWidget::MaterialPropertyPositionTop;

  this->UseScalarColorFunctionInScalarOpacityEditor        = 0;

  this->VolumePropertyChangedCommand  = NULL;
  this->VolumePropertyChangingCommand = NULL;

  // GUI

  this->EditorFrame                     = vtkKWFrameWithLabel::New();

  this->InnerLeftFrame                  = vtkKWFrame::New();

  this->InterpolationTypeOptionMenu     = vtkKWMenuButtonWithLabel::New();

  this->EnableShadingCheckButton        = vtkKWCheckButton::New();

  this->InteractiveApplyCheckButton     = vtkKWCheckButton::New();

  this->LockOpacityAndColorCheckButton  = vtkKWCheckButton::New();

  this->ScalarOpacityUnitDistanceScale  = vtkKWScaleWithEntry::New();

  this->EnableGradientOpacityOptionMenu = vtkKWMenuButton::New();

  this->ComponentWeightScaleSet         = vtkKWScaleWithEntrySetWithLabel::New();

  this->ComponentSelectionWidget = 
    vtkKWScalarComponentSelectionWidget::New();

  this->MaterialPropertyWidget = 
    vtkKWVolumeMaterialPropertyWidget::New();

  this->ScalarOpacityFunctionEditor   = 
    vtkKWPiecewiseFunctionEditor::New();

  this->GradientOpacityFunctionEditor = 
    vtkKWPiecewiseFunctionEditor::New();

  this->ScalarColorFunctionEditor = 
    vtkKWColorTransferFunctionEditor::New();

  this->BottomFrame                  = vtkKWFrame::New();

  for (i = 0; i < VTK_MAX_VRCOMP; i++)
    {
    this->LockOpacityAndColor[i] = 0;
    this->WindowLevelMode[i] = 0;
    }

  this->HSVColorSelector           = vtkKWHSVColorSelector::New();
}

//----------------------------------------------------------------------------
vtkKWVolumePropertyWidget::~vtkKWVolumePropertyWidget()
{
  // Commands

  if (this->VolumePropertyChangedCommand)
    {
    delete [] this->VolumePropertyChangedCommand;
    this->VolumePropertyChangedCommand = NULL;
    }

  if (this->VolumePropertyChangingCommand)
    {
    delete [] this->VolumePropertyChangingCommand;
    this->VolumePropertyChangingCommand = NULL;
    }

  // GUI

  if (this->EditorFrame)
    {
    this->EditorFrame->Delete();
    this->EditorFrame = NULL;
    }

  if (this->InnerLeftFrame)
    {
    this->InnerLeftFrame->Delete();
    this->InnerLeftFrame = NULL;
    }

  if (this->ComponentSelectionWidget)
    {
    this->ComponentSelectionWidget->Delete();
    this->ComponentSelectionWidget = NULL;
    }

  if (this->InterpolationTypeOptionMenu)
    {
    this->InterpolationTypeOptionMenu->Delete();
    this->InterpolationTypeOptionMenu = NULL;
    }

  if (this->EnableShadingCheckButton)
    {
    this->EnableShadingCheckButton->Delete();
    this->EnableShadingCheckButton = NULL;
    }

  if (this->MaterialPropertyWidget)
    {
    this->MaterialPropertyWidget->Delete();
    this->MaterialPropertyWidget = NULL;
    }

  if (this->InteractiveApplyCheckButton)
    {
    this->InteractiveApplyCheckButton->Delete();
    this->InteractiveApplyCheckButton = NULL;
    }

  if (this->ScalarOpacityFunctionEditor)
    {
    this->ScalarOpacityFunctionEditor->Delete();
    this->ScalarOpacityFunctionEditor = NULL;
    }

  if (this->ScalarOpacityUnitDistanceScale)
    {
    this->ScalarOpacityUnitDistanceScale->Delete();
    this->ScalarOpacityUnitDistanceScale = NULL;
    }

  if (this->LockOpacityAndColorCheckButton)
    {
    this->LockOpacityAndColorCheckButton->Delete();
    this->LockOpacityAndColorCheckButton = NULL;
    }

  if (this->ScalarColorFunctionEditor)
    {
    this->ScalarColorFunctionEditor->Delete();
    this->ScalarColorFunctionEditor = NULL;
    }

  if (this->EnableGradientOpacityOptionMenu)
    {
    this->EnableGradientOpacityOptionMenu->Delete();
    this->EnableGradientOpacityOptionMenu = NULL;
    }

  if (this->GradientOpacityFunctionEditor)
    {
    this->GradientOpacityFunctionEditor->Delete();
    this->GradientOpacityFunctionEditor = NULL;
    }

  if (this->ComponentWeightScaleSet)
    {
    this->ComponentWeightScaleSet->Delete();
    this->ComponentWeightScaleSet = NULL;
    }

  if (this->HSVColorSelector)
    {
    this->HSVColorSelector->Delete();
    this->HSVColorSelector = NULL;
    }

  if (this->BottomFrame)
    {
    this->BottomFrame->Delete();
    this->BottomFrame = NULL;
    }

  this->SetHistogramSet(NULL);
  this->SetVolumeProperty(NULL);
  this->SetDataSet(NULL);
}

//----------------------------------------------------------------------------
void vtkKWVolumePropertyWidget::Create()
{
  // Check if already created

  if (this->IsCreated())
    {
    vtkErrorMacro(<< this->GetClassName() << " already created");
    return;
    }

  // Call the superclass to create the whole widget

  this->Superclass::Create();

  ostrstream tk_cmd;
  int label_width = 12;
  int menu_width = 6;
  char command[256];
  vtkKWMenu *menu;

  // --------------------------------------------------------------
  // Frame

  this->EditorFrame->SetParent(this);
  this->EditorFrame->Create();
  this->EditorFrame->SetLabelText("Volume Appearance Settings");

  vtkKWFrame *frame = this->EditorFrame->GetFrame();

  // --------------------------------------------------------------
  // Inner frame

  this->InnerLeftFrame->SetParent(frame);
  this->InnerLeftFrame->Create();

  // --------------------------------------------------------------
  // Component selection

  this->ComponentSelectionWidget->SetParent(this->InnerLeftFrame);
  this->ComponentSelectionWidget->Create();
  this->ComponentSelectionWidget->SetSelectedComponentChangedCommand(
    this, "SelectedComponentCallback");

  vtkKWMenuButtonWithLabel *menubuttonwl = 
    this->ComponentSelectionWidget->GetSelectedComponentOptionMenu();
  menubuttonwl->SetLabelWidth(label_width);
  menubuttonwl->GetWidget()->SetWidth(menu_width);

  // --------------------------------------------------------------
  // Interpolation type

  if (!this->InterpolationTypeOptionMenu)
    {
    this->InterpolationTypeOptionMenu = vtkKWMenuButtonWithLabel::New();
    }

  this->InterpolationTypeOptionMenu->SetParent(this->InnerLeftFrame);
  this->InterpolationTypeOptionMenu->Create();
  this->InterpolationTypeOptionMenu->ExpandWidgetOff();
  this->InterpolationTypeOptionMenu->SetLabelText("Interpolation:");
  this->InterpolationTypeOptionMenu->SetLabelWidth(label_width);
  this->InterpolationTypeOptionMenu->GetWidget()->SetWidth(menu_width);
  this->InterpolationTypeOptionMenu->SetBalloonHelpString(
    "Set the interpolation type used for sampling the volume.");

  vtkKWMenuButton *menubutton = this->InterpolationTypeOptionMenu->GetWidget();
  menu = menubutton->GetMenu();

  char callback[128];

  sprintf(callback, "InterpolationTypeCallback %d", VTK_LINEAR_INTERPOLATION);
  menu->AddRadioButton(VTK_KW_VPW_INTERPOLATION_LINEAR, this, callback);

  sprintf(callback, "InterpolationTypeCallback %d", VTK_NEAREST_INTERPOLATION);
  menu->AddRadioButton(VTK_KW_VPW_INTERPOLATION_NEAREST, this, callback);

  // --------------------------------------------------------------
  // Enable shading

  this->EnableShadingCheckButton->SetParent(this->InnerLeftFrame);
  this->EnableShadingCheckButton->Create();
  this->EnableShadingCheckButton->SetText("Enable Shading");
  this->EnableShadingCheckButton->SetBalloonHelpString(
    "Enable shading (for all components).");
  this->EnableShadingCheckButton->SetCommand(
    this, "EnableShadingCallback");

  // --------------------------------------------------------------
  // Material properties : widget

  if (!this->MaterialPropertyWidget->GetParent())
    {
    this->MaterialPropertyWidget->SetParent(this);
    }
  this->MaterialPropertyWidget->PopupModeOn();
  this->MaterialPropertyWidget->Create();
  this->MaterialPropertyWidget->GetPopupButton()->SetLabelWidth(label_width);
  this->MaterialPropertyWidget->GetComponentSelectionWidget()
    ->AllowComponentSelectionOff();
  this->MaterialPropertyWidget->SetPropertyChangedCommand(
    this, "MaterialPropertyChangedCallback");
  this->MaterialPropertyWidget->SetPropertyChangingCommand(
    this, "MaterialPropertyChangingCallback");

  // --------------------------------------------------------------
  // Interactive Apply

  this->InteractiveApplyCheckButton->SetParent(this->InnerLeftFrame);
  this->InteractiveApplyCheckButton->Create();
  this->InteractiveApplyCheckButton->SetText("Interactive Apply");
  this->InteractiveApplyCheckButton->SetCommand(
    this, "InteractiveApplyCallback");
  this->InteractiveApplyCheckButton->SetBalloonHelpString(
    "Toggle whether changes are applied to the volume window and image "
    "windows as nodes in the transfer functions are modified, or only after "
    "the mouse button is released.");

  // --------------------------------------------------------------
  // Scalar opacity editor

  if (!this->ScalarOpacityFunctionEditor->GetParent())
    {
    this->ScalarOpacityFunctionEditor->SetParent(frame);
    }
  this->ScalarOpacityFunctionEditor->SetLabelText("Scalar Opacity Mapping:");
  this->ScalarOpacityFunctionEditor->ComputePointColorFromValueOff();
  this->ScalarOpacityFunctionEditor->LockEndPointsParameterOn();
  this->ScalarOpacityFunctionEditor->SetLabelPosition(
    vtkKWParameterValueFunctionEditor::LabelPositionTop);
  this->ScalarOpacityFunctionEditor->SetRangeLabelPosition(
    vtkKWParameterValueFunctionEditor::RangeLabelPositionTop);
  this->ScalarOpacityFunctionEditor->ValueRangeVisibilityOff();
  this->ScalarOpacityFunctionEditor->SetValueRangeLabelVisibility(
    this->ScalarOpacityFunctionEditor->GetValueRangeVisibility());
  this->ScalarOpacityFunctionEditor->WindowLevelModeButtonVisibilityOn();
  this->ScalarOpacityFunctionEditor->Create();

  this->ScalarOpacityFunctionEditor->GetParameterEntry()->SetLabelText("S:");
  this->ScalarOpacityFunctionEditor->GetValueEntry()->SetLabelText("O:");

  this->ScalarOpacityFunctionEditor->SetFunctionChangedCommand(
    this, "ScalarOpacityFunctionChangedCallback");
  this->ScalarOpacityFunctionEditor->SetFunctionChangingCommand(
    this, "ScalarOpacityFunctionChangingCallback");
  this->ScalarOpacityFunctionEditor->SetDoubleClickOnPointCommand(
    this, "DoubleClickOnScalarOpacityPointCallback");

  this->ScalarOpacityFunctionEditor->SetWindowLevelModeChangedCommand(
    this, "WindowLevelModeCallback");

  // --------------------------------------------------------------
  // Scalar Opacity Unit Distance

  this->ScalarOpacityFunctionEditor->UserFrameVisibilityOn();
  if (!this->ScalarOpacityUnitDistanceScale->GetParent())
    {
    this->ScalarOpacityUnitDistanceScale->SetParent(
      this->ScalarOpacityFunctionEditor->GetUserFrame());
    }
  this->ScalarOpacityUnitDistanceScale->PopupModeOn();
  this->ScalarOpacityUnitDistanceScale->Create();
  this->ScalarOpacityUnitDistanceScale->SetLabelText("Scale:");
  this->ScalarOpacityUnitDistanceScale->SetEndCommand(
    this, "ScalarOpacityUnitDistanceChangedCallback");
  this->ScalarOpacityUnitDistanceScale->SetEntryCommand(
    this, "ScalarOpacityUnitDistanceChangedCallback");
  this->ScalarOpacityUnitDistanceScale->SetCommand(
    this, "ScalarOpacityUnitDistanceChangingCallback");
  this->ScalarOpacityUnitDistanceScale->SetBalloonHelpString(
    "Set the unit distance on which the scalar opacity transfer function "
    "is defined.");

  // --------------------------------------------------------------
  // Color transfer function editor

  if (!this->ScalarColorFunctionEditor->GetParent())
    {
    this->ScalarColorFunctionEditor->SetParent(frame);
    }
  this->ScalarColorFunctionEditor->SetLabelText("Scalar Color Mapping:");
  this->ScalarColorFunctionEditor->SetCanvasHeight(
    this->ScalarOpacityFunctionEditor->GetCanvasHeight());
  this->ScalarColorFunctionEditor->LockEndPointsParameterOn();
  this->ScalarColorFunctionEditor->SetPointMarginToCanvas(
    this->ScalarOpacityFunctionEditor->GetPointMarginToCanvas());
  this->ScalarColorFunctionEditor->SetValueRangeVisibility(
    this->ScalarOpacityFunctionEditor->GetValueRangeVisibility());
  this->ScalarColorFunctionEditor->SetValueRangeLabelVisibility(
    this->ScalarColorFunctionEditor->GetValueRangeVisibility());
  this->ScalarColorFunctionEditor->SetLabelPosition(
    this->ScalarOpacityFunctionEditor->GetLabelPosition());
  this->ScalarColorFunctionEditor->SetRangeLabelPosition(
    this->ScalarOpacityFunctionEditor->GetRangeLabelPosition());
  this->ScalarColorFunctionEditor->Create();

  this->ScalarColorFunctionEditor->GetParameterEntry()->SetLabelText(
    this->ScalarOpacityFunctionEditor->GetParameterEntry()->GetLabel()
    ->GetText());

  this->ScalarColorFunctionEditor->SetFunctionChangedCommand(
    this, "RGBTransferFunctionChangedCallback");
  this->ScalarColorFunctionEditor->SetFunctionChangingCommand(
    this, "RGBTransferFunctionChangingCallback");
  this->ScalarColorFunctionEditor->SetSelectionChangedCommand(
    this, "RGBTransferFunctionSelectionChangedCallback");

  // --------------------------------------------------------------
  // Lock opacity and color

  this->ScalarColorFunctionEditor->UserFrameVisibilityOn();
  this->LockOpacityAndColorCheckButton->SetParent(
    this->ScalarColorFunctionEditor->GetUserFrame());
  this->LockOpacityAndColorCheckButton->Create();
  this->LockOpacityAndColorCheckButton->SetPadX(0);
  this->LockOpacityAndColorCheckButton->SetPadY(0);
  this->LockOpacityAndColorCheckButton->SetHighlightThickness(0);
  this->LockOpacityAndColorCheckButton->IndicatorVisibilityOff();
  this->LockOpacityAndColorCheckButton->SetText("Lock");
  this->LockOpacityAndColorCheckButton->SetBalloonHelpString(
    "Lock the opacity and color functions together.");
  this->LockOpacityAndColorCheckButton->SetCommand(
    this, "LockOpacityAndColorCallback");

  this->LockOpacityAndColorCheckButton->SetImageToPredefinedIcon(
    vtkKWIcon::IconLock);
 
  tk_cmd << "pack " << this->LockOpacityAndColorCheckButton->GetWidgetName() 
         << " -side left -fill both -padx 2" << endl;

  // --------------------------------------------------------------
  // Gradient opacity editor

  if (!this->GradientOpacityFunctionEditor->GetParent())
    {
    this->GradientOpacityFunctionEditor->SetParent(frame);
    }
  this->GradientOpacityFunctionEditor->SetLabelText("Gradient Opacity Mapping:");
  this->GradientOpacityFunctionEditor->ComputePointColorFromValueOn();
  this->GradientOpacityFunctionEditor->LockEndPointsParameterOn();
  this->GradientOpacityFunctionEditor->SetPointMarginToCanvas(
    this->ScalarOpacityFunctionEditor->GetPointMarginToCanvas());
  this->GradientOpacityFunctionEditor->SetCanvasHeight(
    this->ScalarColorFunctionEditor->GetCanvasHeight());
  this->GradientOpacityFunctionEditor->SetValueRangeVisibility(
    this->ScalarOpacityFunctionEditor->GetValueRangeVisibility());
  this->GradientOpacityFunctionEditor->SetValueRangeLabelVisibility(
    this->GradientOpacityFunctionEditor->GetValueRangeVisibility());
  this->GradientOpacityFunctionEditor->SetLabelPosition(
    this->ScalarOpacityFunctionEditor->GetLabelPosition());
  this->GradientOpacityFunctionEditor->SetRangeLabelPosition(
    this->ScalarOpacityFunctionEditor->GetRangeLabelPosition());
  this->GradientOpacityFunctionEditor->Create();

  this->GradientOpacityFunctionEditor->GetParameterEntry()->SetLabelText(
    this->ScalarOpacityFunctionEditor->GetParameterEntry()->GetLabel()
    ->GetText());
  this->GradientOpacityFunctionEditor->GetValueEntry()->SetLabelText("O:");

  this->GradientOpacityFunctionEditor->SetFunctionChangedCommand(
    this, "GradientOpacityFunctionChangedCallback");
  this->GradientOpacityFunctionEditor->SetFunctionChangingCommand(
    this, "GradientOpacityFunctionChangingCallback");

  // --------------------------------------------------------------
  // Enable gradient opacity

  this->GradientOpacityFunctionEditor->UserFrameVisibilityOn();
  this->EnableGradientOpacityOptionMenu->SetParent(
    this->GradientOpacityFunctionEditor->GetUserFrame());
  this->EnableGradientOpacityOptionMenu->Create();
  this->EnableGradientOpacityOptionMenu->SetPadX(1);
  this->EnableGradientOpacityOptionMenu->SetPadY(0);
  this->EnableGradientOpacityOptionMenu->IndicatorVisibilityOff();
  this->EnableGradientOpacityOptionMenu->SetBalloonHelpString(
    "Enable modulation of the opacity by the magnitude of the gradient "
    "according to the specified function.");

  menu = this->EnableGradientOpacityOptionMenu->GetMenu();
  menu->AddRadioButton("On", this, "EnableGradientOpacityCallback 1");
  menu->AddRadioButton("Off", this, "EnableGradientOpacityCallback 0");

  tk_cmd << "pack " << this->EnableGradientOpacityOptionMenu->GetWidgetName() 
         << " -side left -fill both -padx 0" << endl;

  // --------------------------------------------------------------
  // Component weights

  this->ComponentWeightScaleSet->SetParent(frame);
  this->ComponentWeightScaleSet->Create();
  this->ComponentWeightScaleSet->SetLabelText("Component Weights:");

  vtkKWScaleWithEntrySet *scaleset = 
    this->ComponentWeightScaleSet->GetWidget();

  scaleset->PackHorizontallyOn();
  scaleset->SetMaximumNumberOfWidgetsInPackingDirection(2);
  scaleset->SetWidgetsPadX(2);

  int i;
  char label[15];

  for (i = 0; i < VTK_MAX_VRCOMP; i++)
    {
    scaleset->AddWidget(i);
    scaleset->HideWidget(i);
    vtkKWScaleWithEntry *scale = scaleset->GetWidget(i);
    scale->SetResolution(0.01);
    sprintf(label, "%d:", i + 1);
    scale->SetLabelText(label);
    sprintf(command, "ComponentWeightChangedCallback %d", i);
    scale->SetEndCommand(this, command);
    scale->SetEntryCommand(this, command);
    sprintf(command, "ComponentWeightChangingCallback %d", i);
    scale->SetCommand(this, command);
    scale->SetEntryWidth(5);
    }

  // --------------------------------------------------------------
  // HSV Color Selector

  this->HSVColorSelector->SetParent(frame);
  this->HSVColorSelector->Create();
  this->HSVColorSelector->ModificationOnlyOn();
  this->HSVColorSelector->SetHueSatWheelRadius(54);
  this->HSVColorSelector->SetSelectionChangedCommand(
    this, "HSVColorSelectionChangedCallback");
  this->HSVColorSelector->SetSelectionChangingCommand(
    this, "HSVColorSelectionChangingCallback");

#if VTK_KW_VPW_TESTING
  cout << this->GetTclName() << endl;
  cout << "opacity: " << this->ScalarOpacityFunctionEditor->GetTclName() << endl;
  cout << "ctf: " << this->ScalarColorFunctionEditor->GetTclName() << endl;

  cout << "gradient: " << this->GradientOpacityFunctionEditor->GetTclName() << endl;
  cout << "hsvsel: " << this->HSVColorSelector->GetTclName() << endl;
  cout << "lock: " << this->LockOpacityAndColorCheckButton->GetTclName() << endl;
#endif

  // --------------------------------------------------------------
  // Bottom frame

  this->BottomFrame->SetParent(frame);
  this->BottomFrame->Create();

  // Sync

  this->ScalarColorFunctionEditor->SynchronizeSingleSelection(
    this->ScalarOpacityFunctionEditor);

  // Pack

  tk_cmd << ends;
  this->Script(tk_cmd.str());
  tk_cmd.rdbuf()->freeze(0);

  this->Pack();

  // Update

  this->Update();
}

//----------------------------------------------------------------------------
void vtkKWVolumePropertyWidget::Pack()
{
  if (!this->IsCreated())
    {
    return;
    }


  ostrstream tk_cmd;

  // Pack the frame

  tk_cmd << "pack " << this->EditorFrame->GetWidgetName() 
         << " -side top -fill both -expand y -pady 0 -padx 0 -ipady 0 -ipadx 0"
         << endl;

  // Regrid the internal widgets

  vtkKWFrame *frame = this->EditorFrame->GetFrame();
  frame->UnpackChildren();

  this->InnerLeftFrame->UnpackChildren();

  int row = 0;

  const char *colspan = " -columnspan 2 ";
  const char *col0 = " -column 0 ";
  const char *col1 = " -column 1 ";
  const char *pad = " -padx 2 -pady 2";
  const char *pad_ed = " -padx 2 -pady 3";
  
  vtksys_stl::string in_frame(" -in ");
  in_frame += frame->GetWidgetName();

  /*
               col0       col1
         +-------------------------
         |     SC        HSV
         |     IT         |
         |     MP         |
         |     ES         |
         |     IA         v
         |     SOF ------->
         |     CTF ------->
         |     GOF ------->
         |     CW  ------->
         |     BF  ------->
  */

  // Inner Frame


  tk_cmd << "grid " << this->InnerLeftFrame->GetWidgetName()
         << " -padx 0 -pady 0 -sticky nw " << col0 << " -row " << row << endl;
  
  // HSV Color Selector (HSV)

  if (this->HSVColorSelectorVisibility)
    {
    tk_cmd << "grid " << this->HSVColorSelector->GetWidgetName()
           << " -sticky nw " << col1 << " -row " << row << pad << endl;
    }

  // Select Component (SC)

  if (this->ComponentSelectionVisibility)
    {
    tk_cmd << "pack " << this->ComponentSelectionWidget->GetWidgetName()
           << " -side top -anchor nw " << pad << endl;
    }

  // Interpolation type (IT)

  if (this->InterpolationTypeVisibility)
    {
    tk_cmd << "pack " << this->InterpolationTypeOptionMenu->GetWidgetName()
           << " -side top -anchor nw " << pad << endl;
    }

  // Material Property (MP)

  if (!this->MaterialPropertyVisibility)
    {
    tk_cmd << "pack forget " << this->MaterialPropertyWidget->GetWidgetName()
           << endl;
    }
  else 
    {
    if (this->MaterialPropertyPosition == 
        vtkKWVolumePropertyWidget::MaterialPropertyPositionTop ||
      this->MaterialPropertyPosition == 
        vtkKWVolumePropertyWidget::MaterialPropertyPositionBottomFrame)
      {
      this->MaterialPropertyWidget->GetPopupButton()->LabelVisibilityOn();
      tk_cmd << "pack " << this->MaterialPropertyWidget->GetWidgetName()
             << " -side top -anchor nw " << pad << " -in ";
      if (this->MaterialPropertyPosition == 
          vtkKWVolumePropertyWidget::MaterialPropertyPositionTop)
        {
        tk_cmd << this->InnerLeftFrame->GetWidgetName() << endl;
        }
      else
        {
        tk_cmd << this->BottomFrame->GetWidgetName() << endl;
        }
      }
    else
      {
      this->MaterialPropertyWidget->GetPopupButton()->LabelVisibilityOff();
      tk_cmd << "pack " << this->MaterialPropertyWidget->GetWidgetName()
             << " -side right -fill both -padx 2 -pady 0 -in ";
      if (this->MaterialPropertyPosition == 
          vtkKWVolumePropertyWidget::MaterialPropertyPositionScalarOpacityUserFrame)
        {
        tk_cmd << 
          this->ScalarOpacityFunctionEditor->GetUserFrame()->GetWidgetName();
        }
      else
        {
        tk_cmd << 
          this->ScalarColorFunctionEditor->GetUserFrame()->GetWidgetName();
        }
      tk_cmd << endl;
      }
    }
  
  // Enable Shading (ES)

  if (this->MaterialPropertyVisibility)
    {
    tk_cmd << "pack " << this->EnableShadingCheckButton->GetWidgetName()
           << " -side top -anchor nw " << pad << endl;
    }

  // Interactive Apply (IA)

  if (this->InteractiveApplyButtonVisibility)
    {
    tk_cmd << "pack " << this->InteractiveApplyCheckButton->GetWidgetName()
           << " -side top -anchor nw " << pad << endl;
    }

  row++;

  // --------------------------------------------------------------

  // Scalar Opacity Function (SOF)

  tk_cmd << "grid " << this->ScalarOpacityFunctionEditor->GetWidgetName()
         << " -sticky ew -column 0 -row " << row << colspan << pad_ed 
         << in_frame.c_str();
  if (!this->ScalarColorFunctionEditor->GetCanvasVisibility())
    {
    tk_cmd << " -pady 0";
    }
  tk_cmd << endl;
  row++;

  if (this->ScalarOpacityUnitDistanceVisibility)
    {
    tk_cmd << "pack " << this->ScalarOpacityUnitDistanceScale->GetWidgetName() 
           << " -side right -fill both -padx 2 -pady 0" << endl;
    }
  else
    {
    tk_cmd << "pack forget " 
           << this->ScalarOpacityUnitDistanceScale->GetWidgetName() << endl;
    }

  // Color Transfer Function (CTF)

  tk_cmd << "grid " << this->ScalarColorFunctionEditor->GetWidgetName()
         << " -sticky ew -column 0 -row " << row << colspan << pad_ed
         << in_frame.c_str();
  if (!this->ScalarColorFunctionEditor->GetCanvasVisibility())
    {
    tk_cmd << " -pady 0";
    }
  tk_cmd << endl;
  row++;

  // Gradient Opacity Function (GOF)

  if (this->GradientOpacityFunctionVisibility)
    {
    tk_cmd << "grid " << this->GradientOpacityFunctionEditor->GetWidgetName()
           << " -sticky ew -column 0 -row " << row << colspan << pad_ed
           << in_frame.c_str() << endl;
    row++;
    }

  // Component weights (CW)

  if (this->ComponentWeightsVisibility)
    {
    tk_cmd << "grid " << this->ComponentWeightScaleSet->GetWidgetName()
           << " -sticky ew -column 0 -row " << row << colspan << pad << endl;

    row++;
    }

  tk_cmd << "grid " << this->BottomFrame->GetWidgetName()
         << " -sticky ew -column 0 -row " << row << colspan << endl;

  row++;

  // Make sure it can resize

  tk_cmd << "grid columnconfigure " 
         << frame->GetWidgetName() << " 0 -weight 1" << endl;
  
  tk_cmd << ends;
  this->Script(tk_cmd.str());
  tk_cmd.rdbuf()->freeze(0);
}

//----------------------------------------------------------------------------
void vtkKWVolumePropertyWidget::Update()
{
  // Update enable state

  this->UpdateEnableState();

  // Update sub-components

  int has_prop = this->VolumeProperty ? 1 : 0;

  int nb_components = this->GetNumberOfComponents();

  char hist_name[1024];
      
  ostrstream tk_cmd;

  // In the dependent case, everything is in the component 0

  if (!this->GetIndependentComponents() ||
      (this->SelectedComponent < 0 ||
       this->SelectedComponent >= nb_components))
    {
    this->SelectedComponent = 0;
    }

  int i;
  double p_range[2];

  // Component selection menu

  if (this->ComponentSelectionWidget)
    {
    this->ComponentSelectionWidget->SetIndependentComponents(
      this->GetIndependentComponents());
    this->ComponentSelectionWidget->SetNumberOfComponents(nb_components);
    this->ComponentSelectionWidget->SetSelectedComponent(
      this->SelectedComponent);
    }

  // Interpolation type

  if (InterpolationTypeOptionMenu)
    {
    vtkKWMenuButton *m = this->InterpolationTypeOptionMenu->GetWidget();
    if (has_prop)
      {
      switch (this->VolumeProperty->GetInterpolationType())
        {
        case VTK_NEAREST_INTERPOLATION:
          m->SetValue(VTK_KW_VPW_INTERPOLATION_NEAREST);
          break;
        case VTK_LINEAR_INTERPOLATION:
          m->SetValue(VTK_KW_VPW_INTERPOLATION_LINEAR);
          break;
        default:
          m->SetValue("Unknown");
        }
      }
    else
      {
      m->SetValue("");
      }
    }
    
  // Lock opacity and color

  if (this->LockOpacityAndColorCheckButton)
    {
    // If dependents or W/L, we can not lock

    if (this->WindowLevelMode[this->SelectedComponent] ||
        (has_prop && !this->GetIndependentComponents()))
      {
      this->LockOpacityAndColor[this->SelectedComponent] = 0;
      this->LockOpacityAndColorCheckButton->SetEnabled(0);
      }

    this->LockOpacityAndColorCheckButton->SetSelectedState(
      this->LockOpacityAndColor[this->SelectedComponent]);
    }

  // Enable shading for all

  if (this->EnableShadingCheckButton)
    {
    if (has_prop)
      {
      this->EnableShadingCheckButton->SetSelectedState(
        this->VolumeProperty->GetShade(0));
      }
    if (!this->EnableShadingForAllComponents)
      {
      this->EnableShadingCheckButton->SetEnabled(0);
      }
    if (EnableShadingCheckButton->IsCreated())
      {
      if (this->EnableShadingForAllComponents)
        {
        tk_cmd << "pack "
               << this->EnableShadingCheckButton->GetWidgetName() 
               << " -side top -anchor nw" << endl;
        }
      else
        {
        tk_cmd << "pack forget "
               << this->EnableShadingCheckButton->GetWidgetName() << endl;
        }
      }
    }
  if (this->EnableShadingForAllComponents && has_prop)
    {
    int nb_shade_comp = this->GetIndependentComponents() ? nb_components : 1;
    for (i = 1; i < nb_shade_comp; i++)
      {
      this->VolumeProperty->SetShade(i, this->VolumeProperty->GetShade(0));
      }
    }

  // Interactive Apply

  if (this->InteractiveApplyCheckButton)
    {
    this->InteractiveApplyCheckButton->SetSelectedState(
      this->InteractiveApplyMode);
    }

  // Material Property

  if (this->MaterialPropertyWidget)
    {
    this->MaterialPropertyWidget->SetVolumeProperty(
      this->VolumeProperty);
    this->MaterialPropertyWidget->SetNumberOfComponents(nb_components);
    this->MaterialPropertyWidget->SetSelectedComponent(
      this->SelectedComponent);
    this->MaterialPropertyWidget->SetAllowEnableShading(
      !this->EnableShadingForAllComponents);
    this->MaterialPropertyWidget->Update();
    if (!has_prop)
      {
      this->MaterialPropertyWidget->SetEnabled(0);
      }
    }

  // Scalar opacity

  if (this->ScalarOpacityFunctionEditor)
    {
    int scalar_field = this->GetIndependentComponents() 
      ? this->SelectedComponent : (nb_components - 1);

    if (has_prop)
      {
      vtkPiecewiseFunction *ofun = 
        this->VolumeProperty->GetScalarOpacity(this->SelectedComponent);
      this->ScalarOpacityFunctionEditor->SetPiecewiseFunction(ofun);

      if (this->GetDataSetAdjustedScalarRange(scalar_field, p_range))
        {
        this->ScalarOpacityFunctionEditor->SetWholeParameterRange(p_range);
        }
      else
        {
        this->ScalarOpacityFunctionEditor->SetWholeParameterRange(
          ofun->GetRange());
        }

      this->ScalarOpacityFunctionEditor->SetWholeValueRange(0.0, 1.0);
      this->ScalarOpacityFunctionEditor->SetWindowLevelMode(
        this->WindowLevelMode[this->SelectedComponent]);
      }
    else
      {
      this->ScalarOpacityFunctionEditor->SetPiecewiseFunction(0);
      }

    if (this->HistogramSet)
      {
      if (this->HistogramSet->ComputeHistogramName(
            this->GetDataSetScalarName(), scalar_field, NULL, hist_name))
        {
        this->ScalarOpacityFunctionEditor->SetHistogram(
          this->HistogramSet->GetHistogramWithName(hist_name));
        }
      }
    else
      {
      this->ScalarOpacityFunctionEditor->SetHistogram(NULL);
      }

    this->ScalarOpacityFunctionEditor->Update();
    }

  // Scalar Opacity Unit Distance

  if (this->ScalarOpacityUnitDistanceScale)
    {
    double soud_range[2], soud_res;
    if (this->GetDataSetScalarOpacityUnitDistanceRangeAndResolution(
          soud_range, &soud_res))
      {
      this->ScalarOpacityUnitDistanceScale->SetResolution(soud_res);
      this->ScalarOpacityUnitDistanceScale->SetRange(
        soud_range[0], soud_range[1]);
      }
    if (has_prop)
      {
      int old_disable = 
        this->ScalarOpacityUnitDistanceScale->GetDisableCommands();
      this->ScalarOpacityUnitDistanceScale->SetDisableCommands(1);
      this->ScalarOpacityUnitDistanceScale->SetValue(
        this->VolumeProperty->GetScalarOpacityUnitDistance(
          this->SelectedComponent));
      this->ScalarOpacityUnitDistanceScale->SetDisableCommands(old_disable);
      }
    }

  // Color transfer function

  int no_rgb = !this->GetIndependentComponents() && nb_components > 2;

  if (this->ScalarColorFunctionEditor)
    {
    int scalar_field = this->GetIndependentComponents() 
      ? this->SelectedComponent : 0;

    if (!no_rgb && has_prop && 
        this->VolumeProperty->GetColorChannels(this->SelectedComponent) == 3)
      {
      vtkColorTransferFunction *cfun = 
        this->VolumeProperty->GetRGBTransferFunction(this->SelectedComponent);
      this->ScalarColorFunctionEditor->SetColorTransferFunction(cfun);

      if (this->UseScalarColorFunctionInScalarOpacityEditor)
        {
        this->ScalarOpacityFunctionEditor->SetPointColorTransferFunction(cfun);
        }
      else
        {
        this->ScalarOpacityFunctionEditor->SetPointColorTransferFunction(NULL);
        }

      if (this->GetDataSetAdjustedScalarRange(scalar_field, p_range))
        {
        this->ScalarColorFunctionEditor->SetWholeParameterRange(p_range);
        }
      else
        {
        this->ScalarColorFunctionEditor->SetWholeParameterRange(
          cfun->GetRange());
        }
      }
    else
      {
      this->ScalarColorFunctionEditor->SetColorTransferFunction(0);
      }

    if (!no_rgb && this->HistogramSet)
      {
      if (this->HistogramSet->ComputeHistogramName(
            this->GetDataSetScalarName(), scalar_field, NULL, hist_name))
        {
        this->ScalarColorFunctionEditor->SetHistogram(
          this->HistogramSet->GetHistogramWithName(hist_name));
        }
      }
    else
      {
      this->ScalarColorFunctionEditor->SetHistogram(NULL);
      }

    this->ScalarColorFunctionEditor->Update();

    // Disable the RGB tfunc editor if the color of the volume is set to 
    // a gray level tfunc (not supported at the moment)

    int rgb_out = no_rgb || 
      (has_prop && 
       this->VolumeProperty->GetColorChannels(this->SelectedComponent) != 3);
    if (rgb_out)
      {
      this->ScalarColorFunctionEditor->SetEnabled(0);
      }
    if (this->ScalarColorFunctionEditor->IsCreated())
      {
      tk_cmd << "grid " << (rgb_out ? "remove" : "") << " " 
             << this->ScalarColorFunctionEditor->GetWidgetName() << endl;
      }
    }

  // Synchronize both

  if (this->ScalarOpacityFunctionEditor && this->ScalarColorFunctionEditor)
    {
    int have_funcs = (this->ScalarOpacityFunctionEditor->HasFunction() &&
                      this->ScalarColorFunctionEditor->HasFunction());

    // Synchronize the parameter range if RGB and opacity are the same
    // scalar field

    if (this->GetIndependentComponents() && have_funcs)
      {
      this->ScalarColorFunctionEditor->SynchronizeVisibleParameterRange(
        this->ScalarOpacityFunctionEditor);
      }
    else
      {
      this->ScalarColorFunctionEditor->DoNotSynchronizeVisibleParameterRange(
        this->ScalarOpacityFunctionEditor);
      }

    // (un)Synchronize both opacity and color functions points
    
    if (this->GetIndependentComponents() &&
        this->LockOpacityAndColor[this->SelectedComponent] && have_funcs)
      {
      this->ScalarColorFunctionEditor->SynchronizePoints(
        this->ScalarOpacityFunctionEditor);

      this->ScalarColorFunctionEditor->DoNotSynchronizeSingleSelection(
        this->ScalarOpacityFunctionEditor);

      this->ScalarColorFunctionEditor->SynchronizeSameSelection(
        this->ScalarOpacityFunctionEditor);
      }
    else
      {
      this->ScalarColorFunctionEditor->DoNotSynchronizePoints(
        this->ScalarOpacityFunctionEditor);

      this->ScalarColorFunctionEditor->DoNotSynchronizeSameSelection(
        this->ScalarOpacityFunctionEditor);

      if (have_funcs)
        {
        this->ScalarColorFunctionEditor->SynchronizeSingleSelection(
          this->ScalarOpacityFunctionEditor);
        }
      else
        {
        this->ScalarColorFunctionEditor->DoNotSynchronizeSingleSelection(
          this->ScalarOpacityFunctionEditor);
        }
      }
    }

  // Enable Gradient opacity

  if (this->EnableGradientOpacityOptionMenu && has_prop)
    {
    this->EnableGradientOpacityOptionMenu->SetValue(
      this->VolumeProperty->GetDisableGradientOpacity(
        this->SelectedComponent) ? "Off" : "On");
    }

  // Gradient opacity

  if (this->GradientOpacityFunctionEditor)
    {
    int scalar_field = this->GetIndependentComponents() 
      ? this->SelectedComponent : (nb_components - 1);
    
    if (has_prop)
      {
      vtkPiecewiseFunction *gfun = 
       this->VolumeProperty->GetStoredGradientOpacity(this->SelectedComponent);
      this->GradientOpacityFunctionEditor->SetPiecewiseFunction(gfun);

      if (this->GetDataSetScalarRange(scalar_field, p_range))
        {
        // WARNING: hard-coded value here according to the raycast mapper
        // behaviour (1/4 of the range)

        p_range[1] = (float)(0.25 * ((double)p_range[1] - (double)p_range[0]));
        p_range[0] = 0.0;
        this->GradientOpacityFunctionEditor->SetWholeParameterRange(p_range);
        }
      else
        {
        this->GradientOpacityFunctionEditor->SetWholeParameterRange(
          gfun->GetRange());
        }
      this->GradientOpacityFunctionEditor->SetWholeValueRange(0.0, 1.0);
      }
    else
      {
      this->GradientOpacityFunctionEditor->SetPiecewiseFunction(0);
      }

    if (this->HistogramSet)
      {
      if (this->HistogramSet->ComputeHistogramName(
            this->GetDataSetScalarName(), 
            this->GetIndependentComponents() ? this->SelectedComponent : 0, 
            "gradient", 
            hist_name))
        {
        this->GradientOpacityFunctionEditor->SetHistogram(
          this->HistogramSet->GetHistogramWithName(hist_name));
        }
      }
    else
      {
      this->GradientOpacityFunctionEditor->SetHistogram(NULL);
      }

    this->GradientOpacityFunctionEditor->Update();
    }

  // Component weights (CW)

  if (this->ComponentWeightScaleSet)
    {
    vtkKWScaleWithEntrySet *scaleset = 
      this->ComponentWeightScaleSet->GetWidget();
    if (has_prop)
      {
      for (i = 0; i < VTK_MAX_VRCOMP; i++)
        {
        if (scaleset->GetWidget(i))
          {
          int old_disable = scaleset->GetWidget(i)->GetDisableCommands();
          scaleset->GetWidget(i)->SetDisableCommands(1);
          scaleset->GetWidget(i)->SetValue(
            this->VolumeProperty->GetComponentWeight(i));
          scaleset->GetWidget(i)->SetDisableCommands(old_disable);
          }
        }
      }
    if (this->ComponentWeightScaleSet->IsCreated())
      {
      if (scaleset->GetNumberOfVisibleWidgets() != nb_components)
        {
        for (i = 0; i < VTK_MAX_VRCOMP; i++)
          {
          scaleset->SetWidgetVisibility(i, (i < nb_components ? 1 : 0));
          }
        }
      int scales_out = (!this->GetIndependentComponents() || nb_components < 2);
      if (scales_out)
        {
        this->ComponentWeightScaleSet->SetEnabled(0);
        }
      if (this->ComponentWeightScaleSet->IsCreated())
        {
        tk_cmd << "grid " << (scales_out ? "remove" : "") << " "
               << this->ComponentWeightScaleSet->GetWidgetName() << endl;
        }
      }
    }

  // HSV Color Selector

  if (this->HSVColorSelector)
    {
    if (no_rgb)
      {
      this->HSVColorSelector->SetEnabled(0);
      }
    else
      {
      this->UpdateHSVColorSelectorFromScalarColorFunctionEditor();
      }
    this->HSVColorSelector->Update();
    }

  // Execute (if any)

  tk_cmd << ends;
  if (*tk_cmd.str())
    {
    this->Script(tk_cmd.str());
    }
  tk_cmd.rdbuf()->freeze(0);
}

//----------------------------------------------------------------------------
void vtkKWVolumePropertyWidget::UpdateEnableState()
{
  this->Superclass::UpdateEnableState();

  this->PropagateEnableState(this->EditorFrame);
  this->PropagateEnableState(this->InnerLeftFrame);
  this->PropagateEnableState(this->ComponentSelectionWidget);
  this->PropagateEnableState(this->InterpolationTypeOptionMenu);
  this->PropagateEnableState(this->InteractiveApplyCheckButton);
  this->PropagateEnableState(this->ScalarOpacityFunctionEditor);
  this->PropagateEnableState(this->ScalarOpacityUnitDistanceScale);
  this->PropagateEnableState(this->EnableShadingCheckButton);
  this->PropagateEnableState(this->MaterialPropertyWidget);
  this->PropagateEnableState(this->LockOpacityAndColorCheckButton);
  this->PropagateEnableState(this->ScalarColorFunctionEditor);
  this->PropagateEnableState(this->EnableGradientOpacityOptionMenu);
  this->PropagateEnableState(this->GradientOpacityFunctionEditor);
  this->PropagateEnableState(this->ComponentWeightScaleSet);
  this->PropagateEnableState(this->HSVColorSelector);
  this->PropagateEnableState(this->BottomFrame);
}

//----------------------------------------------------------------------------
void vtkKWVolumePropertyWidget::SetVolumeProperty(
  vtkVolumeProperty *arg)
{
  if (this->VolumeProperty == arg)
    {
    return;
    }

  if (this->VolumeProperty)
    {
    this->VolumeProperty->UnRegister(this);
    }
    
  this->VolumeProperty = arg;

  if (this->VolumeProperty)
    {
    this->VolumeProperty->Register(this);
    }

  this->Modified();

  this->Update();
}

//----------------------------------------------------------------------------
int vtkKWVolumePropertyWidget::GetIndependentComponents()
{
  return (this->VolumeProperty && 
          this->VolumeProperty->GetIndependentComponents());
}


//----------------------------------------------------------------------------
void vtkKWVolumePropertyWidget::SetDataSet(
  vtkDataSet *arg)
{
  if (this->DataSet == arg)
    {
    return;
    }

  if (this->DataSet)
    {
    this->DataSet->UnRegister(this);
    }
    
  this->DataSet = arg;

  if (this->DataSet)
    {
    this->DataSet->Register(this);
    }

  this->Modified();

  this->Update();
}

// ---------------------------------------------------------------------------
int vtkKWVolumePropertyWidget::GetNumberOfComponents()
{
  if (this->DataSet)
    {
    vtkDataArray *scalars = this->DataSet->GetPointData()->GetScalars();
    if (scalars)
      {
      return scalars->GetNumberOfComponents();
      }
    }
  return VTK_MAX_VRCOMP;
}

// ---------------------------------------------------------------------------
int vtkKWVolumePropertyWidget::GetDataSetScalarRange(
  int comp, double range[2])
{
  if (this->DataSet)
    {
    vtkDataArray *scalars = this->DataSet->GetPointData()->GetScalars();
    if (scalars)
      {
      scalars->GetRange(range, comp);
      return 1;
      }
    }
  return 0;
}

// ---------------------------------------------------------------------------
int vtkKWVolumePropertyWidget::GetDataSetAdjustedScalarRange(
  int comp, double range[2])
{
  if (this->DataSet)
    {
    vtkDataArray *scalars = this->DataSet->GetPointData()->GetScalars();
    if (scalars)
      {
      return vtkMath::GetAdjustedScalarRange(scalars, comp, range);
      }
    }
  return 0;
}

// ---------------------------------------------------------------------------
const char* vtkKWVolumePropertyWidget::GetDataSetScalarName()
{
  if (this->DataSet)
    {
    vtkDataArray *scalars = this->DataSet->GetPointData()->GetScalars();
    if (scalars)
      {
      return scalars->GetName();
      }
    }
  return NULL;
}

// ---------------------------------------------------------------------------
int vtkKWVolumePropertyWidget::GetDataSetScalarOpacityUnitDistanceRangeAndResolution(
  double range[2], double *resolution)
{
  vtkImageData *img = vtkImageData::SafeDownCast(this->DataSet);
  if (img)
    {
    double *spacing = img->GetSpacing();
    double avg_spacing = (spacing[0] + spacing[1] + spacing[2]) / 3.0;
    double small_spacing = avg_spacing / 10.0;
    *resolution = small_spacing;
    range[0] = small_spacing;
    range[1] = avg_spacing * 10;
    return 1;
    }

  return 0;
}

// ---------------------------------------------------------------------------
void vtkKWVolumePropertyWidget::SetHistogramSet(vtkKWHistogramSet *arg)
{
  if (this->HistogramSet == arg)
    {
    return;
    }

  if (this->HistogramSet)
    {
    this->HistogramSet->UnRegister(this);
    }
    
  this->HistogramSet = arg;
  
  if (this->HistogramSet)
    {
    this->HistogramSet->Register(this);
    }
  
  this->Modified();
  
  this->Update();
}

//----------------------------------------------------------------------------
void vtkKWVolumePropertyWidget::SetEnableShadingForAllComponents(int arg)
{
  if (this->EnableShadingForAllComponents == arg)
    {
    return;
    }

  this->EnableShadingForAllComponents = arg;

  this->Modified();

  this->Update();
}

//----------------------------------------------------------------------------
void vtkKWVolumePropertyWidget::SetComponentSelectionVisibility(int arg)
{
  if (this->ComponentSelectionVisibility == arg)
    {
    return;
    }

  this->ComponentSelectionVisibility = arg;

  this->Modified();

  this->Pack();
}

//----------------------------------------------------------------------------
void vtkKWVolumePropertyWidget::SetHSVColorSelectorVisibility(int arg)
{
  if (this->HSVColorSelectorVisibility == arg)
    {
    return;
    }

  this->HSVColorSelectorVisibility = arg;

  this->Modified();

  this->Pack();
}

//----------------------------------------------------------------------------
void vtkKWVolumePropertyWidget::SetInteractiveApplyButtonVisibility(int arg)
{
  if (this->InteractiveApplyButtonVisibility == arg)
    {
    return;
    }

  this->InteractiveApplyButtonVisibility = arg;

  this->Modified();

  this->Pack();
}

//----------------------------------------------------------------------------
void vtkKWVolumePropertyWidget::SetScalarOpacityUnitDistanceVisibility(int arg)
{
  if (this->ScalarOpacityUnitDistanceVisibility == arg)
    {
    return;
    }

  this->ScalarOpacityUnitDistanceVisibility = arg;

  this->Modified();

  this->Pack();
}

//----------------------------------------------------------------------------
void vtkKWVolumePropertyWidget::SetInterpolationTypeVisibility(int arg)
{
  if (this->InterpolationTypeVisibility == arg)
    {
    return;
    }

  this->InterpolationTypeVisibility = arg;

  this->Modified();

  this->Pack();
}

//----------------------------------------------------------------------------
void vtkKWVolumePropertyWidget::SetMaterialPropertyVisibility(int arg)
{
  if (this->MaterialPropertyVisibility == arg)
    {
    return;
    }

  this->MaterialPropertyVisibility = arg;

  this->Modified();

  this->Pack();
}

//----------------------------------------------------------------------------
void vtkKWVolumePropertyWidget::SetMaterialPropertyPosition(int arg)
{
  if (arg < vtkKWVolumePropertyWidget::MaterialPropertyPositionTop)
    {
    arg = vtkKWVolumePropertyWidget::MaterialPropertyPositionTop;
    }
  else if (arg > 
     vtkKWVolumePropertyWidget::MaterialPropertyPositionScalarColorUserFrame)
    {
    arg = vtkKWVolumePropertyWidget::MaterialPropertyPositionScalarColorUserFrame;
    }

  if (this->MaterialPropertyPosition == arg)
    {
    return;
    }

  this->MaterialPropertyPosition = arg;

  this->Modified();

  this->Pack();
}

//----------------------------------------------------------------------------
void vtkKWVolumePropertyWidget::SetGradientOpacityFunctionVisibility(int arg)
{
  if (this->GradientOpacityFunctionVisibility == arg)
    {
    return;
    }

  this->GradientOpacityFunctionVisibility = arg;

  this->Modified();

  this->Pack();
}

//----------------------------------------------------------------------------
void vtkKWVolumePropertyWidget::SetComponentWeightsVisibility(int arg)
{
  if (this->ComponentWeightsVisibility == arg)
    {
    return;
    }

  this->ComponentWeightsVisibility = arg;

  this->Modified();

  this->Pack();
}

//----------------------------------------------------------------------------
void vtkKWVolumePropertyWidget::SetSelectedComponent(int arg)
{
  if (this->SelectedComponent == arg ||
      arg < 0 || arg >= this->GetNumberOfComponents())
    {
    return;
    }

  this->SelectedComponent = arg;
  this->Modified();

  this->Update();
}

//----------------------------------------------------------------------------
void vtkKWVolumePropertyWidget::SetWindowLevel(float window, float level)
{
  if (this->ScalarOpacityFunctionEditor)
    {
    this->ScalarOpacityFunctionEditor->SetWindowLevel(window, level);
    }
}

//----------------------------------------------------------------------------
void vtkKWVolumePropertyWidget::SetInteractiveWindowLevel(
  float window, float level)
{
  if (this->ScalarOpacityFunctionEditor)
    {
    this->ScalarOpacityFunctionEditor->SetInteractiveWindowLevel(
      window, level);
    }
}

//----------------------------------------------------------------------------
int vtkKWVolumePropertyWidget::IsInWindowLevelMode()
{
  int res = 0;

  if (this->ScalarOpacityFunctionEditor && 
      this->ScalarOpacityFunctionEditor->GetWindowLevelMode())
    {
    res = 1;
    }

  return res;
}

//----------------------------------------------------------------------------
void vtkKWVolumePropertyWidget::SetUseScalarColorFunctionInScalarOpacityEditor(int arg)
{
  if (this->UseScalarColorFunctionInScalarOpacityEditor == arg)
    {
    return;
    }

  this->UseScalarColorFunctionInScalarOpacityEditor = arg;

  this->Modified();

  this->Update();
}

//----------------------------------------------------------------------------
void vtkKWVolumePropertyWidget::MergeScalarOpacityAndColorEditors()
{
  this->UseScalarColorFunctionInScalarOpacityEditor = 1;

  if (this->ScalarOpacityFunctionEditor)
    {
    this->ScalarOpacityFunctionEditor->SetLabelText(
      "Scalar Opacity And Color Mapping:");
    this->ScalarOpacityFunctionEditor->WindowLevelModeButtonVisibilityOff();
    this->ScalarOpacityFunctionEditor->ParameterRangeVisibilityOff();
    this->ScalarOpacityFunctionEditor->ComputePointColorFromValueOn();
    }

  if (this->ScalarColorFunctionEditor)
    {
    this->ScalarColorFunctionEditor->LabelVisibilityOff();
    this->ScalarColorFunctionEditor->ParameterRangeLabelVisibilityOff();
    this->ScalarColorFunctionEditor->PointEntriesVisibilityOff();
    this->ScalarColorFunctionEditor->CanvasVisibilityOff();
    this->ScalarColorFunctionEditor->HistogramLogModeOptionMenuVisibilityOff();
    this->ScalarColorFunctionEditor->ColorSpaceOptionMenuVisibilityOff();
    this->ScalarColorFunctionEditor->UserFrameVisibilityOff();
    }

  for (int i = 0; i < VTK_MAX_VRCOMP; i++)
    {
    this->LockOpacityAndColor[i] = 1;
    }

  this->Pack();
  this->Update();
}

//----------------------------------------------------------------------------
void vtkKWVolumePropertyWidget::InvokeObjectMethodCommand(const char *command)
{
  if (!this->DisableCommands)
    {
    this->Superclass::InvokeObjectMethodCommand(command);
    }
}

//----------------------------------------------------------------------------
void vtkKWVolumePropertyWidget::SetVolumePropertyChangedCommand(
  vtkObject *object, const char *method)
{
  this->SetObjectMethodCommand(
    &this->VolumePropertyChangedCommand, object, method);
}

//----------------------------------------------------------------------------
void vtkKWVolumePropertyWidget::InvokeVolumePropertyChangedCommand()
{
  this->InvokeObjectMethodCommand(this->VolumePropertyChangedCommand);

  this->InvokeEvent(vtkKWEvent::VolumePropertyChangedEvent, NULL);
}

//----------------------------------------------------------------------------
void vtkKWVolumePropertyWidget::SetVolumePropertyChangingCommand(
  vtkObject *object, const char *method)
{
  this->SetObjectMethodCommand(
    &this->VolumePropertyChangingCommand, object, method);
}

//----------------------------------------------------------------------------
void vtkKWVolumePropertyWidget::InvokeVolumePropertyChangingCommand()
{
  this->InvokeObjectMethodCommand(this->VolumePropertyChangingCommand);

  this->InvokeEvent(vtkKWEvent::VolumePropertyChangingEvent, NULL);
}

//----------------------------------------------------------------------------
void vtkKWVolumePropertyWidget::SelectedComponentCallback(int n)
{
  this->SelectedComponent = n;
  this->Update();
}

//----------------------------------------------------------------------------
void vtkKWVolumePropertyWidget::InterpolationTypeCallback(int type)
{
  if (this->VolumeProperty && 
      this->VolumeProperty->GetInterpolationType()!= type)
    {
    this->VolumeProperty->SetInterpolationType(type);
    this->InvokeVolumePropertyChangedCommand();
    }
}

//----------------------------------------------------------------------------
void vtkKWVolumePropertyWidget::EnableShadingCallback(int state)
{
  if (!this->VolumeProperty || 
      !this->EnableShadingForAllComponents)
    {
    return;
    }

  unsigned long mtime = this->VolumeProperty->GetMTime();

  // Set the first component

  this->VolumeProperty->SetShade(0, state ? 1 : 0);

  // Update the others

  int nb_shade_comp = this->GetIndependentComponents() 
    ? this->GetNumberOfComponents() : 1;
  for (int i = 1; i < nb_shade_comp; i++)
    {
    this->VolumeProperty->SetShade(i, this->VolumeProperty->GetShade(0));
    }

  // Was something modified ?

  if (this->VolumeProperty->GetMTime() > mtime)
    {
    this->InvokeVolumePropertyChangedCommand();
    }

  this->Update();
}

//----------------------------------------------------------------------------
void vtkKWVolumePropertyWidget::LockOpacityAndColorCallback(int state)
{
  this->LockOpacityAndColor[this->SelectedComponent] = state;
  this->Update();
}

//----------------------------------------------------------------------------
void vtkKWVolumePropertyWidget::EnableGradientOpacityCallback(int val)
{
  if (this->EnableGradientOpacityOptionMenu && this->VolumeProperty)
    {
    this->VolumeProperty->SetDisableGradientOpacity(
      this->SelectedComponent, val ? 0 : 1);
    this->InvokeVolumePropertyChangedCommand();
    }
}

//----------------------------------------------------------------------------
void vtkKWVolumePropertyWidget::MaterialPropertyChangedCallback()
{
  this->InvokeVolumePropertyChangedCommand();
}

//----------------------------------------------------------------------------
void vtkKWVolumePropertyWidget::MaterialPropertyChangingCallback()
{
  if (this->InteractiveApplyMode)
    {
    this->InvokeVolumePropertyChangingCommand();
    }
}

//----------------------------------------------------------------------------
void vtkKWVolumePropertyWidget::InteractiveApplyCallback(int state)
{
  this->SetInteractiveApplyMode(state ? 1 : 0);
}

//----------------------------------------------------------------------------
void vtkKWVolumePropertyWidget::ScalarOpacityFunctionChangedCallback()
{
  if (this->ScalarOpacityFunctionEditor &&
      this->ScalarOpacityFunctionEditor->GetWindowLevelMode())
    {
    float fargs[2];
    fargs[0] = this->ScalarOpacityFunctionEditor->GetWindow();
    fargs[1] = this->ScalarOpacityFunctionEditor->GetLevel();
    this->InvokeEvent(vtkKWEvent::WindowLevelChangedEvent, fargs);
    }

  this->InvokeVolumePropertyChangedCommand();
}

//----------------------------------------------------------------------------
void vtkKWVolumePropertyWidget::ScalarOpacityFunctionChangingCallback()
{
  if (this->ScalarOpacityFunctionEditor &&
      this->ScalarOpacityFunctionEditor->GetWindowLevelMode())
    {
    float fargs[2];
    fargs[0] = this->ScalarOpacityFunctionEditor->GetWindow();
    fargs[1] = this->ScalarOpacityFunctionEditor->GetLevel();
    this->InvokeEvent(vtkKWEvent::WindowLevelChangingEvent, fargs);
    }

  if (this->InteractiveApplyMode)
    {
    this->InvokeVolumePropertyChangingCommand();
    }
}

//----------------------------------------------------------------------------
void vtkKWVolumePropertyWidget::DoubleClickOnScalarOpacityPointCallback(int id)
{
  // If we are using the color tfunc in the scalar opacity function editor,
  // try to modify the color

  if (!this->UseScalarColorFunctionInScalarOpacityEditor ||
      !this->ScalarOpacityFunctionEditor ||
      !this->ScalarColorFunctionEditor ||
      (this->ScalarOpacityFunctionEditor->GetPointColorTransferFunction() !=
       this->ScalarColorFunctionEditor->GetColorTransferFunction()))
    {
    return;
    }

  // Query the parameter of the point in the scalar opacity editor, and
  // find the corresponding point in the color transfer function editor

  double soe_parameter;
  int sce_id;
  if (!this->ScalarOpacityFunctionEditor->GetFunctionPointParameter(
        id, &soe_parameter) ||
      !this->ScalarColorFunctionEditor->GetFunctionPointId(
        soe_parameter, &sce_id))
    {
    return;
    }
      
  // Change the point color

  double rgb[3];
  if (!this->ScalarColorFunctionEditor->FunctionPointValueIsLocked(sce_id) &&
      this->ScalarColorFunctionEditor->GetPointColorAsRGB(sce_id, rgb) &&
      vtkKWTkUtilities::QueryUserForColor(
        this->GetApplication(),
        this->GetWidgetName(),
        NULL,
        rgb[0], rgb[1], rgb[2],
        &rgb[0], &rgb[1], &rgb[2]))
    {
    unsigned long mtime = this->ScalarColorFunctionEditor->GetFunctionMTime();

    this->ScalarColorFunctionEditor->SetPointColorAsRGB(sce_id, rgb);

    if (this->ScalarColorFunctionEditor->GetFunctionMTime() > mtime)
      {
      this->RGBTransferFunctionChangedCallback();
      }
    }
}

//----------------------------------------------------------------------------
void vtkKWVolumePropertyWidget::ScalarOpacityUnitDistanceChangedCallback(
  double value)
{
  if (!this->VolumeProperty)
    {
    return;
    }

  this->VolumeProperty->SetScalarOpacityUnitDistance(
    this->SelectedComponent, value);
  
  this->InvokeVolumePropertyChangedCommand();
}

//----------------------------------------------------------------------------
void vtkKWVolumePropertyWidget::ScalarOpacityUnitDistanceChangingCallback(
  double value)
{
  if (!this->VolumeProperty)
    {
    return;
    }

  if (this->InteractiveApplyMode)
    {
    this->VolumeProperty->SetScalarOpacityUnitDistance(
      this->SelectedComponent, value);
  
    this->InvokeVolumePropertyChangingCommand();
    }
}


//----------------------------------------------------------------------------
void vtkKWVolumePropertyWidget::WindowLevelModeCallback(int mode)
{
  this->WindowLevelMode[this->SelectedComponent] = mode;
  this->Update();
}

//----------------------------------------------------------------------------
void vtkKWVolumePropertyWidget::RGBTransferFunctionChangedCallback()
{
  this->UpdateHSVColorSelectorFromScalarColorFunctionEditor();

  if (this->UseScalarColorFunctionInScalarOpacityEditor &&
      this->ScalarOpacityFunctionEditor)
    {
    this->ScalarOpacityFunctionEditor->Update();
    }

  this->InvokeVolumePropertyChangedCommand();
}

//----------------------------------------------------------------------------
void vtkKWVolumePropertyWidget::RGBTransferFunctionChangingCallback()
{
  this->UpdateHSVColorSelectorFromScalarColorFunctionEditor();

  if (this->UseScalarColorFunctionInScalarOpacityEditor &&
      this->ScalarOpacityFunctionEditor)
    {
    this->ScalarOpacityFunctionEditor->Update();
    }

  if (this->InteractiveApplyMode)
    {
    this->InvokeVolumePropertyChangingCommand();
    }
}

//----------------------------------------------------------------------------
void vtkKWVolumePropertyWidget::RGBTransferFunctionSelectionChangedCallback()
{
  this->UpdateHSVColorSelectorFromScalarColorFunctionEditor();
}

//----------------------------------------------------------------------------
void vtkKWVolumePropertyWidget::UpdateHSVColorSelectorFromScalarColorFunctionEditor()
{
  if (!this->ScalarColorFunctionEditor || !this->HSVColorSelector)
    {
    return;
    }

  /* 
     Here is the deal: 
     - select a point in the scalar color mapping function,
     - using the HSV wheel/color selector, select a color, like yellow, and
       a value around 50% ,
     - now drop the value to 0% (black),
     - the HS cursor will jump automatically to the center, where H = S = 0. 
     Why ? Even if the color is selected in the HSV color selector, at the 
     end of the day the transfer function stores that color in RGB internally
     using vtkMath::HSVToRGB. In vtkMath::HSVToRGB, all R, G, B components 
     are multiplied by V. Thus, if V = 0 then RGB = (0, 0, 0), i.e. black, 
     whatever the values of HS. Since the HSV color selector is automatically
     updated to match the color of the selected point in the tfunc, it 
     converts RGB (0, 0, 0) back to HSV, and loses the HS information 
     (thus jumping back to the center).
         
     Among the possible hacks to volve this issue, we could have checked if
     the value was 0.0, and set it to something like 0.0001, which would still
     be very dark and would keep the HS close enough. I'm not confident with
     that hack, and I have the feeling that even a close-enough-black on 
     screen could be not-that-close when printing (for example). 

     Instead, I did the following, under the assumption that if the user sets
     the V to 0, he really wants a "black", whatever the HS: I look at the HSV
     of the selected tfunc point, and the HSV selected in the HSV color 
     selector. If both have the same V = 0, and lead to the same RGB, I do 
     not update the HSV color selector with the current selected point color. 
     This allows the users to play with V while keeping the HS in the color 
     selector, but the correct value is stored in the tfunc. 
  */

  if (this->ScalarColorFunctionEditor->HasSelection())
    {
    double tfunc_hsv[3];
    if (this->ScalarColorFunctionEditor->GetPointColorAsHSV(
          this->ScalarColorFunctionEditor->GetSelectedPoint(), tfunc_hsv))
      {
      int ok = 1;
      if (this->HSVColorSelector->HasSelection())
        {
        double *sel_hsv = this->HSVColorSelector->GetSelectedColor();
        if (sel_hsv[2] == 0.0 && tfunc_hsv[2] == 0.0)
          {
          double tfunc_rgb[3], sel_rgb[3];
          vtkMath::HSVToRGB(tfunc_hsv, tfunc_rgb);
          vtkMath::HSVToRGB(sel_hsv, sel_rgb);
          if (tfunc_rgb[0] == sel_rgb[0] &&
              tfunc_rgb[1] == sel_rgb[1] &&
              tfunc_rgb[2] == sel_rgb[2])
            {
            ok = 0;
            }
          }
        }
      if (ok)
        {
        this->HSVColorSelector->SetSelectedColor(tfunc_hsv);
        }
      }
    }
  else
    {
    this->HSVColorSelector->ClearSelection();
    }
}

//----------------------------------------------------------------------------
void vtkKWVolumePropertyWidget::GradientOpacityFunctionChangedCallback()
{
  this->InvokeVolumePropertyChangedCommand();
}

//----------------------------------------------------------------------------
void vtkKWVolumePropertyWidget::GradientOpacityFunctionChangingCallback()
{
  if (this->InteractiveApplyMode)
    {
    this->InvokeVolumePropertyChangingCommand();
    }
}

//----------------------------------------------------------------------------
void vtkKWVolumePropertyWidget::HSVColorSelectionChangedCallback(
  double h, double s, double v)
{
  if (!this->ScalarColorFunctionEditor || 
      !this->ScalarColorFunctionEditor->HasFunction() || 
      !this->ScalarColorFunctionEditor->HasSelection())
    {
    return;
    }
  
  this->ScalarColorFunctionEditor->SetPointColorAsHSV(
    this->ScalarColorFunctionEditor->GetSelectedPoint(), h, s, v);

  if (this->UseScalarColorFunctionInScalarOpacityEditor &&
      this->ScalarOpacityFunctionEditor)
    {
    this->ScalarOpacityFunctionEditor->Update();
    }

  this->InvokeVolumePropertyChangedCommand();
}

//----------------------------------------------------------------------------
void vtkKWVolumePropertyWidget::HSVColorSelectionChangingCallback(
  double h, double s, double v)
{
  if (!this->ScalarColorFunctionEditor || 
      !this->ScalarColorFunctionEditor->HasFunction() || 
      !this->ScalarColorFunctionEditor->HasSelection())
    {
    return;
    }

  unsigned long mtime = 
    this->ScalarColorFunctionEditor->GetColorTransferFunction()->GetMTime();

  this->ScalarColorFunctionEditor->SetPointColorAsHSV(
    this->ScalarColorFunctionEditor->GetSelectedPoint(), h, s, v);

  if (this->ScalarColorFunctionEditor->GetColorTransferFunction()->GetMTime() >
      mtime)
    {
    if (this->UseScalarColorFunctionInScalarOpacityEditor &&
        this->ScalarOpacityFunctionEditor)
      {
      this->ScalarOpacityFunctionEditor->Update();
      }
  if (this->InteractiveApplyMode)
      {
      this->InvokeVolumePropertyChangingCommand();
      }
    }
}

//----------------------------------------------------------------------------
void vtkKWVolumePropertyWidget::ComponentWeightChangedCallback(
  int index, double value)
{
  if (!this->VolumeProperty)
    {
    return;
    }

  this->VolumeProperty->SetComponentWeight(index, value);
  
  float fargs[2];
  fargs[0] = index;
  fargs[1] = value;
  this->InvokeEvent(vtkKWEvent::ScalarComponentWeightChangedEvent, fargs);

  this->InvokeVolumePropertyChangedCommand();
}

//----------------------------------------------------------------------------
void vtkKWVolumePropertyWidget::ComponentWeightChangingCallback(
  int index, double value)
{
  this->VolumeProperty->SetComponentWeight(index, value);
  
  float fargs[2];
  fargs[0] = index;
  fargs[1] = value;
  this->InvokeEvent(vtkKWEvent::ScalarComponentWeightChangingEvent, fargs);

  if (this->InteractiveApplyMode)
    {
    this->InvokeVolumePropertyChangingCommand();
    }
}

//----------------------------------------------------------------------------
void vtkKWVolumePropertyWidget::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  
  os << indent << "SelectedComponent: " 
     << this->SelectedComponent << endl;
  os << indent << "DisableCommands: "
     << (this->DisableCommands ? "On" : "Off") << endl;
  os << indent << "EnableShadingForAllComponents: "
     << (this->EnableShadingForAllComponents ? "On" : "Off") << endl;
  os << indent << "ComponentSelectionVisibility: "
     << (this->ComponentSelectionVisibility ? "On" : "Off") << endl;
  os << indent << "InteractiveApplyMode: "
     << (this->InteractiveApplyMode ? "On" : "Off") << endl;
  os << indent << "InteractiveApplyButtonVisibility: "
     << (this->InteractiveApplyButtonVisibility ? "On" : "Off") << endl;
  os << indent << "ScalarOpacityUnitDistanceVisibility: "
     << (this->ScalarOpacityUnitDistanceVisibility ? "On" : "Off") << endl;
  os << indent << "HSVColorSelectorVisibility: "
     << (this->HSVColorSelectorVisibility ? "On" : "Off") << endl;
  os << indent << "InterpolationTypeVisibility: "
     << (this->InterpolationTypeVisibility ? "On" : "Off") << endl;
  os << indent << "MaterialPropertyPosition: " << this->MaterialPropertyPosition << endl;
  os << indent << "MaterialPropertyVisibility: "
     << (this->MaterialPropertyVisibility ? "On" : "Off") << endl;
  os << indent << "GradientOpacityFunctionVisibility: "
     << (this->GradientOpacityFunctionVisibility ? "On" : "Off") << endl;
  os << indent << "ComponentWeightsVisibility: "
     << (this->ComponentWeightsVisibility ? "On" : "Off") << endl;
  os << indent << "ScalarOpacityFunctionEditor: ";
  os << indent << "UseScalarColorFunctionInScalarOpacityEditor: "
     << (this->UseScalarColorFunctionInScalarOpacityEditor ? "On" : "Off") << endl;

  if (this->ScalarOpacityFunctionEditor)
    {
    os << endl;
    this->ScalarOpacityFunctionEditor->PrintSelf(os, indent.GetNextIndent());
    }
  else
    {
    os << "None" << endl;
    }
  os << indent << "ScalarColorFunctionEditor: ";
  if (this->ScalarColorFunctionEditor)
    {
    os << endl;
    this->ScalarColorFunctionEditor->PrintSelf(os, indent.GetNextIndent());
    }
  else
    {
    os << "None" << endl;
    }
  os << indent << "GradientOpacityFunctionEditor: ";
  if (this->GradientOpacityFunctionEditor)
    {
    os << endl;
    this->GradientOpacityFunctionEditor->PrintSelf(os, indent.GetNextIndent());
    }
  else
    {
    os << "None" << endl;
    }
  os << indent << "ScalarOpacityUnitDistanceScale: ";
  if (this->ScalarOpacityUnitDistanceScale)
    {
    os << endl;
    this->ScalarOpacityUnitDistanceScale->PrintSelf(os, indent.GetNextIndent());
    }
  else
    {
    os << "None" << endl;
    }
  os << indent << "VolumeProperty: ";
  if (this->VolumeProperty)
    {
    os << endl;
    this->VolumeProperty->PrintSelf(os, indent.GetNextIndent());
    }
  else
    {
    os << "None" << endl;
    }
  os << indent << "DataSet: ";
  if (this->DataSet)
    {
    os << endl;
    this->DataSet->PrintSelf(os, indent.GetNextIndent());
    }
  else
    {
    os << "None" << endl;
    }
  os << indent << "HistogramSet: ";
  if (this->HistogramSet)
    {
    os << endl;
    this->HistogramSet->PrintSelf(os, indent.GetNextIndent());
    }
  else
    {
    os << "None" << endl;
    }
  os << indent << "HSVColorSelector: ";
  if (this->HSVColorSelector)
    {
    os << endl;
    this->HSVColorSelector->PrintSelf(os, indent.GetNextIndent());
    }
  else
    {
    os << "None" << endl;
    }
}
