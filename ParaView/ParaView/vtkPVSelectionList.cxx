/*=========================================================================

  Program:   ParaView
  Module:    vtkPVSelectionList.cxx
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
#include "vtkPVSelectionList.h"
#include "vtkStringList.h"

int vtkPVSelectionListCommand(ClientData cd, Tcl_Interp *interp,
		     int argc, char *argv[]);


//----------------------------------------------------------------------------
vtkPVSelectionList::vtkPVSelectionList()
{
  this->CommandFunction = vtkPVSelectionListCommand;

  this->CurrentValue = 0;
  this->CurrentName = NULL;
  this->Command = NULL;
  
  this->MenuButton = vtkKWMenuButton::New();

  this->Names = vtkStringList::New();
}

//----------------------------------------------------------------------------
vtkPVSelectionList::~vtkPVSelectionList()
{
  this->SetCurrentName(NULL);
  
  this->MenuButton->Delete();
  this->MenuButton = NULL;
  this->Names->Delete();
  this->Names = NULL;

  if (this->Command)
    {
    delete [] this->Command;
    this->Command = NULL;
    }
}

//----------------------------------------------------------------------------
vtkPVSelectionList* vtkPVSelectionList::New()
{
  return new vtkPVSelectionList();
}

//----------------------------------------------------------------------------
int vtkPVSelectionList::Create(vtkKWApplication *app)
{
  if (this->Application != NULL)
    {
    vtkErrorMacro("Object has already been created.");
    return 0;
    }
  this->SetApplication(app);
  
  // create the top level
  this->Script("frame %s", this->GetWidgetName());

  this->MenuButton->SetParent(this);
  this->MenuButton->Create(app, "");
  this->Script("pack %s -side left", this->MenuButton->GetWidgetName());

  return 1;
}

//----------------------------------------------------------------------------
void vtkPVSelectionList::SetCommand(vtkKWObject *o, const char *method)
{
  if (this->Command)
    {
    delete [] this->Command;
    this->Command = NULL;
    }
  if (o != NULL || method != NULL)
    {
    ostrstream event;
    event << o->GetTclName() << " " << method << ends;
    this->Command = event.str();
    }
}

//----------------------------------------------------------------------------
void vtkPVSelectionList::AddItem(const char *name, int value)
{
  char tmp[1024];
  
  // Save for internal use
  this->Names->SetString(value, name);

  sprintf(tmp, "SelectCallback {%s} %d", name, value);
  this->MenuButton->AddCommand(name, this, tmp);
  
  if (value == this->CurrentValue)
    {
    this->MenuButton->SetButtonText(name);
    }
}

//----------------------------------------------------------------------------
void vtkPVSelectionList::SetCurrentValue(int value)
{
  char *name;

  if (this->CurrentValue == value)
    {
    return;
    }
  this->Modified();
  this->CurrentValue = value;
  name = this->Names->GetString(value);
  if (name)
    {
    this->SelectCallback(name, value);
    }
}

//----------------------------------------------------------------------------
void vtkPVSelectionList::SelectCallback(const char *name, int value)
{
  this->CurrentValue = value;
  this->SetCurrentName(name);
  
  this->MenuButton->SetButtonText(name);
  if (this->Command)
    {
    this->Script(this->Command);
    }
}

