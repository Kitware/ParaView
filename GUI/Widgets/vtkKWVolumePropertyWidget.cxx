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
#include "vtkDataSet.h"
#include "vtkImageData.h"
#include "vtkKWCheckButton.h"
#include "vtkKWColorTransferFunctionEditor.h"
#include "vtkKWEvent.h"
#include "vtkKWFrame.h"
#include "vtkKWHSVColorSelector.h"
#include "vtkKWHistogramSet.h"
#include "vtkKWIcon.h"
#include "vtkKWLabel.h"
#include "vtkKWLabeledEntry.h"
#include "vtkKWLabeledFrame.h"
#include "vtkKWLabeledOptionMenu.h"
#include "vtkKWLabeledPopupButton.h"
#include "vtkKWLabeledScaleSet.h"
#include "vtkKWMath.h"
#include "vtkKWOptionMenu.h"
#include "vtkKWPiecewiseFunctionEditor.h"
#include "vtkKWScalarComponentSelectionWidget.h"
#include "vtkKWScale.h"
#include "vtkKWScaleSet.h"
#include "vtkKWVolumeMaterialPropertyWidget.h"
#include "vtkMath.h"
#include "vtkObjectFactory.h"
#include "vtkPointData.h"
#include "vtkVolumeProperty.h"
#ifndef DO_NOT_BUILD_XML_RW
#include "vtkXMLVolumePropertyWriter.h"
#endif

#define VTK_KW_VPW_INTERPOLATION_LINEAR     "Linear"
#define VTK_KW_VPW_INTERPOLATION_NEAREST    "Nearest"

#define VTK_KW_VPW_TESTING 0

//----------------------------------------------------------------------------
vtkCxxRevisionMacro(vtkKWVolumePropertyWidget, "1.2");
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

  this->ShowComponentSelection = 1;
  this->ShowInterpolationType = 1;
  this->ShowMaterialProperty = 1;
  this->ShowGradientOpacityFunction = 1;
  this->ShowComponentWeights = 1;

  this->VolumePropertyChangedCommand  = NULL;
  this->VolumePropertyChangingCommand = NULL;

  // GUI

  this->EditorFrame                     = vtkKWLabeledFrame::New();

  this->InterpolationTypeOptionMenu     = vtkKWLabeledOptionMenu::New();

  this->EnableShadingCheckButton        = vtkKWCheckButton::New();

  this->InteractiveApplyCheckButton     = vtkKWCheckButton::New();

  this->LockOpacityAndColorCheckButton  = vtkKWCheckButton::New();

  this->ScalarOpacityUnitDistanceScale  = vtkKWScale::New();

  this->EnableGradientOpacityOptionMenu = vtkKWOptionMenu::New();

  this->ComponentWeightScaleSet         = vtkKWLabeledScaleSet::New();

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
  this->SetHistogramSet(NULL);
  this->SetVolumeProperty(NULL);
  this->SetDataSet(NULL);

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
}

