/*=========================================================================

  Program:   ParaView
  Module:    vtkPVExtractPartsWidget.cxx

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkPVExtractPartsWidget.h"

#include "vtkCollection.h"
#include "vtkCollectionIterator.h"
#include "vtkKWLabel.h"
#include "vtkKWListBox.h"
#include "vtkKWPushButton.h"
#include "vtkKWWidget.h"
#include "vtkObjectFactory.h"
#include "vtkPVApplication.h"
#include "vtkPVDisplayGUI.h"
#include "vtkSMPart.h"
#include "vtkPVProcessModule.h"
#include "vtkPVSource.h"
#include "vtkPVDataInformation.h"
#include "vtkSMIntVectorProperty.h"

//----------------------------------------------------------------------------
vtkStandardNewMacro(vtkPVExtractPartsWidget);
vtkCxxRevisionMacro(vtkPVExtractPartsWidget, "1.23");

int vtkPVExtractPartsWidgetCommand(ClientData cd, Tcl_Interp *interp,
                                int argc, char *argv[]);

//----------------------------------------------------------------------------
vtkPVExtractPartsWidget::vtkPVExtractPartsWidget()
{
  this->CommandFunction = vtkPVExtractPartsWidgetCommand;
  
  this->ButtonFrame = vtkKWWidget::New();
  this->AllOnButton = vtkKWPushButton::New();
  this->AllOffButton = vtkKWPushButton::New();

  this->PartSelectionList = vtkKWListBox::New();
  this->PartLabelCollection = vtkCollection::New();
}

//----------------------------------------------------------------------------
vtkPVExtractPartsWidget::~vtkPVExtractPartsWidget()
{
  this->ButtonFrame->Delete();
  this->ButtonFrame = NULL;
  this->AllOnButton->Delete();
  this->AllOnButton = NULL;
  this->AllOffButton->Delete();
  this->AllOffButton = NULL;

  this->PartSelectionList->Delete();
  this->PartSelectionList = NULL;
  this->PartLabelCollection->Delete();
  this->PartLabelCollection = NULL;
}

//----------------------------------------------------------------------------
void vtkPVExtractPartsWidget::Create(vtkKWApplication *app)
{
  // Call the superclass to create the widget and set the appropriate flags

  if (!this->vtkKWWidget::Create(app, "frame", "-bd 0 -relief flat"))
    {
    vtkErrorMacro("Failed creating widget " << this->GetClassName());
    return;
    }

  vtkPVApplication* pvApp = vtkPVApplication::SafeDownCast(app);

  this->ButtonFrame->SetParent(this);
  this->ButtonFrame->Create(pvApp, "frame", "");
  this->Script("pack %s -side top -fill x",
               this->ButtonFrame->GetWidgetName());

  this->AllOnButton->SetParent(this->ButtonFrame);
  this->AllOnButton->Create(pvApp, "");
  this->AllOnButton->SetLabel("All On");
  this->AllOnButton->SetCommand(this, "AllOnCallback");

  this->AllOffButton->SetParent(this->ButtonFrame);
  this->AllOffButton->Create(pvApp, "");
  this->AllOffButton->SetLabel("All Off");
  this->AllOffButton->SetCommand(this, "AllOffCallback");

  this->Script("pack %s %s -side left -fill x -expand t",
               this->AllOnButton->GetWidgetName(),
               this->AllOffButton->GetWidgetName());

  this->PartSelectionList->SetParent(this);
  this->PartSelectionList->ScrollbarOff();
  this->PartSelectionList->Create(app, "-selectmode extended");
  this->PartSelectionList->SetHeight(0);
  // I assume we need focus for control and alt modifiers.
  this->Script("bind %s <Enter> {focus %s}",
               this->PartSelectionList->GetWidgetName(),
               this->PartSelectionList->GetWidgetName());

  this->Script("pack %s -side top -fill both -expand t",
               this->PartSelectionList->GetWidgetName());

  int num = this->PVSource->GetPVInput(0)->GetNumberOfParts();
  vtkSMIntVectorProperty *ivp = vtkSMIntVectorProperty::SafeDownCast(
    this->GetSMProperty());
  if (ivp)
    {
    int i;

    for (i = 0; i < num; i++)
      {
      ivp->SetElement(i, 1);
      }
    }
  
  // There is no current way to get a modified call back, so assume
  // the user will change the list.  This widget will only be used once anyway.
  this->ModifiedCallback();
}

//----------------------------------------------------------------------------
void vtkPVExtractPartsWidget::Inactivate()
{
  int num, idx;
  vtkKWLabel* label;

  this->Script("pack forget %s %s", this->ButtonFrame->GetWidgetName(),
               this->PartSelectionList->GetWidgetName());

  num = this->PartSelectionList->GetNumberOfItems();
  for (idx = 0; idx < num; ++idx)
    {
    if (this->PartSelectionList->GetSelectState(idx))
      {
      label = vtkKWLabel::New();
      label->SetParent(this);
      label->SetLabel(this->PartSelectionList->GetItem(idx));
      label->Create(this->GetApplication(), "");
      this->Script("pack %s -side top -anchor w",
                   label->GetWidgetName());
      this->PartLabelCollection->AddItem(label);
      label->Delete();
      }
    }
}

//----------------------------------------------------------------------------
void vtkPVExtractPartsWidget::Accept()
{
  int num, idx;
  int modFlag = this->GetModifiedFlag();
  
  num = this->PartSelectionList->GetNumberOfItems();

  if (modFlag)
    {
    this->Inactivate();
    }

  // Now loop through the input mask setting the selection states.
  vtkSMIntVectorProperty *ivp = vtkSMIntVectorProperty::SafeDownCast(
    this->GetSMProperty());
  if (!ivp)
    {
    vtkErrorMacro(
      "Could not find property of name: "
      << (this->GetSMPropertyName()?this->GetSMPropertyName():"(null)")
      << " for widget: " << this->GetTraceName());
    return;
    }
  
  for (idx = 0; idx < num; ++idx)
    {
    ivp->SetElement(idx, this->PartSelectionList->GetSelectState(idx));
    }

  this->Superclass::Accept();
}


//---------------------------------------------------------------------------
void vtkPVExtractPartsWidget::SetSelectState(int idx, int val)
{
  this->PartSelectionList->SetSelectState(idx, val);
}


//---------------------------------------------------------------------------
void vtkPVExtractPartsWidget::Trace(ofstream *file)
{
  int idx, num;

  vtkSMIntVectorProperty *ivp = vtkSMIntVectorProperty::SafeDownCast(
    this->GetSMProperty());

  if ( ! this->InitializeTrace(file) || !ivp)
    {
    return;
    }

  num = this->PartSelectionList->GetNumberOfItems();
  for (idx = 0; idx < num; ++idx)
    {
    *file << "$kw(" << this->GetTclName() << ") SetSelectState "
          << idx << " " << ivp->GetElement(idx) << endl;
    }
}

//----------------------------------------------------------------------------
void vtkPVExtractPartsWidget::ResetInternal()
{
  vtkPVSource *input;
  vtkSMPart *part;
  int num, idx;

  this->PartSelectionList->DeleteAll();
  // Loop through all of the parts of the input adding to the list.
  input = this->PVSource->GetPVInput(0);
  num = input->GetNumberOfParts();
  for (idx = 0; idx < num; ++idx)
    {
    part = input->GetPart(idx);
    this->PartSelectionList->InsertEntry(idx, 
                                  part->GetDataInformation()->GetName());
    }

  vtkSMIntVectorProperty *ivp = vtkSMIntVectorProperty::SafeDownCast(
    this->GetSMProperty());
  if (!ivp)
    {
    return;
    }
  
  // Now loop through the input mask setting the selection states.
  for (idx = 0; idx < num; ++idx)
    {
    this->PartSelectionList->SetSelectState(
      idx, ivp->GetElement(idx));
    }

  // Because list box does not notify us when it is modified ...
  if (this->AcceptCalled)
    {
    this->ModifiedFlag = 0;
    }
}

//----------------------------------------------------------------------------
void vtkPVExtractPartsWidget::AllOnCallback()
{
  int num, idx;

  num = this->PartSelectionList->GetNumberOfItems();
  for (idx = 0; idx < num; ++idx)
    {
    this->PartSelectionList->SetSelectState(idx, 1);
    }

  // Because list box does not notify us when it is modified ...
  //this->ModifiedFlag = 0;
}

//----------------------------------------------------------------------------
void vtkPVExtractPartsWidget::AllOffCallback()
{
  int num, idx;

  num = this->PartSelectionList->GetNumberOfItems();
  for (idx = 0; idx < num; ++idx)
    {
    this->PartSelectionList->SetSelectState(idx, 0);
    }

  // Because list box does not notify us when it is modified ...
  //this->ModifiedFlag = 0;
}

//----------------------------------------------------------------------------
// Multiple input filter has only one VTK source.
void vtkPVExtractPartsWidget::SaveInBatchScript(ofstream *file)
{
  int num, idx;

  vtkClientServerID sourceID = this->PVSource->GetVTKSourceID(0);

  if (sourceID.ID == 0 || !this->SMPropertyName)
    {
    vtkErrorMacro("Sanity check failed. " << this->GetClassName());
    return;
    }

  num = this->PartSelectionList->GetNumberOfItems();

  *file << "  [$pvTemp" << sourceID << " GetProperty "
        << this->SMPropertyName << "] SetNumberOfElements "
        << num << endl;

  // Now loop through the input mask setting the selection states.
  for (idx = 0; idx < num; idx++)
    {
    *file << "  [$pvTemp" << sourceID << " GetProperty "
          << this->SMPropertyName << "] SetElement "
          << idx << " " << this->PartSelectionList->GetSelectState(idx)
          << endl;
    }
}

//----------------------------------------------------------------------------
void vtkPVExtractPartsWidget::UpdateEnableState()
{
  this->Superclass::UpdateEnableState();

  this->PropagateEnableState(this->ButtonFrame);
  this->PropagateEnableState(this->AllOnButton);
  this->PropagateEnableState(this->AllOffButton);

  this->PropagateEnableState(this->PartSelectionList);

  vtkCollectionIterator* sit = this->PartLabelCollection->NewIterator();
  for ( sit->InitTraversal(); !sit->IsDoneWithTraversal(); sit->GoToNextItem() )
    {
    this->PropagateEnableState(vtkKWWidget::SafeDownCast(sit->GetObject()));
    }
  sit->Delete();
}

//----------------------------------------------------------------------------
void vtkPVExtractPartsWidget::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);
}
