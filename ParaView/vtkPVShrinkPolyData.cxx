/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkPVShrinkPolyData.cxx
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

#include "vtkPVShrinkPolyData.h"
#include "vtkKWApplication.h"
#include "vtkKWView.h"
#include "vtkKWRenderView.h"
#include "vtkPVComposite.h"

int vtkPVShrinkPolyDataCommand(ClientData cd, Tcl_Interp *interp,
			   int argc, char *argv[]);

vtkPVShrinkPolyData::vtkPVShrinkPolyData()
{
  this->CommandFunction = vtkPVShrinkPolyDataCommand;
  
  this->Label = vtkKWLabel::New();
  this->Label->SetParent(this);
  this->Accept = vtkKWWidget::New();
  this->Accept->SetParent(this);
  this->ShrinkFactorScale = vtkKWScale::New();
  this->ShrinkFactorScale->SetParent(this);
  this->Shrink = vtkShrinkPolyData::New();
}

vtkPVShrinkPolyData::~vtkPVShrinkPolyData()
{
  this->Label->Delete();
  this->Label = NULL;
  
  this->Accept->Delete();
  this->Accept = NULL;
  
  this->ShrinkFactorScale->Delete();
  this->ShrinkFactorScale = NULL;
  
  this->Shrink->Delete();
  this->Shrink = NULL;
}

vtkPVShrinkPolyData* vtkPVShrinkPolyData::New()
{
  return new vtkPVShrinkPolyData();
}

void vtkPVShrinkPolyData::Create(vtkKWApplication *app, char *args)
{  
  // must set the application
  if (this->Application)
    {
    vtkErrorMacro("vtkPVShrinkPolyData already created");
    return;
    }
  this->SetApplication(app);
  
  // create the top level
  this->Script("frame %s %s", this->GetWidgetName(), args);
  
  this->Label->Create(this->Application, "");
  this->Label->SetLabel("vtkShrinkPolyData label");
  
  this->Script("pack %s", this->Label->GetWidgetName());
  
  this->ShrinkFactorScale->Create(this->Application,
			    "-showvalue 1 -resolution 0.1");
  this->ShrinkFactorScale->SetRange(0, 1);
  this->Accept->Create(this->Application, "button", "-text Accept");
  this->Accept->SetCommand(this, "ShrinkFactorChanged");
  this->Script("pack %s %s", this->ShrinkFactorScale->GetWidgetName(),
	this->Accept->GetWidgetName());
}

void vtkPVShrinkPolyData::ShrinkFactorChanged()
{  
  this->Shrink->SetShrinkFactor(this->ShrinkFactorScale->GetValue());
  this->Shrink->Modified();
  this->Shrink->Update();
  
  this->Composite->GetView()->Render();
}


