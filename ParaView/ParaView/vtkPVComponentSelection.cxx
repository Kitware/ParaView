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

#include "vtkArrayMap.txx"
#include "vtkKWCheckButton.h"
#include "vtkKWWidgetCollection.h"
#include "vtkObjectFactory.h"
#include "vtkPVApplication.h"
#include "vtkPVXMLElement.h"
#include "vtkPVProcessModule.h"
#include "vtkPVSource.h"

//---------------------------------------------------------------------------
vtkStandardNewMacro(vtkPVComponentSelection);
vtkCxxRevisionMacro(vtkPVComponentSelection, "1.10");

//---------------------------------------------------------------------------
vtkPVComponentSelection::vtkPVComponentSelection()
{
  this->CheckButtons = vtkKWWidgetCollection::New();
  this->Initialized = 0;
  this->ObjectTclName = NULL;
  this->VariableName = NULL;
  this->NumberOfComponents = 0;
  this->LastAcceptedState = 0;
}

//---------------------------------------------------------------------------
vtkPVComponentSelection::~vtkPVComponentSelection()
{
  this->CheckButtons->Delete();
  this->SetObjectTclName(NULL);
  this->SetVariableName(NULL);
  if (this->LastAcceptedState)
    {
    delete [] this->LastAcceptedState;
    this->LastAcceptedState = NULL;
    }
}

//---------------------------------------------------------------------------
void vtkPVComponentSelection::Create(vtkKWApplication *app)
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
  
  this->SetApplication(app);
  vtkPVApplication *pvApp = this->GetPVApplication();
  
  // create the top level
  wname = this->GetWidgetName();
  this->Script("frame %s -borderwidth 0 -relief flat", wname);

  this->LastAcceptedState = new unsigned char[this->NumberOfComponents];
  
  for (i = 0; i <= this->NumberOfComponents; i++)
    {
    button = vtkKWCheckButton::New();
    button->SetParent(this);
    button->Create(app, "");
    button->SetCommand(this, "ModifiedCallback");
    button->SetState(1);
    sprintf(compId, "%d", i);
    button->SetText(compId);
    this->CheckButtons->AddItem(button);
    pvApp->GetProcessModule()->ServerScript(
      "%s Set%s %d %d", this->ObjectTclName, this->VariableName, i, i);
    this->Script("pack %s", button->GetWidgetName());
    button->Delete();
    this->LastAcceptedState[i] = 1;
    }
}


//---------------------------------------------------------------------------
void vtkPVComponentSelection::Trace(ofstream *file)
{
  int i;
  
  if ( ! this->InitializeTrace(file))
    {
    return;
    }

  for (i = 0; i < this->CheckButtons->GetNumberOfItems(); i++)
    {
    *file << "$kw(" << this->GetTclName() << ") SetState "
          << i << " " << this->GetState(i) << endl;
    }
}

//---------------------------------------------------------------------------
void vtkPVComponentSelection::AcceptInternal(const char* sourceTclName)
{
  vtkPVApplication *pvApp = this->GetPVApplication();
  int i;
  
  if (this->ModifiedFlag)
    {
    pvApp->GetProcessModule()->ServerScript(
      "%s RemoveAllValues", sourceTclName);
    for (i = 0; i < this->CheckButtons->GetNumberOfItems(); i++)
      {
      if (this->GetState(i))
        {
        pvApp->GetProcessModule()->ServerScript(
          "%s Set%s %d %d", sourceTclName, this->VariableName, i, i);
        }
      else
        {
        pvApp->GetProcessModule()->ServerScript(
          "%s Set%s %d %d", sourceTclName, this->VariableName, i, -1);
        }
      this->LastAcceptedState[i] = this->GetState(i);
      }
    }
  
  
  this->ModifiedFlag = 0;
}

//---------------------------------------------------------------------------
void vtkPVComponentSelection::ResetInternal()
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
    this->SetState(i, this->LastAcceptedState[i]);
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

vtkPVComponentSelection* vtkPVComponentSelection::ClonePrototype(
  vtkPVSource* pvSource, vtkArrayMap<vtkPVWidget*, vtkPVWidget*>* map)
{
  vtkPVWidget* clone = this->ClonePrototypeInternal(pvSource, map);
  return vtkPVComponentSelection::SafeDownCast(clone);
}

void vtkPVComponentSelection::CopyProperties(vtkPVWidget* clone, 
                                             vtkPVSource* pvSource,
                              vtkArrayMap<vtkPVWidget*, vtkPVWidget*>* map)
{
  this->Superclass::CopyProperties(clone, pvSource, map);
  vtkPVComponentSelection* pvcs = vtkPVComponentSelection::SafeDownCast(clone);
  if (pvcs)
    {
    pvcs->SetNumberOfComponents(this->NumberOfComponents);
    }
  else 
    {
    vtkErrorMacro("Internal error. Could not downcast clone to PVComponentSelection.");
    }
}

//----------------------------------------------------------------------------
int vtkPVComponentSelection::ReadXMLAttributes(vtkPVXMLElement* element,
                                               vtkPVXMLPackageParser* parser)
{
  if(!this->Superclass::ReadXMLAttributes(element, parser)) { return 0; }
  
  if(!element->GetScalarAttribute("number_of_components",
                                  &this->NumberOfComponents))
    {
    this->NumberOfComponents = 1;
    }
  
  return 1;
}


//----------------------------------------------------------------------------
void vtkPVComponentSelection::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  os << "NumberOfComponents: " << this->NumberOfComponents << endl;
}
