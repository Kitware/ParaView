/*=========================================================================

Copyright (c) 1998-2003 Kitware Inc. 469 Clifton Corporate Parkway,
Clifton Park, NY, 12065, USA.
All rights reserved.

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are met:

 * Redistributions of source code must retain the above copyright notice,
   this list of conditions and the following disclaimer.

 * Redistributions in binary form must reproduce the above copyright notice,
   this list of conditions and the following disclaimer in the documentation
   and/or other materials provided with the distribution.

 * Neither the name of Kitware nor the names of any contributors may be used
   to endorse or promote products derived from this software without specific
   prior written permission.

 * Modified source versions must be plainly marked as such, and must not be
   misrepresented as being the original software.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS ``AS IS''
AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
ARE DISCLAIMED. IN NO EVENT SHALL THE AUTHORS OR CONTRIBUTORS BE LIABLE FOR
ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

=========================================================================*/

#include "vtkKWRadioButtonSet.h"

#include "vtkKWApplication.h"
#include "vtkKWRadioButton.h"
#include "vtkLinkedList.txx"
#include "vtkLinkedListIterator.txx"
#include "vtkObjectFactory.h"

//----------------------------------------------------------------------------

vtkStandardNewMacro(vtkKWRadioButtonSet);
vtkCxxRevisionMacro(vtkKWRadioButtonSet, "1.11");

int vtkvtkKWRadioButtonSetCommand(ClientData cd, Tcl_Interp *interp,
                                  int argc, char *argv[]);

//----------------------------------------------------------------------------
vtkKWRadioButtonSet::vtkKWRadioButtonSet()
{
  this->PackHorizontally = 0;
  this->Buttons = vtkKWRadioButtonSet::ButtonsContainer::New();
}

