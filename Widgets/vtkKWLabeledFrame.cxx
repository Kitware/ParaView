/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkKWLabeledFrame.cxx
  Language:  C++
  Date:      $Date$
  Version:   $Revision$

Copyright (c) 1998-1999 Kitware Inc. 469 Clifton Corporate Parkway,
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
#include "vtkKWApplication.h"
#include "vtkKWLabeledFrame.h"
#include "vtkObjectFactory.h"



//------------------------------------------------------------------------------
vtkKWLabeledFrame* vtkKWLabeledFrame::New()
{
  // First try to create the object from the vtkObjectFactory
  vtkObject* ret = vtkObjectFactory::CreateInstance("vtkKWLabeledFrame");
  if(ret)
    {
    return (vtkKWLabeledFrame*)ret;
    }
  // If the factory was unable to create the object, then create it here.
  return new vtkKWLabeledFrame;
}




int vtkKWLabeledFrameCommand(ClientData cd, Tcl_Interp *interp,
		      int argc, char *argv[]);

vtkKWLabeledFrame::vtkKWLabeledFrame()
{
  this->CommandFunction = vtkKWLabeledFrameCommand;

  this->Border = vtkKWWidget::New();
  this->Border->SetParent(this);
  this->Groove = vtkKWWidget::New();
  this->Groove->SetParent(this);
  this->Border2 = vtkKWWidget::New();
  this->Border2->SetParent(this->Groove);
  this->Frame = vtkKWWidget::New();
  this->Frame->SetParent(this->Groove);
  this->Label = vtkKWWidget::New();
  this->Label->SetParent(this);
}

vtkKWLabeledFrame::~vtkKWLabeledFrame()
{
  this->Label->Delete();
  this->Frame->Delete();
  this->Border->Delete();
  this->Border2->Delete();
  this->Groove->Delete();
}

void vtkKWLabeledFrame::SetLabel(const char *text)
{
  this->Script("%s configure -text {%s}",
               this->Label->GetWidgetName(),text);  
}

void vtkKWLabeledFrame::Create(vtkKWApplication *app)
{
  const char *wname;
  
  // must set the application
  if (this->Application)
    {
    vtkErrorMacro("LabeledFrame already created");
    return;
    }

  this->SetApplication(app);

  // create the top level
  wname = this->GetWidgetName();
  this->Script("frame %s -borderwidth 0 -relief flat",wname);

  this->Border->Create(app,"frame","-height 10 -borderwidth 0 -relief flat");
  this->Label->Create(app,"label","");
  this->Groove->Create(app,"frame","-borderwidth 2 -relief groove");
  this->Border2->Create(app,"frame","-height 10 -borderwidth 0 -relief flat");
  this->Frame->Create(app,"frame","-borderwidth 0 -relief flat");
  
  this->Script("pack %s -fill x -side top", this->Border->GetWidgetName());
  this->Script("pack %s -fill x -side top", this->Groove->GetWidgetName());
  this->Script("pack %s -fill x -side top", this->Border2->GetWidgetName());
  this->Script("pack %s -fill both -expand yes",this->Frame->GetWidgetName());
  this->Script("place %s -relx 0 -x 5 -y 0 -anchor nw",
               this->Label->GetWidgetName());
  this->Script("raise %s", this->Label->GetWidgetName());
}


