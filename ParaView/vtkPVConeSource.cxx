/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkPVConeSource.cxx
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

#include "vtkPVConeSource.h"
#include "vtkKWApplication.h"
#include "vtkKWView.h"
#include "vtkKWRenderView.h"
#include "vtkPVComposite.h"

int vtkPVConeSourceCommand(ClientData cd, Tcl_Interp *interp,
			   int argc, char *argv[]);

vtkPVConeSource::vtkPVConeSource()
{
  this->CommandFunction = vtkPVConeSourceCommand;
  
  this->Label = vtkKWLabel::New();
  this->Label->SetParent(this);
  this->HeightLabel = vtkKWLabel::New();
  this->HeightLabel->SetParent(this);
  this->RadiusLabel = vtkKWLabel::New();
  this->RadiusLabel->SetParent(this);
  this->ResolutionLabel = vtkKWLabel::New();
  this->ResolutionLabel->SetParent(this);
  this->HeightEntry = vtkKWEntry::New();
  this->HeightEntry->SetParent(this);
  this->RadiusEntry = vtkKWEntry::New();
  this->RadiusEntry->SetParent(this);
  this->ResolutionEntry = vtkKWEntry::New();
  this->ResolutionEntry->SetParent(this);
  this->Accept = vtkKWWidget::New();
  this->Accept->SetParent(this);
  this->ConeSource = vtkConeSource::New();
}

vtkPVConeSource::~vtkPVConeSource()
{
  this->Label->Delete();
  this->Label = NULL;
  
  this->HeightLabel->Delete();
  this->HeightLabel = NULL;
  this->RadiusLabel->Delete();
  this->RadiusLabel = NULL;
  this->ResolutionLabel->Delete();
  this->ResolutionLabel = NULL;
  
  this->HeightEntry->Delete();
  this->HeightEntry = NULL;
  this->RadiusEntry->Delete();
  this->RadiusEntry = NULL;
  this->ResolutionEntry->Delete();
  this->ResolutionEntry = NULL;

  this->Accept->Delete();
  this->Accept = NULL;
    
  this->ConeSource->Delete();
  this->ConeSource = NULL;
}

vtkPVConeSource* vtkPVConeSource::New()
{
  return new vtkPVConeSource();
}

void vtkPVConeSource::Create(vtkKWApplication *app, char *args)
{  
  // must set the application
  if (this->Application)
    {
    vtkErrorMacro("vtkPVConeSource already created");
    return;
    }
  this->SetApplication(app);
  
  // create the top level
  this->Script("frame %s %s", this->GetWidgetName(), args);
  
  this->Label->Create(this->Application, "");
  this->Label->SetLabel("vtkPVConeSource label");
  
  this->Script("pack %s", this->Label->GetWidgetName());
  
  this->RadiusLabel->Create(this->Application, "");
  this->RadiusLabel->SetLabel("Radius:");
  this->HeightLabel->Create(this->Application, "");
  this->HeightLabel->SetLabel("Height:");
  this->ResolutionLabel->Create(this->Application, "");
  this->ResolutionLabel->SetLabel("Resolution:");
  this->RadiusEntry->Create(this->Application, "");
  this->RadiusEntry->SetValue(this->ConeSource->GetRadius(), 2);
  this->HeightEntry->Create(this->Application, "");
  this->HeightEntry->SetValue(this->ConeSource->GetHeight(), 2);
  this->ResolutionEntry->Create(this->Application, "");
  this->ResolutionEntry->SetValue(this->ConeSource->GetResolution());
  this->Accept->Create(this->Application, "button",
	                     "-text Accept");
  this->Accept->SetCommand(this, "ConeParameterChanged");
  this->Script("pack %s", this->Accept->GetWidgetName());
  this->Script("pack %s %s %s %s %s %s %s", 
               this->RadiusLabel->GetWidgetName(),
               this->RadiusEntry->GetWidgetName(),
               this->HeightLabel->GetWidgetName(),
               this->HeightEntry->GetWidgetName(),
               this->ResolutionLabel->GetWidgetName(),
               this->ResolutionEntry->GetWidgetName(),
               this->Accept->GetWidgetName());
}

void vtkPVConeSource::ConeParameterChanged()
{
  this->ConeSource->SetRadius(this->RadiusEntry->GetValueAsFloat());
  this->ConeSource->SetHeight(this->HeightEntry->GetValueAsFloat());
  this->ConeSource->SetResolution(this->ResolutionEntry->GetValueAsInt());
  
  this->Composite->GetView()->Render();
}

