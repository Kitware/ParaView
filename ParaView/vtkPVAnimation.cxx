/*=========================================================================

  Program:   ParaView
  Module:    vtkPVAnimation.cxx
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

#include "vtkPVAnimation.h"
#include "vtkPVApplication.h"
#include "vtkKWEntry.h"
#include "vtkKWScale.h"
#include "vtkKWPushButton.h"
#include "vtkKWView.h"

int vtkPVAnimationCommand(ClientData cd, Tcl_Interp *interp,
			   int argc, char *argv[]);

//----------------------------------------------------------------------------
vtkPVAnimation::vtkPVAnimation()
{
  this->CommandFunction = vtkPVAnimationCommand;

  this->Start = 0;
  this->Current = 0;
  this->End = 10;
  this->Step = 1;

  this->Object = NULL;
  this->Method = NULL;

  this->TimeMin = NULL;
  this->TimeMax = NULL;
  this->TimeStep = NULL;
  this->TimeScale = NULL;
}

//----------------------------------------------------------------------------
vtkPVAnimation::~vtkPVAnimation()
{
  this->CommandFunction = vtkPVAnimationCommand;

  if (this->TimeMin != NULL)
    {
    this->TimeMin->Delete();
    this->TimeMin = NULL;
    }
  if (this->TimeMax != NULL)
    {
    this->TimeMax->Delete();
    this->TimeMax = NULL;
    }
  if (this->TimeStep != NULL)
    {
    this->TimeStep->Delete();
    this->TimeStep = NULL;
    }
  if (this->TimeScale != NULL)
    {
    this->TimeScale->Delete();
    this->TimeScale = NULL;
    }
}

//----------------------------------------------------------------------------
vtkPVAnimation* vtkPVAnimation::New()
{
  return new vtkPVAnimation();
}

//----------------------------------------------------------------------------
void vtkPVAnimation::CreateProperties()
{  
  this->vtkPVSource::CreateProperties();

  // Just display the object (selection will come later).
  // First a frame to hold the other widgets.
  vtkKWWidget *frame = vtkKWWidget::New();
  this->Widgets->AddItem(frame);
  frame->SetParent(this->ParameterFrame->GetFrame());
  frame->Create(this->Application, "frame", "");
  this->Script("pack %s -fill x -expand t", frame->GetWidgetName());
  // Now a label
  vtkKWLabel *labelWidget = vtkKWLabel::New();
  this->Widgets->AddItem(labelWidget);
  labelWidget->SetParent(frame);
  labelWidget->Create(this->Application, "-width 19 -justify right");
  labelWidget->SetLabel("Object:");
  this->Script("pack %s -side left", labelWidget->GetWidgetName());
  labelWidget->Delete();
  labelWidget = NULL;
  // Now just display the name of the object
  labelWidget = vtkKWLabel::New();
  this->Widgets->AddItem(labelWidget);
  labelWidget->SetParent(frame);
  labelWidget->Create(this->Application, "");
  if (this->Object)
    {
    labelWidget->SetLabel(this->Object->GetName());
    }
  else
    {
    labelWidget->SetLabel("None");
    }
  this->Script("pack %s -side left", labelWidget->GetWidgetName());
  labelWidget->Delete();
  labelWidget = NULL;
  frame->Delete();
  frame = NULL;

  this->AddStringEntry("Method:", "SetMethod", "GetMethod", NULL, this);
  this->TimeMin = this->AddLabeledEntry("Start:", "SetStart", "GetStart",
                                        NULL, this);
  this->TimeMin->Register(this);
  this->TimeMax = this->AddLabeledEntry("End:", "SetEnd", "GetEnd", NULL,
                                        this);
  this->TimeMax->Register(this);
  this->TimeStep = this->AddLabeledEntry("Step:", "SetStep", "GetStep", NULL,
                                         this);
  this->TimeStep->Register(this);
  this->TimeScale = this->AddScale("Value:", "SetCurrent", "GetCurrent", 
                                   this->Start, this->End, this->Step, NULL,
                                   this);

  vtkKWPushButton *playButton = vtkKWPushButton::New();
  playButton->SetParent(this->ParameterFrame->GetFrame());
  playButton->Create(this->Application, "");
  playButton->SetLabel("Play");
  playButton->SetCommand(this, "Play");
  this->Widgets->AddItem(playButton);
  this->Script("pack %s -side top -expand t -fill x", playButton->GetWidgetName());
  playButton->Delete();
  playButton = NULL;

  this->UpdateParameterWidgets();
}


//----------------------------------------------------------------------------
void vtkPVAnimation::AcceptCallback()
{
  this->vtkPVSource::AcceptCallback();

  // Reconfigure the time scale.
  this->TimeScale->SetRange(this->TimeMin->GetValueAsFloat(),
                            this->TimeMax->GetValueAsFloat());
  this->TimeScale->SetResolution(this->TimeStep->GetValueAsFloat());
}

//----------------------------------------------------------------------------
void vtkPVAnimation::SetObject(vtkPVSource *object)
{
  if (object == this->Object)
    {
    return;
    }

  if (this->Object)
    {
    this->Object->UnRegister(this);
    this->Object = NULL;
    this->SetMethod(NULL);
    }
  if (object)
    {
    object->Register(this);
    this->Object = object;
    // This is a dirty way to get around formating my own string.
    this->Script("%s SetMethod {%s SetTime $time}", this->GetTclName(),
                 this->Object->GetVTKSourceTclName());
    }
}

//----------------------------------------------------------------------------
void vtkPVAnimation::SetCurrent(float time)
{  
  vtkPVApplication *pvApp = this->GetPVApplication();

  if (this->Current == time)
    {
    return;
    }

  this->Script("set time %f", time);
  //pvApp->BroadcastScript("set time %f", time);
  this->Script("catch \"%s\"", this->Method);
  //this->Script(this->Method);
  //pvApp->BroadcastScript(this->Method);

  this->TimeScale->SetValue(time);
  this->Current = time;
}


//----------------------------------------------------------------------------
void vtkPVAnimation::Play()
{  
  float t;

  while (this->Current < this->End)
    {
    t = this->Current + this->Step;
    if (t > this->End)
      {
      t = this->End;
      }
    this->SetCurrent(t);
    this->GetView()->Render();
    }
}