//----------------------------------------------------------------------------
void vtkKWVolumePropertyWidget::Create(vtkKWApplication *app, const char *args)
{
  // Call the superclass to create the widget and set the appropriate flags

  if (!this->Superclass::Create(app, "frame", "-relief flat -bd 0"))
    {
    vtkErrorMacro("Failed creating widget " << this->GetClassName());
    return;
    }

  this->ConfigureOptions(args);

  ostrstream tk_cmd;

  int label_width = 12;
  int menu_width = 6;
  char command[256];

  // --------------------------------------------------------------
  // Frame

  this->EditorFrame->SetParent(this);
  this->EditorFrame->ShowHideFrameOn();
  this->EditorFrame->Create(app,0);
  this->EditorFrame->SetLabel("Volume Appearance Settings");

  vtkKWFrame *frame = this->EditorFrame->GetFrame();

  // --------------------------------------------------------------
  // Component selection

  this->ComponentSelectionWidget->SetParent(frame);
  this->ComponentSelectionWidget->Create(app, 0);
  this->ComponentSelectionWidget->SetSelectedComponentChangedCommand(
    this, "SelectedComponentCallback");

  vtkKWLabeledOptionMenu *omenu = 
    this->ComponentSelectionWidget->GetSelectedComponentOptionMenu();
  omenu->SetLabelWidth(label_width);
  omenu->GetOptionMenu()->SetWidth(menu_width);

  // --------------------------------------------------------------
  // Interpolation type

  if (!this->InterpolationTypeOptionMenu)
    {
    this->InterpolationTypeOptionMenu = vtkKWLabeledOptionMenu::New();
    }

  this->InterpolationTypeOptionMenu->SetParent(frame);
  this->InterpolationTypeOptionMenu->Create(app);
  this->InterpolationTypeOptionMenu->SetLabel("Interpolation:");
  this->InterpolationTypeOptionMenu->SetLabelWidth(label_width);
  this->InterpolationTypeOptionMenu->GetOptionMenu()->SetWidth(menu_width);
  this->InterpolationTypeOptionMenu->SetBalloonHelpString(
    "Set the interpolation type used for sampling the volume.");

  vtkKWOptionMenu *menu = this->InterpolationTypeOptionMenu->GetOptionMenu();

  char callback[128];

  sprintf(callback, "InterpolationTypeCallback %d", VTK_LINEAR_INTERPOLATION);
  menu->AddEntryWithCommand(VTK_KW_VPW_INTERPOLATION_LINEAR, this, callback);

  sprintf(callback, "InterpolationTypeCallback %d", VTK_NEAREST_INTERPOLATION);
  menu->AddEntryWithCommand(VTK_KW_VPW_INTERPOLATION_NEAREST, this, callback);

  // --------------------------------------------------------------
  // Enable shading

  this->EnableShadingCheckButton->SetParent(frame);
  this->EnableShadingCheckButton->Create(app, "");
  this->EnableShadingCheckButton->SetText("Enable Shading");
  this->EnableShadingCheckButton->SetBalloonHelpString(
    "Enable shading (for all components).");
  this->EnableShadingCheckButton->SetCommand(
    this, "EnableShadingCallback");

  // --------------------------------------------------------------
  // Material properties : widget

  this->MaterialPropertyWidget->SetParent(frame);
  this->MaterialPropertyWidget->PopupModeOn();
  this->MaterialPropertyWidget->Create(app, 0);
  this->MaterialPropertyWidget->GetPopupButton()->SetLabelWidth(label_width);
  this->MaterialPropertyWidget->GetComponentSelectionWidget()
    ->AllowComponentSelectionOff();
  this->MaterialPropertyWidget->SetPropertyChangedCommand(
    this, "MaterialPropertyChangedCallback");
  this->MaterialPropertyWidget->SetPropertyChangingCommand(
    this, "MaterialPropertyChangingCallback");

  // --------------------------------------------------------------
  // Interactive Apply

  this->InteractiveApplyCheckButton->SetParent(frame);
  this->InteractiveApplyCheckButton->Create(app, "");
  this->InteractiveApplyCheckButton->SetText("Interactive Apply");
  this->InteractiveApplyCheckButton->SetBalloonHelpString(
    "Toggle whether changes are applied to the volume window and image "
    "windows as nodes in the transfer functions are modified, or only after "
    "the mouse button is released.");

  // --------------------------------------------------------------
  // Scalar opacity editor

  this->ScalarOpacityFunctionEditor->SetParent(frame);
  this->ScalarOpacityFunctionEditor->SetLabel("Scalar Opacity Mapping:");
  this->ScalarOpacityFunctionEditor->ComputePointColorFromValueOff();
  this->ScalarOpacityFunctionEditor->LockEndPointsParameterOn();
  this->ScalarOpacityFunctionEditor->SetLabelPosition(
    vtkKWParameterValueFunctionEditor::LabelPositionAtTop);
  this->ScalarOpacityFunctionEditor->SetRangeLabelPosition(
    vtkKWParameterValueFunctionEditor::RangeLabelPositionAtTop);
  this->ScalarOpacityFunctionEditor->ShowValueRangeOff();
  this->ScalarOpacityFunctionEditor->ShowWindowLevelModeButtonOn();
  this->ScalarOpacityFunctionEditor->Create(app, "");

  this->ScalarOpacityFunctionEditor->GetParameterEntry()->SetLabel("S:");
  this->ScalarOpacityFunctionEditor->GetValueEntry()->SetLabel("O:");

  this->ScalarOpacityFunctionEditor->SetFunctionChangedCommand(
    this, "ScalarOpacityFunctionChangedCallback");
  this->ScalarOpacityFunctionEditor->SetFunctionChangingCommand(
    this, "ScalarOpacityFunctionChangingCallback");
  this->ScalarOpacityFunctionEditor->SetWindowLevelModeChangedCommand(
    this, "WindowLevelModeCallback");

  // --------------------------------------------------------------
  // Scalar Opacity Unit Distance

  this->ScalarOpacityFunctionEditor->ShowUserFrameOn();
  this->ScalarOpacityUnitDistanceScale->SetParent(
    this->ScalarOpacityFunctionEditor->GetUserFrame());
  this->ScalarOpacityUnitDistanceScale->PopupScaleOn();
  this->ScalarOpacityUnitDistanceScale->Create(app, "");
  this->ScalarOpacityUnitDistanceScale->DisplayEntry();
  this->ScalarOpacityUnitDistanceScale->DisplayLabel("Scale:");
  this->ScalarOpacityUnitDistanceScale->DisplayEntryAndLabelOnTopOff();
  this->ScalarOpacityUnitDistanceScale->SetEndCommand(
    this, "ScalarOpacityUnitDistanceChangedCallback");
  this->ScalarOpacityUnitDistanceScale->SetEntryCommand(
    this, "ScalarOpacityUnitDistanceChangedCallback");
  this->ScalarOpacityUnitDistanceScale->SetCommand(
    this, "ScalarOpacityUnitDistanceChangingCallback");
  this->ScalarOpacityUnitDistanceScale->SetBalloonHelpString(
    "Set the unit distance on which the scalar opacity transfer function "
    "is defined.");

  tk_cmd << "pack " << this->ScalarOpacityUnitDistanceScale->GetWidgetName() 
         << " -side right -fill both -padx 2 -pady 0" << endl;

  // --------------------------------------------------------------
  // Color transfer function editor

  this->ScalarColorFunctionEditor->SetParent(frame);
  this->ScalarColorFunctionEditor->SetLabel("Scalar Color Mapping:");
  this->ScalarColorFunctionEditor->SetCanvasHeight(
    this->ScalarOpacityFunctionEditor->GetCanvasHeight());
  this->ScalarColorFunctionEditor->LockEndPointsParameterOn();
  this->ScalarColorFunctionEditor->SetPointMarginToCanvas(
    this->ScalarOpacityFunctionEditor->GetPointMarginToCanvas());
  this->ScalarColorFunctionEditor->SetShowValueRange(
    this->ScalarOpacityFunctionEditor->GetShowValueRange());
  this->ScalarColorFunctionEditor->SetLabelPosition(
    this->ScalarOpacityFunctionEditor->GetLabelPosition());
  this->ScalarColorFunctionEditor->SetRangeLabelPosition(
    this->ScalarOpacityFunctionEditor->GetRangeLabelPosition());
  this->ScalarColorFunctionEditor->Create(app, "");

  this->ScalarColorFunctionEditor->GetParameterEntry()->SetLabel(
    this->ScalarOpacityFunctionEditor->GetParameterEntry()->GetLabel()
    ->GetLabel());

  this->ScalarColorFunctionEditor->SetFunctionChangedCommand(
    this, "RGBTransferFunctionChangedCallback");
  this->ScalarColorFunctionEditor->SetFunctionChangingCommand(
    this, "RGBTransferFunctionChangingCallback");
  this->ScalarColorFunctionEditor->SetSelectionChangedCommand(
    this, "RGBTransferFunctionSelectionChangedCallback");

  // --------------------------------------------------------------
  // Lock opacity and color

  this->ScalarColorFunctionEditor->ShowUserFrameOn();
  this->LockOpacityAndColorCheckButton->SetParent(
    this->ScalarColorFunctionEditor->GetUserFrame());
  this->LockOpacityAndColorCheckButton->Create(
    app, "-padx 0 -pady 0 -highlightthickness 0");
  this->LockOpacityAndColorCheckButton->SetIndicator(0);
  this->LockOpacityAndColorCheckButton->SetText("Lock");
  this->LockOpacityAndColorCheckButton->SetBalloonHelpString(
    "Lock the opacity and color functions together.");
  this->LockOpacityAndColorCheckButton->SetCommand(
    this, "LockOpacityAndColorCallback");

  this->LockOpacityAndColorCheckButton->SetImageOption(
    vtkKWIcon::ICON_LOCK);
 
  tk_cmd << "pack " << this->LockOpacityAndColorCheckButton->GetWidgetName() 
         << " -side left -fill both -padx 2" << endl;

  // --------------------------------------------------------------
  // Gradient opacity editor

  this->GradientOpacityFunctionEditor->SetParent(frame);
  this->GradientOpacityFunctionEditor->SetLabel("Gradient Opacity Mapping:");
  this->GradientOpacityFunctionEditor->ComputePointColorFromValueOn();
  this->GradientOpacityFunctionEditor->WindowLevelModeOn();
  this->GradientOpacityFunctionEditor->WindowLevelModeLockEndPointValueOn();
  this->GradientOpacityFunctionEditor->LockEndPointsParameterOn();
  this->GradientOpacityFunctionEditor->SetPointMarginToCanvas(
    this->ScalarOpacityFunctionEditor->GetPointMarginToCanvas());
  this->GradientOpacityFunctionEditor->DisableAddAndRemoveOn();
  this->GradientOpacityFunctionEditor->SetCanvasHeight(
    this->ScalarColorFunctionEditor->GetCanvasHeight());
  this->GradientOpacityFunctionEditor->SetShowValueRange(
    this->ScalarOpacityFunctionEditor->GetShowValueRange());
  this->GradientOpacityFunctionEditor->SetLabelPosition(
    this->ScalarOpacityFunctionEditor->GetLabelPosition());
  this->GradientOpacityFunctionEditor->SetRangeLabelPosition(
    this->ScalarOpacityFunctionEditor->GetRangeLabelPosition());
  this->GradientOpacityFunctionEditor->Create(app, "");

  this->GradientOpacityFunctionEditor->GetParameterEntry()->SetLabel(
    this->ScalarOpacityFunctionEditor->GetParameterEntry()->GetLabel()
    ->GetLabel());
  this->GradientOpacityFunctionEditor->GetValueEntry()->SetLabel("O:");

  this->GradientOpacityFunctionEditor->SetFunctionChangedCommand(
    this, "GradientOpacityFunctionChangedCallback");
  this->GradientOpacityFunctionEditor->SetFunctionChangingCommand(
    this, "GradientOpacityFunctionChangingCallback");

  // --------------------------------------------------------------
  // Enable gradient opacity

  this->GradientOpacityFunctionEditor->ShowUserFrameOn();
  this->EnableGradientOpacityOptionMenu->SetParent(
    this->GradientOpacityFunctionEditor->GetUserFrame());
  this->EnableGradientOpacityOptionMenu->Create(app, "-padx 1 -pady 0");
  this->EnableGradientOpacityOptionMenu->IndicatorOff();
  this->EnableGradientOpacityOptionMenu->SetBalloonHelpString(
    "Enable modulation of the opacity by the magnitude of the gradient "
    "according to the specified function.");
  this->EnableGradientOpacityOptionMenu->AddEntryWithCommand(
    "On", this, "EnableGradientOpacityCallback 1");
  this->EnableGradientOpacityOptionMenu->AddEntryWithCommand(
    "Off", this, "EnableGradientOpacityCallback 0");

  tk_cmd << "pack " << this->EnableGradientOpacityOptionMenu->GetWidgetName() 
         << " -side left -fill both -padx 0" << endl;

  // --------------------------------------------------------------
  // Component weights

  this->ComponentWeightScaleSet->SetParent(frame);
  this->ComponentWeightScaleSet->Create(app, "");
  this->ComponentWeightScaleSet->SetLabel("Component Weights:");

  vtkKWScaleSet *scaleset = this->ComponentWeightScaleSet->GetScaleSet();

  scaleset->PackHorizontallyOn();
  scaleset->SetMaximumNumberOfWidgetInPackingDirection(2);
  scaleset->SetPadding(2, 0);

  int i;
  char label[15];

  for (i = 0; i < VTK_MAX_VRCOMP; i++)
    {
    scaleset->AddScale(i);
    scaleset->HideScale(i);
    vtkKWScale *scale = scaleset->GetScale(i);
    scale->SetResolution(0.01);
    scale->DisplayEntry();
    sprintf(label, "%d:", i + 1);
    scale->DisplayLabel(label);
    scale->DisplayEntryAndLabelOnTopOff();
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
  this->HSVColorSelector->Create(app, "");
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

  // Sync

  vtkKWParameterValueFunctionEditor::SynchronizeSingleSelection(
    this->ScalarColorFunctionEditor, this->ScalarOpacityFunctionEditor);

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

  int row = 0;
  const char *colspan = " -columnspan 2 ";
  const char *col0 = " -column 0 ";
  const char *col1 = " -column 1 ";
  const char *pad = " -padx 2 -pady 2";
  const char *pad_ed = " -padx 2 -pady 3";
  
  /*
               col0       col1
         +-------------------------
         |     SC        HSV
         |     IT         |
         |     MP         |
         |     IA         |
         |     ES         |
         |     SC         |
         |     LOC        |
         |     WL         | 
         |     DGO        v
         |     SOF ------->
         |     CTF ------->
         |     GOF ------->
         |     CW  ------->
  */

  // HSV Color Selector (HSV)

  tk_cmd << "grid " << this->HSVColorSelector->GetWidgetName()
         << " -sticky nw " << col1 << " -row " << row << pad << endl;
  
  // Select Component (SC)

  if (this->ShowComponentSelection)
    {
    tk_cmd << "grid " << this->ComponentSelectionWidget->GetWidgetName()
           << " -sticky nw " << col0 << " -row " << row << pad << endl;

    row++;
    }

  // Interpolation type (IT)

  if (this->ShowInterpolationType)
    {
    tk_cmd << "grid " << this->InterpolationTypeOptionMenu->GetWidgetName()
           << " -sticky nw " << col0 << " -row " << row << pad
           << endl;
    row++;
    }

  // Material Property (MP)

  if (this->ShowMaterialProperty)
    {
    tk_cmd << "grid " << this->MaterialPropertyWidget->GetWidgetName()
           << " -sticky nw " << col0 << " -row " << row << pad << endl;
    row++;
    }

  // Enable Shading (ES)

  if (this->ShowMaterialProperty)
    {
    tk_cmd << "grid " << this->EnableShadingCheckButton->GetWidgetName()
           << " -sticky nw " << col0 << " -row " << row << pad << endl;
    row++;
    }

  // Interactive Apply (IA)

  tk_cmd << "grid " << this->InteractiveApplyCheckButton->GetWidgetName()
         << " -sticky nw " << col0 << " -row " << row << pad << endl;

  row++;

  tk_cmd << "grid " << this->HSVColorSelector->GetWidgetName()
         << " -rowspan " << row << endl;

  // --------------------------------------------------------------

  // Scalar Opacity Function (SOF)

  tk_cmd << "grid " << this->ScalarOpacityFunctionEditor->GetWidgetName()
         << " -sticky ew -column 0 -row " << row << colspan << pad_ed << endl;
  row++;

  // Color Transfer Function (CTF)

  tk_cmd << "grid " << this->ScalarColorFunctionEditor->GetWidgetName()
         << " -sticky ew -column 0 -row " << row << colspan << pad_ed << endl;
  row++;

  // Gradient Opacity Function (GOF)

  if (this->ShowGradientOpacityFunction)
    {
    tk_cmd << "grid " << this->GradientOpacityFunctionEditor->GetWidgetName()
           << " -sticky ew -column 0 -row " << row << colspan << pad_ed << endl;
    row++;
    }

  // Component weights (CW)

  if (this->ShowComponentWeights)
    {
    tk_cmd << "grid " << this->ComponentWeightScaleSet->GetWidgetName()
           << " -sticky ew -column 0 -row " << row << colspan << pad << endl;

    row++;
    }

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

  int nb_components = this->GetDataSetNumberOfComponents();

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
    vtkKWOptionMenu *m = this->InterpolationTypeOptionMenu->GetOptionMenu();
    if (has_prop)
      {
      switch (this->VolumeProperty->GetInterpolationType())
        {
        case VTK_NEAREST_INTERPOLATION:
          m->SetCurrentEntry(VTK_KW_VPW_INTERPOLATION_NEAREST);
          break;
        case VTK_LINEAR_INTERPOLATION:
          m->SetCurrentEntry(VTK_KW_VPW_INTERPOLATION_LINEAR);
          break;
        default:
          m->SetCurrentEntry("Unknown");
        }
      }
    else
      {
      m->SetCurrentEntry("");
      }
    }
    
  // Lock opacity and color

  if (this->LockOpacityAndColorCheckButton)
    {
    // If dependents or W/L, we can not lock

    if (this->WindowLevelMode[this->SelectedComponent] ||
        !this->GetIndependentComponents())
      {
      this->LockOpacityAndColor[this->SelectedComponent] = 0;
      this->LockOpacityAndColorCheckButton->SetEnabled(0);
      }

    this->LockOpacityAndColorCheckButton->SetState(
      this->LockOpacityAndColor[this->SelectedComponent]);
    }

  // Enable shading for all

  if (this->EnableShadingCheckButton)
    {
    if (has_prop)
      {
      this->EnableShadingCheckButton->SetState(
        this->VolumeProperty->GetShade(0));
      }
    if (!this->EnableShadingForAllComponents)
      {
      this->EnableShadingCheckButton->SetEnabled(0);
      }
    if (EnableShadingCheckButton->IsCreated())
      {
      tk_cmd << "grid " 
             << (this->EnableShadingForAllComponents ? "" : "remove")
            << " " << this->EnableShadingCheckButton->GetWidgetName() << endl;
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
      if (this->GetDataSetAdjustedScalarRange(scalar_field, p_range))
        {
        this->ScalarOpacityFunctionEditor->SetPiecewiseFunction(
          this->VolumeProperty->GetScalarOpacity(this->SelectedComponent));
        this->ScalarOpacityFunctionEditor
          ->SetWholeParameterRangeAndMaintainVisible(p_range);
      }

      this->ScalarOpacityFunctionEditor
        ->SetWholeValueRangeAndMaintainVisible(0.0, 1.0);
      this->ScalarOpacityFunctionEditor->SetWindowLevelMode(
        this->WindowLevelMode[this->SelectedComponent]);
      }
    else
      {
      this->ScalarOpacityFunctionEditor->SetPiecewiseFunction(0);
      }

    if (this->HistogramSet)
      {
      const char *name = this->GetDataSetScalarName();
      sprintf(hist_name, "%s%d", (name ? name : ""), scalar_field);
      this->ScalarOpacityFunctionEditor->SetHistogram(
        this->HistogramSet->GetHistogram(hist_name));
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
      if (this->GetDataSetAdjustedScalarRange(scalar_field, p_range))
        {
        this->ScalarColorFunctionEditor->SetColorTransferFunction(
          this->VolumeProperty->GetRGBTransferFunction(
            this->SelectedComponent));
        this->ScalarColorFunctionEditor
          ->SetWholeParameterRangeAndMaintainVisible(p_range);
        }
      }
    else
      {
      this->ScalarColorFunctionEditor->SetColorTransferFunction(0);
      }

    if (!no_rgb && this->HistogramSet)
      {
      const char *name = this->GetDataSetScalarName();
      sprintf(hist_name, "%s%d", (name ? name : ""), scalar_field);
      this->ScalarColorFunctionEditor->SetHistogram(
        this->HistogramSet->GetHistogram(hist_name));
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
      vtkKWParameterValueFunctionEditor::SynchronizeVisibleParameterRange(
        this->ScalarColorFunctionEditor, this->ScalarOpacityFunctionEditor);
      }
    else
      {
      vtkKWParameterValueFunctionEditor::DoNotSynchronizeVisibleParameterRange(
        this->ScalarColorFunctionEditor, this->ScalarOpacityFunctionEditor);
      }

    // (un)Synchronize both opacity and color functions points
    
    if (this->GetIndependentComponents() &&
        this->LockOpacityAndColor[this->SelectedComponent] && have_funcs)
      {
      vtkKWParameterValueFunctionEditor::SynchronizePoints(
        this->ScalarColorFunctionEditor, this->ScalarOpacityFunctionEditor);

      vtkKWParameterValueFunctionEditor::DoNotSynchronizeSingleSelection(
        this->ScalarColorFunctionEditor, this->ScalarOpacityFunctionEditor);

      vtkKWParameterValueFunctionEditor::SynchronizeSameSelection(
        this->ScalarColorFunctionEditor, this->ScalarOpacityFunctionEditor);
      }
    else
      {
      vtkKWParameterValueFunctionEditor::DoNotSynchronizePoints(
        this->ScalarColorFunctionEditor, this->ScalarOpacityFunctionEditor);

      vtkKWParameterValueFunctionEditor::DoNotSynchronizeSameSelection(
        this->ScalarColorFunctionEditor, this->ScalarOpacityFunctionEditor);

      if (have_funcs)
        {
        vtkKWParameterValueFunctionEditor::SynchronizeSingleSelection(
          this->ScalarColorFunctionEditor, this->ScalarOpacityFunctionEditor);
        }
      else
        {
        vtkKWParameterValueFunctionEditor::DoNotSynchronizeSingleSelection(
          this->ScalarColorFunctionEditor, this->ScalarOpacityFunctionEditor);
        }
      }
    }

  // Enable Gradient opacity

  if (this->EnableGradientOpacityOptionMenu && has_prop)
    {
    this->EnableGradientOpacityOptionMenu->SetCurrentEntry(
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
      if (this->GetDataSetScalarRange(scalar_field, p_range))
        {
        this->GradientOpacityFunctionEditor->SetPiecewiseFunction(
          this->VolumeProperty->GetStoredGradientOpacity(
            this->SelectedComponent));

        // WARNING: hard-coded value here according to the raycast mapper
        // behaviour (1/4 of the range)

        p_range[1] = (float)(0.25 * ((double)p_range[1] - (double)p_range[0]));
        p_range[0] = 0.0;
        this->GradientOpacityFunctionEditor
          ->SetWholeParameterRangeAndMaintainVisible(p_range);
        }
      this->GradientOpacityFunctionEditor
        ->SetWholeValueRangeAndMaintainVisible(0.0, 1.0);
      }
    else
      {
      this->GradientOpacityFunctionEditor->SetPiecewiseFunction(0);
      }

    if (this->HistogramSet)
      {
      sprintf(hist_name, "%s%d", "gradient", 
              this->GetIndependentComponents() ? this->SelectedComponent : 0);
      this->GradientOpacityFunctionEditor->SetHistogram(
        this->HistogramSet->GetHistogram(hist_name));
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
    vtkKWScaleSet *scaleset = this->ComponentWeightScaleSet->GetScaleSet();
    if (has_prop)
      {
      for (i = 0; i < VTK_MAX_VRCOMP; i++)
        {
        if (scaleset->GetScale(i))
          {
          int old_disable = scaleset->GetScale(i)->GetDisableCommands();
          scaleset->GetScale(i)->SetDisableCommands(1);
          scaleset->GetScale(i)->SetValue(
            this->VolumeProperty->GetComponentWeight(i));
          scaleset->GetScale(i)->SetDisableCommands(old_disable);
          }
        }
      }
    if (this->ComponentWeightScaleSet->IsCreated())
      {
      if (scaleset->GetNumberOfVisibleScales() != nb_components)
        {
        for (i = 0; i < VTK_MAX_VRCOMP; i++)
          {
          scaleset->SetScaleVisibility(i, (i < nb_components ? 1 : 0));
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

  if (this->EditorFrame)
    {
    this->EditorFrame->SetEnabled(this->Enabled);
    }

  if (this->ComponentSelectionWidget)
    {
    this->ComponentSelectionWidget->SetEnabled(this->Enabled);
    }

  if (this->InterpolationTypeOptionMenu)
    {
    this->InterpolationTypeOptionMenu->SetEnabled(this->Enabled);
    }

  if (this->InteractiveApplyCheckButton)
    {
    this->InteractiveApplyCheckButton->SetEnabled(this->Enabled);
    }

  if (this->ScalarOpacityFunctionEditor)
    {
    this->ScalarOpacityFunctionEditor->SetEnabled(this->Enabled);
    }

  if (this->ScalarOpacityUnitDistanceScale)
    {
    this->ScalarOpacityUnitDistanceScale->SetEnabled(this->Enabled);
    }

  if (this->EnableShadingCheckButton)
    {
    this->EnableShadingCheckButton->SetEnabled(this->Enabled);
    }

  if (this->MaterialPropertyWidget)
    {
    this->MaterialPropertyWidget->SetEnabled(this->Enabled);
    }

  if (this->LockOpacityAndColorCheckButton)
    {
    this->LockOpacityAndColorCheckButton->SetEnabled(this->Enabled);
    }

  if (this->ScalarColorFunctionEditor)
    {
    this->ScalarColorFunctionEditor->SetEnabled(this->Enabled);
    }

  if (this->EnableGradientOpacityOptionMenu)
    {
    this->EnableGradientOpacityOptionMenu->SetEnabled(this->Enabled);
    }

  if (this->GradientOpacityFunctionEditor)
    {
    this->GradientOpacityFunctionEditor->SetEnabled(this->Enabled);
    }

  if (this->ComponentWeightScaleSet)
    {
    this->ComponentWeightScaleSet->SetEnabled(this->Enabled);
    }

  if (this->HSVColorSelector)
    {
    this->HSVColorSelector->SetEnabled(this->Enabled);
    }
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
int vtkKWVolumePropertyWidget::GetDataSetNumberOfComponents()
{
  if (this->DataSet)
    {
    vtkDataArray *scalars = this->DataSet->GetPointData()->GetScalars();
    if (scalars)
      {
      return scalars->GetNumberOfComponents();
      }
    }
  return 0;
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
      return vtkKWMath::GetScalarRange(scalars, comp, range);
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
      return vtkKWMath::GetAdjustedScalarRange(scalars, comp, range);
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
void vtkKWVolumePropertyWidget::SetShowComponentSelection(int arg)
{
  if (this->ShowComponentSelection == arg)
    {
    return;
    }

  this->ShowComponentSelection = arg;

  this->Modified();

  this->Pack();
}

//----------------------------------------------------------------------------
void vtkKWVolumePropertyWidget::SetShowInterpolationType(int arg)
{
  if (this->ShowInterpolationType == arg)
    {
    return;
    }

  this->ShowInterpolationType = arg;

  this->Modified();

  this->Pack();
}

//----------------------------------------------------------------------------
void vtkKWVolumePropertyWidget::SetShowMaterialProperty(int arg)
{
  if (this->ShowMaterialProperty == arg)
    {
    return;
    }

  this->ShowMaterialProperty = arg;

  this->Modified();

  this->Pack();
}

//----------------------------------------------------------------------------
void vtkKWVolumePropertyWidget::SetShowGradientOpacityFunction(int arg)
{
  if (this->ShowGradientOpacityFunction == arg)
    {
    return;
    }

  this->ShowGradientOpacityFunction = arg;

  this->Modified();

  this->Pack();
}

//----------------------------------------------------------------------------
void vtkKWVolumePropertyWidget::SetShowComponentWeights(int arg)
{
  if (this->ShowComponentWeights == arg)
    {
    return;
    }

  this->ShowComponentWeights = arg;

  this->Modified();

  this->Pack();
}

//----------------------------------------------------------------------------
void vtkKWVolumePropertyWidget::SetSelectedComponent(int arg)
{
  if (this->SelectedComponent == arg ||
      arg < 0 || arg >= this->GetDataSetNumberOfComponents())
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
void vtkKWVolumePropertyWidget::InvokeCommand(const char *command)
{
  if (command && *command && !this->DisableCommands)
    {
    this->Script("eval %s", command);
    }
}

//----------------------------------------------------------------------------
void vtkKWVolumePropertyWidget::InvokeVolumePropertyChangedCommand()
{
  this->InvokeCommand(this->VolumePropertyChangedCommand);

#ifdef DO_NOT_BUILD_XML_RW
  this->InvokeEvent(vtkKWEvent::VolumePropertyChangedEvent, NULL);
#else
  if (!this->VolumeProperty)
    {
    this->InvokeEvent(vtkKWEvent::VolumePropertyChangedEvent, NULL);
    }
  else
    {
    ostrstream event;

    vtkXMLVolumePropertyWriter *xmlw = vtkXMLVolumePropertyWriter::New();
    xmlw->SetObject(this->VolumeProperty);
    xmlw->SetNumberOfComponents(this->GetDataSetNumberOfComponents());
    xmlw->WriteToStream(event);
    xmlw->Delete();

    event << ends;
    
    this->InvokeEvent(vtkKWEvent::VolumePropertyChangedEvent, event.str());
    event.rdbuf()->freeze(0);
    }
#endif
}

//----------------------------------------------------------------------------
void vtkKWVolumePropertyWidget::InvokeVolumePropertyChangingCommand()
{
  this->InvokeCommand(this->VolumePropertyChangingCommand);
  this->InvokeEvent(vtkKWEvent::VolumePropertyChangingEvent, NULL);
}

//----------------------------------------------------------------------------
void vtkKWVolumePropertyWidget::SetVolumePropertyChangedCommand(
  vtkKWObject *object, const char *method)
{
  this->SetObjectMethodCommand(
    &this->VolumePropertyChangedCommand, object, method);
}

//----------------------------------------------------------------------------
void vtkKWVolumePropertyWidget::SetVolumePropertyChangingCommand(
  vtkKWObject *object, const char *method)
{
  this->SetObjectMethodCommand(
    &this->VolumePropertyChangingCommand, object, method);
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
void vtkKWVolumePropertyWidget::EnableShadingCallback()
{
  if (!this->EnableShadingCheckButton || 
      !this->VolumeProperty || 
      !this->EnableShadingForAllComponents)
    {
    return;
    }

  unsigned long mtime = this->VolumeProperty->GetMTime();

  // Set the first component

  this->VolumeProperty->SetShade(
    0, this->EnableShadingCheckButton->GetState() ? 1 : 0);

  // Update the others

  int nb_shade_comp = this->GetIndependentComponents() 
    ? this->GetDataSetNumberOfComponents() : 1;
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
void vtkKWVolumePropertyWidget::LockOpacityAndColorCallback()
{
  if (!this->LockOpacityAndColorCheckButton)
    {
    return;
    }

  this->LockOpacityAndColor[this->SelectedComponent] = 
    this->LockOpacityAndColorCheckButton->GetState();

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
  if (this->InteractiveApplyCheckButton && 
      this->InteractiveApplyCheckButton->GetState())
    {
    this->InvokeVolumePropertyChangingCommand();
    }
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

  if (this->InteractiveApplyCheckButton && 
      this->InteractiveApplyCheckButton->GetState())
    {
    this->InvokeVolumePropertyChangingCommand();
    }
}

//----------------------------------------------------------------------------
void vtkKWVolumePropertyWidget::ScalarOpacityUnitDistanceChangedCallback()
{
  if (!this->IsCreated() || !this->VolumeProperty)
    {
    return;
    }

  float d = this->ScalarOpacityUnitDistanceScale->GetValue();
  this->VolumeProperty->SetScalarOpacityUnitDistance(
    this->SelectedComponent, d);
  
  this->InvokeVolumePropertyChangedCommand();
}

//----------------------------------------------------------------------------
void vtkKWVolumePropertyWidget::ScalarOpacityUnitDistanceChangingCallback()
{
  if (!this->IsCreated() || !this->VolumeProperty)
    {
    return;
    }

  if (this->InteractiveApplyCheckButton && 
      this->InteractiveApplyCheckButton->GetState())
    {
    float d = this->ScalarOpacityUnitDistanceScale->GetValue();
    this->VolumeProperty->SetScalarOpacityUnitDistance(
      this->SelectedComponent, d);
  
    this->InvokeVolumePropertyChangingCommand();
    }
}


//----------------------------------------------------------------------------
void vtkKWVolumePropertyWidget::WindowLevelModeCallback()
{
  this->WindowLevelMode[this->SelectedComponent] = 
    this->ScalarOpacityFunctionEditor->GetWindowLevelMode();

  this->Update();
}

//----------------------------------------------------------------------------
void vtkKWVolumePropertyWidget::RGBTransferFunctionChangedCallback()
{
  this->UpdateHSVColorSelectorFromScalarColorFunctionEditor();

  this->InvokeVolumePropertyChangedCommand();
}

//----------------------------------------------------------------------------
void vtkKWVolumePropertyWidget::RGBTransferFunctionChangingCallback()
{
  this->UpdateHSVColorSelectorFromScalarColorFunctionEditor();

  if (this->InteractiveApplyCheckButton && 
      this->InteractiveApplyCheckButton->GetState())
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
  if (this->InteractiveApplyCheckButton && 
      this->InteractiveApplyCheckButton->GetState())
    {
    this->InvokeVolumePropertyChangingCommand();
    }
}

//----------------------------------------------------------------------------
void vtkKWVolumePropertyWidget::HSVColorSelectionChangedCallback()
{
  if (!this->HSVColorSelector ||
      !this->HSVColorSelector->HasSelection() ||
      !this->ScalarColorFunctionEditor || 
      !this->ScalarColorFunctionEditor->HasFunction() || 
      !this->ScalarColorFunctionEditor->HasSelection())
    {
    return;
    }
  
  this->ScalarColorFunctionEditor->SetPointColorAsHSV(
    this->ScalarColorFunctionEditor->GetSelectedPoint(), 
    this->HSVColorSelector->GetSelectedColor());

  this->InvokeVolumePropertyChangedCommand();
}

//----------------------------------------------------------------------------
void vtkKWVolumePropertyWidget::HSVColorSelectionChangingCallback()
{
  if (!this->HSVColorSelector ||
      !this->HSVColorSelector->HasSelection() ||
      !this->ScalarColorFunctionEditor || 
      !this->ScalarColorFunctionEditor->HasFunction() || 
      !this->ScalarColorFunctionEditor->HasSelection())
    {
    return;
    }

  unsigned long mtime = 
    this->ScalarColorFunctionEditor->GetColorTransferFunction()->GetMTime();

  this->ScalarColorFunctionEditor->SetPointColorAsHSV(
    this->ScalarColorFunctionEditor->GetSelectedPoint(), 
    this->HSVColorSelector->GetSelectedColor());

  if (this->ScalarColorFunctionEditor->GetColorTransferFunction()->GetMTime() >
      mtime &&
      this->InteractiveApplyCheckButton && 
      this->InteractiveApplyCheckButton->GetState())
    {
    this->InvokeVolumePropertyChangingCommand();
    }
}

//----------------------------------------------------------------------------
void vtkKWVolumePropertyWidget::ComponentWeightChangedCallback(int index)
{
  if (!this->IsCreated() || !this->VolumeProperty)
    {
    return;
    }

  vtkKWScaleSet *scaleset = this->ComponentWeightScaleSet->GetScaleSet();
  if (index < 0 || index > scaleset->GetNumberOfVisibleScales())
    {
    return;
    }

  float weight = scaleset->GetScale(index)->GetValue();
  this->VolumeProperty->SetComponentWeight(index, weight);
  
  float fargs[2];
  fargs[0] = index;
  fargs[1] = weight;
  this->InvokeEvent(vtkKWEvent::ScalarComponentWeightChangedEvent, fargs);

  this->InvokeVolumePropertyChangedCommand();
}

//----------------------------------------------------------------------------
void vtkKWVolumePropertyWidget::ComponentWeightChangingCallback(int index)
{
  if (!this->IsCreated() || !this->VolumeProperty)
    {
    return;
    }

  vtkKWScaleSet *scaleset = this->ComponentWeightScaleSet->GetScaleSet();
  if (index < 0 || index > scaleset->GetNumberOfVisibleScales())
    {
    return;
    }

  float weight = scaleset->GetScale(index)->GetValue();
  this->VolumeProperty->SetComponentWeight(index, weight);
  
  float fargs[2];
  fargs[0] = index;
  fargs[1] = weight;
  this->InvokeEvent(vtkKWEvent::ScalarComponentWeightChangingEvent, fargs);

  if (this->InteractiveApplyCheckButton && 
      this->InteractiveApplyCheckButton->GetState())
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
  os << indent << "ShowComponentSelection: "
     << (this->ShowComponentSelection ? "On" : "Off") << endl;
  os << indent << "ShowInterpolationType: "
     << (this->ShowInterpolationType ? "On" : "Off") << endl;
  os << indent << "ShowMaterialProperty: "
     << (this->ShowMaterialProperty ? "On" : "Off") << endl;
  os << indent << "ShowGradientOpacityFunction: "
     << (this->ShowGradientOpacityFunction ? "On" : "Off") << endl;
  os << indent << "ShowComponentWeights: "
     << (this->ShowComponentWeights ? "On" : "Off") << endl;
  os << indent << "ScalarOpacityFunctionEditor: ";
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
}
