/*=========================================================================

  Module:    vtkKWCheckButtonSet.cxx

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include "vtkKWCheckButtonSet.h"

#include "vtkKWApplication.h"
#include "vtkKWCheckButton.h"
#include "vtkLinkedList.txx"
#include "vtkLinkedListIterator.txx"
#include "vtkObjectFactory.h"

//----------------------------------------------------------------------------

vtkStandardNewMacro(vtkKWCheckButtonSet);
vtkCxxRevisionMacro(vtkKWCheckButtonSet, "1.8");

int vtkvtkKWCheckButtonSetCommand(ClientData cd, Tcl_Interp *interp,
                                  int argc, char *argv[]);

//----------------------------------------------------------------------------
vtkKWCheckButtonSet::vtkKWCheckButtonSet()
{
  this->PackHorizontally = 0;
  this->Buttons = vtkKWCheckButtonSet::ButtonsContainer::New();
}

//----------------------------------------------------------------------------
vtkKWCheckButtonSet::~vtkKWCheckButtonSet()
{
  // Delete all checkbuttons

  vtkKWCheckButtonSet::ButtonSlot *button_slot = NULL;
  vtkKWCheckButtonSet::ButtonsContainerIterator *it = 
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

  // Delete the container

  this->Buttons->Delete();
}

//----------------------------------------------------------------------------
vtkKWCheckButtonSet::ButtonSlot* 
vtkKWCheckButtonSet::GetButtonSlot(int id)
{
  vtkKWCheckButtonSet::ButtonSlot *button_slot = NULL;
  vtkKWCheckButtonSet::ButtonSlot *found = NULL;
  vtkKWCheckButtonSet::ButtonsContainerIterator *it = 
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
vtkKWCheckButton* vtkKWCheckButtonSet::GetButton(int id)
{
  vtkKWCheckButtonSet::ButtonSlot *button_slot = 
    this->GetButtonSlot(id);

  if (!button_slot)
    {
    return NULL;
    }

  return button_slot->Button;
}

//----------------------------------------------------------------------------
int vtkKWCheckButtonSet::HasButton(int id)
{
  return this->GetButtonSlot(id) ? 1 : 0;
}

//----------------------------------------------------------------------------
void vtkKWCheckButtonSet::Create(vtkKWApplication *app, const char *args)
{
  // Set the application

  if (this->IsCreated())
    {
    vtkErrorMacro("The checkbutton set is already created");
    return;
    }

  this->SetApplication(app);

  // Create the container frame

  this->Script("frame %s %s", this->GetWidgetName(), args ? args : "");

  // Update enable state

  this->UpdateEnableState();
}

//----------------------------------------------------------------------------
void vtkKWCheckButtonSet::UpdateEnableState()
{
  this->Superclass::UpdateEnableState();

  vtkKWCheckButtonSet::ButtonSlot *button_slot = NULL;
  vtkKWCheckButtonSet::ButtonsContainerIterator *it = 
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
int vtkKWCheckButtonSet::AddButton(int id, 
                                   const char *text, 
                                   vtkKWObject *object, 
                                   const char *method_and_arg_string,
                                   const char *balloonhelp_string)
{
  // Widget must have been created

  if (!this->IsCreated())
    {
    vtkErrorMacro("The checkbutton set must be created before any button "
                  "is added.");
    return 0;
    }

  // Check if the new checkbutton has a unique id

  if (this->HasButton(id))
    {
    vtkErrorMacro("A checkbutton with that id (" << id << ") already exists "
                  "in the checkbutton set.");
    return 0;
    }

  // Add the checkbutton slot to the manager

  vtkKWCheckButtonSet::ButtonSlot *button_slot = 
    new vtkKWCheckButtonSet::ButtonSlot;

  if (this->Buttons->AppendItem(button_slot) != VTK_OK)
    {
    vtkErrorMacro("Error while adding a checkbutton to the set.");
    delete button_slot;
    return 0;
    }
  
  // Create the checkbutton

  button_slot->Button = vtkKWCheckButton::New();
  button_slot->Id = id;

  button_slot->Button->SetParent(this);
  button_slot->Button->Create(this->GetApplication(), 0);
  button_slot->Button->SetEnabled(this->Enabled);

  // Set text command and balloon help, if any

  if (text)
    {
    button_slot->Button->SetText(text);
    }

  if (object && method_and_arg_string)
    {
    button_slot->Button->SetCommand(object, method_and_arg_string);
    }

  if (balloonhelp_string)
    {
    button_slot->Button->SetBalloonHelpString(balloonhelp_string);
    }

  // Pack the button

  this->Pack();

  return 1;
}

// ----------------------------------------------------------------------------
void vtkKWCheckButtonSet::Pack()
{
  if (!this->IsCreated())
    {
    return;
    }

  ostrstream tk_cmd;

  tk_cmd << "catch {eval grid forget [grid slaves " << this->GetWidgetName() 
         << "]}" << endl;

  vtkKWCheckButtonSet::ButtonSlot *button_slot = NULL;
  vtkKWCheckButtonSet::ButtonsContainerIterator *it = 
    this->Buttons->NewIterator();

  int i = 0;
  it->InitTraversal();
  while (!it->IsDoneWithTraversal())
    {
    if (it->GetData(button_slot) == VTK_OK)
      {
      tk_cmd << "grid " << button_slot->Button->GetWidgetName() 
             << " -sticky " << (this->PackHorizontally ? "ews" : "nsw")
             << " -column " << (this->PackHorizontally ? i : 0)
             << " -row " << (this->PackHorizontally ? 0 : i)
             << endl;
      i++;
      }
    it->GoToNextItem();
    }
  it->Delete();

  tk_cmd << "grid " << (this->PackHorizontally ? "row" : "column") 
         << "configure " << this->GetWidgetName() << " 0 -weight 1" << endl;

  tk_cmd << ends;
  this->Script(tk_cmd.str());
  tk_cmd.rdbuf()->freeze(0);
}

// ----------------------------------------------------------------------------
void vtkKWCheckButtonSet::SetPackHorizontally(int _arg)
{
  if (this->PackHorizontally == _arg)
    {
    return;
    }
  this->PackHorizontally = _arg;
  this->Modified();

  this->Pack();
}

//----------------------------------------------------------------------------
void vtkKWCheckButtonSet::SelectButton(int id)
{
  vtkKWCheckButtonSet::ButtonSlot *button_slot = 
    this->GetButtonSlot(id);

  if (button_slot && button_slot->Button)
    {
    button_slot->Button->SetState(1);
    }
}

//----------------------------------------------------------------------------
void vtkKWCheckButtonSet::DeselectButton(int id)
{
  vtkKWCheckButtonSet::ButtonSlot *button_slot = 
    this->GetButtonSlot(id);

  if (button_slot && button_slot->Button)
    {
    button_slot->Button->SetState(0);
    }
}

//----------------------------------------------------------------------------
int vtkKWCheckButtonSet::IsButtonSelected(int id)
{
  vtkKWCheckButtonSet::ButtonSlot *button_slot = 
    this->GetButtonSlot(id);

  return (button_slot && 
          button_slot->Button && 
          button_slot->Button->GetState()) ? 1 : 0;
}

//----------------------------------------------------------------------------
void vtkKWCheckButtonSet::SetButtonState(int id, int state)
{
  if (state)
    {
    this->SelectButton(id);
    }
  else
    {
    this->DeselectButton(id);
    }
}

//----------------------------------------------------------------------------
void vtkKWCheckButtonSet::SelectAllButtons()
{
  vtkKWCheckButtonSet::ButtonSlot *button_slot = NULL;
  vtkKWCheckButtonSet::ButtonsContainerIterator *it = 
    this->Buttons->NewIterator();

  it->InitTraversal();
  while (!it->IsDoneWithTraversal())
    {
    if (it->GetData(button_slot) == VTK_OK)
      {
      button_slot->Button->SetState(1);
      }
    it->GoToNextItem();
    }
  it->Delete();
}

//----------------------------------------------------------------------------
void vtkKWCheckButtonSet::DeselectAllButtons()
{
  vtkKWCheckButtonSet::ButtonSlot *button_slot = NULL;
  vtkKWCheckButtonSet::ButtonsContainerIterator *it = 
    this->Buttons->NewIterator();

  it->InitTraversal();
  while (!it->IsDoneWithTraversal())
    {
    if (it->GetData(button_slot) == VTK_OK)
      {
      button_slot->Button->SetState(0);
      }
    it->GoToNextItem();
    }
  it->Delete();
}

//----------------------------------------------------------------------------
void vtkKWCheckButtonSet::HideButton(int id)
{
  this->SetButtonVisibility(id, 0);
}

//----------------------------------------------------------------------------
void vtkKWCheckButtonSet::ShowButton(int id)
{
  this->SetButtonVisibility(id, 1);
}

//----------------------------------------------------------------------------
void vtkKWCheckButtonSet::SetButtonVisibility(int id, int flag)
{
  vtkKWCheckButtonSet::ButtonSlot *button_slot = 
    this->GetButtonSlot(id);

  if (button_slot && button_slot->Button)
    {
    this->Script("grid %s %s", 
                 (flag ? "" : "remove"),
                 button_slot->Button->GetWidgetName());
    }
}

//----------------------------------------------------------------------------
int vtkKWCheckButtonSet::GetNumberOfVisibleButtons()
{
  if (!this->IsCreated())
    {
    return 0;
    }
  return atoi(this->Script("llength [grid slaves %s]", this->GetWidgetName()));
}

//----------------------------------------------------------------------------
void vtkKWCheckButtonSet::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);

  os << indent << "PackHorizontally: " 
     << (this->PackHorizontally ? "On" : "Off") << endl;
}


