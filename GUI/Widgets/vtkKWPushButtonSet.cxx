/*=========================================================================

  Module:    vtkKWPushButtonSet.cxx

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include "vtkKWPushButtonSet.h"

#include "vtkKWApplication.h"
#include "vtkKWPushButton.h"
#include "vtkLinkedList.txx"
#include "vtkLinkedListIterator.txx"
#include "vtkObjectFactory.h"

//----------------------------------------------------------------------------

vtkStandardNewMacro(vtkKWPushButtonSet);
vtkCxxRevisionMacro(vtkKWPushButtonSet, "1.11");

int vtkvtkKWPushButtonSetCommand(ClientData cd, Tcl_Interp *interp,
                                  int argc, char *argv[]);

//----------------------------------------------------------------------------
vtkKWPushButtonSet::vtkKWPushButtonSet()
{
  this->PackHorizontally = 0;
  this->MaximumNumberOfWidgetInPackingDirection = 0;
  this->PadX = 0;
  this->PadY = 0;
  this->Buttons = vtkKWPushButtonSet::ButtonsContainer::New();
}

//----------------------------------------------------------------------------
vtkKWPushButtonSet::~vtkKWPushButtonSet()
{
  this->DeleteAllButtons();

  // Delete the container

  this->Buttons->Delete();
}

//----------------------------------------------------------------------------
void vtkKWPushButtonSet::DeleteAllButtons()
{
  // Delete all pushbuttons

  vtkKWPushButtonSet::ButtonSlot *button_slot = NULL;
  vtkKWPushButtonSet::ButtonsContainerIterator *it = 
    this->Buttons->NewIterator();

  it->InitTraversal();
  while (!it->IsDoneWithTraversal())
    {
    if (it->GetData(button_slot) == VTK_OK)
      {
      if (button_slot->Button)
        {
        button_slot->Button->Delete();
        button_slot->Button = NULL;
        }
      delete button_slot;
      }
    it->GoToNextItem();
    }
  it->Delete();

  this->Buttons->RemoveAllItems();
}

//----------------------------------------------------------------------------
vtkKWPushButtonSet::ButtonSlot* 
vtkKWPushButtonSet::GetButtonSlot(int id)
{
  vtkKWPushButtonSet::ButtonSlot *button_slot = NULL;
  vtkKWPushButtonSet::ButtonSlot *found = NULL;
  vtkKWPushButtonSet::ButtonsContainerIterator *it = 
    this->Buttons->NewIterator();

  it->InitTraversal();
  while (!it->IsDoneWithTraversal())
    {
    if (it->GetData(button_slot) == VTK_OK && button_slot->Id == id)
      {
      found = button_slot;
      break;
      }
    it->GoToNextItem();
    }
  it->Delete();

  return found;
}

//----------------------------------------------------------------------------
vtkKWPushButton* vtkKWPushButtonSet::GetButton(int id)
{
  vtkKWPushButtonSet::ButtonSlot *button_slot = 
    this->GetButtonSlot(id);

  if (!button_slot)
    {
    return NULL;
    }

  return button_slot->Button;
}

//----------------------------------------------------------------------------
int vtkKWPushButtonSet::HasButton(int id)
{
  return this->GetButtonSlot(id) ? 1 : 0;
}

//----------------------------------------------------------------------------
void vtkKWPushButtonSet::Create(vtkKWApplication *app, const char *args)
{
  // Set the application

  if (this->IsCreated())
    {
    vtkErrorMacro("The pushbutton set is already created");
    return;
    }

  this->SetApplication(app);

  // Create the container frame

  this->Script("frame %s %s", this->GetWidgetName(), args ? args : "");

  // Update enable state

  this->UpdateEnableState();
}

//----------------------------------------------------------------------------
void vtkKWPushButtonSet::UpdateEnableState()
{
  this->Superclass::UpdateEnableState();

  vtkKWPushButtonSet::ButtonSlot *button_slot = NULL;
  vtkKWPushButtonSet::ButtonsContainerIterator *it = 
    this->Buttons->NewIterator();

  it->InitTraversal();
  while (!it->IsDoneWithTraversal())
    {
    if (it->GetData(button_slot) == VTK_OK)
      {
      button_slot->Button->SetEnabled(this->Enabled);
      }
    it->GoToNextItem();
    }
  it->Delete();
}

//----------------------------------------------------------------------------
int vtkKWPushButtonSet::AddButton(int id, 
                                  const char *text, 
                                  vtkKWObject *object, 
                                  const char *method_and_arg_string,
                                  const char *balloonhelp_string)
{
  // Widget must have been created

  if (!this->IsCreated())
    {
    vtkErrorMacro("The pushbutton set must be created before any button "
                  "is added.");
    return 0;
    }

  // Check if the new pushbutton has a unique id

  if (this->HasButton(id))
    {
    vtkErrorMacro("A pushbutton with that id (" << id << ") already exists "
                  "in the pushbutton set.");
    return 0;
    }

  // Add the pushbutton slot to the manager

  vtkKWPushButtonSet::ButtonSlot *button_slot = 
    new vtkKWPushButtonSet::ButtonSlot;

  if (this->Buttons->AppendItem(button_slot) != VTK_OK)
    {
    vtkErrorMacro("Error while adding a pushbutton to the set.");
    delete button_slot;
    return 0;
    }
  
  // Create the pushbutton

  button_slot->Button = vtkKWPushButton::New();
  button_slot->Id = id;

  button_slot->Button->SetParent(this);
  button_slot->Button->Create(this->GetApplication(), 0);
  button_slot->Button->SetEnabled(this->Enabled);

  // Set text command and balloon help, if any

  if (text)
    {
    button_slot->Button->SetLabel(text);
    }

  if (object && method_and_arg_string)
    {
    button_slot->Button->SetCommand(object, method_and_arg_string);
    }

  if (balloonhelp_string)
    {
    button_slot->Button->SetBalloonHelpString(balloonhelp_string);
    }

  // Pack the pushbutton

  this->Pack();

  return 1;
}

// ----------------------------------------------------------------------------
void vtkKWPushButtonSet::Pack()
{
  if (!this->IsCreated())
    {
    return;
    }

  ostrstream tk_cmd;

  tk_cmd << "catch {eval grid forget [grid slaves " << this->GetWidgetName() 
         << "]}" << endl;

  vtkKWPushButtonSet::ButtonSlot *button_slot = NULL;
  vtkKWPushButtonSet::ButtonsContainerIterator *it = 
    this->Buttons->NewIterator();

  int col = 0;
  int row = 0;

  it->InitTraversal();
  while (!it->IsDoneWithTraversal())
    {
    if (it->GetData(button_slot) == VTK_OK)
      {
      tk_cmd << "grid " << button_slot->Button->GetWidgetName() 
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
void vtkKWPushButtonSet::SetPackHorizontally(int _arg)
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
void vtkKWPushButtonSet::SetMaximumNumberOfWidgetInPackingDirection(int _arg)
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
void vtkKWPushButtonSet::SetPadding(int x, int y)
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
void vtkKWPushButtonSet::HideButton(int id)
{
  this->SetButtonVisibility(id, 0);
}

//----------------------------------------------------------------------------
void vtkKWPushButtonSet::ShowButton(int id)
{
  this->SetButtonVisibility(id, 1);
}

//----------------------------------------------------------------------------
void vtkKWPushButtonSet::SetButtonVisibility(int id, int flag)
{
  vtkKWPushButtonSet::ButtonSlot *button_slot = 
    this->GetButtonSlot(id);

  if (button_slot && button_slot->Button)
    {
    this->Script("grid %s %s", 
                 (flag ? "" : "remove"),
                 button_slot->Button->GetWidgetName());
    }
}

//----------------------------------------------------------------------------
int vtkKWPushButtonSet::GetNumberOfVisibleButtons()
{
  if (!this->IsCreated())
    {
    return 0;
    }
  return atoi(this->Script("llength [grid slaves %s]", this->GetWidgetName()));
}

//----------------------------------------------------------------------------
void vtkKWPushButtonSet::SetBorderWidth(int bd)
{
  ostrstream tk_cmd;

  vtkKWPushButtonSet::ButtonSlot *button_slot = NULL;
  vtkKWPushButtonSet::ButtonsContainerIterator *it = 
    this->Buttons->NewIterator();

  it->InitTraversal();
  while (!it->IsDoneWithTraversal())
    {
    if (it->GetData(button_slot) == VTK_OK)
      {
      tk_cmd << button_slot->Button->GetWidgetName() 
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
void vtkKWPushButtonSet::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);

  os << indent << "PackHorizontally: " 
     << (this->PackHorizontally ? "On" : "Off") << endl;

  os << indent << "MaximumNumberOfWidgetInPackingDirection: " 
     << (this->MaximumNumberOfWidgetInPackingDirection ? "On" : "Off") << endl;
}

