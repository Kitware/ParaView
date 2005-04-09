/*=========================================================================

  Program:   ParaView
  Module:    vtkPVValueList.cxx

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkPVValueList.h"

#include "vtkArrayMap.txx"
#include "vtkContourValues.h"
#include "vtkKWEntry.h"
#include "vtkKWFrame.h"
#include "vtkKWLabel.h"
#include "vtkKWFrameLabeled.h"
#include "vtkKWListBox.h"
#include "vtkKWMenu.h"
#include "vtkKWPushButton.h"
#include "vtkKWRange.h"
#include "vtkKWScale.h"
#include "vtkObjectFactory.h"
#include "vtkPVAnimationInterfaceEntry.h"
#include "vtkPVApplication.h"
#include "vtkPVSource.h"
#include "vtkPVVectorEntry.h"
#include "vtkPVXMLElement.h"
#include "vtkSMDoubleVectorProperty.h"
#include "vtkPVTraceHelper.h"

//-----------------------------------------------------------------------------
vtkCxxRevisionMacro(vtkPVValueList, "1.25");

int vtkPVValueListCommand(ClientData cd, Tcl_Interp *interp,
                        int argc, char *argv[]);

const int vtkPVValueList::MAX_NUMBER_ENTRIES = 200;

//-----------------------------------------------------------------------------
vtkPVValueList::vtkPVValueList()
{
  this->CommandFunction = vtkPVValueListCommand;

  this->ContourValuesFrame = vtkKWFrameLabeled::New();
  this->ContourValuesFrame2 = vtkKWFrame::New();
  this->ContourValuesList = vtkKWListBox::New();

  this->ContourValuesButtonsFrame = vtkKWFrame::New();
  this->DeleteValueButton = vtkKWPushButton::New();
  this->DeleteAllButton = vtkKWPushButton::New();

  this->NewValueFrame = vtkKWFrameLabeled::New();
  this->NewValueLabel = vtkKWLabel::New();
  this->NewValueEntry = vtkKWScale::New();
  this->NewValueEntry->ClampValueOff();
  this->AddValueButton = vtkKWPushButton::New();

  this->GenerateFrame = vtkKWFrameLabeled::New();
  this->GenerateNumberFrame = vtkKWFrame::New();
  this->GenerateRangeFrame = vtkKWFrame::New();

  this->GenerateLabel = vtkKWLabel::New();
  this->GenerateEntry = vtkKWScale::New();
  this->GenerateButton = vtkKWPushButton::New();

  this->GenerateRangeLabel = vtkKWLabel::New();
  this->GenerateRangeWidget = vtkKWRange::New();
  this->GenerateRangeWidget->ClampRangeOff();
  
  this->ContourValues = vtkContourValues::New();
}

//-----------------------------------------------------------------------------
vtkPVValueList::~vtkPVValueList()
{
  this->ContourValuesFrame->Delete();
  this->ContourValuesFrame = NULL;
  this->ContourValuesFrame2->Delete();
  this->ContourValuesFrame2 = NULL;
  this->ContourValuesList->Delete();
  this->ContourValuesList = NULL;

  this->ContourValuesButtonsFrame->Delete();
  this->ContourValuesButtonsFrame = NULL;
  this->DeleteValueButton->Delete();
  this->DeleteValueButton = NULL;
  this->DeleteAllButton->Delete();
  this->DeleteAllButton = NULL;

  this->NewValueLabel->Delete();
  this->NewValueLabel = NULL;
  this->NewValueEntry->Delete();
  this->NewValueEntry = NULL;
  this->AddValueButton->Delete();
  this->AddValueButton = NULL;
  this->NewValueFrame->Delete();
  this->NewValueFrame = NULL;

  this->GenerateFrame->Delete();
  this->GenerateFrame = NULL;
  this->GenerateNumberFrame->Delete();
  this->GenerateNumberFrame = NULL;
  this->GenerateRangeFrame->Delete();
  this->GenerateRangeFrame = NULL;

  this->GenerateLabel->Delete();
  this->GenerateLabel = NULL;
  this->GenerateEntry->Delete();
  this->GenerateEntry = NULL;
  this->GenerateButton->Delete();
  this->GenerateButton = NULL;

  this->GenerateRangeLabel->Delete();
  this->GenerateRangeLabel = NULL;
  this->GenerateRangeWidget->Delete();
  this->GenerateRangeWidget = NULL;
  
  this->SetPVSource(NULL);
  
  this->ContourValues->Delete();
  this->ContourValues = NULL;
}

//-----------------------------------------------------------------------------
void vtkPVValueList::SetLabel(const char* str)
{
  this->ContourValuesFrame->SetLabelText(str);
  if (str && str[0] &&
      (this->GetTraceHelper()->GetObjectNameState() == 
       vtkPVTraceHelper::ObjectNameStateUninitialized ||
       this->GetTraceHelper()->GetObjectNameState() == 
       vtkPVTraceHelper::ObjectNameStateDefault) )
    {
    this->GetTraceHelper()->SetObjectName(str);
    this->GetTraceHelper()->SetObjectNameState(
      vtkPVTraceHelper::ObjectNameStateSelfInitialized);
    }
}

//-----------------------------------------------------------------------------
void vtkPVValueList::Create(vtkKWApplication *app)
{
  // Call the superclass to create the widget and set the appropriate flags

  if (!this->vtkKWWidget::Create(app, "frame", NULL))
    {
    vtkErrorMacro("Failed creating widget " << this->GetClassName());
    return;
    }

  this->ContourValuesFrame->SetParent(this); 
  this->ContourValuesFrame->Create(app, "");
  this->Script("pack %s -expand yes -fill x",
               this->ContourValuesFrame->GetWidgetName());

  this->ContourValuesFrame2->SetParent(this->ContourValuesFrame->GetFrame()); 
  this->ContourValuesFrame2->Create(app, "");
  this->Script("pack %s", this->ContourValuesFrame2->GetWidgetName());

  this->ContourValuesList->SetParent(this->ContourValuesFrame2->GetFrame());
  this->ContourValuesList->Create(app, "");
  this->ContourValuesList->SetHeight(5);
  this->Script("bind %s <Delete> {%s DeleteValueCallback}",
               this->ContourValuesList->GetWidgetName(),
               this->GetTclName());

  this->ContourValuesButtonsFrame->SetParent(
    this->ContourValuesFrame2->GetFrame());
  this->ContourValuesButtonsFrame->Create(app, "");

  this->DeleteValueButton->SetParent(
    this->ContourValuesButtonsFrame->GetFrame());
  this->DeleteValueButton->Create(app, "-text {Delete}");
  this->DeleteValueButton->SetCommand(this, "DeleteValueCallback");
  this->DeleteValueButton->SetBalloonHelpString(
    "Remove the currently selected value from the list");

  this->DeleteAllButton->SetParent(
    this->ContourValuesButtonsFrame->GetFrame());
  this->DeleteAllButton->Create(app, "-text {Delete All}");
  this->DeleteAllButton->SetCommand(this, "RemoveAllValues");
  this->DeleteAllButton->SetBalloonHelpString(
    "Remove all entries from the list");

  this->Script("pack %s %s -side top -anchor n -expand yes -fill x -pady 2",
               this->DeleteValueButton->GetWidgetName(),
               this->DeleteAllButton->GetWidgetName());

  this->Script("pack %s -side left",
               this->ContourValuesList->GetWidgetName());
  this->Script("pack %s  -side left -padx 10 -pady 10 -expand yes -fill y",
               this->ContourValuesButtonsFrame->GetWidgetName());

  // We need focus for delete binding.
  this->Script("bind %s <Enter> {+focus %s}",
               this->ContourValuesList->GetWidgetName(),
               this->ContourValuesList->GetWidgetName());
  
  this->NewValueFrame->SetParent(this);
  this->NewValueFrame->SetLabelText("Add value");
  this->NewValueFrame->Create(app, "");
  
  this->Script("pack %s -expand yes -fill x",
               this->NewValueFrame->GetWidgetName());
  
  this->NewValueLabel->SetParent(this->NewValueFrame->GetFrame());
  this->NewValueLabel->Create(app, "");
  this->NewValueLabel->SetText("New Value");
  this->NewValueLabel->SetBalloonHelpString("Enter a new value");
  this->NewValueLabel->SetWidth(17);
  this->NewValueLabel->SetBalloonHelpString(
    "Add a single value to the list");
  
  this->NewValueEntry->SetParent(this->NewValueFrame->GetFrame());
  this->NewValueEntry->Create(app, "");
  this->NewValueEntry->SetDisplayEntryAndLabelOnTop(0);
  this->NewValueEntry->DisplayEntry();
  this->NewValueEntry->SetRange(-VTK_LARGE_FLOAT, 
                                 VTK_LARGE_FLOAT);
  this->NewValueEntry->SetResolution(1);
  this->NewValueEntry->GetEntry()->SetWidth(7);
  this->Script("bind %s <KeyPress-Return> {+%s AddValueCallback}",
               this->NewValueEntry->GetEntry()->GetWidgetName(),
               this->GetTclName());
  this->NewValueEntry->SetBalloonHelpString(
    "Add a single value to the list");
  
  this->AddValueButton->SetParent(this->NewValueFrame->GetFrame());
  this->AddValueButton->Create(app, "-text {Add}");
  this->AddValueButton->SetCommand(this, "AddValueCallback");
  this->AddValueButton->SetBalloonHelpString("Add the new value to the list");
  this->AddValueButton->SetWidth(10);
  this->Script("bind %s <Enter> {+focus %s}",
               this->AddValueButton->GetWidgetName(),
               this->AddValueButton->GetWidgetName());
  
  this->Script("pack %s -side left",
               this->NewValueLabel->GetWidgetName());
  this->Script("pack %s -side left -expand yes -fill x",
               this->NewValueEntry->GetWidgetName());
  this->Script("pack %s -side left -padx 2",
               this->AddValueButton->GetWidgetName());
  
  this->GenerateFrame->SetParent(this);
  this->GenerateFrame->Create(app, "");
  this->GenerateFrame->SetLabelText("Generate range of values");
  
  this->Script("pack %s -fill x -expand yes", 
               this->GenerateFrame->GetWidgetName());

  this->GenerateNumberFrame->SetParent(this->GenerateFrame->GetFrame());
  this->GenerateNumberFrame->Create(app, "");
  this->Script("pack %s -fill x -expand yes", 
               this->GenerateNumberFrame->GetWidgetName());
  
  this->GenerateLabel->SetParent(this->GenerateNumberFrame->GetFrame());
  this->GenerateLabel->Create(app, "");
  this->GenerateLabel->SetText("Number of Values");
  this->GenerateLabel->SetWidth(17);
  
  this->GenerateEntry->SetParent(this->GenerateNumberFrame->GetFrame());
  this->GenerateEntry->Create(app, "");
  this->GenerateEntry->SetDisplayEntryAndLabelOnTop(0);
  this->GenerateEntry->DisplayEntry();
  this->GenerateEntry->SetRange(1, vtkPVValueList::MAX_NUMBER_ENTRIES);
  this->GenerateEntry->SetValue(1);
  this->GenerateEntry->SetResolution(1);
  this->GenerateEntry->GetEntry()->SetWidth(7);
  this->GenerateEntry->SetBalloonHelpString(
    "The number of values to be added to the list");
  
  this->GenerateButton->SetParent(this->GenerateNumberFrame->GetFrame());
  this->GenerateButton->Create(app, "");
  this->GenerateButton->SetText("Generate");
  this->GenerateButton->SetWidth(10);
  this->GenerateButton->SetCommand(this, "GenerateValuesCallback");
  this->GenerateButton->SetBalloonHelpString(
    "Add a range values to the list");
  this->Script("bind %s <Enter> {+focus %s}",
               this->GenerateButton->GetWidgetName(),
               this->GenerateButton->GetWidgetName());
  
  this->Script("pack %s -side left",
               this->GenerateLabel->GetWidgetName());
  this->Script("pack %s -side left -expand yes -fill x",
               this->GenerateEntry->GetWidgetName());
  this->Script("pack %s -side left",
               this->GenerateButton->GetWidgetName());
  
  this->GenerateRangeFrame->SetParent(this->GenerateFrame->GetFrame());
  this->GenerateRangeFrame->Create(app, "");
  this->Script("pack %s -fill x -expand yes -pady 3", 
               this->GenerateRangeFrame->GetWidgetName());

  this->GenerateRangeLabel->SetParent(this->GenerateRangeFrame->GetFrame());
  this->GenerateRangeLabel->Create(app, "");
  this->GenerateRangeLabel->SetText("Range");
  this->GenerateRangeLabel->SetWidth(17);
  this->GenerateRangeLabel->SetBalloonHelpString(
    "Set the minimum and maximum of the values to be added");

  this->GenerateRangeWidget->SetParent(this->GenerateRangeFrame->GetFrame());
  this->GenerateRangeWidget->Create(app, "");
  this->GenerateRangeWidget->SetWholeRange(
    -VTK_LARGE_FLOAT, VTK_LARGE_FLOAT);
  this->GenerateRangeWidget->ShowEntriesOn();
  this->GenerateRangeWidget->ShowLabelOff();
  this->GenerateRangeWidget->GetEntry1()->SetWidth(7);
  this->GenerateRangeWidget->GetEntry2()->SetWidth(7);
  this->GenerateRangeWidget->SetEntry1PositionToLeft();
  this->GenerateRangeWidget->SetEntry2PositionToRight();
  this->GenerateRangeWidget->SetBalloonHelpString(
    "Set the minimum and maximum of the values to be added");

  this->Script("pack %s -side left",
               this->GenerateRangeLabel->GetWidgetName());
  this->Script("pack %s -side left -fill x -expand yes",
                 this->GenerateRangeWidget->GetWidgetName());

  this->ContourValues->SetNumberOfContours(0);
  
  // Get the default values in the UI.
  this->Update();
}

//-----------------------------------------------------------------------------
void vtkPVValueList::Update()
{
  char str[256];

  if (this->GetApplication() == NULL)
    {
    return;
    }

  this->ContourValuesList->DeleteAll();
  vtkSMDoubleVectorProperty* prop = vtkSMDoubleVectorProperty::SafeDownCast(
    this->GetSMProperty());
  if (prop)
    {
    unsigned int num = prop->GetNumberOfElements();

    for (unsigned int idx = 0; idx < num; ++idx)
      {
      sprintf(str, "%g", prop->GetElement(idx));
      this->ContourValuesList->AppendUnique(str);
      }    
    }

  if (!this->ComputeWidgetRange())
    {
    return;
    }

  float oldRange[2];
  float range[2];
  range[0] = this->WidgetRange[0];
  range[1] = this->WidgetRange[1];
  
  if (range[0] > range[1])
    {
    return;
    }
  
  if (range[0] == range[1])
    {
    // Special case to avoid log(0).
    this->NewValueEntry->SetRange(range[0], range[1]);
    this->NewValueEntry->SetValue(range[0]);

    this->GenerateRangeWidget->SetWholeRange(range[0], range[1]);
    this->GenerateRangeWidget->SetRange(range[0], range[1]);

    return;
    }

  // Find the place value resolution.
  int place = 
    static_cast<int>(
      floor(log10(static_cast<double>(range[1]-range[0])) - 1.5));
  double resolution = pow(10.0, static_cast<double>(place));

  this->NewValueEntry->GetRange(oldRange);

  // Detect when the array has changed.
  if (oldRange[0] != range[0] || oldRange[1] != range[1])
    {
    this->GenerateRangeWidget->SetResolution(resolution);
    this->GenerateRangeWidget->SetWholeRange(range[0], range[1]);
    this->GenerateRangeWidget->SetRange(range[0], range[1]);

    this->NewValueEntry->SetResolution(resolution);
    this->NewValueEntry->SetRange(range[0], range[1]);
    this->NewValueEntry->SetValue((range[0]+range[1])/2.0);
    }
}

//-----------------------------------------------------------------------------
void vtkPVValueList::SetBalloonHelpString(const char *str)
{
  this->Superclass::SetBalloonHelpString(str);

  if (this->ContourValuesList)
    {
    this->ContourValuesList->SetBalloonHelpString(str);
    }
}

//-----------------------------------------------------------------------------
void vtkPVValueList::AddValueCallback()
{
  double value = this->NewValueEntry->GetValue();
  this->ContourValues->SetValue(this->ContourValues->GetNumberOfContours(),
                                value);
  char str[256];
  sprintf(str, "%g", value);
  this->ContourValuesList->AppendUnique(str);
  this->ModifiedCallback();
}

//-----------------------------------------------------------------------------
void vtkPVValueList::RemoveAllValues()
{
  this->ContourValues->SetNumberOfContours(0);
  this->ContourValuesList->DeleteAll();
  this->ModifiedCallback();
}

//-----------------------------------------------------------------------------
void vtkPVValueList::DeleteValueCallback()
{
  int index;
  int num, idx;
  
  num = this->ContourValues->GetNumberOfContours();

  // First look for selected values in the value list.
  index = this->ContourValuesList->GetSelectionIndex();
  if (index == -1)
    {
    float val = this->NewValueEntry->GetValue();
    // Find the index of the value in the entry box.
    // If the entry value is not in the list,
    // this will just clear the entry and return.
    for (idx = 0; idx < num && index < 0; ++idx)
      {
      if ( this->ContourValues->GetValue(idx) == val )
        {
        index = idx;
        }
      }
    if (index == -1)
      {
      // Finally just delete the last in the list.
      index = num - 1;
      }
    }
  
  if ( index >= 0 )
    {
    for (idx = index; idx < num-1; idx++)
      {
      this->ContourValues->SetValue(idx, this->ContourValues->GetValue(idx+1));
      }
    this->ContourValues->SetNumberOfContours(num-1);
    this->ContourValuesList->DeleteRange(index, index);
    this->ModifiedCallback();
    }
}

//-----------------------------------------------------------------------------
void vtkPVValueList::GenerateValuesCallback()
{
  double range[2];
  this->GenerateRangeWidget->GetRange(range);

  if (range[0] == 0 && range[1] == 0) // happens if the entries are empty
    {
    if (!this->ComputeWidgetRange())
      {
      return;
      }
    range[0] = this->WidgetRange[0];
    range[1] = this->WidgetRange[1];
    }

  int numContours = static_cast<int>(this->GenerateEntry->GetValue());
  
  if (numContours == 1)
    {
    this->AddValue((range[1] + range[0])/2.0);
    return;
    }

  double step = (range[1] - range[0]) / (float)(numContours-1);
  
  int i;
  for (i = 0; i < numContours; i++)
    {
    this->AddValue(i*step+range[0]);
    }
}

//-----------------------------------------------------------------------------
void vtkPVValueList::AddValueNoModified(double val)
{
  this->ContourValues->SetValue(this->ContourValues->GetNumberOfContours(),
                                val);
  char str[256];
  sprintf(str, "%g", val);
  this->ContourValuesList->AppendUnique(str);
}

//-----------------------------------------------------------------------------
void vtkPVValueList::AddValue(double val)
{
  this->AddValueNoModified(val);
  this->ModifiedCallback();
}

//-----------------------------------------------------------------------------
int vtkPVValueList::GetNumberOfValues()
{
  return this->ContourValues->GetNumberOfContours();
}

//-----------------------------------------------------------------------------
double vtkPVValueList::GetValue(int index)
{
  if (index >= this->ContourValues->GetNumberOfContours())
    {
    vtkErrorMacro("Invalid index " << index);
    return 0;
    }
  return this->ContourValues->GetValue(index);
}

//-----------------------------------------------------------------------------
void vtkPVValueList::Accept()
{
  int numContours;

  numContours = this->ContourValues->GetNumberOfContours();

  if (numContours == 0)
    {
    // Hit the add value button incase the user forgot.
    // This does nothing if there is no value in there.
    this->ContourValues->SetValue(0, this->NewValueEntry->GetValue());
    char str[256];
    sprintf(str, "%g", this->NewValueEntry->GetValue());
    this->ContourValuesList->AppendUnique(str);
    numContours = 1;
    }

}

//-----------------------------------------------------------------------------
void vtkPVValueList::Trace(ofstream *file)
{
  if ( ! this->GetTraceHelper()->Initialize(file))
    {
    return;
    }

  *file << "$kw(" << this->GetTclName() << ") RemoveAllValues\n";

  vtkSMDoubleVectorProperty* prop = vtkSMDoubleVectorProperty::SafeDownCast(
    this->GetSMProperty());
  if (prop)
    {
    unsigned int numContours = prop->GetNumberOfElements();
    for (unsigned int i = 0; i < numContours; i++)
      {
      *file << "$kw(" << this->GetTclName() << ") AddValue "
            << prop->GetElement(i) << endl;
      }
    }
}

//-----------------------------------------------------------------------------
void vtkPVValueList::AddAnimationScriptsToMenu(
  vtkKWMenu *menu, vtkPVAnimationInterfaceEntry *ai)
{
  char methodAndArgs[500];
  
  sprintf(methodAndArgs, "AnimationMenuCallback %s", ai->GetTclName()); 
  menu->AddCommand(this->GetTraceHelper()->GetObjectName(), this, methodAndArgs, 0,"");
}

//-----------------------------------------------------------------------------
void vtkPVValueList::CopyProperties(
  vtkPVWidget* clone, 
  vtkPVSource* pvSource,
  vtkArrayMap<vtkPVWidget*, vtkPVWidget*>* map)
{
  this->Superclass::CopyProperties(clone, pvSource, map);
  vtkPVValueList* pvce = vtkPVValueList::SafeDownCast(clone);
  if (pvce)
    {
    pvce->SetLabel(this->ContourValuesFrame->GetLabel()->GetText());
    }
  else 
    {
    vtkErrorMacro("Internal error. Could not downcast clone to PVValueList.");
    }
}

//-----------------------------------------------------------------------------
int vtkPVValueList::ReadXMLAttributes(vtkPVXMLElement* element,
                                         vtkPVXMLPackageParser* parser)
{
  if(!this->Superclass::ReadXMLAttributes(element, parser)) { return 0; }
  
  const char* attr;

  attr = element->GetAttribute("label");
  if(!attr)
    {
    vtkErrorMacro("No label attribute.");
    return 0;
    }
  this->SetLabel(attr);
  
  return 1;
}

//-----------------------------------------------------------------------------
const char* vtkPVValueList::GetLabel() 
{
  return this->ContourValuesFrame->GetLabel()->GetText();
}

//----------------------------------------------------------------------------
void vtkPVValueList::UpdateEnableState()
{
  this->Superclass::UpdateEnableState();

  this->PropagateEnableState(this->ContourValuesFrame);
  this->PropagateEnableState(this->ContourValuesFrame2);
  this->PropagateEnableState(this->ContourValuesList);

  this->PropagateEnableState(this->ContourValuesButtonsFrame);
  this->PropagateEnableState(this->DeleteValueButton);
  this->PropagateEnableState(this->DeleteAllButton);

  this->PropagateEnableState(this->NewValueFrame);
  this->PropagateEnableState(this->NewValueLabel);
  this->PropagateEnableState(this->NewValueEntry);
  this->PropagateEnableState(this->AddValueButton);

  this->PropagateEnableState(this->GenerateFrame);
  this->PropagateEnableState(this->GenerateNumberFrame);
  this->PropagateEnableState(this->GenerateRangeFrame);

  this->PropagateEnableState(this->GenerateLabel);
  this->PropagateEnableState(this->GenerateRangeLabel);
  this->PropagateEnableState(this->GenerateEntry);
  this->PropagateEnableState(this->GenerateButton);

  this->PropagateEnableState(this->GenerateRangeWidget);
}

//-----------------------------------------------------------------------------
void vtkPVValueList::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);
}
