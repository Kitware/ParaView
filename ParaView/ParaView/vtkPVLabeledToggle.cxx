/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkPVLabeledToggle.cxx
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
#include "vtkPVLabeledToggle.h"

#include "vtkPVApplication.h"
#include "vtkObjectFactory.h"
#include "vtkArrayMap.txx"
#include "vtkPVXMLElement.h"
#include "vtkKWLabel.h"
#include "vtkKWCheckButton.h"

//----------------------------------------------------------------------------
vtkPVLabeledToggle* vtkPVLabeledToggle::New()
{
  // First try to create the object from the vtkObjectFactory
  vtkObject* ret = vtkObjectFactory::CreateInstance("vtkPVLabeledToggle");
  if (ret)
    {
    return (vtkPVLabeledToggle*)ret;
    }
  // If the factory was unable to create the object, then create it here.
  return new vtkPVLabeledToggle;
}

//----------------------------------------------------------------------------
vtkPVLabeledToggle::vtkPVLabeledToggle()
{
  this->Label = vtkKWLabel::New();
  this->Label->SetParent(this);
  this->CheckButton = vtkKWCheckButton::New();
  this->CheckButton->SetParent(this);
}

//----------------------------------------------------------------------------
vtkPVLabeledToggle::~vtkPVLabeledToggle()
{
  this->CheckButton->Delete();
  this->CheckButton = NULL;
  this->Label->Delete();
  this->Label = NULL;
}

void vtkPVLabeledToggle::SetBalloonHelpString(const char *str)
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
    this->Label->SetBalloonHelpString(this->BalloonHelpString);
    this->CheckButton->SetBalloonHelpString(this->BalloonHelpString);
    this->BalloonHelpInitialized = 1;
    }
}

//----------------------------------------------------------------------------
void vtkPVLabeledToggle::Create(vtkKWApplication *pvApp)
{
  const char* wname;
  
  if (this->Application)
    {
    vtkErrorMacro("LabeledToggle already created");
    return;
    }

  this->SetApplication(pvApp);
  
  // create the top level
  wname = this->GetWidgetName();
  this->Script("frame %s -borderwidth 0 -relief flat", wname);
  
  // Now a label
  this->Label->Create(pvApp, "-width 18 -justify right");
  this->Script("pack %s -side left", this->Label->GetWidgetName());
  
  // Now the check button
  this->CheckButton->Create(pvApp, "");
  this->CheckButton->SetCommand(this, "ModifiedCallback");
  if (this->BalloonHelpString)
    {
    this->SetBalloonHelpString(this->BalloonHelpString);
    }
  this->Script("pack %s -side left", this->CheckButton->GetWidgetName());
}

//----------------------------------------------------------------------------
void vtkPVLabeledToggle::SetState(int val)
{
  int oldVal;
  
  oldVal = this->CheckButton->GetState();
  if (val == oldVal)
    {
    return;
    }

  this->CheckButton->SetState(val); 
  this->ModifiedCallback();
}

//----------------------------------------------------------------------------
void vtkPVLabeledToggle::Accept()
{
  vtkPVApplication *pvApp = this->GetPVApplication();

  if (this->ModifiedFlag)
    {
    this->AddTraceEntry("$kw(%s) SetState %d", this->GetTclName(), 
                        this->GetState());
    }

  pvApp->BroadcastScript("%s Set%s %d", this->ObjectTclName, 
                         this->VariableName, this->GetState());

  this->ModifiedFlag = 0;

}

//----------------------------------------------------------------------------
void vtkPVLabeledToggle::Reset()
{
  if ( ! this->ModifiedFlag)
    {
    return;
    }

  this->Script("%s SetState [%s Get%s]", this->CheckButton->GetTclName(),
               this->ObjectTclName, this->VariableName);

  this->ModifiedFlag = 0;

}

vtkPVLabeledToggle* vtkPVLabeledToggle::ClonePrototype(vtkPVSource* pvSource,
				 vtkArrayMap<vtkPVWidget*, vtkPVWidget*>* map)
{
  vtkPVWidget* clone = this->ClonePrototypeInternal(pvSource, map);
  return vtkPVLabeledToggle::SafeDownCast(clone);
}

void vtkPVLabeledToggle::CopyProperties(vtkPVWidget* clone, 
					vtkPVSource* pvSource,
			      vtkArrayMap<vtkPVWidget*, vtkPVWidget*>* map)
{
  this->Superclass::CopyProperties(clone, pvSource, map);
  vtkPVLabeledToggle* pvlt = vtkPVLabeledToggle::SafeDownCast(clone);
  if (pvlt)
    {
    pvlt->Label->SetLabel(this->Label->GetLabel());
    pvlt->SetTraceName(this->Label->GetLabel());
    }
  else 
    {
    vtkErrorMacro("Internal error. Could not downcast clone to PVLabeledToggle.");
    }
}

//----------------------------------------------------------------------------
int vtkPVLabeledToggle::ReadXMLAttributes(vtkPVXMLElement* element,
                                          vtkPVXMLPackageParser* parser)
{
  if(!this->Superclass::ReadXMLAttributes(element, parser)) { return 0; }
  
  // Setup the Label.
  const char* label = element->GetAttribute("label");
  if(label)
    {
    this->Label->SetLabel(label);
    }
  else
    {
    this->Label->SetLabel(this->VariableName);
    }
  
  return 1;
}

//----------------------------------------------------------------------------
void vtkPVLabeledToggle::SetLabel(const char *str) 
{
  this->Label->SetLabel(str); this->SetTraceName(str);
}

//----------------------------------------------------------------------------
const char* vtkPVLabeledToggle::GetLabel() 
{ 
  return this->Label->GetLabel();
}

//----------------------------------------------------------------------------
int vtkPVLabeledToggle::GetState() 
{ 
  return this->CheckButton->GetState(); 
}
