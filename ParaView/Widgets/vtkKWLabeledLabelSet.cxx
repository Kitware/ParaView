/*=========================================================================

  Module:    vtkKWLabeledLabelSet.cxx

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include "vtkKWLabeledLabelSet.h"

#include "vtkKWApplication.h"
#include "vtkKWLabel.h"
#include "vtkKWLabeledLabel.h"
#include "vtkKWTkUtilities.h"
#include "vtkLinkedList.txx"
#include "vtkLinkedListIterator.txx"
#include "vtkObjectFactory.h"

//----------------------------------------------------------------------------

vtkStandardNewMacro(vtkKWLabeledLabelSet);
vtkCxxRevisionMacro(vtkKWLabeledLabelSet, "1.7");

int vtkvtkKWLabeledLabelSetCommand(ClientData cd, Tcl_Interp *interp,
                                  int argc, char *argv[]);

//----------------------------------------------------------------------------
vtkKWLabeledLabelSet::vtkKWLabeledLabelSet()
{
  this->LabeledLabels = vtkKWLabeledLabelSet::LabeledLabelsContainer::New();
}

//----------------------------------------------------------------------------
vtkKWLabeledLabelSet::~vtkKWLabeledLabelSet()
{
  // Delete all labeled labels

  vtkKWLabeledLabelSet::LabeledLabelSlot *labeledlabel_slot = NULL;
  vtkKWLabeledLabelSet::LabeledLabelsContainerIterator *it = 
    this->LabeledLabels->NewIterator();

  it->InitTraversal();
  while (!it->IsDoneWithTraversal())
    {
    if (it->GetData(labeledlabel_slot) == VTK_OK)
      {
      if (labeledlabel_slot->LabeledLabel)
        {
        labeledlabel_slot->LabeledLabel->Delete();
        labeledlabel_slot->LabeledLabel = NULL;
        }
      delete labeledlabel_slot;
      }
    it->GoToNextItem();
    }
  it->Delete();

  // Delete the container

  this->LabeledLabels->Delete();
}

//----------------------------------------------------------------------------
vtkKWLabeledLabelSet::LabeledLabelSlot* 
vtkKWLabeledLabelSet::GetLabeledLabelSlot(int id)
{
  vtkKWLabeledLabelSet::LabeledLabelSlot *labeledlabel_slot = NULL;
  vtkKWLabeledLabelSet::LabeledLabelSlot *found = NULL;
  vtkKWLabeledLabelSet::LabeledLabelsContainerIterator *it = 
    this->LabeledLabels->NewIterator();

  it->InitTraversal();
  while (!it->IsDoneWithTraversal())
    {
    if (it->GetData(labeledlabel_slot) == VTK_OK && labeledlabel_slot->Id == id)
      {
      found = labeledlabel_slot;
      break;
      }
    it->GoToNextItem();
    }
  it->Delete();

  return found;
}

//----------------------------------------------------------------------------
vtkKWLabeledLabel* vtkKWLabeledLabelSet::GetLabeledLabel(int id)
{
  vtkKWLabeledLabelSet::LabeledLabelSlot *labeledlabel_slot = 
    this->GetLabeledLabelSlot(id);

  if (!labeledlabel_slot)
    {
    return NULL;
    }

  return labeledlabel_slot->LabeledLabel;
}

//----------------------------------------------------------------------------
int vtkKWLabeledLabelSet::HasLabeledLabel(int id)
{
  return this->GetLabeledLabelSlot(id) ? 1 : 0;
}

//----------------------------------------------------------------------------
void vtkKWLabeledLabelSet::Create(vtkKWApplication *app, const char *args)
{
  // Set the application

  if (this->IsCreated())
    {
    vtkErrorMacro("The labeled label set is already created");
    return;
    }

  this->SetApplication(app);

  // Create the container frame

  this->Script("frame %s %s", this->GetWidgetName(), args ? args : "");

  // Update enable state

  this->UpdateEnableState();
}

//----------------------------------------------------------------------------
void vtkKWLabeledLabelSet::UpdateEnableState()
{
  this->Superclass::UpdateEnableState();

  vtkKWLabeledLabelSet::LabeledLabelSlot *labeledlabel_slot = NULL;
  vtkKWLabeledLabelSet::LabeledLabelsContainerIterator *it = 
    this->LabeledLabels->NewIterator();

  it->InitTraversal();
  while (!it->IsDoneWithTraversal())
    {
    if (it->GetData(labeledlabel_slot) == VTK_OK)
      {
      labeledlabel_slot->LabeledLabel->SetEnabled(this->Enabled);
      }
    it->GoToNextItem();
    }
  it->Delete();
}

//----------------------------------------------------------------------------
int vtkKWLabeledLabelSet::AddLabeledLabel(int id, 
                                          const char *text, 
                                          const char *text2, 
                                          const char *balloonhelp_string)
{
  // Widget must have been created

  if (!this->IsCreated())
    {
    vtkErrorMacro("The labeled label set must be created before any labeled "
                  "label is added.");
    return 0;
    }

  // Check if the new labeled label has a unique id

  if (this->HasLabeledLabel(id))
    {
    vtkErrorMacro("A labeled label with that id (" << id << ") already exists "
                  "in the labeled label set.");
    return 0;
    }

  // Add the labeled label slot to the manager

  vtkKWLabeledLabelSet::LabeledLabelSlot *labeledlabel_slot = 
    new vtkKWLabeledLabelSet::LabeledLabelSlot;
  
  if (this->LabeledLabels->AppendItem(labeledlabel_slot) != VTK_OK)
    {
    vtkErrorMacro("Error while adding a labeled label to the set.");
    delete labeledlabel_slot;
    return 0;
    }
  
  // Create the labeled label

  labeledlabel_slot->LabeledLabel = vtkKWLabeledLabel::New();
  labeledlabel_slot->Id = id;

  labeledlabel_slot->LabeledLabel->SetParent(this);
  labeledlabel_slot->LabeledLabel->Create(this->Application, 0);
  labeledlabel_slot->LabeledLabel->SetEnabled(this->Enabled);

  // Set text balloon help, if any

  if (text)
    {
    labeledlabel_slot->LabeledLabel->SetLabel(text);
    }

  if (text2)
    {
    labeledlabel_slot->LabeledLabel->SetLabel2(text2);
    }

  if (balloonhelp_string)
    {
    labeledlabel_slot->LabeledLabel->SetBalloonHelpString(balloonhelp_string);
    }

  // Pack the pushlabeledlabel

  this->Pack();

  return 1;
}

// ----------------------------------------------------------------------------
void vtkKWLabeledLabelSet::Pack()
{
  if (!this->IsCreated())
    {
    return;
    }

  ostrstream tk_cmd;

  tk_cmd << "catch {eval grid forget [grid slaves " << this->GetWidgetName() 
         << "]}" << endl;

  vtkKWLabeledLabelSet::LabeledLabelSlot *labeledlabel_slot = NULL;
  vtkKWLabeledLabelSet::LabeledLabelsContainerIterator *it = 
    this->LabeledLabels->NewIterator();

  int i = 0;
  it->InitTraversal();
  while (!it->IsDoneWithTraversal())
    {
    if (it->GetData(labeledlabel_slot) == VTK_OK)
      {
      tk_cmd << "grid " << labeledlabel_slot->LabeledLabel->GetWidgetName() 
             << " -sticky nsw -column 0 -row " << i << endl;
      i++;
      }
    it->GoToNextItem();
    }
  it->Delete();

  tk_cmd << "grid columnconfigure " 
         << this->GetWidgetName() << " 0 -weight 1" << endl;

  tk_cmd << ends;
  this->Script(tk_cmd.str());
  tk_cmd.rdbuf()->freeze(0);
}

//----------------------------------------------------------------------------
void vtkKWLabeledLabelSet::HideLabeledLabel(int id)
{
  this->SetLabeledLabelVisibility(id, 0);
}

//----------------------------------------------------------------------------
void vtkKWLabeledLabelSet::ShowLabeledLabel(int id)
{
  this->SetLabeledLabelVisibility(id, 1);
}

//----------------------------------------------------------------------------
void vtkKWLabeledLabelSet::SetLabeledLabelVisibility(int id, int flag)
{
  vtkKWLabeledLabelSet::LabeledLabelSlot *labeledlabel_slot = 
    this->GetLabeledLabelSlot(id);

  if (labeledlabel_slot && labeledlabel_slot->LabeledLabel)
    {
    this->Script("grid %s %s", 
                 (flag ? "" : "remove"),
                 labeledlabel_slot->LabeledLabel->GetWidgetName());
    }
}

//----------------------------------------------------------------------------
void vtkKWLabeledLabelSet::SetLabel(int id, const char *text)
{
  vtkKWLabeledLabelSet::LabeledLabelSlot *labeledlabel_slot = 
    this->GetLabeledLabelSlot(id);

  if (labeledlabel_slot && labeledlabel_slot->LabeledLabel)
    {
    labeledlabel_slot->LabeledLabel->SetLabel(text);
    }
}

//----------------------------------------------------------------------------
void vtkKWLabeledLabelSet::SetLabel2(int id, const char *text)
{
  vtkKWLabeledLabelSet::LabeledLabelSlot *labeledlabel_slot = 
    this->GetLabeledLabelSlot(id);

  if (labeledlabel_slot && labeledlabel_slot->LabeledLabel)
    {
    labeledlabel_slot->LabeledLabel->SetLabel2(text);
    }
}
//----------------------------------------------------------------------------
int vtkKWLabeledLabelSet::GetNumberOfVisibleLabeledLabels()
{
  if (!this->IsCreated())
    {
    return 0;
    }
  return atoi(this->Script("llength [grid slaves %s]", this->GetWidgetName()));
}

//----------------------------------------------------------------------------
void vtkKWLabeledLabelSet::SynchroniseLabelsMaximumWidth()
{
  if (!this->IsCreated())
    {
    return;
    }

  const char **labels = 
    new const char* [this->LabeledLabels->GetNumberOfItems()];

  vtkKWLabeledLabelSet::LabeledLabelSlot *labeledlabel_slot = NULL;
  vtkKWLabeledLabelSet::LabeledLabelsContainerIterator *it = 
    this->LabeledLabels->NewIterator();

  int nb = 0;
  it->InitTraversal();
  while (!it->IsDoneWithTraversal())
    {
    if (it->GetData(labeledlabel_slot) == VTK_OK)
      {
      labels[nb++] = 
        labeledlabel_slot->LabeledLabel->GetLabel()->GetWidgetName();
      }
    it->GoToNextItem();
    }
  it->Delete();

  vtkKWTkUtilities::SynchroniseLabelsMaximumWidth(
    this->Application->GetMainInterp(), nb, labels);

  delete [] labels;
}

//----------------------------------------------------------------------------
void vtkKWLabeledLabelSet::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);
}

