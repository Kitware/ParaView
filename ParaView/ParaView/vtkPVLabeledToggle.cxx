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
#include "vtkObjectFactory.h"

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

vtkPVLabeledToggle::vtkPVLabeledToggle()
{
  this->Label = vtkKWLabel::New();
  this->Label->SetParent(this);
  this->CheckButton = vtkKWCheckButton::New();
  this->CheckButton->SetParent(this);

  this->PVSource = NULL;
}

vtkPVLabeledToggle::~vtkPVLabeledToggle()
{
  this->CheckButton->Delete();
  this->CheckButton = NULL;
  this->Label->Delete();
  this->Label = NULL;
}

void vtkPVLabeledToggle::Create(vtkKWApplication *pvApp, char *label,
                                char *setCmd, char *getCmd, char *help,
                                const char *tclName)
{
  const char* wname;
  
  if (this->Application)
    {
    vtkErrorMacro("LabeledToggle already created");
    return;
    }
  if ( ! this->PVSource)
    {
    vtkErrorMacro("PVSource must be set before calling create");
    return;
    }

  // For getting the widget in a script.
  this->SetName(label);
  
  this->SetApplication(pvApp);
  
  // create the top level
  wname = this->GetWidgetName();
  this->Script("frame %s -borderwidth 0 -relief flat", wname);
  
  // Now a label
  if (label && label[0] != '\0')
    {
    this->Label->Create(pvApp, "-width 18 -justify right");
    this->Label->SetLabel(label);
    if (help)
      {
      this->Label->SetBalloonHelpString(help);
      }
    this->Script("pack %s -side left", this->Label->GetWidgetName());
    }
  
  // Now the check button
  this->CheckButton->Create(pvApp, "");
  this->CheckButton->SetCommand(this, "ModifiedCallback");
  if (help)
    {
    this->CheckButton->SetBalloonHelpString(help);
    }
  this->Script("pack %s -side left", this->CheckButton->GetWidgetName());

  // Command to update the UI.
  this->ResetCommands->AddString("%s SetState [%s %s]",
                                 this->CheckButton->GetTclName(), tclName,
                                 getCmd); 
  // Format a command to move value from widget to vtkObjects (on all
  // processes).
  // The VTK objects do not yet have to have the same Tcl name!
  //this->AcceptCommands->AddString("%s AcceptHelper2 %s %s [%s GetState]",
  //                                this->PVSource->GetTclName(), tclName,
  //                                setCmd, this->CheckButton->GetTclName()); 
  this->AcceptCommands->AddString("%s %s [%s GetState]",
                              tclName, setCmd, this->CheckButton->GetTclName()); 
}


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



void vtkPVLabeledToggle::Accept()
{
  vtkPVApplication *pvApp = this->GetPVApplication();

  if (this->ModifiedFlag && this->PVSource)
    {  
    if ( ! this->TraceInitialized)
      {
      pvApp->AddTraceEntry("set pv(%s) [$pv(%s) GetPVWidget {%s}]",
                           this->GetTclName(), this->PVSource->GetTclName(),
                           this->Name);
      this->TraceInitialized = 1;
      }

    pvApp->AddTraceEntry("$pv(%s) SetState %d", this->GetTclName(), 
                         this->CheckButton->GetState());
    }

  this->vtkPVWidget::Accept();
}


