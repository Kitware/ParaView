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
#include "vtkPVData.h"
#include "vtkPVPart.h"
#include "vtkPVProcessModule.h"
#include "vtkPVScalarListWidgetProperty.h"
#include "vtkPVSource.h"

//----------------------------------------------------------------------------
vtkStandardNewMacro(vtkPVExtractPartsWidget);
vtkCxxRevisionMacro(vtkPVExtractPartsWidget, "1.15");

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
  
  this->Property = NULL;
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
  
  this->SetProperty(NULL);
}

//----------------------------------------------------------------------------
void vtkPVExtractPartsWidget::Create(vtkKWApplication *app)
{
  vtkPVApplication* pvApp = vtkPVApplication::SafeDownCast(app);

  if (this->Application)
    {
    vtkErrorMacro("PVWidget already created");
    return;
    }
  this->SetApplication(app);

  // create the top level
  this->Script("frame %s -borderwidth 0 -relief flat", this->GetWidgetName());

  
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
  int i;
  float *scalars = new float[2*num];
  
  for (i = 0; i < num; i++)
    {
    scalars[2*i] = i;
    scalars[2*i+1] = 1;
    }
  
  this->Property->SetScalars(num*2, scalars);
  delete [] scalars;
  
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
      label->Create(this->Application, "");
      this->Script("pack %s -side top -anchor w",
                   label->GetWidgetName());
      this->PartLabelCollection->AddItem(label);
      label->Delete();
      }
    }
}

//----------------------------------------------------------------------------
void vtkPVExtractPartsWidget::AcceptInternal(vtkClientServerID vtkSourceID)
{
  int num, idx;

  num = this->PartSelectionList->GetNumberOfItems();

  if (this->ModifiedFlag)
    {
    this->Inactivate();
    }

  // Now loop through the input mask setting the selection states.
  float *scalars = new float[num*2];
  char **cmds = new char*[num];
  int *numScalars = new int[num];
  
  for (idx = 0; idx < num; ++idx)
    {
    scalars[2*idx] = idx;
    scalars[2*idx+1] = this->PartSelectionList->GetSelectState(idx);
    cmds[idx] = new char[13];
    strcpy(cmds[idx], "SetInputMask");
    numScalars[idx] = 2;
    }
  this->Property->SetVTKCommands(num, cmds, numScalars);
  this->Property->SetScalars(num*2, scalars);
  this->Property->SetVTKSourceID(vtkSourceID);
  this->Property->AcceptInternal();
  
  for (idx = 0; idx < num; idx++)
    {
    delete [] cmds[idx];
    }
  delete [] cmds;
  delete [] scalars;
  delete [] numScalars;
  
  this->ModifiedFlag = 0;
}


//---------------------------------------------------------------------------
void vtkPVExtractPartsWidget::SetSelectState(int idx, int val)
{
  this->PartSelectionList->SetSelectState(idx, val);
  
  if (!this->AcceptCalled)
    {
    float *scalars = this->Property->GetScalars();
    scalars[2*idx+1] = val;
    }
}


//---------------------------------------------------------------------------
void vtkPVExtractPartsWidget::Trace(ofstream *file)
{
  int idx, num;

  if ( ! this->InitializeTrace(file))
    {
    return;
    }

  num = this->Property->GetNumberOfScalars() / 2;
  for (idx = 0; idx < num; ++idx)
    {
    *file << "$kw(" << this->GetTclName() << ") SetSelectState "
          << idx << " " << this->Property->GetScalars()[2*idx+1] << endl;
    }
}

//----------------------------------------------------------------------------
void vtkPVExtractPartsWidget::ResetInternal()
{
  vtkPVSource *input;
  vtkPVPart *part;
  int num, idx;

  this->PartSelectionList->DeleteAll();
  // Loop through all of the parts of the input adding to the list.
  input = this->PVSource->GetPVInput(0);
  num = input->GetNumberOfParts();
  for (idx = 0; idx < num; ++idx)
    {
    part = input->GetPart(idx);
    this->PartSelectionList->InsertEntry(idx, part->GetName());
    }

  // Now loop through the input mask setting the selection states.
  float *scalars = this->Property->GetScalars();
  for (idx = 0; idx < num; ++idx)
    {
    this->PartSelectionList->SetSelectState(
      idx, static_cast<int>(scalars[2*idx+1]));
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
  int state;

  num = this->PartSelectionList->GetNumberOfItems();

  // Now loop through the input mask setting the selection states.
  for (idx = 0; idx < num; ++idx)
    {
    state = this->PartSelectionList->GetSelectState(idx);  
    *file << "\t" << "pvTemp" << this->PVSource->GetVTKSourceID(0) 
          << " SetInputMask " << idx << " " << state << endl;  
    }
}

//----------------------------------------------------------------------------
void vtkPVExtractPartsWidget::SetProperty(vtkPVWidgetProperty *prop)
{
  this->Property = vtkPVScalarListWidgetProperty::SafeDownCast(prop);
}

//----------------------------------------------------------------------------
vtkPVWidgetProperty* vtkPVExtractPartsWidget::GetProperty()
{
  return this->Property;
}

//----------------------------------------------------------------------------
vtkPVWidgetProperty* vtkPVExtractPartsWidget::CreateAppropriateProperty()
{
  return vtkPVScalarListWidgetProperty::New();
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
