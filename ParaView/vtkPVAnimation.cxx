/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkPVAnimation.cxx
  Language:  C++
  Date:      $Date$
  Version:   $Revision$

Copyright (c) 1998-2000 Kitware Inc. 469 Clifton Corporate Parkway,
Clifton Park, NY, 12065, USA.

All rights reserved. No part of this software may be reproduced, distributed,
or modified, in any form or by any means, without permission in writing from
Kitware Inc.

IN NO EVENT SHALL THE AUTHORS OR DISTRIBUTORS BE LIABLE TO ANY PARTY FOR
DIRECT, INDIRECT, SPECIAL, INCIDENTAL, OR CONSEQUENTIAL DAMAGES ARISING OUT
OF THE USE OF THIS SOFTWARE, ITS DOCUMENTATION, OR ANY DERIVATIVES THEREOF,
EVEN IF THE AUTHORS HAVE BEEN ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

THE AUTHORS AND DISTRIBUTORS SPECIFICALLY DISCLAIM ANY WARRANTIES, INCLUDING,
BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY, FITNESS FOR A
PARTICULAR PURPOSE, AND NON-INFRINGEMENT.  THIS SOFTWARE IS PROVIDED ON AN
"AS IS" BASIS, AND THE AUTHORS AND DISTRIBUTORS HAVE NO OBLIGATION TO PROVIDE
MAINTENANCE, SUPPORT, UPDATES, ENHANCEMENTS, OR MODIFICATIONS.

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

  this->AddLabeledEntry("Method:", "SetMethod", "GetMethod", this);
  this->TimeMin = this->AddLabeledEntry("Start:", "SetStart", "GetStart",this);
  this->TimeMin->Register(this);
  this->TimeMax = this->AddLabeledEntry("End:", "SetEnd", "GetEnd", this);
  this->TimeMax->Register(this);
  this->TimeStep = this->AddLabeledEntry("Step:", "SetStep", "GetStep", this);
  this->TimeStep->Register(this);
  this->TimeScale = this->AddScale("Value:", "SetCurrent", "GetCurrent", 
                                   this->Start, this->End, this->Step, this);

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
void vtkPVAnimation::SetCurrent(float time)
{  
  if (this->Current == time)
    {
    return;
    }

  this->Script(this->Method, this->Object->GetTclName(), time);
  this->GetPVApplication()->BroadcastScript(this->Method, 
                                              this->Object->GetTclName(), time);
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







