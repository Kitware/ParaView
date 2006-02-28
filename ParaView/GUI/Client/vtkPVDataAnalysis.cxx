/*=========================================================================

  Program:   ParaView
  Module:    vtkPVDataAnalysis.cxx

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkPVDataAnalysis.h"

#include "vtkCellData.h"
#include "vtkCommand.h"
#include "vtkDataArray.h"
#include "vtkDataArraySelection.h"
#include "vtkObjectFactory.h"
#include "vtkKWCheckButton.h"
#include "vtkKWEntry.h"
#include "vtkKWEntryWithLabel.h"
#include "vtkKWFrameWithLabel.h"
#include "vtkKWFrameWithScrollbar.h"
#include "vtkKWLabel.h"
#include "vtkKWLoadSaveButton.h"
#include "vtkKWLoadSaveDialog.h"
#include "vtkKWMenu.h"
#include "vtkKWMenuButton.h"
#include "vtkKWMultiColumnList.h"
#include "vtkKWMultiColumnListWithScrollbars.h"
#include "vtkKWRange.h"
#include "vtkPickProbeFilter.h"
#include "vtkPointData.h"
#include "vtkPVApplication.h"
#include "vtkPVPlotArraySelection.h"
#include "vtkPVDisplayGUI.h"
#include "vtkPVPlotDisplayLabelPropertiesDialog.h"
#include "vtkPVSourceNotebook.h"
#include "vtkPVReaderModule.h"
#include "vtkPVRenderView.h"
#include "vtkPVSelectWidget.h"
#include "vtkPVTraceHelper.h"
#include "vtkPVWindow.h"
#include "vtkSMDoubleVectorProperty.h"
#include "vtkSMIntVectorProperty.h"
#include "vtkSMInputProperty.h"
#include "vtkSMPointLabelDisplayProxy.h"
#include "vtkSMProxyManager.h"
#include "vtkSMSourceProxy.h"
#include "vtkSMStringVectorProperty.h"
#include "vtkSMStringListDomain.h"
#include "vtkSMTemporalXYPlotDisplayProxy.h"
#include "vtkStdString.h"
#include "vtkUnstructuredGrid.h"

#include <vtksys/ios/sstream>

//*****************************************************************************
class vtkPVDataAnalysisObserver : public vtkCommand
{
public:
  static vtkPVDataAnalysisObserver* New()
    { return new vtkPVDataAnalysisObserver; }

  void SetTarget(vtkPVDataAnalysis* target)
    {
    this->Target = target;
    }
  
  virtual void Execute(vtkObject* , unsigned long , void* )
    {
    if (this->Target)
      {
      this->Target->Script("update");
      }
    }
protected:
  vtkPVDataAnalysisObserver() { this->Target = 0; }
  ~vtkPVDataAnalysisObserver() { this->Target = 0; } 
  vtkPVDataAnalysis* Target;
};

//*****************************************************************************

vtkStandardNewMacro(vtkPVDataAnalysis);
vtkCxxRevisionMacro(vtkPVDataAnalysis, "1.1");
//-----------------------------------------------------------------------------
vtkPVDataAnalysis::vtkPVDataAnalysis()
{

  this->DataInformationFrame = vtkKWFrameWithLabel::New();
  this->DataInformationList = vtkKWMultiColumnListWithScrollbars::New();

  this->PlotParametersFrame = vtkKWFrameWithLabel::New();
  this->ArraySelectionFrame = vtkKWFrame::New();
  this->PointArraySelection = vtkPVPlotArraySelection::New();
  this->CellArraySelection = vtkPVPlotArraySelection::New();
  this->PlotOverTimeCheckButton = vtkKWCheckButton::New();
  this->ShowXYPlotCheckButton = vtkKWCheckButton::New();
  this->SaveCSVButton = vtkKWLoadSaveButton::New();
  
  this->TemporalParametersFrame = vtkKWFrameWithLabel::New();
  this->SourceNameLabelLabel = vtkKWLabel::New();
  this->SourceNameLabel = vtkKWLabel::New();
  this->RangeLabel = vtkKWLabel::New();
  this->Range = vtkKWRange::New();
  this->GenerateButton = vtkKWPushButton::New();
  this->LockTemporalCacheCheckButton = vtkKWCheckButton::New();

  this->PlotDisplayPropertiesFrame = vtkKWFrameWithLabel::New();
  this->PlotTitleLabel = vtkKWLabel::New();
  this->PlotTitleFrame = vtkKWFrame::New();
  this->PlotTitleEntry = vtkKWEntry::New();
  this->PlotTitleXPositionWidget = vtkKWEntryWithLabel::New();
  this->PlotTitleYPositionWidget = vtkKWEntryWithLabel::New();
  this->AdjustTitlePositionCheckButton = vtkKWCheckButton::New();
  this->XLabelLabel = vtkKWLabel::New();
  this->XLabelEntry = vtkKWEntry::New();
  this->YLabelLabel = vtkKWLabel::New();
  this->YLabelEntry = vtkKWEntry::New();
  this->XLabelEditButton = vtkKWPushButton::New();
  this->YLabelEditButton = vtkKWPushButton::New();

  this->LegendLabel = vtkKWLabel::New();
  this->ShowLegendCheckButton = vtkKWCheckButton::New();
  this->LegendXPositionWidget = vtkKWEntryWithLabel::New();
  this->LegendYPositionWidget = vtkKWEntryWithLabel::New();
  this->PlotTypeLabel = vtkKWLabel::New();
  this->PlotTypeMenuButton = vtkKWMenuButton::New();

  this->LabelPropertiesDialog = 0;
  
  this->Observer = vtkPVDataAnalysisObserver::New();
  this->Observer->SetTarget(this);
    
  this->PlotDisplayProxy = 0;
  this->PlotDisplayProxyName = 0;

  this->AnimationCueProxy = 0;
  this->AnimationManipulatorProxy = 0;

  this->PlottingPointData = 1;
  this->TimeSupportAvailable = 0;
  this->LastAcceptedQueryMethod = 0;
  this->SetLastAcceptedQueryMethod("");
}

//-----------------------------------------------------------------------------
vtkPVDataAnalysis::~vtkPVDataAnalysis()
{
  this->CleanupDisplays();

  this->DataInformationFrame->Delete();
  this->DataInformationList->Delete();
  
  this->PlotParametersFrame->Delete();
  this->ArraySelectionFrame->Delete();
  this->PointArraySelection->Delete();
  this->CellArraySelection->Delete();
  this->ShowXYPlotCheckButton->Delete();
  this->PlotOverTimeCheckButton->Delete();
  this->SaveCSVButton->Delete();
  
  this->TemporalParametersFrame->Delete();
  this->SourceNameLabelLabel->Delete();
  this->SourceNameLabel->Delete();
  this->RangeLabel->Delete();
  this->Range->Delete();
  this->GenerateButton->Delete();
  this->LockTemporalCacheCheckButton->Delete();

  this->PlotDisplayPropertiesFrame->Delete();
  this->PlotTitleLabel->Delete();
  this->PlotTitleFrame->Delete();
  this->PlotTitleEntry->Delete();
  this->PlotTitleXPositionWidget->Delete();
  this->PlotTitleYPositionWidget->Delete();
  this->AdjustTitlePositionCheckButton->Delete();
  this->XLabelLabel->Delete();
  this->XLabelEntry->Delete();
  this->YLabelLabel->Delete();
  this->YLabelEntry->Delete();
  this->XLabelEditButton->Delete();
  this->YLabelEditButton->Delete();

  this->LegendLabel->Delete();
  this->ShowLegendCheckButton->Delete();
  this->LegendXPositionWidget->Delete();
  this->LegendYPositionWidget->Delete();
  this->PlotTypeLabel->Delete();
  this->PlotTypeMenuButton->Delete();

  if (this->LabelPropertiesDialog)
    {
    this->LabelPropertiesDialog->Delete();
    }

  this->Observer->SetTarget(0);
  this->Observer->Delete();

  vtkSMProxyManager* pxm = vtkSMProxyManager::GetProxyManager();
  if (this->AnimationCueProxy)
    {
    const char* name = pxm->GetProxyName("animation",
      this->AnimationCueProxy);
    if (name)
      {
      pxm->UnRegisterProxy("animation", name);
      }
    this->AnimationCueProxy->Delete();
    }

  if (this->AnimationManipulatorProxy)
    {
    const char* name = pxm->GetProxyName("animation_manipulators",
      this->AnimationManipulatorProxy);
    if (name)
      {
      pxm->UnRegisterProxy("animation_manipulators", name);
      }
    this->AnimationManipulatorProxy->Delete();
    }

  this->SetLastAcceptedQueryMethod(0);
}

//-----------------------------------------------------------------------------
void vtkPVDataAnalysis::CreateProperties()
{
  this->Superclass::CreateProperties();
  
  this->PlotParametersFrame->SetParent(this->ParameterFrame->GetFrame());
  this->PlotParametersFrame->Create();
  this->PlotParametersFrame->SetLabelText("XY Scalar Plot");

  this->ShowXYPlotCheckButton->SetParent(this->PlotParametersFrame->GetFrame());
  this->ShowXYPlotCheckButton->Create();
  this->ShowXYPlotCheckButton->SetText("Show XY-Plot");
  this->ShowXYPlotCheckButton->SetSelectedState(0);
  this->ShowXYPlotCheckButton->SetBalloonHelpString(
    "Toggle the XY Plot visibility.");
  this->Script("%s configure -command {%s SetAcceptButtonColorToModified}",
    this->ShowXYPlotCheckButton->GetWidgetName(), this->GetTclName());
  this->Script("pack %s -anchor w", this->ShowXYPlotCheckButton->GetWidgetName());

  this->PlotOverTimeCheckButton->SetParent(this->PlotParametersFrame->GetFrame());
  this->PlotOverTimeCheckButton->Create();
  this->PlotOverTimeCheckButton->SetText("Enable temporal plotting.");
  this->PlotOverTimeCheckButton->SetSelectedState(0);
  this->PlotOverTimeCheckButton->SetCommand(this, 
    "PlotOverTimeCheckButtonCallback");
  this->PlotOverTimeCheckButton->SetBalloonHelpString(
    "Enable temporal plotting.");
  this->Script("pack %s -anchor w", 
    this->PlotOverTimeCheckButton->GetWidgetName());
  
  // Add a button to save XYPlotActor as CSV file
  this->SaveCSVButton->SetParent(this->PlotParametersFrame->GetFrame());
  this->SaveCSVButton->Create();
  this->SaveCSVButton->SetCommand(this, "SaveDialogCallback");
  this->SaveCSVButton->SetText("Save as CSV");
  vtkKWLoadSaveDialog *dlg = this->SaveCSVButton->GetLoadSaveDialog();
  dlg->SetDefaultExtension(".csv");
  dlg->SetFileTypes("{{CSV Document} {.csv}}");
  dlg->SaveDialogOn();
  this->Script("pack %s -anchor w", this->SaveCSVButton->GetWidgetName());

  this->ArraySelectionFrame->SetParent(this->PlotParametersFrame->GetFrame());
  this->ArraySelectionFrame->Create();
  this->Script("pack %s -fill x -expand true -anchor w",
    this->ArraySelectionFrame->GetWidgetName());
  
  this->PointArraySelection->SetParent(this->ArraySelectionFrame);
  this->PointArraySelection->GetTraceHelper()->SetReferenceHelper(
    this->GetTraceHelper());
  this->PointArraySelection->GetTraceHelper()->SetReferenceCommand(
    "GetPointArraySelection");
  this->PointArraySelection->SetPVSource(this);
  this->PointArraySelection->SetLabelText("Point Scalars");
  this->PointArraySelection->SetModifiedCommand(this->GetTclName(), 
    "PointArraySelectionModifiedCallback");

  this->CellArraySelection->SetParent(this->ArraySelectionFrame);
  this->CellArraySelection->GetTraceHelper()->SetReferenceHelper(
    this->GetTraceHelper());
  this->CellArraySelection->GetTraceHelper()->SetReferenceCommand(
    "GetCellArraySelection");
  this->CellArraySelection->SetPVSource(this);
  this->CellArraySelection->SetLabelText("Cell Scalars");
  this->CellArraySelection->SetModifiedCommand(this->GetTclName(), 
    "CellArraySelectionModifiedCallback");

  // Temporal parameters.
  this->TemporalParametersFrame->SetParent(this->PlotParametersFrame->GetFrame());
  this->TemporalParametersFrame->Create();
  this->TemporalParametersFrame->SetLabelText("Temporal Parameters");

  this->SourceNameLabelLabel->SetParent(this->TemporalParametersFrame->GetFrame());
  this->SourceNameLabelLabel->Create();
  this->SourceNameLabelLabel->SetText("Source ");
  
  this->SourceNameLabel->SetParent(this->TemporalParametersFrame->GetFrame());
  this->SourceNameLabel->Create();

  this->Script("grid %s %s - - -sticky w",
    this->SourceNameLabelLabel->GetWidgetName(),
    this->SourceNameLabel->GetWidgetName());

  this->RangeLabel->SetParent(this->TemporalParametersFrame->GetFrame());
  this->RangeLabel->Create();
  this->RangeLabel->SetText("Range ");
  
  this->Range->SetParent(this->TemporalParametersFrame->GetFrame());
  this->Range->Create();
  this->Range->EntriesVisibilityOn();
  this->Range->LabelVisibilityOff();
  this->Range->SetEntry1PositionToLeft();
  this->Range->SetEntry2PositionToRight();
  this->Range->ClampRangeOn();
  this->Range->SetBalloonHelpString(
    "Set the range of timesteps to generate the temporal plot.");

  this->Script("grid %s %s - - -sticky ew",
    this->RangeLabel->GetWidgetName(),
    this->Range->GetWidgetName());
 
  this->GenerateButton->SetParent(this->TemporalParametersFrame->GetFrame());
  this->GenerateButton->Create();
  this->GenerateButton->SetText("Generate");
  this->GenerateButton->SetCommand(this, "GenerateTemporalPlot");
  this->GenerateButton->SetBalloonHelpString("Generate Temporal Plot.");
 
  this->LockTemporalCacheCheckButton->SetParent(
    this->TemporalParametersFrame->GetFrame());
  this->LockTemporalCacheCheckButton->Create();
  this->LockTemporalCacheCheckButton->SetAnchorToEast();
  this->LockTemporalCacheCheckButton->SetText("Lock");
  this->LockTemporalCacheCheckButton->SetSelectedState(0);
  this->LockTemporalCacheCheckButton->SetCommand(this,
    "LockTemporalCacheCheckButtonCallback");
  this->LockTemporalCacheCheckButton->SetBalloonHelpString(
    "Set to lock the generated temporal graph, unless the Query Method is changed.");

  this->Script("grid x %s %s x -sticky w", this->GenerateButton->GetWidgetName(),
    this->LockTemporalCacheCheckButton->GetWidgetName());

  this->Script("grid columnconfigure %s 3 -weight 2",
    this->TemporalParametersFrame->GetFrame()->GetWidgetName());

  this->Script("pack %s -fill x -expand true", 
    this->PlotParametersFrame->GetWidgetName());


  // Plot display properties frame.
  this->PlotDisplayPropertiesFrame->SetParent(this->ParameterFrame->GetFrame());
  this->PlotDisplayPropertiesFrame->Create();
  this->PlotDisplayPropertiesFrame->SetLabelText("XY Plot Display");

  this->PlotTitleLabel->SetParent(
    this->PlotDisplayPropertiesFrame->GetFrame());
  this->PlotTitleLabel->Create();
  this->PlotTitleLabel->SetText("Plot Title ");
  this->PlotTitleLabel->SetAnchorToWest();

  this->PlotTitleFrame->SetParent(
    this->PlotDisplayPropertiesFrame->GetFrame());
  this->PlotTitleFrame->Create();
  
  this->PlotTitleEntry->SetParent(this->PlotTitleFrame);
  this->PlotTitleEntry->Create();
  this->PlotTitleEntry->SetCommand(this, "SetPlotTitle");
  this->PlotTitleEntry->SetBalloonHelpString("Edit Plot Title.");

  this->PlotTitleXPositionWidget->SetParent(this->PlotTitleFrame);
  this->PlotTitleXPositionWidget->Create();
  this->PlotTitleXPositionWidget->SetLabelPositionToLeft();
  this->PlotTitleXPositionWidget->SetLabelText("X: ");
  this->PlotTitleXPositionWidget->GetWidget()->SetValueAsDouble(0.5);
  this->PlotTitleXPositionWidget->GetWidget()->SetCommand(this, 
    "SetPlotTitlePositionCallback");
  this->PlotTitleXPositionWidget->GetWidget()->SetWidth(4); 
  this->PlotTitleXPositionWidget->SetBalloonHelpString(
    "Set the normalized X Position for the title"
    " relative to the window size.");
  
  this->PlotTitleYPositionWidget->SetParent(this->PlotTitleFrame);
  this->PlotTitleYPositionWidget->Create();
  this->PlotTitleYPositionWidget->SetLabelPositionToLeft();
  this->PlotTitleYPositionWidget->SetLabelText("Y: ");
  this->PlotTitleYPositionWidget->GetWidget()->SetValueAsDouble(0.5);
  this->PlotTitleYPositionWidget->GetWidget()->SetCommand(this, 
    "SetPlotTitlePositionCallback");
  this->PlotTitleYPositionWidget->GetWidget()->SetWidth(4); 
  this->PlotTitleYPositionWidget->SetBalloonHelpString(
    "Set the normalized Y Position for the title"
    " relative to the window size.");   
  
  this->Script("pack %s -fill x -expand true -side left",
    this->PlotTitleEntry->GetWidgetName());
  this->Script("pack %s %s -side left",
    this->PlotTitleXPositionWidget->GetWidgetName(),
    this->PlotTitleYPositionWidget->GetWidgetName());

  this->AdjustTitlePositionCheckButton->SetParent(
    this->PlotDisplayPropertiesFrame->GetFrame());
  this->AdjustTitlePositionCheckButton->Create();
  this->AdjustTitlePositionCheckButton->SetSelectedState(1);
  this->AdjustTitlePositionCheckButton->SetCommand(this,
    "SetAdjustTitlePosition");
  this->AdjustTitlePositionCheckButton->SetAnchorToEast();
  this->AdjustTitlePositionCheckButton->SetText("Auto");
  this->AdjustTitlePositionCheckButton->SetBalloonHelpString(
    "Toggle auto adjusting of the Title position.");

  this->Script("grid %s %s - - - %s -sticky ew",
    this->PlotTitleLabel->GetWidgetName(),
    this->PlotTitleFrame->GetWidgetName(),
    this->AdjustTitlePositionCheckButton->GetWidgetName());

  this->XLabelLabel->SetParent(
    this->PlotDisplayPropertiesFrame->GetFrame());
  this->XLabelLabel->Create();
  this->XLabelLabel->SetText("X Axis Title ");
  this->XLabelLabel->SetAnchorToWest();

  this->XLabelEntry->SetParent(
    this->PlotDisplayPropertiesFrame->GetFrame());
  this->XLabelEntry->Create();
  this->XLabelEntry->SetCommand(this, "SetXAxisLabel");
  this->XLabelEntry->SetBalloonHelpString("Edit X Axis Title.");

  this->XLabelEditButton->SetParent(
    this->PlotDisplayPropertiesFrame->GetFrame());
  this->XLabelEditButton->Create();
  this->XLabelEditButton->SetText("Edit");
  this->XLabelEditButton->SetCommand(this, "EditXLabelCallback 1");
  this->XLabelEditButton->SetBalloonHelpString("Edit X Axis Labels.");

  this->Script("grid %s %s - - - %s -sticky ew",
    this->XLabelLabel->GetWidgetName(),
    this->XLabelEntry->GetWidgetName(),
    this->XLabelEditButton->GetWidgetName());

  this->YLabelLabel->SetParent(
    this->PlotDisplayPropertiesFrame->GetFrame());
  this->YLabelLabel->Create();
  this->YLabelLabel->SetText("Y Axis Title ");
  this->YLabelLabel->SetAnchorToWest();

  this->YLabelEntry->SetParent(
    this->PlotDisplayPropertiesFrame->GetFrame());
  this->YLabelEntry->Create();
  this->YLabelEntry->SetCommand(this, "SetYAxisLabel");
  this->YLabelEntry->SetBalloonHelpString("Edit Y Axis Title.");

  this->YLabelEditButton->SetParent(
    this->PlotDisplayPropertiesFrame->GetFrame());
  this->YLabelEditButton->Create();
  this->YLabelEditButton->SetText("Edit");
  this->YLabelEditButton->SetCommand(this, "EditYLabelCallback 1");
  this->YLabelEditButton->SetBalloonHelpString("Edit Y Axis Labels.");

  this->Script("grid %s %s - - - %s -sticky ew",
    this->YLabelLabel->GetWidgetName(),
    this->YLabelEntry->GetWidgetName(),
    this->YLabelEditButton->GetWidgetName());

  this->LegendLabel->SetParent(
    this->PlotDisplayPropertiesFrame->GetFrame());
  this->LegendLabel->Create();
  this->LegendLabel->SetText("Legend ");

  this->ShowLegendCheckButton->SetParent(
    this->PlotDisplayPropertiesFrame->GetFrame());
  this->ShowLegendCheckButton->Create();
  this->ShowLegendCheckButton->SetSelectedState(1);
  this->ShowLegendCheckButton->SetCommand(this, "SetLegendVisibility");
  this->ShowLegendCheckButton->SetAnchorToEast();
  this->ShowLegendCheckButton->SetText("Show Legend");
  this->ShowLegendCheckButton->SetBalloonHelpString(
    "Toggle the visibility of the legend on the XY plot.");

  this->LegendXPositionWidget->SetParent(
    this->PlotDisplayPropertiesFrame->GetFrame());
  this->LegendXPositionWidget->Create();
  this->LegendXPositionWidget->SetLabelPositionToLeft();
  this->LegendXPositionWidget->SetLabelText("X Pos");
  this->LegendXPositionWidget->GetWidget()->SetValueAsDouble(0.85);
  this->LegendXPositionWidget->GetWidget()->SetWidth(7);
  this->LegendXPositionWidget->GetWidget()->SetCommand(this,
    "LegendPositionCallback");
  this->LegendXPositionWidget->SetBalloonHelpString(
    "Set the normalized X position for the legend relative to the plot size");

  this->LegendYPositionWidget->SetParent(
    this->PlotDisplayPropertiesFrame->GetFrame());
  this->LegendYPositionWidget->Create();
  this->LegendYPositionWidget->SetLabelPositionToLeft();
  this->LegendYPositionWidget->SetLabelText("Y Pos");
  this->LegendYPositionWidget->GetWidget()->SetValueAsDouble(0.75);
  this->LegendYPositionWidget->GetWidget()->SetWidth(7);
  this->LegendYPositionWidget->GetWidget()->SetCommand(this,
    "LegendPositionCallback");
  this->LegendYPositionWidget->SetBalloonHelpString(
    "Set the normalized X position for the legend relative to the plot size");

  this->Script("grid %s %s %s %s x x -sticky w",
    this->LegendLabel->GetWidgetName(),
    this->LegendXPositionWidget->GetWidgetName(),
    this->LegendYPositionWidget->GetWidgetName(),
    this->ShowLegendCheckButton->GetWidgetName());


  this->PlotTypeLabel->SetParent(
    this->PlotDisplayPropertiesFrame->GetFrame());
  this->PlotTypeLabel->Create();
  this->PlotTypeLabel->SetText("Plot Type ");
  
  this->PlotTypeMenuButton->SetParent(
    this->PlotDisplayPropertiesFrame->GetFrame());
  this->PlotTypeMenuButton->Create();
  this->PlotTypeMenuButton->AddRadioButton("Points",
    this, "SetPlotTypeToPoints", "Plot data values as points.");
  this->PlotTypeMenuButton->AddRadioButton("Lines",
    this, "SetPlotTypeToLines", "Plot data values as lines.");
  this->PlotTypeMenuButton->AddRadioButton("Points & Lines",
    this, "SetPlotTypeToPointsAndLines",
    "Plot data values as points joined by lines.");
  this->PlotTypeMenuButton->SetValue("Points & Lines");

  this->Script("grid %s %s - x x x -sticky w",
    this->PlotTypeLabel->GetWidgetName(),
    this->PlotTypeMenuButton->GetWidgetName());
  
  this->Script("grid columnconfigure %s 4 -weight 2 -pad 4",
    this->PlotDisplayPropertiesFrame->GetFrame()->GetWidgetName());

  // Data Information frame.
  this->DataInformationFrame->SetParent(this->ParameterFrame->GetFrame());
  this->DataInformationFrame->Create();
  this->DataInformationFrame->SetLabelText("Data Analysis");

  this->DataInformationList->SetParent(this->DataInformationFrame->GetFrame());
  this->DataInformationList->SetHorizontalScrollbarVisibility(0);
  this->DataInformationList->SetVerticalScrollbarVisibility(0);
  this->DataInformationList->Create();
  this->DataInformationList->GetWidget()->StretchableColumnsOn();
  this->DataInformationList->GetWidget()->AddColumn(" ");
  this->DataInformationList->GetWidget()->AddColumn("Name");
  this->DataInformationList->GetWidget()->AddColumn("Type");
  this->DataInformationList->GetWidget()->AddColumn("Data Type");
  this->DataInformationList->GetWidget()->AddColumn("Value");

  this->Script("pack %s -fill x -expand true",
    this->DataInformationList->GetWidgetName());
}

//-----------------------------------------------------------------------------
void vtkPVDataAnalysis::UpdateVTKSourceParameters()
{
  this->Superclass::UpdateVTKSourceParameters();
  
  vtkSMProxy* proxy = this->GetProxy();
  
  vtkPVSelectWidget* pvs = vtkPVSelectWidget::SafeDownCast(
    this->GetPVWidget("QueryMethod"));
  if (!pvs)
    {
    vtkErrorMacro("Failed to locate widget QueryMethod.");
    return;
    }
  const char* label = pvs->GetCurrentLabel();

  vtkSMIntVectorProperty* ivp = vtkSMIntVectorProperty::SafeDownCast(
    proxy->GetProperty("PickCell"));

  if (!ivp)
    {
    vtkErrorMacro("Failed to locate property PickCell.");
    return;
    }

  if (!strcmp(label, "Cell") || !strcmp(label, "Cell Id"))
    {
    ivp->SetElement(0, 1);
    this->PlottingPointData = 0;
    }
  else
    {
    ivp->SetElement(0, 0);
    this->PlottingPointData = 1;
    }

  ivp = vtkSMIntVectorProperty::SafeDownCast(
    proxy->GetProperty("UseIdToPick"));
  if (!ivp)
    {
    vtkErrorMacro("Failed to locate property UseIdToPick.");
    return;
    }
  if (!strcmp(label, "Cell Id") || !strcmp(label, "Point Id"))
    {
    ivp->SetElement(0, 1);
    }
  else
    {
    ivp->SetElement(0, 0);
    }

  ivp = vtkSMIntVectorProperty::SafeDownCast(
    proxy->GetProperty("Mode"));
  int mode = ivp->GetElement(0);

  if (mode == vtkPickProbeFilter::PICK)
    {
    // Source is used only when in Probe mode.
    vtkSMProxyProperty* sourceProperty = vtkSMProxyProperty::SafeDownCast(
      proxy->GetProperty("Source"));
    if (sourceProperty && sourceProperty->GetNumberOfProxies() > 0 &&
      sourceProperty->GetProxy(0)!=0)
      {
      sourceProperty->RemoveAllProxies();
      sourceProperty->AddProxy(0);
      }
    }

  if (strcmp(this->LastAcceptedQueryMethod, label) != 0)
    {
    // query method changed. We must uncheck lock.
    if (this->LockTemporalCacheCheckButton->GetSelectedState())
      {
      this->LockTemporalCacheCheckButtonCallback(0);
      }
    this->SetLastAcceptedQueryMethod(label);
    }
  
  this->GetProxy()->UpdateVTKObjects();

}

//-----------------------------------------------------------------------------
void vtkPVDataAnalysis::PointArraySelectionModifiedCallback()
{
  this->PointArraySelection->Accept();
  
  this->PlotDisplayProxy->UpdateVTKObjects();
  this->GetPVRenderView()->EventuallyRender();

  // Since changing arrays leads to changing of YTitle.
  this->UpdatePlotDisplayGUI();
}

//-----------------------------------------------------------------------------
void vtkPVDataAnalysis::CellArraySelectionModifiedCallback()
{
  this->CellArraySelection->Accept();
  this->PlotDisplayProxy->UpdateVTKObjects();
  this->GetPVRenderView()->EventuallyRender();

  // Since changing arrays leads to changing of YTitle.
  this->UpdatePlotDisplayGUI();
}
//-----------------------------------------------------------------------------
void vtkPVDataAnalysis::SetPlotDisplayVisibility(int state)
{
  this->GetTraceHelper()->AddEntry("$kw(%s) SetPlotDisplayVisibility %d",
    this->GetTclName(), state);
  this->SetPlotDisplayVisibilityInternal(state);
}

//-----------------------------------------------------------------------------
void vtkPVDataAnalysis::SetPlotDisplayVisibilityInternal(int state)
{
  this->ShowXYPlotCheckButton->SetSelectedState(state);
  if (this->PlotDisplayProxy->GetVisibilityCM() != state)
    {
    this->PlotDisplayProxy->SetVisibilityCM(state);
    }
}

//-----------------------------------------------------------------------------
void vtkPVDataAnalysis::PlotOverTimeInternal(int state)
{
  int plot_display_visibility = this->ShowXYPlotCheckButton->GetSelectedState();
  if (!state && !this->PlottingPointData && plot_display_visibility)
    {
    // Cannot plot anything when temporal plotting is disabled 
    // and plotting cell data.
    this->PlotDisplayProxy->SetVisibilityCM(0);
    }
  else if (!this->ShowXYPlotCheckButton->GetEnabled())
    {
    // this is done only if the visibity check button was disabled when the
    // user changed the state of the temporal check button.
    this->SetPlotDisplayVisibilityInternal(plot_display_visibility);
    }

  vtkSMIntVectorProperty* ivp = vtkSMIntVectorProperty::SafeDownCast(
    this->PlotDisplayProxy->GetProperty("UseCache"));
  if (!ivp)
    {
    vtkErrorMacro("Failed to located property UseCache.");
    return;
    }
  ivp->SetElement(0, state);
  
  // Update the X Axis label, if the user hasn't changed it.
  vtkSMStringVectorProperty* svp = vtkSMStringVectorProperty::SafeDownCast(
    this->PlotDisplayProxy->GetProperty("XTitle"));
  if (svp->GetElement(0) && state 
    && strcmp(svp->GetElement(0), "Samples") == 0)
    {
    this->SetXAxisLabel("Time");
    }
  else if (svp->GetElement(0) && !state 
    && strcmp(svp->GetElement(0), "Time") == 0)
    {
    this->SetXAxisLabel("Samples");
    }
  
  this->PlotDisplayProxy->UpdateVTKObjects();
  
  this->UpdateEnableState();
  this->GetPVRenderView()->EventuallyRender();

  if (state && this->DataInformationFrame->IsPacked())
    {
    this->Script("pack forget %s", 
      this->DataInformationFrame->GetWidgetName());
    }
}

//-----------------------------------------------------------------------------
int vtkPVDataAnalysis::GetPlotOverTime()
{
  return this->PlotOverTimeCheckButton->GetSelectedState();
}

//-----------------------------------------------------------------------------
void vtkPVDataAnalysis::PlotOverTimeCheckButtonCallback(int state)
{
  this->GetTraceHelper()->AddEntry(
    "$kw(%s) PlotOverTimeCheckButtonCallback %d", this->GetTclName(), state);
  this->PlotOverTimeInternal(state);

}

//-----------------------------------------------------------------------------
void vtkPVDataAnalysis::LockTemporalCacheCheckButtonCallback(int state)
{
  this->GetTraceHelper()->AddEntry(
    "$kw(%s) LockTemporalCacheCheckButtonCallback %d",
    this->GetTclName(), state);
  this->LockTemporalCacheCheckButton->SetSelectedState(state);
  vtkSMIntVectorProperty* ivp = vtkSMIntVectorProperty::SafeDownCast(
    this->PlotDisplayProxy->GetProperty("LockTemporalCache"));
  if (!ivp)
    {
    vtkErrorMacro("Failed to locate property LockTemporalCache.");
    return;
    }
  ivp->SetElement(0, state);
  this->PlotDisplayProxy->UpdateVTKObjects();
}

//-----------------------------------------------------------------------------
vtkPVReaderModule* vtkPVDataAnalysis::LocateUpstreamSourceWithTimeSupport()
{
  // For now, only vtkPVReaderModule can support time,
  // an vtkPVReaderModule cannot have anything upstream. So,
  // we just go upstream to till we read a vtkPVReaderModule.
  vtkPVSource* pvs = this->GetPVInput(0);
  while (pvs && !vtkPVReaderModule::SafeDownCast(pvs) 
    && pvs->GetNumberOfPVInputs() > 0)
    {
    pvs = pvs->GetPVInput(0);
    }
  return vtkPVReaderModule::SafeDownCast(pvs);
}

//-----------------------------------------------------------------------------
void vtkPVDataAnalysis::AcceptCallbackInternal()
{
  int initialized = this->GetInitialized();

  this->Superclass::AcceptCallbackInternal();

  int show_plot = this->ShowXYPlotCheckButton->GetSelectedState();
  int override_show_plot = 0; 

  if (!this->PlotDisplayProxy)
    {
    return;
    }

  this->PlotDisplayProxy->UpdatePropertyInformation();
  if (!initialized)
    {
    this->PointArraySelection->SetSMProperty(
      this->PlotDisplayProxy->GetProperty("PointArrayNames"));
    this->PointArraySelection->SetColorProperty(
      vtkSMDoubleVectorProperty::SafeDownCast(
        this->PlotDisplayProxy->GetProperty("PlotColors")));
    this->PointArraySelection->Create();

    this->CellArraySelection->SetSMProperty(
      this->PlotDisplayProxy->GetProperty("CellArrayNames"));
    this->CellArraySelection->SetColorProperty(
      vtkSMDoubleVectorProperty::SafeDownCast(
        this->PlotDisplayProxy->GetProperty("PlotColors")));
    this->CellArraySelection->Create(); 
    }

  // Make sure that the user is shown the correct array selection widget
  // depending upon the Query Method he choose.
  vtkSMIntVectorProperty* plotPointDataProperty = vtkSMIntVectorProperty::SafeDownCast(
    this->PlotDisplayProxy->GetProperty("PlotPointData"));

  if (!this->PlottingPointData)
    {
    // We are plotting cell data.
    // Ensure that right array selection widget is shown.
    if (this->PointArraySelection->IsPacked())
      {
      this->Script("pack forget %s", this->PointArraySelection->GetWidgetName()); 
      }
    if (!this->CellArraySelection->IsPacked())
      {
      this->Script("pack %s -fill x -expand true", 
        this->CellArraySelection->GetWidgetName());
      plotPointDataProperty->SetElement(0, 0);
      }
    // When plotting cell data, plot can be shown only if,
    // plot over time is enabled.
    if (show_plot && !this->PlotOverTimeCheckButton->GetSelectedState())
      {
      override_show_plot = 1;
      this->PlotDisplayProxy->SetVisibilityCM(0);
      }
    }
  else
    {
    // We are plotting point data.
    if (this->CellArraySelection->IsPacked())
      {
      this->Script("pack forget %s", this->CellArraySelection->GetWidgetName());
      }
    if (!this->PointArraySelection->IsPacked())
      {
      this->Script("pack %s -fill x -expand true",
        this->PointArraySelection->GetWidgetName());
      plotPointDataProperty->SetElement(0, 1);
      }
    }

  if (show_plot != this->PlotDisplayProxy->GetVisibilityCM() && !override_show_plot)
    {
    this->SetPlotDisplayVisibility(show_plot);
    }

  // Accept the current array selections.
  if (this->PlottingPointData)
    {
    // It is possible that the data arrays changed. 
    // If they have changed, then we need to update the array selection 
    // widgets.
    this->CheckAndUpdateArraySelections(this->PointArraySelection);
    this->PointArraySelection->Accept();
    }
  else
    {
    // It is possible that the data arrays changed. 
    // If they have changed, then we need to update the array selection 
    // widgets.
    this->CheckAndUpdateArraySelections(this->CellArraySelection);
    this->CellArraySelection->Accept();
    }

  this->PlotDisplayProxy->UpdateVTKObjects();

  if (!initialized)
    {
    // The display properies are changed only for first accept,
    // after which, we should not touch the display properties,
    // since the user should be free to change them.
    this->Notebook->GetDisplayGUI()->DrawWireframe();
    this->Notebook->GetDisplayGUI()->ColorByProperty();
    this->Notebook->GetDisplayGUI()->ChangeActorColor(0.8, 0.0, 0.2);
    this->Notebook->GetDisplayGUI()->SetLineWidth(2);

    this->SetXAxisLabel("Samples");

    // Pack the plot display properties frame.
    this->Script("pack %s -fill x -expand true",
      this->PlotDisplayPropertiesFrame->GetWidgetName());

    // MarkSourcesForUpdate is called before diplays are created, which is 
    // too early to init temporal support. Hence we do the init here.
    this->InitializeTemporalSupport();
    }

  if (this->LastAcceptedQueryMethod && 
    strcmp(this->LastAcceptedQueryMethod, "Line")==0)
    {
    // When probing line, one cannot plot over time.
    this->PlotOverTimeInternal(0);
    }
  if (!this->PlottingPointData)
    {
    // Show data labels when cell data is picked.
    this->SetPointLabelVisibilityNoTrace(1);
    }
    
  this->UpdateDataInformationList();
  this->UpdatePlotDisplayGUI();
}

//-----------------------------------------------------------------------------
void vtkPVDataAnalysis::UpdateDataInformationList()
{
  this->PointLabelDisplayProxy->Update(); 
  vtkDataSet* data = this->PointLabelDisplayProxy->GetCollectedData();
  vtkKWMultiColumnList* list = this->DataInformationList->GetWidget();

  list->DeleteAllRows();

  if (this->LastAcceptedQueryMethod && 
    strcmp(this->LastAcceptedQueryMethod, "Line")==0 || 
    (this->TimeSupportAvailable 
     && this->PlotOverTimeCheckButton->GetSelectedState()))
    {
    // Don't show information if probing line, or when temporal is enabled.
    this->Script("pack forget %s", this->DataInformationFrame->GetWidgetName());
    return;
    }
  else if (!this->DataInformationFrame->IsPacked())
    {
    this->Script("pack %s -fill x -expand true", 
      this->DataInformationFrame->GetWidgetName());
    }

  int cc;
  for (cc=0; cc < data->GetNumberOfPoints(); cc++)
    {
    this->AppendData(1, cc, data->GetPointData());
    }
  for (cc=0; cc < data->GetNumberOfCells(); cc++)
    {
    this->AppendData(0, cc, data->GetCellData());
    }
  list->SetHeight(list->GetNumberOfRows());
}

//-----------------------------------------------------------------------------
template <class T>
void vtkPVDataAnalysisPrintTuple(ostream& os, T *tuple, int num_of_components)
{
  for (int cc=0; cc < num_of_components; cc++)
    {
    if (cc > 0)
      {
      os << ", ";
      } 
    os << tuple[cc];
    }
}
//-----------------------------------------------------------------------------
void vtkPVDataAnalysis::AppendData(int point_data, int index, vtkFieldData* fieldData)
{
  vtkKWMultiColumnList* list = this->DataInformationList->GetWidget();
  int numRows = list->GetNumberOfRows();
  int numArrays = fieldData->GetNumberOfArrays();
  
  vtksys_ios::ostringstream name;
  name << (point_data? "Point " : "Cell ") << index;

  for (int i = 0; i < numArrays; i++)
    {
    vtkDataArray* array = fieldData->GetArray(i);
    if (!array || !array->GetName())
      {
      continue;
      }
    int num_of_components = array->GetNumberOfComponents();

    vtksys_ios::ostringstream stream;
    switch (array->GetDataType())
      {
      vtkExtendedTemplateMacro(::vtkPVDataAnalysisPrintTuple(stream,
          static_cast<VTK_TT*>(array->GetVoidPointer(index*num_of_components)),
          num_of_components));
      
    default:
      vtkErrorMacro("Unsupported data type: " << array->GetDataType());
      continue;
      }
   
    vtksys_ios::ostringstream data_type;
    data_type << num_of_components << " - " << array->GetDataTypeAsString();
    list->InsertCellText(numRows, 0, name.str().c_str());
    list->InsertCellText(numRows, 1, array->GetName());
    list->InsertCellText(numRows, 2, (point_data? "point" : "cell"));
    list->InsertCellText(numRows, 3, data_type.str().c_str());
    list->InsertCellText(numRows, 4, stream.str().c_str());
    numRows++;
    }
}

//-----------------------------------------------------------------------------
void vtkPVDataAnalysis::MarkSourcesForUpdate()
{
  this->Superclass::MarkSourcesForUpdate();

  if (this->PlotDisplayProxy)
    {
    this->InitializeTemporalSupport();
    }
}

//-----------------------------------------------------------------------------
void vtkPVDataAnalysis::InitializeTemporalSupport()
{
  // Set up stuff for creating the animation for time support.
  vtkPVReaderModule* pvRM = this->LocateUpstreamSourceWithTimeSupport();
  int num_of_timesteps = (pvRM)? pvRM->GetNumberOfTimeSteps() : 0;
  if (!pvRM || num_of_timesteps <= 1)
    {
    // cannot plot over time.
    this->TimeSupportAvailable = 0;
    }
  else
    {
    this->TimeSupportAvailable = 1;
    }

  if (!this->TimeSupportAvailable)
    {
    this->PlotOverTimeCheckButton->SetSelectedState(0);
    if (this->AnimationCueProxy)
      {
      // release the Source proxy if set previously.
      vtkSMProxyProperty* pp = vtkSMProxyProperty::SafeDownCast(
        this->AnimationCueProxy->GetProperty("AnimatedProxy"));
      pp->RemoveAllProxies();
      pp->AddProxy(0);
      this->AnimationCueProxy->UpdateVTKObjects();
      }
    if (this->TemporalParametersFrame->IsPacked())
      {
      this->Script("pack forget %s", 
        this->TemporalParametersFrame->GetWidgetName());
      }
    return;
    }

  this->Script("pack %s -fill x -expand true",
    this->TemporalParametersFrame->GetWidgetName());
  this->SourceNameLabel->SetText(pvRM->GetLabel());
  this->Range->SetWholeRange(0, num_of_timesteps - 1);
  this->Range->SetRange(0, num_of_timesteps - 1);
  vtkSMProperty* timeValueProperty = 
    pvRM->GetTimeStepWidget()->GetSMProperty();
  if (vtkSMDoubleVectorProperty::SafeDownCast(timeValueProperty))
    {
    this->Range->SetResolution(0.1);
    }
  else
    {
    this->Range->SetResolution(1);
    }

  vtkSMDoubleVectorProperty* dvp;
  vtkSMProxyProperty* pp;
  vtkSMStringVectorProperty* svp;
  
  if (!this->AnimationCueProxy)
    {
    vtkSMProxyManager* pxm = vtkSMObject::GetProxyManager();
    this->AnimationCueProxy = pxm->NewProxy("animation", "AnimationCue");
    vtksys_ios::ostringstream name_str;
    name_str << this->GetSourceList() << "." << this->GetName()
      << ".AnimationCue";
    pxm->RegisterProxy("animation", name_str.str().c_str(),
      this->AnimationCueProxy);

    this->AnimationManipulatorProxy = pxm->NewProxy("animation_manipulators",
      "LinearAnimationCueManipulator");
    name_str.clear();
    name_str << this->GetSourceList() << "." << this->GetName()
      << ".AnimationManipulatorProxy";
    pxm->RegisterProxy("animation_manipulators",
      name_str.str().c_str(), this->AnimationManipulatorProxy);

    pp = vtkSMProxyProperty::SafeDownCast(
      this->AnimationCueProxy->GetProperty("Manipulator"));
    pp->RemoveAllProxies();
    pp->AddProxy(this->AnimationManipulatorProxy);
    }

  dvp = vtkSMDoubleVectorProperty::SafeDownCast(
    this->AnimationCueProxy->GetProperty("EndTime"));
  dvp->SetElement(0, num_of_timesteps - 1);

  pp = vtkSMProxyProperty::SafeDownCast(
    this->AnimationCueProxy->GetProperty("AnimatedProxy"));
  pp->RemoveAllProxies();
  pp->AddProxy(pvRM->GetProxy());

  svp = vtkSMStringVectorProperty::SafeDownCast(
    this->AnimationCueProxy->GetProperty("AnimatedPropertyName"));
  svp->SetElement(0, pvRM->GetTimeStepWidget()->GetSMPropertyName());

  dvp = vtkSMDoubleVectorProperty::SafeDownCast(
    this->AnimationManipulatorProxy->GetProperty("StartValue"));
  dvp->SetElement(0, 0);
  
  dvp = vtkSMDoubleVectorProperty::SafeDownCast(
    this->AnimationManipulatorProxy->GetProperty("EndValue"));
  dvp->SetElement(0, num_of_timesteps-1);

  this->AnimationManipulatorProxy->UpdateVTKObjects(); 
  this->AnimationCueProxy->UpdateVTKObjects();

  pp = vtkSMProxyProperty::SafeDownCast(
    this->PlotDisplayProxy->GetProperty("AnimationCue"));
  pp->RemoveAllProxies();
  pp->AddProxy(this->AnimationCueProxy);

  this->PlotDisplayProxy->UpdateVTKObjects();
}

//-----------------------------------------------------------------------------
void vtkPVDataAnalysis::SetupDisplays()
{
  this->Superclass::SetupDisplays();
  
  // Create the plot display.
  if (this->PlotDisplayProxy)
    {
    return;
    }

  vtkSMProxyManager* pxm = vtkSMObject::GetProxyManager();
  vtkSMProxy* p = pxm->NewProxy("displays", "TemporalXYPlotDisplay");
  if (!p)
    {
    vtkErrorMacro("Failed to create Plot display proxy.");
    return;
    }
  
  this->PlotDisplayProxy = vtkSMTemporalXYPlotDisplayProxy::SafeDownCast(p);
  if (!this->PlotDisplayProxy)
    {
    vtkErrorMacro("Plot display proxy is not of correct type!");
    p->Delete();
    return;
    }
  
  vtksys_ios::ostringstream name_str;
  name_str << this->GetSourceList() << "." << this->GetName() 
    << ".TemporalXYPlotDisplay";
  pxm->RegisterProxy("displays", name_str.str().c_str(), 
    this->PlotDisplayProxy);
  this->SetPlotDisplayProxyName(name_str.str().c_str());
  
  vtkSMInputProperty* ip = vtkSMInputProperty::SafeDownCast(
    this->PlotDisplayProxy->GetProperty("Input"));
  if (!ip)
    {
    vtkErrorMacro("Failed to find property Input on the PlotDisplayProxy.");
    }

  ip->RemoveAllProxies();
  ip->AddProxy(this->GetProxy());
  this->SetPlotDisplayVisibilityInternal(0);
  this->AddDisplayToRenderModule(this->PlotDisplayProxy);

  this->PlotDisplayProxy->AddObserver(vtkCommand::AnimationCueTickEvent,
    this->Observer);

  // This property determines if the ServerManager should automatically decide 
  // the colors used for the plot curves. In this case, NO!
  vtkSMIntVectorProperty* ivp = vtkSMIntVectorProperty::SafeDownCast(
    this->PlotDisplayProxy->GetProperty("ComputeColors"));
  if (ivp)
    {
    ivp->SetElement(0, 0);
    }
  else
    {
    vtkErrorMacro("Failed to locate property ComputeColors.");
    }
  this->PlotDisplayProxy->UpdateVTKObjects();
}

//-----------------------------------------------------------------------------
void vtkPVDataAnalysis::CleanupDisplays()
{
  this->Superclass::CleanupDisplays();

  // Clean the plot display.
  if (!this->PlotDisplayProxy)
    {
    return;
    }
  this->RemoveDisplayFromRenderModule(this->PlotDisplayProxy);
  
  vtkSMProxyManager* pxm = vtkSMProxyManager::GetProxyManager();
  pxm->UnRegisterProxy("displays", this->PlotDisplayProxyName);

  this->PlotDisplayProxy->RemoveObserver(this->Observer);
  
  this->PlotDisplayProxy->Delete();
  this->PlotDisplayProxy = 0;
  this->SetPlotDisplayProxyName(0);
}

//-----------------------------------------------------------------------------
void vtkPVDataAnalysis::UpdateEnableState()
{
  if (!this->GetPlotOverTime() && !this->PlottingPointData)
    {
    this->ShowXYPlotCheckButton->SetEnabled(0);
    }
  else
    {
    this->PropagateEnableState(this->ShowXYPlotCheckButton);
    }

  if (this->TimeSupportAvailable && 
    (!this->LastAcceptedQueryMethod 
     || strcmp(this->LastAcceptedQueryMethod, "Line") != 0))
    {
    this->PropagateEnableState(this->PlotOverTimeCheckButton);
    }
  else
    {
    this->PlotOverTimeCheckButton->SetEnabled(0);
    }

  if (this->TimeSupportAvailable && 
    this->PlotOverTimeCheckButton->GetSelectedState())
    {
    this->PropagateEnableState(this->Range);
    this->PropagateEnableState(this->GenerateButton);
    this->PropagateEnableState(this->LockTemporalCacheCheckButton);
    }
  else
    {
    this->Range->SetEnabled(0);
    this->GenerateButton->SetEnabled(0);
    this->LockTemporalCacheCheckButton->SetEnabled(0);
    }

  this->PropagateEnableState(this->AdjustTitlePositionCheckButton);
  if (this->AdjustTitlePositionCheckButton->GetSelectedState())
    {
    this->PlotTitleXPositionWidget->SetEnabled(0);
    this->PlotTitleYPositionWidget->SetEnabled(0);
    }
  else
    {
    this->PropagateEnableState(this->PlotTitleXPositionWidget);
    this->PropagateEnableState(this->PlotTitleYPositionWidget);
    }

  this->PropagateEnableState(this->SaveCSVButton);
  this->PropagateEnableState(this->PointArraySelection);
  this->PropagateEnableState(this->CellArraySelection);

  this->Superclass::UpdateEnableState();
}

//-----------------------------------------------------------------------------
void vtkPVDataAnalysis::GenerateTemporalPlot()
{
  this->GetTraceHelper()->AddEntry("$kw(%s) GenerateTemporalPlot",
    this->GetTclName());

  double range[2];
  this->Range->GetRange(range);

  vtkSMDoubleVectorProperty* dvp;
  dvp = vtkSMDoubleVectorProperty::SafeDownCast(
    this->AnimationManipulatorProxy->GetProperty("StartValue"));
  dvp->SetElement(0, range[0]);

  dvp = vtkSMDoubleVectorProperty::SafeDownCast(
    this->AnimationManipulatorProxy->GetProperty("EndValue"));
  dvp->SetElement(0, range[1]);

  dvp = vtkSMDoubleVectorProperty::SafeDownCast(
    this->AnimationCueProxy->GetProperty("StartTime"));
  dvp->SetElement(0, range[0]);
  dvp = vtkSMDoubleVectorProperty::SafeDownCast(
    this->AnimationCueProxy->GetProperty("EndTime"));
  dvp->SetElement(0, range[1]);
  
  this->AnimationManipulatorProxy->UpdateVTKObjects();
  this->AnimationCueProxy->UpdateVTKObjects();
  
  this->GenerateButton->SetText("Abort");
  this->GenerateButton->SetCommand(this,
    "AbortGenerateTemporalPlot");
  
  this->PlotDisplayProxy->GenerateTemporalPlot();

  this->GenerateButton->SetText("Generate");
  this->GenerateButton->SetCommand(this,
    "GenerateTemporalPlot");
  this->GetPVRenderView()->EventuallyRender();

}

//-----------------------------------------------------------------------------
void vtkPVDataAnalysis::AbortGenerateTemporalPlot()
{
  this->PlotDisplayProxy->AbortGenerateTemporalPlot();
}

//-----------------------------------------------------------------------------
void vtkPVDataAnalysis::SaveDialogCallback()
{
  const char *filename = this->SaveCSVButton->GetFileName();
  if (filename && this->PlotDisplayProxy)
    {
    this->PlotDisplayProxy->PrintAsCSV(filename);
    }
}

//-----------------------------------------------------------------------------
void vtkPVDataAnalysis::SetPlotTitle(const char* str)
{
  this->GetTraceHelper()->AddEntry("$kw(%s) SetPlotTitle {%s}", 
    this->GetTclName(), str);
  if (!this->PlotDisplayProxy)
    {
    vtkErrorMacro("SetPlotTitle can only be called after the first Accept.");
    return;
    }
  vtkSMStringVectorProperty* svp = vtkSMStringVectorProperty::SafeDownCast(
    this->PlotDisplayProxy->GetProperty("Title"));
  if (svp)
    {
    svp->SetElement(0, str);
    this->PlotDisplayProxy->UpdateVTKObjects();
    this->GetPVRenderView()->EventuallyRender();
    }
  else
    {
    vtkErrorMacro("Failed to locate property Title");
    }
  this->PlotTitleEntry->SetValue(str);
}

//-----------------------------------------------------------------------------
void vtkPVDataAnalysis::SetPlotTitlePositionCallback(const char*)
{
  this->SetPlotTitlePosition(
    this->PlotTitleXPositionWidget->GetWidget()->GetValueAsDouble(), 
    this->PlotTitleYPositionWidget->GetWidget()->GetValueAsDouble());
}

//-----------------------------------------------------------------------------
void vtkPVDataAnalysis::SetPlotTitlePosition(double x, double y)
{
  this->GetTraceHelper()->AddEntry("$kw(%s) SetPlotTitlePosition %f %f",
    this->GetTclName(), x, y);
  if (!this->PlotDisplayProxy)
    {
    vtkErrorMacro("SetPlotTitlePosition can only be called after "
      "the first Accept.");
    return;
    }
  vtkSMDoubleVectorProperty* dvp = vtkSMDoubleVectorProperty::SafeDownCast(
    this->PlotDisplayProxy->GetProperty("TitlePosition"));
  if (dvp)
    {
    dvp->SetElement(0, x);
    dvp->SetElement(1, y);
    this->PlotDisplayProxy->UpdateVTKObjects();
    this->GetPVRenderView()->EventuallyRender();
    }
  else
    {
    vtkErrorMacro("Failed to locate property TitlePosition.");
    }
  this->PlotTitleXPositionWidget->GetWidget()->SetValueAsDouble(x);
  this->PlotTitleYPositionWidget->GetWidget()->SetValueAsDouble(y);
}

//-----------------------------------------------------------------------------
void vtkPVDataAnalysis::SetAdjustTitlePosition(int state)
{
  this->GetTraceHelper()->AddEntry("$kw(%s) SetAdjustTitlePosition %d",
    this->GetTclName(), state);
  if (!this->PlotDisplayProxy)
    {
    vtkErrorMacro("SetAdjustTitlePosition can only be called after "
      "the first Accept.");
    return;
    }
  vtkSMIntVectorProperty* ivp = vtkSMIntVectorProperty::SafeDownCast(
    this->PlotDisplayProxy->GetProperty("AdjustTitlePosition"));
  if (ivp)
    {
    ivp->SetElement(0, state);
    this->PlotDisplayProxy->UpdateVTKObjects();
    this->GetPVRenderView()->EventuallyRender();
    }
  else
    {
    vtkErrorMacro("Failed to locate property AdjustTitlePosition.");
    }
    
  this->AdjustTitlePositionCheckButton->SetSelectedState(state);
  this->UpdateEnableState();
}

//-----------------------------------------------------------------------------
void vtkPVDataAnalysis::SetXAxisLabel(const char* str)
{
  this->GetTraceHelper()->AddEntry("$kw(%s) SetXAxisLabel {%s}", 
    this->GetTclName(), str);

  if (!this->PlotDisplayProxy)
    {
    vtkErrorMacro("SetXAxisLabel can only be called after the first Accept.");
    return;
    }

  vtkSMStringVectorProperty* svp = vtkSMStringVectorProperty::SafeDownCast(
    this->PlotDisplayProxy->GetProperty("XTitle"));
  if (svp)
    {
    svp->SetElement(0, str);
    this->PlotDisplayProxy->UpdateVTKObjects();
    this->GetPVRenderView()->EventuallyRender();
    }
  else
    {
    vtkErrorMacro("Failed to locate property XTitle");
    }
  this->XLabelEntry->SetValue(str);

}

//-----------------------------------------------------------------------------
void vtkPVDataAnalysis::SetYAxisLabel(const char* str)
{
  this->GetTraceHelper()->AddEntry("$kw(%s) SetYAxisLabel {%s}", 
    this->GetTclName(), str);

  if (!this->PlotDisplayProxy)
    {
    vtkErrorMacro("SetYAxisLabel can only be called after the first Accept.");
    return;
    }

  vtkSMStringVectorProperty* svp = vtkSMStringVectorProperty::SafeDownCast(
    this->PlotDisplayProxy->GetProperty("YTitle"));
  if (svp)
    {
    svp->SetElement(0, str);
    this->PlotDisplayProxy->UpdateVTKObjects();
    this->GetPVRenderView()->EventuallyRender();
    }
  else
    {
    vtkErrorMacro("Failed to locate property YTitle");
    }
  this->YLabelEntry->SetValue(str);
}

//-----------------------------------------------------------------------------
void vtkPVDataAnalysis::SetLegendPosition(double x, double y)
{
  this->GetTraceHelper()->AddEntry("$kw(%s) SetLegendPosition %f %f",
    this->GetTclName(), x, y);
  if (!this->PlotDisplayProxy)
    {
    vtkErrorMacro("SetLegendPosition can only be called after the first Accept.");
    return;
    }
  vtkSMDoubleVectorProperty* dvp = vtkSMDoubleVectorProperty::SafeDownCast(
    this->PlotDisplayProxy->GetProperty("LegendPosition"));
  if (dvp)
    {
    dvp->SetElement(0, x);
    dvp->SetElement(1, y);
    this->PlotDisplayProxy->UpdateVTKObjects();
    this->GetPVRenderView()->EventuallyRender();
    }
  else
    {
    vtkErrorMacro("Failed to locate property LegendPosition");
    }

  this->LegendXPositionWidget->GetWidget()->SetValueAsDouble(x);
  this->LegendYPositionWidget->GetWidget()->SetValueAsDouble(y);
}

//-----------------------------------------------------------------------------
void vtkPVDataAnalysis::LegendPositionCallback(const char*)
{
  double pos[2];
  pos[0] = this->LegendXPositionWidget->GetWidget()->GetValueAsDouble();
  pos[1] = this->LegendYPositionWidget->GetWidget()->GetValueAsDouble();
  this->SetLegendPosition(pos);
}

//-----------------------------------------------------------------------------
void vtkPVDataAnalysis::SetLegendVisibility(int visible)
{
  this->GetTraceHelper()->AddEntry("$kw(%s) SetLegendVisibility %d",
    this->GetTclName(), visible);
  
  if (!this->PlotDisplayProxy)
    {
    vtkErrorMacro("SetLegendVisibility can only be called after the first Accept.");
    return;
    }

  vtkSMIntVectorProperty* ivp = vtkSMIntVectorProperty::SafeDownCast(
    this->PlotDisplayProxy->GetProperty("LegendVisibility"));
  if (ivp)
    {
    ivp->SetElement(0, visible);
    this->PlotDisplayProxy->UpdateVTKObjects();
    this->GetPVRenderView()->EventuallyRender();
    }
  else
    {
    vtkErrorMacro("Failed to locate property LegendVisibility.");
    }
  this->ShowLegendCheckButton->SetSelectedState(visible);
}

//-----------------------------------------------------------------------------
void vtkPVDataAnalysis::UpdatePlotDisplayGUI()
{
  vtkSMStringVectorProperty* svp;
  vtkSMIntVectorProperty* ivp, *ivp2;
  vtkSMDoubleVectorProperty* dvp;

  this->PlotDisplayProxy->UpdatePropertyInformation();
 
  svp = vtkSMStringVectorProperty::SafeDownCast(
    this->PlotDisplayProxy->GetProperty("Title"));
  this->PlotTitleEntry->SetValue(svp->GetElement(0));

  dvp = vtkSMDoubleVectorProperty::SafeDownCast(
    this->PlotDisplayProxy->GetProperty("TitlePosition"));
  this->PlotTitleXPositionWidget->GetWidget()->SetValueAsDouble(
    dvp->GetElement(0));
  this->PlotTitleYPositionWidget->GetWidget()->SetValueAsDouble(
    dvp->GetElement(1));

  ivp = vtkSMIntVectorProperty::SafeDownCast(
    this->PlotDisplayProxy->GetProperty("AdjustTitlePosition"));
  this->AdjustTitlePositionCheckButton->SetSelectedState(
    ivp->GetElement(0));


  svp = vtkSMStringVectorProperty::SafeDownCast(
    this->PlotDisplayProxy->GetProperty("XTitleInfo"));
  this->XLabelEntry->SetValue(svp->GetElement(0));

  svp = vtkSMStringVectorProperty::SafeDownCast(
    this->PlotDisplayProxy->GetProperty("YTitleInfo"));
  this->YLabelEntry->SetValue(svp->GetElement(0));


  dvp = vtkSMDoubleVectorProperty::SafeDownCast(
    this->PlotDisplayProxy->GetProperty("LegendPosition"));
  this->LegendXPositionWidget->GetWidget()->SetValueAsDouble(
    dvp->GetElement(0));
  this->LegendYPositionWidget->GetWidget()->SetValueAsDouble(
    dvp->GetElement(1));
  
  ivp = vtkSMIntVectorProperty::SafeDownCast(
    this->PlotDisplayProxy->GetProperty("LegendVisibility"));
  this->ShowLegendCheckButton->SetSelectedState(ivp->GetElement(0));
  
  ivp = vtkSMIntVectorProperty::SafeDownCast(
    this->PlotDisplayProxy->GetProperty("PlotLines"));
  ivp2 = vtkSMIntVectorProperty::SafeDownCast(
    this->PlotDisplayProxy->GetProperty("PlotPoints"));
  if (ivp->GetElement(0) && ivp2->GetElement(0))
    {
    this->PlotTypeMenuButton->SetValue("Points & Lines");
    }
  else if (ivp->GetElement(0))
    {
    this->PlotTypeMenuButton->SetValue("Lines");
    }
  else
    {
    this->PlotTypeMenuButton->SetValue("Points");
    }
  this->UpdateEnableState();
}

//-----------------------------------------------------------------------------
void vtkPVDataAnalysis::CheckAndUpdateArraySelections(
  vtkPVPlotArraySelection* widget)
{
  vtkSMProperty* p = widget->GetSMProperty();
  vtkSMStringListDomain* sld = (p)?
    vtkSMStringListDomain::SafeDownCast(p->GetDomain("array_list")) : 0;
  
  if (!sld)
    {
    vtkErrorMacro("Failed to locate domain with array information.");
    return;
    }

  vtkDataArraySelection* selection = widget->GetSelection();
  int update = 0;
  if (!selection)
    {
    update = 1;
    }
  
  // Check is arrays changed.
  if (!update && selection->GetNumberOfArrays() 
    != static_cast<int>(sld->GetNumberOfStrings()))
    {
    update = 1;
    }
  
  if (!update)
    {
    // Check is the array names changed.
    for (unsigned int i=0; i < sld->GetNumberOfStrings(); i++)
      {
      if (!selection->ArrayExists(sld->GetString(i)))
        {
        update = 1;
        break;
        }
      }
    }

  if (update)
    {
    // changed! dump the old arrays, get new ones.
    selection->RemoveAllArrays();
    widget->Reset();
    }
  
}

//-----------------------------------------------------------------------------
void vtkPVDataAnalysis::SetVisibilityNoTrace(int val)
{
  if (this->PlotDisplayProxy && 
    this->ShowXYPlotCheckButton->GetSelectedState())
    {
    this->PlotDisplayProxy->SetVisibilityCM(val);
    }
  this->Superclass::SetVisibilityNoTrace(val);
}

//-----------------------------------------------------------------------------
void vtkPVDataAnalysis::EditXLabelCallback(int popup_dialog)
{
  this->GetTraceHelper()->AddEntry("$kw(%s) EditXLabelCallback 0",
    this->GetTclName());
  if (!this->LabelPropertiesDialog)
    {
    this->LabelPropertiesDialog = vtkPVPlotDisplayLabelPropertiesDialog::New();
    this->LabelPropertiesDialog->SetMasterWindow(
      this->GetPVApplication()->GetMainWindow());
    this->LabelPropertiesDialog->Create();
    this->LabelPropertiesDialog->GetTraceHelper()->SetReferenceHelper(
      this->GetTraceHelper());
    this->LabelPropertiesDialog->GetTraceHelper()->SetReferenceCommand(
      "GetLabelPropertiesDialog");
    }

  this->LabelPropertiesDialog->SetTitle("X Axes Label Properties Dialog");
  this->LabelPropertiesDialog->SetPositionLabelText("X Axis Title Position ");
  this->LabelPropertiesDialog->SetLabelFormatProperty(
    vtkSMStringVectorProperty::SafeDownCast(
      this->PlotDisplayProxy->GetProperty("XLabelFormat")));
  this->LabelPropertiesDialog->SetNumberOfLabelsProperty(
    vtkSMIntVectorProperty::SafeDownCast(
      this->PlotDisplayProxy->GetProperty("NumberOfXLabels")));
  this->LabelPropertiesDialog->SetAutoAdjustProperty(
    vtkSMIntVectorProperty::SafeDownCast(
      this->PlotDisplayProxy->GetProperty("AdjustXLabels")));
  this->LabelPropertiesDialog->SetNumberOfMinorTicksProperty(
    vtkSMIntVectorProperty::SafeDownCast(
      this->PlotDisplayProxy->GetProperty("NumberOfXMinorTicks")));
  this->LabelPropertiesDialog->SetTitlePositionProperty(
    vtkSMDoubleVectorProperty::SafeDownCast(
      this->PlotDisplayProxy->GetProperty("XTitlePosition")));
  this->LabelPropertiesDialog->SetDataRangeProperty(
    vtkSMDoubleVectorProperty::SafeDownCast(
      this->PlotDisplayProxy->GetProperty("XRange")));

  if (popup_dialog && this->LabelPropertiesDialog->Invoke())
    {
    this->PlotDisplayProxy->UpdateVTKObjects();
    this->GetPVRenderView()->EventuallyRender();
    }

}
//-----------------------------------------------------------------------------
void vtkPVDataAnalysis::EditYLabelCallback(int popup_dialog)
{
  this->GetTraceHelper()->AddEntry("$kw(%s) EditYLabelCallback 0",
    this->GetTclName());
  if (!this->LabelPropertiesDialog)
    {
    this->LabelPropertiesDialog = vtkPVPlotDisplayLabelPropertiesDialog::New();
    this->LabelPropertiesDialog->SetMasterWindow(this);
    this->LabelPropertiesDialog->Create();
    this->LabelPropertiesDialog->GetTraceHelper()->SetReferenceHelper(
      this->GetTraceHelper());
    this->LabelPropertiesDialog->GetTraceHelper()->SetReferenceCommand(
      "GetLabelPropertiesDialog");
    }
  this->LabelPropertiesDialog->SetTitle("Y Axes Label Properties Dialog");
  this->LabelPropertiesDialog->SetPositionLabelText("Y Axis Title Position ");
  this->LabelPropertiesDialog->SetLabelFormatProperty(
    vtkSMStringVectorProperty::SafeDownCast(
      this->PlotDisplayProxy->GetProperty("YLabelFormat")));
  this->LabelPropertiesDialog->SetNumberOfLabelsProperty(
    vtkSMIntVectorProperty::SafeDownCast(
      this->PlotDisplayProxy->GetProperty("NumberOfYLabels")));
  this->LabelPropertiesDialog->SetAutoAdjustProperty(
    vtkSMIntVectorProperty::SafeDownCast(
      this->PlotDisplayProxy->GetProperty("AdjustYLabels")));
  this->LabelPropertiesDialog->SetNumberOfMinorTicksProperty(
    vtkSMIntVectorProperty::SafeDownCast(
      this->PlotDisplayProxy->GetProperty("NumberOfYMinorTicks")));
  this->LabelPropertiesDialog->SetTitlePositionProperty(
    vtkSMDoubleVectorProperty::SafeDownCast(
      this->PlotDisplayProxy->GetProperty("YTitlePosition")));
  this->LabelPropertiesDialog->SetDataRangeProperty(
    vtkSMDoubleVectorProperty::SafeDownCast(
      this->PlotDisplayProxy->GetProperty("YRange")));
  if (popup_dialog && this->LabelPropertiesDialog->Invoke())
    {
    this->PlotDisplayProxy->UpdateVTKObjects();
    this->GetPVRenderView()->EventuallyRender();
    }
}

//-----------------------------------------------------------------------------
void vtkPVDataAnalysis::SetPlotTypeToPoints()
{
  this->GetTraceHelper()->AddEntry("$kw(%s) SetPlotTypeToPoints",
    this->GetTclName());
  this->SetPlotType(0, 1);
  this->PlotTypeMenuButton->SetValue("Points");
}

//-----------------------------------------------------------------------------
void vtkPVDataAnalysis::SetPlotTypeToLines()
{
  this->GetTraceHelper()->AddEntry("$kw(%s) SetPlotTypeToLines",
    this->GetTclName());
  this->SetPlotType(1, 0);
  this->PlotTypeMenuButton->SetValue("Lines");
}

//-----------------------------------------------------------------------------
void vtkPVDataAnalysis::SetPlotTypeToPointsAndLines()
{
  this->GetTraceHelper()->AddEntry("$kw(%s) SetPlotTypeToPointsAndLines",
    this->GetTclName());
  this->SetPlotType(1, 1);
  this->PlotTypeMenuButton->SetValue("Points & Lines");
}

//-----------------------------------------------------------------------------
void vtkPVDataAnalysis::SetPlotType(int plot_lines, int plot_points)
{
  if (!this->PlotDisplayProxy)
    {
    vtkErrorMacro("SetPlotType cannot be called before the first Accept.");
    return;
    }
  vtkSMIntVectorProperty* ivp = vtkSMIntVectorProperty::SafeDownCast(
    this->PlotDisplayProxy->GetProperty("PlotLines"));
  if (ivp)
    {
    ivp->SetElement(0, plot_lines);
    }
  else
    {
    vtkErrorMacro("Failed to locate property PlotLines.");
    }

  ivp = vtkSMIntVectorProperty::SafeDownCast(
    this->PlotDisplayProxy->GetProperty("PlotPoints"));
  if (ivp)
    {
    ivp->SetElement(0, plot_points);
    }
  else
    {
    vtkErrorMacro("Failed to locate property PlotPoints.");
    }
  this->PlotDisplayProxy->UpdateVTKObjects();
  this->GetPVRenderView()->EventuallyRender();
}

//-----------------------------------------------------------------------------
void vtkPVDataAnalysis::SaveState(ofstream* file)
{
  this->Superclass::SaveState(file);

  *file << "# Saving state of the PlotDisplay Proxy associated with the source"
    << endl;

  vtksys_ios::ostringstream name;
  name << "pvDisp(" << this->GetTclName() << ".Plot)";
  *file << "set " << name.str().c_str() 
    << " [$kw(" << this->GetTclName() << ") GetPlotDisplayProxy] " << endl;
  this->SaveStateDisplayInternal(file, name.str().c_str(), this->PlotDisplayProxy); 

  *file << "$kw(" << this->GetTclName() << ") UpdatePlotDisplayGUI" << endl;

  // Save the state of the array selection widgets.
  this->PointArraySelection->SaveState(file);
  this->CellArraySelection->SaveState(file);
}

//-----------------------------------------------------------------------------
void vtkPVDataAnalysis::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent); 
  os << indent << "LabelPropertiesDialog: " << this->LabelPropertiesDialog
    << endl;
  os << indent << "PointArraySelection: " << this->PointArraySelection
    << endl;
  os << indent << "CellArraySelection: " << this->CellArraySelection
    << endl;
  os << indent << "PlotDisplayProxy: " << this->PlotDisplayProxy
    << endl;
}
