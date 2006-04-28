/*=========================================================================

  Program:   ParaView
  Module:    vtkPVPlotDisplayLabelPropertiesDialog.cxx

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkPVPlotDisplayLabelPropertiesDialog.h"

#include "vtkObjectFactory.h"
#include "vtkKWCheckButton.h"
#include "vtkKWEntry.h"
#include "vtkKWEntryWithLabel.h"
#include "vtkKWFrame.h"
#include "vtkKWLabel.h"
#include "vtkKWPushButton.h"
#include "vtkKWScaleWithEntry.h"
#include "vtkPVApplication.h"
#include "vtkPVTraceHelper.h"
#include "vtkSMDoubleVectorProperty.h"
#include "vtkSMIntVectorProperty.h"
#include "vtkSMProxy.h"
#include "vtkSMStringVectorProperty.h"

vtkStandardNewMacro(vtkPVPlotDisplayLabelPropertiesDialog);
vtkCxxRevisionMacro(vtkPVPlotDisplayLabelPropertiesDialog, "1.3");
vtkCxxSetObjectMacro(vtkPVPlotDisplayLabelPropertiesDialog, LabelFormatProperty,
  vtkSMStringVectorProperty);
vtkCxxSetObjectMacro(vtkPVPlotDisplayLabelPropertiesDialog, NumberOfLabelsProperty,
  vtkSMIntVectorProperty);
vtkCxxSetObjectMacro(vtkPVPlotDisplayLabelPropertiesDialog, AutoAdjustProperty,
  vtkSMIntVectorProperty);
vtkCxxSetObjectMacro(vtkPVPlotDisplayLabelPropertiesDialog, NumberOfMinorTicksProperty,
  vtkSMIntVectorProperty);
vtkCxxSetObjectMacro(vtkPVPlotDisplayLabelPropertiesDialog, TitlePositionProperty,
  vtkSMDoubleVectorProperty);
vtkCxxSetObjectMacro(vtkPVPlotDisplayLabelPropertiesDialog, DataRangeProperty,
  vtkSMDoubleVectorProperty);
vtkCxxSetObjectMacro(vtkPVPlotDisplayLabelPropertiesDialog, PlotDisplayProxy,
  vtkSMProxy);
//-----------------------------------------------------------------------------
vtkPVPlotDisplayLabelPropertiesDialog::vtkPVPlotDisplayLabelPropertiesDialog()
{
  this->FrameForGrid = vtkKWFrame::New();
  this->TitlePositionLabel = vtkKWLabel::New();
  this->TitlePositionEntry = vtkKWEntry::New();

  this->DataRangeLabel = vtkKWLabel::New();
  this->DataRangeMinWidget = vtkKWEntryWithLabel::New();
  this->DataRangeMaxWidget = vtkKWEntryWithLabel::New();
  this->DataRangeAutoCheckButton = vtkKWCheckButton::New();
  
  this->LabelFormatLabel = vtkKWLabel::New();
  this->LabelFormatEntry = vtkKWEntry::New();
  this->NumberOfLabelsLabel = vtkKWLabel::New();
  this->NumberOfLabelsScale = vtkKWScaleWithEntry::New();
  this->AutoAdjustCheckButton = vtkKWCheckButton::New();
  this->NumberOfMinorTicksLabel = vtkKWLabel::New();
  this->NumberOfMinorTicksScale = vtkKWScaleWithEntry::New();

  this->OKButton = vtkKWPushButton::New();
  this->CancelButton = vtkKWPushButton::New();

  this->LabelFormatProperty = 0;
  this->NumberOfLabelsProperty = 0;
  this->AutoAdjustProperty = 0;
  this->NumberOfMinorTicksProperty = 0;
  this->TitlePositionProperty = 0;
  this->DataRangeProperty = 0;
  this->PlotDisplayProxy = 0;

  this->TraceHelper = vtkPVTraceHelper::New();
  this->TraceHelper->SetTraceObject(this);

}

//-----------------------------------------------------------------------------
vtkPVPlotDisplayLabelPropertiesDialog::~vtkPVPlotDisplayLabelPropertiesDialog()
{
  this->FrameForGrid->Delete();
  this->TitlePositionLabel->Delete();
  this->TitlePositionEntry->Delete();
  this->DataRangeLabel->Delete();
  this->DataRangeMinWidget->Delete();
  this->DataRangeMaxWidget->Delete();
  this->DataRangeAutoCheckButton->Delete();
  this->LabelFormatLabel->Delete();
  this->LabelFormatEntry->Delete();
  this->NumberOfLabelsLabel->Delete();
  this->NumberOfLabelsScale->Delete();
  this->AutoAdjustCheckButton->Delete();
  this->NumberOfMinorTicksLabel->Delete();
  this->NumberOfMinorTicksScale->Delete();
  this->OKButton->Delete();
  this->CancelButton->Delete();

  this->SetLabelFormatProperty(0);
  this->SetNumberOfLabelsProperty(0);
  this->SetAutoAdjustProperty(0);
  this->SetNumberOfMinorTicksProperty(0);
  this->SetTitlePositionProperty(0);
  this->SetDataRangeProperty(0);
  this->SetPlotDisplayProxy(0);

  this->TraceHelper->Delete();
}

//-----------------------------------------------------------------------------
vtkPVApplication* vtkPVPlotDisplayLabelPropertiesDialog::GetPVApplication()
{
  return vtkPVApplication::SafeDownCast(this->GetApplication());
}

//-----------------------------------------------------------------------------
void vtkPVPlotDisplayLabelPropertiesDialog::SetPositionLabelText(const char* txt)
{
  this->TitlePositionLabel->SetText(txt);
}

//-----------------------------------------------------------------------------
void vtkPVPlotDisplayLabelPropertiesDialog::CreateWidget()
{
  if (this->IsCreated())
    {
    return;
    }
  
  this->Superclass::CreateWidget();

  if (!this->IsCreated())
    {
    return;
    }

  this->FrameForGrid->SetParent(this->GetFrame());
  this->FrameForGrid->Create();
   
  this->Script("pack %s -fill both -expand true",
    this->FrameForGrid->GetWidgetName());

  this->TitlePositionLabel->SetParent(this->FrameForGrid);
  this->TitlePositionLabel->Create();
  this->TitlePositionLabel->SetText("Must be set by the user.");
  this->TitlePositionLabel->SetAnchorToWest();

  this->TitlePositionEntry->SetParent(this->FrameForGrid);
  this->TitlePositionEntry->Create();
  this->TitlePositionEntry->SetCommand(this,
    "SetTitlePositionCallback");

  this->Script("grid %s %s - - -sticky ew",
    this->TitlePositionLabel->GetWidgetName(),
    this->TitlePositionEntry->GetWidgetName());

  this->LabelFormatLabel->SetParent(this->FrameForGrid);
  this->LabelFormatLabel->Create();
  this->LabelFormatLabel->SetText("Label Format ");
  this->LabelFormatLabel->SetAnchorToWest();

  this->LabelFormatEntry->SetParent(this->FrameForGrid);
  this->LabelFormatEntry->Create();
  this->LabelFormatEntry->SetCommand(this,
    "SetLabelFormat");

  this->Script("grid %s %s - - -sticky ew", 
    this->LabelFormatLabel->GetWidgetName(),
    this->LabelFormatEntry->GetWidgetName());
  
  this->DataRangeLabel->SetParent(this->FrameForGrid);
  this->DataRangeLabel->Create();
  this->DataRangeLabel->SetText("Data Range");
  this->DataRangeLabel->SetAnchorToWest();

  this->DataRangeMinWidget->SetParent(this->FrameForGrid);
  this->DataRangeMinWidget->Create();
  this->DataRangeMinWidget->SetLabelPositionToLeft();
  this->DataRangeMinWidget->SetLabelText("Min:");
  this->DataRangeMinWidget->GetWidget()->SetWidth(7);
  this->DataRangeMinWidget->GetWidget()->SetCommand(this,
    "SetDataRangeCallback");
  
  this->DataRangeMaxWidget->SetParent(this->FrameForGrid);
  this->DataRangeMaxWidget->Create();
  this->DataRangeMaxWidget->SetLabelPositionToLeft();
  this->DataRangeMaxWidget->SetLabelText("Max:");
  this->DataRangeMaxWidget->GetWidget()->SetWidth(7);
  this->DataRangeMaxWidget->GetWidget()->SetCommand(this,
    "SetDataRangeCallback");

  this->DataRangeAutoCheckButton->SetParent(this->FrameForGrid);
  this->DataRangeAutoCheckButton->Create();
  this->DataRangeAutoCheckButton->SetAnchorToEast();
  this->DataRangeAutoCheckButton->SetText("Auto");
  this->DataRangeAutoCheckButton->SetCommand(this, "SetDataRangeAuto");
  
  this->Script("grid %s %s %s %s -sticky ew",
    this->DataRangeLabel->GetWidgetName(),
    this->DataRangeMinWidget->GetWidgetName(),
    this->DataRangeMaxWidget->GetWidgetName(),
    this->DataRangeAutoCheckButton->GetWidgetName());

  this->NumberOfLabelsLabel->SetParent(this->FrameForGrid);
  this->NumberOfLabelsLabel->Create();
  this->NumberOfLabelsLabel->SetText("Number of Labels ");
  this->NumberOfLabelsLabel->SetAnchorToWest();
  
  this->NumberOfLabelsScale->SetParent(this->FrameForGrid);
  this->NumberOfLabelsScale->Create();
  this->NumberOfLabelsScale->SetRange(0, 25);
  this->NumberOfLabelsScale->SetResolution(1);
  this->NumberOfLabelsScale->SetEntryWidth(7);
  this->NumberOfLabelsScale->SetCommand(this,
    "SetNumberOfLabels");
    
  this->AutoAdjustCheckButton->SetParent(this->FrameForGrid);
  this->AutoAdjustCheckButton->Create();
  this->AutoAdjustCheckButton->SetSelectedState(1);
  this->AutoAdjustCheckButton->SetAnchorToEast();
  this->AutoAdjustCheckButton->SetText("Auto");
  this->AutoAdjustCheckButton->SetBalloonHelpString(
    "Automatically decide the number of labels.");
  this->AutoAdjustCheckButton->SetCommand(this,
    "SetAutoAdjust");
  
  this->Script("grid %s %s - %s -sticky ew",
    this->NumberOfLabelsLabel->GetWidgetName(),
    this->NumberOfLabelsScale->GetWidgetName(),
    this->AutoAdjustCheckButton->GetWidgetName());

  this->NumberOfMinorTicksLabel->SetParent(this->FrameForGrid);
  this->NumberOfMinorTicksLabel->Create();
  this->NumberOfMinorTicksLabel->SetText("Number of Minor Ticks ");
  this->NumberOfMinorTicksLabel->SetAnchorToWest();

  this->NumberOfMinorTicksScale->SetParent(this->FrameForGrid);
  this->NumberOfMinorTicksScale->Create();
  this->NumberOfMinorTicksScale->SetRange(0, 25);
  this->NumberOfMinorTicksScale->SetResolution(1);
  this->NumberOfMinorTicksScale->SetEntryWidth(7);
  this->NumberOfMinorTicksScale->SetCommand(this,
    "SetNumberOfMinorTicks");

  this->Script("grid %s %s - x -sticky ew",
    this->NumberOfMinorTicksLabel->GetWidgetName(),
    this->NumberOfMinorTicksScale->GetWidgetName());

  this->Script("grid columnconfigure %s 1 -weight 2",
    this->GetFrame()->GetWidgetName());

  this->Script("grid columnconfigure %s 2 -weight 2",
    this->GetFrame()->GetWidgetName());

  this->OKButton->SetParent(this->GetFrame());
  this->OKButton->Create();
  this->OKButton->SetText("OK");
  this->OKButton->SetCommand(this, "OK");

  this->CancelButton->SetParent(this->GetFrame());
  this->CancelButton->Create();
  this->CancelButton->SetText("Cancel");
  this->CancelButton->SetCommand(this, "Cancel");

  this->Script("pack %s %s -fill x -expand true -side left",
    this->OKButton->GetWidgetName(),
    this->CancelButton->GetWidgetName());
}

//-----------------------------------------------------------------------------
int vtkPVPlotDisplayLabelPropertiesDialog::Invoke()
{
  this->PushWidgetValues(0);
  return this->Superclass::Invoke();
}

//-----------------------------------------------------------------------------
void vtkPVPlotDisplayLabelPropertiesDialog::PushWidgetValues(int from_gui)
{
  if (from_gui)
    {
    if (this->LabelFormatProperty)
      {
      this->LabelFormatProperty->SetElement(0,
        this->LabelFormatEntry->GetValue());
      }

    if (this->NumberOfLabelsProperty)
      {
      this->NumberOfLabelsProperty->SetElement(0,
        static_cast<int>(this->NumberOfLabelsScale->GetValue()));
      }

    if (this->AutoAdjustProperty)
      {
      this->AutoAdjustProperty->SetElement(0,
        this->AutoAdjustCheckButton->GetSelectedState());
      }

    if (this->NumberOfMinorTicksProperty)
      {
      this->NumberOfMinorTicksProperty->SetElement(0, 
        static_cast<int>(this->NumberOfMinorTicksScale->GetValue()));
      }
    if (this->TitlePositionProperty)
      {
      this->TitlePositionProperty->SetElement(0,
        this->TitlePositionEntry->GetValueAsDouble());
      }
    if (this->DataRangeProperty)
      {
      double range[2] = {0.0, 0.0};
      if (!this->DataRangeAutoCheckButton->GetSelectedState())
        {
        range[0] = this->DataRangeMinWidget->GetWidget()->GetValueAsDouble();
        range[1] = this->DataRangeMaxWidget->GetWidget()->GetValueAsDouble();
        }
      this->DataRangeProperty->SetElements(range);
      }
    }
  else
    {
    if (this->LabelFormatProperty)
      {
      this->LabelFormatEntry->SetValue(this->LabelFormatProperty->GetElement(0));
      }
    if (this->NumberOfLabelsProperty)
      {
      this->NumberOfLabelsScale->SetValue(
        this->NumberOfLabelsProperty->GetElement(0));
      }
    if (this->AutoAdjustProperty)
      {
      this->AutoAdjustCheckButton->SetSelectedState(
        this->AutoAdjustProperty->GetElement(0));
      this->NumberOfLabelsScale->SetEnabled(
        !this->AutoAdjustCheckButton->GetSelectedState());
      }
    if (this->NumberOfMinorTicksProperty)
      {
      this->NumberOfMinorTicksScale->SetValue(
        this->NumberOfMinorTicksProperty->GetElement(0));
      }
    if (this->TitlePositionProperty)
      {
      this->TitlePositionEntry->SetValueAsDouble(
        this->TitlePositionProperty->GetElement(0));
      }   
    if (this->DataRangeProperty)
      {
      double range[2];
      range[0] = this->DataRangeProperty->GetElement(0);
      range[1] = this->DataRangeProperty->GetElement(1);
      this->DataRangeMinWidget->GetWidget()->SetValueAsDouble(range[0]);
      this->DataRangeMaxWidget->GetWidget()->SetValueAsDouble(range[1]);
      this->SetDataRangeAuto((range[0]==range[1]));
      }
    if (this->PlotDisplayProxy)
      {
      this->PlotDisplayProxy->UpdateVTKObjects();
      }
    }
  this->GetTraceHelper()->AddEntry("$kw(%s) PushWidgetValues %d", 
    this->GetTclName(), from_gui);
}

//-----------------------------------------------------------------------------
void vtkPVPlotDisplayLabelPropertiesDialog::OK()
{
  this->PushWidgetValues(1);
  this->Superclass::OK();
}

//-----------------------------------------------------------------------------
void vtkPVPlotDisplayLabelPropertiesDialog::SetLabelFormat(const char* str)
{
  this->LabelFormatEntry->SetValue(str);
  this->GetTraceHelper()->AddEntry("$kw(%s) SetLabelFormat %s",
    this->GetTclName(), str);
}

//-----------------------------------------------------------------------------
void vtkPVPlotDisplayLabelPropertiesDialog::SetDataRangeAuto(int state)
{
  this->DataRangeAutoCheckButton->SetSelectedState(state);
  this->DataRangeMaxWidget->SetEnabled(!state);
  this->DataRangeMinWidget->SetEnabled(!state);

  this->GetTraceHelper()->AddEntry("$kw(%s) SetDataRangeAuto %d",
    this->GetTclName(), state);
}

//-----------------------------------------------------------------------------
void vtkPVPlotDisplayLabelPropertiesDialog::SetNumberOfLabels(int num)
{
  this->NumberOfLabelsScale->SetValue(num);
  this->GetTraceHelper()->AddEntry("$kw(%s) SetNumberOfLabels %d",
    this->GetTclName(), num);
}

//-----------------------------------------------------------------------------
void vtkPVPlotDisplayLabelPropertiesDialog::SetAutoAdjust(int state)
{
  this->AutoAdjustCheckButton->SetSelectedState(state);
  this->NumberOfLabelsScale->SetEnabled(!state);
  this->GetTraceHelper()->AddEntry("$kw(%s) SetAutoAdjust %d",
    this->GetTclName(), state);
}

//-----------------------------------------------------------------------------
void vtkPVPlotDisplayLabelPropertiesDialog::SetNumberOfMinorTicks(
  int num)
{
  this->NumberOfMinorTicksScale->SetValue(num);
  this->GetTraceHelper()->AddEntry("$kw(%s) SetNumberOfMinorTicks %d",
    this->GetTclName(), num);
}

//-----------------------------------------------------------------------------
void vtkPVPlotDisplayLabelPropertiesDialog::SetTitlePositionCallback(const char*)
{
  this->SetTitlePosition(this->TitlePositionEntry->GetValueAsDouble());
}

//-----------------------------------------------------------------------------
void vtkPVPlotDisplayLabelPropertiesDialog::SetTitlePosition(double pos)
{
  this->TitlePositionEntry->SetValueAsDouble(pos);
  this->GetTraceHelper()->AddEntry("$kw(%s) SetTitlePosition %f",
    this->GetTclName(), pos);
}

//-----------------------------------------------------------------------------
void vtkPVPlotDisplayLabelPropertiesDialog::SetDataRangeCallback(const char*)
{
  double range[2];
  range[0] = this->DataRangeMinWidget->GetWidget()->GetValueAsDouble();
  range[1] = this->DataRangeMaxWidget->GetWidget()->GetValueAsDouble();
  this->SetDataRange(range[0], range[1]);
}

//-----------------------------------------------------------------------------
void vtkPVPlotDisplayLabelPropertiesDialog::SetDataRange(double min, double max)
{
  this->DataRangeMinWidget->GetWidget()->SetValueAsDouble(min);
  this->DataRangeMaxWidget->GetWidget()->SetValueAsDouble(max);
  this->GetTraceHelper()->AddEntry("$kw(%s) SetDataRange %f %f",
    this->GetTclName(), min, max);
}

//-----------------------------------------------------------------------------
void vtkPVPlotDisplayLabelPropertiesDialog::Cancel()
{
  this->GetTraceHelper()->AddEntry("$kw(%s) Cancel",
    this->GetTclName());
  this->Superclass::Cancel();
}

//-----------------------------------------------------------------------------
void vtkPVPlotDisplayLabelPropertiesDialog::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  os << indent << "TraceHelper: " << this->TraceHelper << endl;
}

