/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkPVPolyData.cxx
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
#include "vtkPVPolyData.h"
#include "vtkPVPolyDataComposite.h"
#include "vtkShrinkPolyData.h"
#include "vtkKWView.h"

#include "vtkKWScale.h"
#include "vtkKWPushButton.h"
#include "vtkPVWindow.h"

vtkPVPolyData::vtkPVPolyData()
{
  this->PolyData = NULL;

  this->Label = vtkKWLabel::New();
  
  this->FiltersMenuButton = vtkPVMenuButton::New();
  
  this->Mapper = vtkPolyDataMapper::New();
  this->Actor = vtkActor::New();
  
  this->Comp = vtkPVPolyDataComposite::New();
  
  this->ShrinkFactorScale = vtkKWScale::New();
  this->ShrinkFactorScale->SetParent(this);
  
  this->Accept = vtkKWWidget::New();
  this->Accept->SetParent(this);
}

vtkPVPolyData::~vtkPVPolyData()
{
  this->SetPolyData(NULL);
  
  this->Label->Delete();
  this->Label = NULL;
  
  this->FiltersMenuButton->Delete();
  this->FiltersMenuButton = NULL;
  
  this->Mapper->Delete();
  this->Mapper = NULL;
  
  this->Actor->Delete();
  this->Actor = NULL;
  
  this->ShrinkFactorScale->Delete();
  this->ShrinkFactorScale = NULL;
}

vtkPVPolyData* vtkPVPolyData::New()
{
  return new vtkPVPolyData();
}

void vtkPVPolyData::SetupShrink()
{
  //1st need to set up slider to get % to shrink by
  this->ShrinkFactorScale->Create(this->Application,
				  "-showvalue 1 -resolution 0.1");
  this->ShrinkFactorScale->SetRange(0, 1);
  this->Script("pack %s", this->ShrinkFactorScale->GetWidgetName());
  
  this->Accept->Create(this->Application, "button",
		       "-text Accept");
  this->Accept->SetCommand(this, "Shrink");
  this->Script("pack %s", this->Accept->GetWidgetName());
}

void vtkPVPolyData::Shrink()
{
  vtkShrinkPolyData *shrink = vtkShrinkPolyData::New();
  shrink->SetShrinkFactor(this->ShrinkFactorScale->GetValue());
  shrink->SetInput((vtkPolyData*)this->PolyData);
  
  vtkPVPolyData *pd = vtkPVPolyData::New();
  pd->PolyData = shrink->GetOutput();
  pd->Mapper->SetInput(pd->PolyData);
  pd->Actor->Update();
  
  vtkPVPolyDataComposite *comp = vtkPVPolyDataComposite::New();
  pd->SetComposite(comp);
  comp->SetTabLabels("PolyData", "ShrinkPolyData");
  comp->GetConeSource()->SetShrinkInput(pd->PolyData);
  comp->SetPVPolyData(pd);
  
  vtkPVWindow *window = this->Comp->GetWindow();
  
  comp->SetPropertiesParent(window->GetDataPropertiesParent());
  comp->CreateProperties(this->Application, "");
  this->Comp->GetView()->AddComposite(comp);
  this->Comp->GetView()->RemoveComposite(this->Comp);
  
  comp->SetWindow(window);
  
  window->SetCurrentDataComposite(comp);
  
  pd->Comp->GetView()->Render();
  
  pd->Delete();
  comp->Delete();
}


void vtkPVPolyData::Create(vtkKWApplication *app, char *args)
{
  // must set the application
  if (this->Application)
    {
    vtkErrorMacro("Label already created");
    return;
    }
  this->SetApplication(app);
  
  // create the top level
  this->Script("frame %s %s", this->GetWidgetName(), args);
  
  this->Label->SetParent(this);
  this->Label->Create(app, "");
  this->Label->SetLabel("vtkPVPolyData label");
  this->Script("pack %s", this->Label->GetWidgetName());
  
  this->FiltersMenuButton->SetParent(this);
  this->FiltersMenuButton->Create(app, "");
  this->FiltersMenuButton->SetButtonText("Filters");
  this->FiltersMenuButton->AddCommand("vtkShrinkPolyData", this, "SetupShrink");
  this->Script("pack %s", this->FiltersMenuButton->GetWidgetName());
  
  this->Mapper->SetInput(this->PolyData);
  this->Actor->SetMapper(this->Mapper);
}

vtkProp* vtkPVPolyData::GetProp()
{
  return this->Actor;
}

void vtkPVPolyData::SetComposite(vtkPVPolyDataComposite *pvComp)
{
  this->Comp = pvComp;
}
