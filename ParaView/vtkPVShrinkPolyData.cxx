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
#include "vtkPVComposite.h"
#include "vtkPVPolyData.h"

int vtkPVShrinkPolyDataCommand(ClientData cd, Tcl_Interp *interp,
			   int argc, char *argv[]);

//----------------------------------------------------------------------------
vtkPVShrinkPolyData::vtkPVShrinkPolyData()
{
  vtkPVPolyData *pd;

  this->CommandFunction = vtkPVShrinkPolyDataCommand;
  
  this->Accept = vtkKWWidget::New();
  this->Accept->SetParent(this);
  this->ShrinkFactorScale = vtkKWScale::New();
  this->ShrinkFactorScale->SetParent(this);
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
int vtkPVShrinkPolyData::Create(char *args)
{  
  // must set the application
  if (this->vtkPVSource::Create(args) == 0)
    {
    return 0;
    }
  
  this->ShrinkFactorScale->Create(this->Application,
				  "-showvalue 1");
  this->ShrinkFactorScale->SetResolution(0.1);
  this->ShrinkFactorScale->SetRange(0, 1);
  this->ShrinkFactorScale->SetValue(this->GetShrink()->GetShrinkFactor());
  
  this->Accept->Create(this->Application, "button", "-text Accept");
  this->Accept->SetCommand(this, "ShrinkFactorChanged");
  this->Script("pack %s %s", this->ShrinkFactorScale->GetWidgetName(),
	this->Accept->GetWidgetName());

  return 1;
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
  this->Shrink->SetShrinkFactor(this->ShrinkFactorScale->GetValue());
  this->Shrink->Modified();
  this->Shrink->Update();
  
  this->Composite->GetView()->Render();
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
}


