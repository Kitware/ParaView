/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkPVScale.cxx
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
#include "vtkPVScale.h"
#include "vtkObjectFactory.h"

//----------------------------------------------------------------------------
vtkPVScale* vtkPVScale::New()
{
  // First try to create the object from the vtkObjectFactory
  vtkObject* ret = vtkObjectFactory::CreateInstance("vtkPVScale");
  if (ret)
    {
    return (vtkPVScale*)ret;
    }
  // If the factory was unable to create the object, then create it here.
  return new vtkPVScale;
}

//----------------------------------------------------------------------------
vtkPVScale::vtkPVScale()
{
  this->Label = vtkKWLabel::New();
  this->Label->SetParent(this);
  this->Scale = vtkKWScale::New();
  this->Scale->SetParent(this);
}

//----------------------------------------------------------------------------
vtkPVScale::~vtkPVScale()
{
  this->Scale->Delete();
  this->Scale = NULL;
  this->Label->Delete();
  this->Label = NULL;
}

//----------------------------------------------------------------------------
void vtkPVScale::Create(vtkKWApplication *pvApp, char *label,
                        float min, float max, float resolution, char *help)
{
  if (this->Application)
    {
    vtkErrorMacro("PVScale already created");
    return;
    }

  // For getting the widget in a script.
  this->SetTraceName(label);
  
  this->SetApplication(pvApp);

  // create the top level
  this->Script("frame %s -borderwidth 0 -relief flat", this->GetWidgetName());

  // Now a label
  this->Label->SetParent(this);
  this->Label->Create(pvApp, "-width 18 -justify right");
  this->Label->SetLabel(label);
  if (help)
    {
    this->Label->SetBalloonHelpString(help);
    }
  this->Script("pack %s -side left", this->Label->GetWidgetName());

  this->Scale->SetParent(this);
  this->Scale->Create(this->Application, "-showvalue 1");
  this->Scale->SetCommand(this, "ModifiedCallback");
  this->Scale->SetRange(min, max);
  this->Scale->SetResolution(resolution);
  if (help)
    {
    this->Scale->SetBalloonHelpString(help);
    }
  this->Script("pack %s -side left -fill x -expand t", 
               this->Scale->GetWidgetName());
}


//----------------------------------------------------------------------------
void vtkPVScale::SetValue(float val)
{
  float oldVal;
  
  oldVal = this->Scale->GetValue();
  if (val == oldVal)
    {
    return;
    }

  this->Scale->SetValue(val); 
  this->ModifiedCallback();
}



//----------------------------------------------------------------------------
void vtkPVScale::Accept()
{
  vtkPVApplication *pvApp = this->GetPVApplication();

  if (this->ModifiedFlag)
    {  
    this->AddTraceEntry("$kw(%s) SetValue %f", this->GetTclName(), 
                         this->Scale->GetValue());
    }

  if (this->ObjectTclName && this->VariableName)
    {
    pvApp->BroadcastScript("%s Set%s %d", 
                           this->ObjectTclName,
                           this->VariableName, 
                           this->GetValue());
    }

  this->ModifiedFlag = 0;
}

//----------------------------------------------------------------------------
void vtkPVScale::Reset()
{
  if ( ! this->ModifiedFlag)
    {  
    return;
    }

  if (this->ObjectTclName && this->VariableName)
    {
    this->Script("%s SetValue [%s Get%s]", this->Scale->GetTclName(),
                  this->ObjectTclName, this->VariableName);
    }

  this->ModifiedFlag = 0;
}


