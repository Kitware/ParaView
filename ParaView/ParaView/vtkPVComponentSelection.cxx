/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkPVComponentSelection.cxx
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
#include "vtkPVComponentSelection.h"
#include "vtkObjectFactory.h"
#include "vtkKWCheckButton.h"
#include "vtkKWWidgetCollection.h"

//---------------------------------------------------------------------------
vtkPVComponentSelection* vtkPVComponentSelection::New()
{
  // First try to create the object from the vtkObjectFactory
  vtkObject* ret = vtkObjectFactory::CreateInstance("vtkPVComponentSelection");
  if (ret)
    {
    return (vtkPVComponentSelection*)ret;
    }
  // If the factory was unable to create the object, then create it here.
  return new vtkPVComponentSelection;
}

//---------------------------------------------------------------------------
vtkPVComponentSelection::vtkPVComponentSelection()
{
  this->CheckButtons = vtkKWWidgetCollection::New();
  this->PVSource = NULL;
  this->Initialized = 0;
  this->ObjectTclName = NULL;
  this->VariableName = NULL;
}

//---------------------------------------------------------------------------
vtkPVComponentSelection::~vtkPVComponentSelection()
{
  this->CheckButtons->Delete();
  this->SetObjectTclName(NULL);
  this->SetVariableName(NULL);
}

//---------------------------------------------------------------------------
void vtkPVComponentSelection::Create(vtkKWApplication *pvApp,
                                     int numComponents, char *help)
{
  const char* wname;
  int i;
  vtkKWCheckButton *button;
  char compId[10];
  
  if (this->Application)
    {
    vtkErrorMacro("ComponentSelection already created");
    return;
    }
  
  this->SetApplication(pvApp);
  
  // create the top level
  wname = this->GetWidgetName();
  this->Script("frame %s -borderwidth 0 -relief flat", wname);
  
  for (i = 0; i <= numComponents; i++)
    {
    button = vtkKWCheckButton::New();
    button->SetParent(this);
    button->Create(pvApp, "");
    button->SetCommand(this, "ModifiedCallback");
    button->SetState(1);
    sprintf(compId, "%d", i);
    button->SetText(compId);
    this->CheckButtons->AddItem(button);
    this->Script("pack %s", button->GetWidgetName());
    button->Delete();
    }
}

//---------------------------------------------------------------------------
void vtkPVComponentSelection::Accept()
{
  vtkPVApplication *pvApp = this->GetPVApplication();
  int i;
  
  if (this->ModifiedFlag)
    {
    pvApp->BroadcastScript("%s RemoveAllValues", this->ObjectTclName);
    for (i = 0; i < this->CheckButtons->GetNumberOfItems(); i++)
      {
      this->AddTraceEntry("$kw(%s) SetState %d %d", this->GetTclName(),
                          i, this->GetState(i));
      if (this->GetState(i))
        {
        pvApp->BroadcastScript("%s Set%s %d %d",
                               this->ObjectTclName, this->VariableName,
                               i, i);
        }
      }
    }
  
  
  this->ModifiedFlag = 0;
}

//---------------------------------------------------------------------------
void vtkPVComponentSelection::Reset()
{
  if ( ! this->ModifiedFlag)
    {
    return;
    }
  
  if ( ! this->Initialized)
    {
    this->Initialized = 1;
    return;
    }
  
  int i;
  
  for (i = 0; i < this->CheckButtons->GetNumberOfItems(); i++)
    {
    this->Script("%s SetState %d [%s GetValue %d]",
                 this->GetTclName, i, this->PVSource->GetVTKSourceTclName(),
                 i);
    }
  
  this->ModifiedFlag = 0;
}

//---------------------------------------------------------------------------
void vtkPVComponentSelection::SetState(int i, int state)
{
  if (i > this->CheckButtons->GetNumberOfItems()-1)
    {
    return;
    }
  
  ((vtkKWCheckButton*)this->CheckButtons->GetItemAsObject(i))->SetState(state);
}

//---------------------------------------------------------------------------
int vtkPVComponentSelection::GetState(int i)
{
  if (i > this->CheckButtons->GetNumberOfItems()-1)
    {
    return -1;
    }
  
  return ((vtkKWCheckButton*)this->CheckButtons->GetItemAsObject(i))->
    GetState();
}

//---------------------------------------------------------------------------
void vtkPVComponentSelection::SetObjectVariable(const char *objName,
                                                const char *varName)
{
  this->SetObjectTclName(objName);
  this->SetVariableName(varName);
}
