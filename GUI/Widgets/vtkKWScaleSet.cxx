/*=========================================================================

  Module:    vtkKWScaleSet.cxx

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include "vtkKWScaleSet.h"

#include "vtkKWApplication.h"
#include "vtkKWScale.h"
#include "vtkKWLabel.h"
#include "vtkLinkedList.txx"
#include "vtkLinkedListIterator.txx"
#include "vtkObjectFactory.h"
#include "vtkKWTkUtilities.h"

//----------------------------------------------------------------------------

vtkStandardNewMacro(vtkKWScaleSet);
vtkCxxRevisionMacro(vtkKWScaleSet, "1.6");

int vtkvtkKWScaleSetCommand(ClientData cd, Tcl_Interp *interp,
                                  int argc, char *argv[]);

//----------------------------------------------------------------------------
vtkKWScaleSet::vtkKWScaleSet()
{
  this->PackHorizontally = 0;
  this->MaximumNumberOfWidgetInPackingDirection = 0;
  this->PadX = 0;
  this->PadY = 0;
  this->Scales = vtkKWScaleSet::ScalesContainer::New();
}

//----------------------------------------------------------------------------
vtkKWScaleSet::~vtkKWScaleSet()
{
  this->DeleteAllScales();

  // Delete the container

  this->Scales->Delete();
}

//----------------------------------------------------------------------------
void vtkKWScaleSet::DeleteAllScales()
{
  // Delete all scales

  vtkKWScaleSet::ScaleSlot *scale_slot = NULL;
  vtkKWScaleSet::ScalesContainerIterator *it = 
    this->Scales->NewIterator();

  it->InitTraversal();
  while (!it->IsDoneWithTraversal())
    {
    if (it->GetData(scale_slot) == VTK_OK)
      {
      if (scale_slot->Scale)
        {
        scale_slot->Scale->Delete();
        scale_slot->Scale = NULL;
        }
      delete scale_slot;
      }
    it->GoToNextItem();
    }
  it->Delete();

  this->Scales->RemoveAllItems();
}

//----------------------------------------------------------------------------
vtkKWScaleSet::ScaleSlot* 
vtkKWScaleSet::GetScaleSlot(int id)
{
  vtkKWScaleSet::ScaleSlot *scale_slot = NULL;
  vtkKWScaleSet::ScaleSlot *found = NULL;
  vtkKWScaleSet::ScalesContainerIterator *it = 
    this->Scales->NewIterator();

  it->InitTraversal();
  while (!it->IsDoneWithTraversal())
    {
    if (it->GetData(scale_slot) == VTK_OK && scale_slot->Id == id)
      {
      found = scale_slot;
      break;
      }
    it->GoToNextItem();
    }
  it->Delete();

  return found;
}

//----------------------------------------------------------------------------
vtkKWScale* vtkKWScaleSet::GetScale(int id)
{
  vtkKWScaleSet::ScaleSlot *scale_slot = 
    this->GetScaleSlot(id);

  if (!scale_slot)
    {
    return NULL;
    }

  return scale_slot->Scale;
}

//----------------------------------------------------------------------------
int vtkKWScaleSet::HasScale(int id)
{
  return this->GetScaleSlot(id) ? 1 : 0;
}

//----------------------------------------------------------------------------
void vtkKWScaleSet::Create(vtkKWApplication *app, const char *args)
{
  // Call the superclass to create the widget and set the appropriate flags

  if (!this->Superclass::Create(app, "frame", args))
    {
    vtkErrorMacro("Failed creating widget " << this->GetClassName());
    return;
    }

  // Update enable state

  this->UpdateEnableState();
}

//----------------------------------------------------------------------------
void vtkKWScaleSet::UpdateEnableState()
{
  this->Superclass::UpdateEnableState();

  vtkKWScaleSet::ScaleSlot *scale_slot = NULL;
  vtkKWScaleSet::ScalesContainerIterator *it = 
    this->Scales->NewIterator();

  it->InitTraversal();
  while (!it->IsDoneWithTraversal())
    {
    if (it->GetData(scale_slot) == VTK_OK)
      {
      scale_slot->Scale->SetEnabled(this->Enabled);
      }
    it->GoToNextItem();
    }
  it->Delete();
}

//----------------------------------------------------------------------------
int vtkKWScaleSet::AddScale(int id, 
                            vtkKWObject *object, 
                            const char *method_and_arg_string,
                            const char *balloonhelp_string)
{
  // Widget must have been created

  if (!this->IsCreated())
    {
    vtkErrorMacro("The scale set must be created before any scale "
                  "is added.");
    return 0;
    }

  // Check if the new scale has a unique id

  if (this->HasScale(id))
    {
    vtkErrorMacro("A scale with that id (" << id << ") already exists "
                  "in the scale set.");
    return 0;
    }

  // Add the scale slot to the manager

  vtkKWScaleSet::ScaleSlot *scale_slot = 
    new vtkKWScaleSet::ScaleSlot;

  if (this->Scales->AppendItem(scale_slot) != VTK_OK)
    {
    vtkErrorMacro("Error while adding a scale to the set.");
    delete scale_slot;
    return 0;
    }
  
  // Create the scale

  scale_slot->Scale = vtkKWScale::New();
  scale_slot->Id = id;

  scale_slot->Scale->SetParent(this);
  scale_slot->Scale->Create(this->GetApplication(), 0);
  scale_slot->Scale->SetEnabled(this->Enabled);

  // Set command and balloon help, if any

  if (object && method_and_arg_string)
    {
    scale_slot->Scale->SetCommand(object, method_and_arg_string);
    }

  if (balloonhelp_string)
    {
    scale_slot->Scale->SetBalloonHelpString(balloonhelp_string);
    }

  // Pack the scale

  this->Pack();

  return 1;
}

// ----------------------------------------------------------------------------
void vtkKWScaleSet::Pack()
{
  if (!this->IsCreated())
    {
    return;
    }

  ostrstream tk_cmd;

  this->UnpackChildren();

  vtkKWScaleSet::ScaleSlot *scale_slot = NULL;
  vtkKWScaleSet::ScalesContainerIterator *it = 
    this->Scales->NewIterator();

  int col = 0;
  int row = 0;

  it->InitTraversal();
  while (!it->IsDoneWithTraversal())
    {
    if (it->GetData(scale_slot) == VTK_OK)
      {
      tk_cmd << "grid " << scale_slot->Scale->GetWidgetName() 
             << " -sticky news"
             << " -column " << (this->PackHorizontally ? col : row)
             << " -row " << (this->PackHorizontally ? row : col)
             << " -padx " << this->PadX
             << " -pady " << this->PadY
             << endl;
      col++;
      if (this->MaximumNumberOfWidgetInPackingDirection &&
          col >= this->MaximumNumberOfWidgetInPackingDirection)
        {
        col = 0;
        row++;
        }
      }
    it->GoToNextItem();
    }
  it->Delete();

  // Weights
  
  int i;
  int maxcol = (row > 0) ? this->MaximumNumberOfWidgetInPackingDirection : col;
  for (i = 0; i < maxcol; i++)
    {
    tk_cmd << "grid " << (this->PackHorizontally ? "column" : "row") 
           << "configure " << this->GetWidgetName() << " " 
           << i << " -weight 1" << endl;
    }

  for (i = 0; i <= row; i++)
    {
    tk_cmd << "grid " << (this->PackHorizontally ? "row" : "column") 
           << "configure " << this->GetWidgetName() << " " 
           << i << " -weight 1" << endl;
    }

  tk_cmd << ends;
  this->Script(tk_cmd.str());
  tk_cmd.rdbuf()->freeze(0);
}

// ----------------------------------------------------------------------------
void vtkKWScaleSet::SetPackHorizontally(int _arg)
{
  if (this->PackHorizontally == _arg)
    {
    return;
    }
  this->PackHorizontally = _arg;
  this->Modified();

  this->Pack();
}

// ----------------------------------------------------------------------------
void vtkKWScaleSet::SetMaximumNumberOfWidgetInPackingDirection(int _arg)
{
  if (this->MaximumNumberOfWidgetInPackingDirection == _arg)
    {
    return;
    }
  this->MaximumNumberOfWidgetInPackingDirection = _arg;
  this->Modified();

  this->Pack();
}

// ----------------------------------------------------------------------------
void vtkKWScaleSet::SetPadding(int x, int y)
{
  if (this->PadX == x && this->PadY == y)
    {
    return;
    }

  this->PadX = x;
  this->PadY = y;

  this->Modified();

  this->Pack();
}

//----------------------------------------------------------------------------
void vtkKWScaleSet::HideScale(int id)
{
  this->SetScaleVisibility(id, 0);
}

//----------------------------------------------------------------------------
void vtkKWScaleSet::ShowScale(int id)
{
  this->SetScaleVisibility(id, 1);
}

//----------------------------------------------------------------------------
void vtkKWScaleSet::SetScaleVisibility(int id, int flag)
{
  vtkKWScaleSet::ScaleSlot *scale_slot = 
    this->GetScaleSlot(id);

  if (scale_slot && scale_slot->Scale)
    {
    this->Script("grid %s %s", 
                 (flag ? "" : "remove"),
                 scale_slot->Scale->GetWidgetName());
    }
}

//----------------------------------------------------------------------------
int vtkKWScaleSet::GetNumberOfVisibleScales()
{
  if (!this->IsCreated())
    {
    return 0;
    }
  return atoi(this->Script("llength [grid slaves %s]", this->GetWidgetName()));
}

//----------------------------------------------------------------------------
void vtkKWScaleSet::SetBorderWidth(int bd)
{
  ostrstream tk_cmd;

  vtkKWScaleSet::ScaleSlot *scale_slot = NULL;
  vtkKWScaleSet::ScalesContainerIterator *it = 
    this->Scales->NewIterator();

  it->InitTraversal();
  while (!it->IsDoneWithTraversal())
    {
    if (it->GetData(scale_slot) == VTK_OK)
      {
      tk_cmd << scale_slot->Scale->GetWidgetName() 
         << " config -bd " << bd << endl;
      }
    it->GoToNextItem();
    }
  it->Delete();

  tk_cmd << ends;
  this->Script(tk_cmd.str());
  tk_cmd.rdbuf()->freeze(0);
}

//----------------------------------------------------------------------------
void vtkKWScaleSet::SynchroniseLabelsMaximumWidth()
{
  if (!this->IsCreated())
    {
    return;
    }

  const char **labels = 
    new const char* [this->Scales->GetNumberOfItems()];

  vtkKWScaleSet::ScaleSlot *scale_slot = NULL;
  vtkKWScaleSet::ScalesContainerIterator *it = this->Scales->NewIterator();

  int nb = 0;
  it->InitTraversal();
  while (!it->IsDoneWithTraversal())
    {
    if (it->GetData(scale_slot) == VTK_OK &&
        scale_slot->Scale->GetLabel() &&
        scale_slot->Scale->GetLabel()->IsCreated())
      {
      labels[nb++] = 
        scale_slot->Scale->GetLabel()->GetWidgetName();
      }
    it->GoToNextItem();
    }
  it->Delete();

  vtkKWTkUtilities::SynchroniseLabelsMaximumWidth(
    this->GetApplication()->GetMainInterp(), nb, labels);

  delete [] labels;
}

//----------------------------------------------------------------------------
void vtkKWScaleSet::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);

  os << indent << "PackHorizontally: " 
     << (this->PackHorizontally ? "On" : "Off") << endl;

  os << indent << "MaximumNumberOfWidgetInPackingDirection: " 
     << (this->MaximumNumberOfWidgetInPackingDirection ? "On" : "Off") << endl;
}

