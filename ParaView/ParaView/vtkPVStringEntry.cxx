/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkPVStringEntry.cxx
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
#include "vtkPVStringEntry.h"

#include "vtkArrayMap.txx"
#include "vtkKWEntry.h"
#include "vtkKWLabel.h"
#include "vtkObjectFactory.h"
#include "vtkPVApplication.h"
#include "vtkPVXMLElement.h"

//----------------------------------------------------------------------------
vtkPVStringEntry* vtkPVStringEntry::New()
{
  // First try to create the object from the vtkObjectFactory
  vtkObject* ret = vtkObjectFactory::CreateInstance("vtkPVStringEntry");
  if (ret)
    {
    return (vtkPVStringEntry*)ret;
    }
  // If the factory was unable to create the object, then create it here.
  return new vtkPVStringEntry;
}

//----------------------------------------------------------------------------
vtkPVStringEntry::vtkPVStringEntry()
{
  this->LabelWidget = vtkKWLabel::New();
  this->LabelWidget->SetParent(this);
  this->Entry = vtkKWEntry::New();
  this->Entry->SetParent(this);
  this->EntryLabel = 0;
}

//----------------------------------------------------------------------------
vtkPVStringEntry::~vtkPVStringEntry()
{
  this->Entry->Delete();
  this->Entry = NULL;
  this->LabelWidget->Delete();
  this->LabelWidget = NULL;
  this->SetEntryLabel(0);
}

void vtkPVStringEntry::SetLabel(const char* label)
{
  this->SetEntryLabel(label);
  this->LabelWidget->SetLabel(label);
}

void vtkPVStringEntry::SetBalloonHelpString(const char *str)
{

  // A little overkill.
  if (this->BalloonHelpString == NULL && str == NULL)
    {
    return;
    }

  // This check is needed to prevent errors when using
  // this->SetBalloonHelpString(this->BalloonHelpString)
  if (str != this->BalloonHelpString)
    {
    // Normal string stuff.
    if (this->BalloonHelpString)
      {
      delete [] this->BalloonHelpString;
      this->BalloonHelpString = NULL;
      }
    if (str != NULL)
      {
      this->BalloonHelpString = new char[strlen(str)+1];
      strcpy(this->BalloonHelpString, str);
      }
    }
  
  if ( this->Application && !this->BalloonHelpInitialized )
    {
    this->LabelWidget->SetBalloonHelpString(this->BalloonHelpString);
    this->Entry->SetBalloonHelpString(this->BalloonHelpString);
    this->BalloonHelpInitialized = 1;
    }
}

//----------------------------------------------------------------------------
void vtkPVStringEntry::Create(vtkKWApplication *pvApp)
{
  const char* wname;
  
  if (this->Application)
    {
    vtkErrorMacro("StringEntry already created");
    return;
    }

  // For getting the widget in a script.
  this->SetTraceName(this->EntryLabel);
  
  this->SetApplication(pvApp);
  
  // create the top level
  wname = this->GetWidgetName();
  this->Script("frame %s -borderwidth 0 -relief flat", wname);
  
  // Now a label
  if (this->EntryLabel && this->EntryLabel[0] != '\0')
    {
    this->LabelWidget->Create(pvApp, "-width 18 -justify right");
    this->LabelWidget->SetLabel(this->EntryLabel);
    this->Script("pack %s -side left", this->LabelWidget->GetWidgetName());
    }
  
  // Now the entry
  this->Entry->Create(pvApp, "");
  this->Script("bind %s <KeyPress> {%s ModifiedCallback}",
               this->Entry->GetWidgetName(), this->GetTclName());
  if (this->BalloonHelpString)
    {
    this->SetBalloonHelpString(this->BalloonHelpString);
    }
  this->Script("pack %s -side left -fill x -expand t",
               this->Entry->GetWidgetName());

}


//----------------------------------------------------------------------------
void vtkPVStringEntry::SetValue(const char* fileName)
{
  const char *old;
  
  if (fileName == NULL)
    {
    fileName = "";
    }

  old = this->Entry->GetValue();
  if (strcmp(old, fileName) == 0)
    {
    return;
    }

  this->Entry->SetValue(fileName); 
  this->ModifiedCallback();
}



//----------------------------------------------------------------------------
void vtkPVStringEntry::Accept()
{
  vtkPVApplication *pvApp = this->GetPVApplication();

  if (this->ModifiedFlag)
    {
    this->AddTraceEntry("$kw(%s) SetValue {%s}", this->GetTclName(), 
                         this->GetValue());
    }

  pvApp->BroadcastScript("%s Set%s {%s}",
                         this->ObjectTclName, this->VariableName,
                         this->GetValue());

  this->ModifiedFlag = 0;
}

//----------------------------------------------------------------------------
void vtkPVStringEntry::Reset()
{
  if ( ! this->ModifiedFlag)
    {
    return;
    }

  // Command to update the UI.
  this->Script("%s SetValue [%s Get%s]", this->Entry->GetTclName(), 
               this->ObjectTclName, this->VariableName); 

  this->ModifiedFlag = 0;
}

vtkPVStringEntry* vtkPVStringEntry::ClonePrototype(vtkPVSource* pvSource,
				 vtkArrayMap<vtkPVWidget*, vtkPVWidget*>* map)
{
  vtkPVWidget* clone = this->ClonePrototypeInternal(pvSource, map);
  return vtkPVStringEntry::SafeDownCast(clone);
}

void vtkPVStringEntry::CopyProperties(vtkPVWidget* clone, 
				      vtkPVSource* pvSource,
			      vtkArrayMap<vtkPVWidget*, vtkPVWidget*>* map)
{
  this->Superclass::CopyProperties(clone, pvSource, map);
  vtkPVStringEntry* pvse = vtkPVStringEntry::SafeDownCast(clone);
  if (pvse)
    {
    pvse->SetLabel(this->EntryLabel);
    }
  else 
    {
    vtkErrorMacro("Internal error. Could not downcast clone to PVStringEntry.");
    }
}

//----------------------------------------------------------------------------
int vtkPVStringEntry::ReadXMLAttributes(vtkPVXMLElement* element,
                                        vtkPVXMLPackageParser* parser)
{
  if(!this->Superclass::ReadXMLAttributes(element, parser)) { return 0; }
  
  // Setup the Label.
  const char* label = element->GetAttribute("label");
  if(label)
    {
    this->SetLabel(label);
    }
  else
    {
    this->SetLabel(this->VariableName);
    }
  
  return 1;
}

//----------------------------------------------------------------------------
const char* vtkPVStringEntry::GetValue() 
{
  return this->Entry->GetValue();
}
