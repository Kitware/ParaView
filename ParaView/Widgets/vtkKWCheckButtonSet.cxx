/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkKWCheckButtonSet.cxx
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

#include "vtkKWCheckButtonSet.h"

#include "vtkKWApplication.h"
#include "vtkKWCheckButton.h"
#include "vtkLinkedList.txx"
#include "vtkLinkedListIterator.txx"
#include "vtkObjectFactory.h"

//------------------------------------------------------------------------------

vtkStandardNewMacro(vtkKWCheckButtonSet);
vtkCxxRevisionMacro(vtkKWCheckButtonSet, "1.1");

int vtkvtkKWCheckButtonSetCommand(ClientData cd, Tcl_Interp *interp,
                                  int argc, char *argv[]);

//----------------------------------------------------------------------------
vtkKWCheckButtonSet::vtkKWCheckButtonSet()
{
  this->CheckButtons = vtkKWCheckButtonSet::CheckButtonsContainer::New();
}

//----------------------------------------------------------------------------
vtkKWCheckButtonSet::~vtkKWCheckButtonSet()
{
  // Delete all checkbuttons

  vtkKWCheckButtonSet::CheckButtonSlot *checkbutton_slot = NULL;
  vtkKWCheckButtonSet::CheckButtonsContainerIterator *it = 
    this->CheckButtons->NewIterator();

  it->InitTraversal();
  while (!it->IsDoneWithTraversal())
    {
    if (it->GetData(checkbutton_slot) == VTK_OK)
      {
      if (checkbutton_slot->CheckButton)
        {
        checkbutton_slot->CheckButton->Delete();
        checkbutton_slot->CheckButton = NULL;
        }
      delete checkbutton_slot;
      }
    it->GoToNextItem();
    }
  it->Delete();

  // Delete the container

  this->CheckButtons->Delete();
}

//----------------------------------------------------------------------------
vtkKWCheckButtonSet::CheckButtonSlot* 
vtkKWCheckButtonSet::GetCheckButtonSlot(int id)
{
  vtkKWCheckButtonSet::CheckButtonSlot *checkbutton_slot = NULL;
  vtkKWCheckButtonSet::CheckButtonSlot *found = NULL;
  vtkKWCheckButtonSet::CheckButtonsContainerIterator *it = 
    this->CheckButtons->NewIterator();

  it->InitTraversal();
  while (!it->IsDoneWithTraversal())
    {
    if (it->GetData(checkbutton_slot) == VTK_OK && checkbutton_slot->Id == id)
      {
      found = checkbutton_slot;
      break;
      }
    it->GoToNextItem();
    }
  it->Delete();

  return found;
}

//----------------------------------------------------------------------------
vtkKWCheckButton* vtkKWCheckButtonSet::GetCheckButton(int id)
{
  vtkKWCheckButtonSet::CheckButtonSlot *checkbutton_slot = 
    this->GetCheckButtonSlot(id);

  if (!checkbutton_slot)
    {
    return NULL;
    }

  return checkbutton_slot->CheckButton;
}

//----------------------------------------------------------------------------
int vtkKWCheckButtonSet::HasCheckButton(int id)
{
  return this->GetCheckButtonSlot(id) ? 1 : 0;
}

//------------------------------------------------------------------------------
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
}

//----------------------------------------------------------------------------
void vtkKWCheckButtonSet::SetEnabled(int arg)
{
  vtkKWCheckButtonSet::CheckButtonSlot *checkbutton_slot = NULL;
  vtkKWCheckButtonSet::CheckButtonsContainerIterator *it = 
    this->CheckButtons->NewIterator();

  it->InitTraversal();
  while (!it->IsDoneWithTraversal())
    {
    if (it->GetData(checkbutton_slot) == VTK_OK)
      {
      checkbutton_slot->CheckButton->SetEnabled(arg);
      }
    it->GoToNextItem();
    }
  it->Delete();
}

