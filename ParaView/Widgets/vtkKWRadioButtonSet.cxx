/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkKWRadioButtonSet.cxx
  Language:  C++
  Date:      $Date$
  Version:   $Revision$

Copyright (c) 2000-2001 Kitware Inc. 469 Clifton Corporate Parkway,
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

//------------------------------------------------------------------------------

vtkStandardNewMacro(vtkKWRadioButtonSet);
vtkCxxRevisionMacro(vtkKWRadioButtonSet, "1.1");

int vtkvtkKWRadioButtonSetCommand(ClientData cd, Tcl_Interp *interp,
                                  int argc, char *argv[]);

//----------------------------------------------------------------------------
vtkKWRadioButtonSet::vtkKWRadioButtonSet()
{
  this->RadioButtons = vtkKWRadioButtonSet::RadioButtonsContainer::New();
}

//----------------------------------------------------------------------------
vtkKWRadioButtonSet::~vtkKWRadioButtonSet()
{
  // Delete all radiobuttons

  vtkKWRadioButtonSet::RadioButtonSlot *radiobutton_slot = NULL;
  vtkKWRadioButtonSet::RadioButtonsContainerIterator *it = 
    this->RadioButtons->NewIterator();

  it->InitTraversal();
  while (!it->IsDoneWithTraversal())
    {
    if (it->GetData(radiobutton_slot) == VTK_OK)
      {
      if (radiobutton_slot->RadioButton)
        {
        radiobutton_slot->RadioButton->Delete();
        radiobutton_slot->RadioButton = NULL;
        }
      delete radiobutton_slot;
      }
    it->GoToNextItem();
    }
  it->Delete();

  // Delete the container

  this->RadioButtons->Delete();
}

//----------------------------------------------------------------------------
vtkKWRadioButtonSet::RadioButtonSlot* 
vtkKWRadioButtonSet::GetRadioButtonSlot(int id)
{
  vtkKWRadioButtonSet::RadioButtonSlot *radiobutton_slot = NULL;
  vtkKWRadioButtonSet::RadioButtonSlot *found = NULL;
  vtkKWRadioButtonSet::RadioButtonsContainerIterator *it = 
    this->RadioButtons->NewIterator();

  it->InitTraversal();
  while (!it->IsDoneWithTraversal())
    {
    if (it->GetData(radiobutton_slot) == VTK_OK && radiobutton_slot->Id == id)
      {
      found = radiobutton_slot;
      break;
      }
    it->GoToNextItem();
    }
  it->Delete();

  return found;
}

//----------------------------------------------------------------------------
vtkKWRadioButton* vtkKWRadioButtonSet::GetRadioButton(int id)
{
  vtkKWRadioButtonSet::RadioButtonSlot *radiobutton_slot = 
    this->GetRadioButtonSlot(id);

  if (!radiobutton_slot)
    {
    return NULL;
    }

  return radiobutton_slot->RadioButton;
}

//----------------------------------------------------------------------------
int vtkKWRadioButtonSet::HasRadioButton(int id)
{
  return this->GetRadioButtonSlot(id) ? 1 : 0;
}

//------------------------------------------------------------------------------
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
}

//----------------------------------------------------------------------------
void vtkKWRadioButtonSet::SetEnabled(int arg)
{
  vtkKWRadioButtonSet::RadioButtonSlot *radiobutton_slot = NULL;
  vtkKWRadioButtonSet::RadioButtonsContainerIterator *it = 
    this->RadioButtons->NewIterator();

  it->InitTraversal();
  while (!it->IsDoneWithTraversal())
    {
    if (it->GetData(radiobutton_slot) == VTK_OK)
      {
      radiobutton_slot->RadioButton->SetEnabled(arg);
      }
    it->GoToNextItem();
    }
  it->Delete();
}

//------------------------------------------------------------------------------
int vtkKWRadioButtonSet::AddRadioButton(int id, 
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

  if (this->HasRadioButton(id))
    {
    vtkErrorMacro("A radiobutton with that id (" << id << ") already exists "
                  "in the radiobutton set.");
    return 0;
    }

  // Add the radiobutton slot to the manager

  vtkKWRadioButtonSet::RadioButtonSlot *radiobutton_slot = 
    new vtkKWRadioButtonSet::RadioButtonSlot;

  if (this->RadioButtons->AppendItem(radiobutton_slot) != VTK_OK)
    {
    vtkErrorMacro("Error while adding a radiobutton to the set.");
    delete radiobutton_slot;
    return 0;
    }
  
  // Create the radiobutton

  radiobutton_slot->RadioButton = vtkKWRadioButton::New();
  radiobutton_slot->Id = id;

  radiobutton_slot->RadioButton->SetParent(this);
  radiobutton_slot->RadioButton->Create(this->Application, 0);
  radiobutton_slot->RadioButton->SetValue(id);

  // All radiobuttons share the same var name

  if (this->RadioButtons->GetNumberOfItems())
    {
    vtkKWRadioButtonSet::RadioButtonSlot *first_slot;
    if (this->RadioButtons->GetItem(0, first_slot) == VTK_OK)
      {
      radiobutton_slot->RadioButton->SetVariableName(
        first_slot->RadioButton->GetVariableName());
      }
    }

  // Set text command and balloon help, if any

  if (text)
    {
    radiobutton_slot->RadioButton->SetText(text);
    }

  if (object && method_and_arg_string)
    {
    radiobutton_slot->RadioButton->SetCommand(object, method_and_arg_string);
    }

  if (balloonhelp_string)
    {
    radiobutton_slot->RadioButton->SetBalloonHelpString(balloonhelp_string);
    }

  // Pack the button

  this->Script("pack %s -side top -anchor w -expand no -fill none",
               radiobutton_slot->RadioButton->GetWidgetName());

  return 1;
}

//----------------------------------------------------------------------------
void vtkKWRadioButtonSet::SelectRadioButton(int id)
{
  vtkKWRadioButtonSet::RadioButtonSlot *radiobutton_slot = 
    this->GetRadioButtonSlot(id);

  if (radiobutton_slot && radiobutton_slot->RadioButton)
    {
    radiobutton_slot->RadioButton->StateOn();
    }
}

//----------------------------------------------------------------------------
int vtkKWRadioButtonSet::IsRadioButtonSelected(int id)
{
  vtkKWRadioButtonSet::RadioButtonSlot *radiobutton_slot = 
    this->GetRadioButtonSlot(id);

  return (radiobutton_slot && 
          radiobutton_slot->RadioButton && 
          radiobutton_slot->RadioButton->GetState()) ? 1 : 0;
}