//----------------------------------------------------------------------------
vtkKWRadioButtonSet::~vtkKWRadioButtonSet()
{
  // Delete all radiobuttons

  vtkKWRadioButtonSet::ButtonSlot *button_slot = NULL;
  vtkKWRadioButtonSet::ButtonsContainerIterator *it = 
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
vtkKWRadioButtonSet::ButtonSlot* 
vtkKWRadioButtonSet::GetButtonSlot(int id)
{
  vtkKWRadioButtonSet::ButtonSlot *button_slot = NULL;
  vtkKWRadioButtonSet::ButtonSlot *found = NULL;
  vtkKWRadioButtonSet::ButtonsContainerIterator *it = 
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
vtkKWRadioButton* vtkKWRadioButtonSet::GetButton(int id)
{
  vtkKWRadioButtonSet::ButtonSlot *button_slot = 
    this->GetButtonSlot(id);

  if (!button_slot)
    {
    return NULL;
    }

  return button_slot->Button;
}

//----------------------------------------------------------------------------
int vtkKWRadioButtonSet::HasButton(int id)
{
  return this->GetButtonSlot(id) ? 1 : 0;
}

//----------------------------------------------------------------------------
void vtkKWRadioButtonSet::Create(vtkKWApplication *app, const char *args)
{
  // Set the application

  if (this->IsCreated())
    {
    vtkErrorMacro("The radiobutton set is already created");
    return;
    }

  this->SetApplication(app);

  // Create the container frame

  this->Script("frame %s %s", this->GetWidgetName(), args ? args : "");

  // Update enable state

  this->UpdateEnableState();
}

//----------------------------------------------------------------------------
void vtkKWRadioButtonSet::UpdateEnableState()
{
  this->Superclass::UpdateEnableState();

  vtkKWRadioButtonSet::ButtonSlot *button_slot = NULL;
  vtkKWRadioButtonSet::ButtonsContainerIterator *it = 
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
int vtkKWRadioButtonSet::AddButton(int id, 
                                   const char *text, 
                                   vtkKWObject *object, 
                                   const char *method_and_arg_string,
                                   const char *balloonhelp_string)
{
  // Widget must have been created

  if (!this->IsCreated())
    {
    vtkErrorMacro("The radiobutton set must be created before any button "
                  "is added.");
    return 0;
    }

  // Check if the new radiobutton has a unique id

  if (this->HasButton(id))
    {
    vtkErrorMacro("A radiobutton with that id (" << id << ") already exists "
                  "in the radiobutton set.");
    return 0;
    }

  // Add the radiobutton slot to the manager

  vtkKWRadioButtonSet::ButtonSlot *button_slot = 
    new vtkKWRadioButtonSet::ButtonSlot;

  if (this->Buttons->AppendItem(button_slot) != VTK_OK)
    {
    vtkErrorMacro("Error while adding a radiobutton to the set.");
    delete button_slot;
    return 0;
    }
  
  // Create the radiobutton

  button_slot->Button = vtkKWRadioButton::New();
  button_slot->Id = id;

  button_slot->Button->SetParent(this);
  button_slot->Button->Create(this->Application, 0);
  button_slot->Button->SetValue(id);
  button_slot->Button->SetEnabled(this->Enabled);

  // All radiobuttons share the same var name

  if (this->Buttons->GetNumberOfItems())
    {
    vtkKWRadioButtonSet::ButtonSlot *first_slot = 0;
    if (this->Buttons->GetItem(0, first_slot) == VTK_OK)
      {
      button_slot->Button->SetVariableName(
        first_slot->Button->GetVariableName());
      }
    }

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

  // Pack the pushbutton

  this->Pack();

  return 1;
}

// ----------------------------------------------------------------------------
void vtkKWRadioButtonSet::Pack()
{
  if (!this->IsCreated())
    {
    return;
    }

  ostrstream tk_cmd;

  tk_cmd << "catch {eval grid forget [grid slaves " << this->GetWidgetName() 
         << "]}" << endl;

  vtkKWRadioButtonSet::ButtonSlot *button_slot = NULL;
  vtkKWRadioButtonSet::ButtonsContainerIterator *it = 
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
void vtkKWRadioButtonSet::SetPackHorizontally(int _arg)
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
void vtkKWRadioButtonSet::SelectButton(int id)
{
  vtkKWRadioButtonSet::ButtonSlot *button_slot = 
    this->GetButtonSlot(id);

  if (button_slot && button_slot->Button)
    {
    button_slot->Button->StateOn();
    }
}

//----------------------------------------------------------------------------
int vtkKWRadioButtonSet::IsButtonSelected(int id)
{
  vtkKWRadioButtonSet::ButtonSlot *button_slot = 
    this->GetButtonSlot(id);

  return (button_slot && 
          button_slot->Button && 
          button_slot->Button->GetState()) ? 1 : 0;
}

//----------------------------------------------------------------------------
int vtkKWRadioButtonSet::IsAnyButtonSelected()
{
  vtkKWRadioButtonSet::ButtonSlot *button_slot = NULL;
  vtkKWRadioButtonSet::ButtonSlot *found = NULL;
  vtkKWRadioButtonSet::ButtonsContainerIterator *it = 
    this->Buttons->NewIterator();

  it->InitTraversal();
  while (!it->IsDoneWithTraversal())
    {
    if (it->GetData(button_slot) == VTK_OK)
      {
      if (this->IsButtonSelected(button_slot->Id))
        {
        found = button_slot;
        break;
        }
      }
    it->GoToNextItem();
    }
  it->Delete();

  return found ? 1 : 0;
}

//----------------------------------------------------------------------------
int vtkKWRadioButtonSet::IsAnyVisibleButtonSelected()
{
  vtkKWRadioButtonSet::ButtonSlot *button_slot = NULL;
  vtkKWRadioButtonSet::ButtonSlot *found = NULL;
  vtkKWRadioButtonSet::ButtonsContainerIterator *it = 
    this->Buttons->NewIterator();

  it->InitTraversal();
  while (!it->IsDoneWithTraversal())
    {
    if (it->GetData(button_slot) == VTK_OK)
      {
      if (this->IsButtonSelected(button_slot->Id) &&
          this->GetButtonVisibility(button_slot->Id))
        {
        found = button_slot;
        break;
        }
      }
    it->GoToNextItem();
    }
  it->Delete();

  return found ? 1 : 0;
}

//----------------------------------------------------------------------------
void vtkKWRadioButtonSet::HideButton(int id)
{
  this->SetButtonVisibility(id, 0);
}

//----------------------------------------------------------------------------
void vtkKWRadioButtonSet::ShowButton(int id)
{
  this->SetButtonVisibility(id, 1);
}

//----------------------------------------------------------------------------
void vtkKWRadioButtonSet::SetButtonVisibility(int id, int flag)
{
  vtkKWRadioButtonSet::ButtonSlot *button_slot = 
    this->GetButtonSlot(id);

  if (button_slot && button_slot->Button)
    {
    this->Script("grid %s %s", 
                 (flag ? "" : "remove"),
                 button_slot->Button->GetWidgetName());
    }
}

//----------------------------------------------------------------------------
int vtkKWRadioButtonSet::GetButtonVisibility(int id)
{
  vtkKWRadioButtonSet::ButtonSlot *button_slot = 
    this->GetButtonSlot(id);

  if (button_slot && button_slot->Button && this->Application)
    {
    const char *res = 
      this->Script("grid info %s", button_slot->Button->GetWidgetName());
    return (res && *res) ? 1 : 0;
    }

  return 0;
}

//----------------------------------------------------------------------------
int vtkKWRadioButtonSet::GetNumberOfVisibleButtons()
{
  if (!this->IsCreated())
    {
    return 0;
    }
  return atoi(this->Script("llength [grid slaves %s]", this->GetWidgetName()));
}

//----------------------------------------------------------------------------
void vtkKWRadioButtonSet::SelectFirstVisibleButton()
{
  vtkKWRadioButtonSet::ButtonSlot *button_slot = NULL;
  vtkKWRadioButtonSet::ButtonsContainerIterator *it = 
    this->Buttons->NewIterator();

  it->InitTraversal();
  while (!it->IsDoneWithTraversal())
    {
    if (it->GetData(button_slot) == VTK_OK)
      {
      if (this->GetButtonVisibility(button_slot->Id))
        {
        this->SelectButton(button_slot->Id);
        break;
        }
      }
    it->GoToNextItem();
    }
  it->Delete();
}

//----------------------------------------------------------------------------
void vtkKWRadioButtonSet::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);

  os << indent << "PackHorizontally: " 
     << (this->PackHorizontally ? "On" : "Off") << endl;
}

