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
  this->AddLabeledEntry("Start:", "SetStart", "GetStart",this);
  this->AddLabeledEntry("End:", "SetEnd", "GetEnd", this);
  this->AddLabeledEntry("Step:", "SetStep", "GetStep", this);

  this->AddScale("Value:", "SetCurrent", "GetCurrent", 
                 this->Start, this->End, this->Step, this);

  this->UpdateParameterWidgets();
}

//----------------------------------------------------------------------------
void vtkPVAnimation::SetCurrent(float time)
{  
  if (this->Current == time)
    {
    return;
    }
  else
    {
    this->Script(this->Method, this->GetTclName(), time);
    this->GetPVApplication()->BroadcastScript(this->Method, 
                                              this->GetTclName(), time);
    }
}









