/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkPVPushButton.cxx
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
#include "vtkPVPushButton.h"

#include "vtkArrayMap.txx"
#include "vtkKWPushButton.h"
#include "vtkObjectFactory.h"
#include "vtkPVApplication.h"
#include "vtkPVXMLElement.h"
#include "vtkString.h"

//----------------------------------------------------------------------------
vtkStandardNewMacro(vtkPVPushButton);

//----------------------------------------------------------------------------
vtkPVPushButton::vtkPVPushButton()
{
  this->Button = vtkKWPushButton::New();
  this->EntryLabel = 0;
}

//----------------------------------------------------------------------------
vtkPVPushButton::~vtkPVPushButton()
{
  this->Button->Delete();
  this->SetEntryLabel(0);
}

void vtkPVPushButton::SetLabel(const char* label)
{
  this->SetEntryLabel(label);
  this->Button->SetLabel(label);
}

void vtkPVPushButton::SetBalloonHelpString(const char *str)
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
      this->BalloonHelpString = vtkString::Duplicate(str);
      }
    }
  
  if ( this->Application && !this->BalloonHelpInitialized )
    {
    this->Button->SetBalloonHelpString(this->BalloonHelpString);
    this->BalloonHelpInitialized = 1;
    }
}

//----------------------------------------------------------------------------
void vtkPVPushButton::ExecuteCommand()
{
  this->ModifiedCallback();
}

//----------------------------------------------------------------------------
void vtkPVPushButton::Create(vtkKWApplication *pvApp)
{
  if (this->Application)
    {
    vtkErrorMacro("PVPushButton already created");
    return;
    }

  // For getting the widget in a script.
  this->SetTraceName(this->EntryLabel);
  
  this->SetApplication(pvApp);

  // create the top level
  this->Script("frame %s -borderwidth 0 -relief flat", this->GetWidgetName());

  // Now a label
  this->Button->SetParent(this);
  this->Button->Create(pvApp, "");
  this->Button->SetLabel(this->EntryLabel);
  this->Button->SetCommand(this, "ExecuteCommand");
  this->Script("pack %s -side left", this->Button->GetWidgetName());
}

//----------------------------------------------------------------------------
void vtkPVPushButton::Accept()
{
  this->ModifiedFlag = 0;
}

//----------------------------------------------------------------------------
void vtkPVPushButton::Reset()
{
  this->ModifiedFlag = 0;
}

vtkPVPushButton* vtkPVPushButton::ClonePrototype(vtkPVSource* pvSource,
                                 vtkArrayMap<vtkPVWidget*, vtkPVWidget*>* map)
{
  vtkPVWidget* clone = this->ClonePrototypeInternal(pvSource, map);
  return vtkPVPushButton::SafeDownCast(clone);
}

void vtkPVPushButton::CopyProperties(vtkPVWidget* clone, vtkPVSource* pvSource,
                              vtkArrayMap<vtkPVWidget*, vtkPVWidget*>* map)
{
  this->Superclass::CopyProperties(clone, pvSource, map);
  vtkPVPushButton* pvs = vtkPVPushButton::SafeDownCast(clone);
  if (pvs)
    {
    /*
    float min, max;
    this->Scale->GetRange(min, max);
    pvs->SetRange(min, max);
    pvs->SetResolution(this->Scale->GetResolution());
    */
    pvs->SetLabel(this->EntryLabel);
    }
  else 
    {
    vtkErrorMacro("Internal error. Could not downcast clone to PVScale.");
    }
}

//----------------------------------------------------------------------------
int vtkPVPushButton::ReadXMLAttributes(vtkPVXMLElement* element,
                                  vtkPVXMLPackageParser* parser)
{
  if(!this->Superclass::ReadXMLAttributes(element, parser)) { return 0; }

  // Setup the Label.
  const char* label = element->GetAttribute("label");
  if(!label)
    {
    vtkErrorMacro("No label attribute.");
    return 0;
    }
  this->SetLabel(label);

  return 1;
  
}

//-------------------------------------------------------------------------
void vtkPVPushButton::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  os << indent << "EntryLabel: " << this->EntryLabel << endl;
}
