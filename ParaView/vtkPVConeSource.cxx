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

vtkPVConeSource::vtkPVConeSource()
{
  this->Label = vtkKWLabel::New();
  this->ConeSource = vtkConeSource::New();
  this->Shrink = vtkShrinkPolyData::New();
  this->Cone = 0;
}

vtkPVConeSource::~vtkPVConeSource()
{
  this->Label->Delete();
  this->Label = NULL;
  
  this->ConeSource->Delete();
  this->ConeSource = NULL;
  
  this->Shrink->Delete();
  this->Shrink = NULL;
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
  
  this->Label->SetParent(this);
  this->Label->Create(this->Application, "");
  if (this->IsConeSource())
    {
    this->Label->SetLabel("vtkPVConeSource label");
    }
  else
    {
    this->Label->SetLabel("vtkShrinkPolyData label");
    }
  
  this->Script("pack %s", this->Label->GetWidgetName());
}

vtkPolyData* vtkPVConeSource::GetOutput()
{
  if (this->IsConeSource())
    {
    return this->ConeSource->GetOutput();
    }
  else
    {
    return this->Shrink->GetOutput();
    }
}

void vtkPVConeSource::SetConeSource()
{  
  this->ConeSource->SetRadius(100.0);
  this->ConeSource->SetHeight(100.0);
  this->Cone = 1;
}

void vtkPVConeSource::SetShrinkInput(vtkPolyData *input)
{
  this->Shrink->SetInput(input);
  this->Cone = 0;
}

int vtkPVConeSource::IsConeSource()
{
  return this->Cone;
}
