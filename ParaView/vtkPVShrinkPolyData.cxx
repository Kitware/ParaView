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
#include "vtkPVApplication.h"
#include "vtkPVRenderView.h"
#include "vtkPVPolyData.h"
#include "vtkPVWindow.h"
#include "vtkPVActorComposite.h"

int vtkPVShrinkPolyDataCommand(ClientData cd, Tcl_Interp *interp,
			   int argc, char *argv[]);

//----------------------------------------------------------------------------
vtkPVShrinkPolyData::vtkPVShrinkPolyData()
{
  this->CommandFunction = vtkPVShrinkPolyDataCommand;
  
  this->Accept = vtkKWWidget::New();
  this->Accept->SetParent(this->Properties);
  this->ShrinkFactorScale = vtkKWScale::New();
  this->ShrinkFactorScale->SetParent(this->Properties);
  this->Shrink = vtkShrinkPolyData::New();
}

//----------------------------------------------------------------------------
vtkPVShrinkPolyData::~vtkPVShrinkPolyData()
{ 
  this->Accept->Delete();
  this->Accept = NULL;
  
  this->ShrinkFactorScale->Delete();
  this->ShrinkFactorScale = NULL;
  
  this->Shrink->Delete();
  this->Shrink = NULL;
}

//----------------------------------------------------------------------------
vtkPVShrinkPolyData* vtkPVShrinkPolyData::New()
{
  return new vtkPVShrinkPolyData();
}

//----------------------------------------------------------------------------
void vtkPVShrinkPolyData::CreateProperties()
{  
  // must set the application
  this->vtkPVSource::CreateProperties();
  
  this->ShrinkFactorScale->Create(this->Application,
				  "-showvalue 1");
  this->ShrinkFactorScale->SetResolution(0.1);
  this->ShrinkFactorScale->SetRange(0, 1);
  this->ShrinkFactorScale->SetValue(this->GetShrink()->GetShrinkFactor());
  
  this->Accept->Create(this->Application, "button", "-text Accept");
  this->Accept->SetCommand(this, "ShrinkFactorChanged");
  this->Script("pack %s %s", this->ShrinkFactorScale->GetWidgetName(),
	this->Accept->GetWidgetName());
}


//----------------------------------------------------------------------------
void vtkPVShrinkPolyData::SetOutput(vtkPVPolyData *pvd)
{
  vtkPVApplication *pvApp = this->GetPVApplication();
  
  if (pvApp && pvApp->GetController()->GetLocalProcessId() == 0)
    {
    pvApp->BroadcastScript("%s SetOutput %s", this->GetTclName(),
			   pvd->GetTclName());
    }  
  
  this->SetPVData(pvd);  
  pvd->SetPolyData(this->Shrink->GetOutput());
}


//----------------------------------------------------------------------------
vtkPVPolyData *vtkPVShrinkPolyData::GetOutput()
{
  return vtkPVPolyData::SafeDownCast(this->Output);
}


//----------------------------------------------------------------------------
void vtkPVShrinkPolyData::ShrinkFactorChanged()
{
  vtkPVApplication *pvApp = (vtkPVApplication *)this->Application;
  vtkPVPolyData *pvd;
  vtkPVAssignment *a;
  vtkPVWindow *window = this->GetWindow();
  vtkPVActorComposite *ac;
  
  this->SetShrinkFactor(this->ShrinkFactorScale->GetValue());

  if (this->GetPVData() == NULL)
    { // This is the first time. Create the data.
    pvd = vtkPVPolyData::New();
    pvd->Clone(pvApp);
    this->SetOutput(pvd);
    a = window->GetPreviousSource()->GetPVData()->GetAssignment();
    pvd->SetAssignment(a);
    this->GetInput()->GetActorComposite()->VisibilityOff();
    ac = this->GetPVData()->GetActorComposite();
    window->GetMainView()->AddComposite(ac);
    this->CreateDataPage();
    }

  window->GetMainView()->SetSelectedComposite(this);
  this->GetView()->Render();
}

//----------------------------------------------------------------------------
void vtkPVShrinkPolyData::SetShrinkFactor(float factor)
{
  vtkPVApplication *pvApp = this->GetPVApplication();
  if (pvApp && pvApp->GetController()->GetLocalProcessId() == 0)
    {
    pvApp->BroadcastScript("%s SetShrinkFactor %f", this->GetTclName(), 
			   factor);
    }
  
  this->Shrink->SetShrinkFactor(factor);
}


//----------------------------------------------------------------------------
void vtkPVShrinkPolyData::SetInput(vtkPVPolyData *pvData)
{
  vtkPVApplication *pvApp = this->GetPVApplication();
  
  if (pvApp && pvApp->GetController()->GetLocalProcessId() == 0)
    {
    pvApp->BroadcastScript("%s SetInput %s", this->GetTclName(),
			   pvData->GetTclName());
    }  
  
  this->GetShrink()->SetInput(pvData->GetPolyData());
  this->Input = pvData;
}