//------------------------------------------------------------------------------
int vtkKWCheckButtonSet::AddCheckButton(int id, 
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

  if (this->HasCheckButton(id))
    {
    vtkErrorMacro("A checkbutton with that id (" << id << ") already exists "
                  "in the checkbutton set.");
    return 0;
    }

  // Add the checkbutton slot to the manager

  vtkKWCheckButtonSet::CheckButtonSlot *checkbutton_slot = 
    new vtkKWCheckButtonSet::CheckButtonSlot;

  if (this->CheckButtons->AppendItem(checkbutton_slot) != VTK_OK)
    {
    vtkErrorMacro("Error while adding a checkbutton to the set.");
    delete checkbutton_slot;
    return 0;
    }
  
  // Create the checkbutton

  checkbutton_slot->CheckButton = vtkKWCheckButton::New();
  checkbutton_slot->Id = id;

  checkbutton_slot->CheckButton->SetParent(this);
  checkbutton_slot->CheckButton->Create(this->Application, 0);

  // Set text command and balloon help, if any

  if (text)
    {
    checkbutton_slot->CheckButton->SetText(text);
    }

  if (object && method_and_arg_string)
    {
    checkbutton_slot->CheckButton->SetCommand(object, method_and_arg_string);
    }

  if (balloonhelp_string)
    {
    checkbutton_slot->CheckButton->SetBalloonHelpString(balloonhelp_string);
    }

  // Pack the button

  this->Script("grid %s -column 0 -row %d -sticky nsw",
               checkbutton_slot->CheckButton->GetWidgetName(),
               this->CheckButtons->GetNumberOfItems() - 1);

  return 1;
}

//----------------------------------------------------------------------------
void vtkKWCheckButtonSet::SelectCheckButton(int id)
{
  vtkKWCheckButtonSet::CheckButtonSlot *checkbutton_slot = 
    this->GetCheckButtonSlot(id);

  if (checkbutton_slot && checkbutton_slot->CheckButton)
    {
    checkbutton_slot->CheckButton->SetState(1);
    }
}

//----------------------------------------------------------------------------
void vtkKWCheckButtonSet::DeselectCheckButton(int id)
{
  vtkKWCheckButtonSet::CheckButtonSlot *checkbutton_slot = 
    this->GetCheckButtonSlot(id);

  if (checkbutton_slot && checkbutton_slot->CheckButton)
    {
    checkbutton_slot->CheckButton->SetState(0);
    }
}

//----------------------------------------------------------------------------
int vtkKWCheckButtonSet::IsCheckButtonSelected(int id)
{
  vtkKWCheckButtonSet::CheckButtonSlot *checkbutton_slot = 
    this->GetCheckButtonSlot(id);

  return (checkbutton_slot && 
          checkbutton_slot->CheckButton && 
          checkbutton_slot->CheckButton->GetState()) ? 1 : 0;
}

//----------------------------------------------------------------------------
void vtkKWCheckButtonSet::SetCheckButtonState(int id, int state)
{
  if (state)
    {
    this->SelectCheckButton(id);
    }
  else
    {
    this->DeselectCheckButton(id);
    }
}

//----------------------------------------------------------------------------
void vtkKWCheckButtonSet::SelectAllCheckButtons()
{
  vtkKWCheckButtonSet::CheckButtonSlot *checkbutton_slot = NULL;
  vtkKWCheckButtonSet::CheckButtonsContainerIterator *it = 
    this->CheckButtons->NewIterator();

  it->InitTraversal();
  while (!it->IsDoneWithTraversal())
    {
    if (it->GetData(checkbutton_slot) == VTK_OK)
      {
      checkbutton_slot->CheckButton->SetState(1);
      }
    it->GoToNextItem();
    }
  it->Delete();
}

//----------------------------------------------------------------------------
void vtkKWCheckButtonSet::DeselectAllCheckButtons()
{
  vtkKWCheckButtonSet::CheckButtonSlot *checkbutton_slot = NULL;
  vtkKWCheckButtonSet::CheckButtonsContainerIterator *it = 
    this->CheckButtons->NewIterator();

  it->InitTraversal();
  while (!it->IsDoneWithTraversal())
    {
    if (it->GetData(checkbutton_slot) == VTK_OK)
      {
      checkbutton_slot->CheckButton->SetState(0);
      }
    it->GoToNextItem();
    }
  it->Delete();
}

//----------------------------------------------------------------------------
void vtkKWCheckButtonSet::HideCheckButton(int id)
{
  this->SetCheckButtonVisibility(id, 0);
}

//----------------------------------------------------------------------------
void vtkKWCheckButtonSet::ShowCheckButton(int id)
{
  this->SetCheckButtonVisibility(id, 1);
}

//----------------------------------------------------------------------------
void vtkKWCheckButtonSet::SetCheckButtonVisibility(int id, int flag)
{
  vtkKWCheckButtonSet::CheckButtonSlot *checkbutton_slot = 
    this->GetCheckButtonSlot(id);

  if (checkbutton_slot && checkbutton_slot->CheckButton)
    {
    this->Script("grid %s %s", 
                 (flag ? "" : "forget"),
                 checkbutton_slot->CheckButton->GetWidgetName());
    }
}

