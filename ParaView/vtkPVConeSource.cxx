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
#include "vtkPVPolyData.h"
#include "vtkPVApplication.h"
#include "vtkPVAssignment.h"
#include "vtkPVWindow.h"
#include "vtkPVActorComposite.h"

int vtkPVConeSourceCommand(ClientData cd, Tcl_Interp *interp,
			   int argc, char *argv[]);

//----------------------------------------------------------------------------
vtkPVConeSource::vtkPVConeSource()
{
  this->CommandFunction = vtkPVConeSourceCommand;
  
  this->HeightLabel = vtkKWLabel::New();
  this->HeightLabel->SetParent(this->Properties);
  this->RadiusLabel = vtkKWLabel::New();
  this->RadiusLabel->SetParent(this->Properties);
  this->ResolutionLabel = vtkKWLabel::New();
  this->ResolutionLabel->SetParent(this->Properties);
  this->HeightEntry = vtkKWEntry::New();
  this->HeightEntry->SetParent(this->Properties);
  this->RadiusEntry = vtkKWEntry::New();
  this->RadiusEntry->SetParent(this->Properties);
  this->ResolutionEntry = vtkKWEntry::New();
  this->ResolutionEntry->SetParent(this->Properties);
  this->Accept = vtkKWWidget::New();
  this->Accept->SetParent(this->Properties);
  this->ConeSource = vtkConeSource::New();  
}

//----------------------------------------------------------------------------
vtkPVConeSource::~vtkPVConeSource()
{
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

//----------------------------------------------------------------------------
void vtkPVConeSource::CreateProperties()
{  
  this->vtkPVSource::CreateProperties();
  
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
  this->Script("pack %s %s %s %s %s %s %s",
	       this->Accept->GetWidgetName(),
               this->RadiusLabel->GetWidgetName(),
               this->RadiusEntry->GetWidgetName(),
               this->HeightLabel->GetWidgetName(),
               this->HeightEntry->GetWidgetName(),
               this->ResolutionLabel->GetWidgetName(),
               this->ResolutionEntry->GetWidgetName());
}

//----------------------------------------------------------------------------
void vtkPVConeSource::SetOutput(vtkPVPolyData *pd)
{
  vtkPVApplication *pvApp = this->GetPVApplication();

  this->SetPVData(pd);  
  pd->SetPolyData(this->ConeSource->GetOutput());
  
  if (pvApp && pvApp->GetController()->GetLocalProcessId() == 0)
    {
    pvApp->BroadcastScript("%s SetOutput %s", this->GetTclName(), 
			   pd->GetTclName());
    }
}


//----------------------------------------------------------------------------
vtkPVPolyData *vtkPVConeSource::GetOutput()
{
  return vtkPVPolyData::SafeDownCast(this->Output);
}


//----------------------------------------------------------------------------
void vtkPVConeSource::ConeParameterChanged()
{
  vtkPVApplication *pvApp = this->GetPVApplication();
  vtkPVPolyData *pvd;
  vtkPVAssignment *a;
  vtkPVActorComposite *ac;
  vtkPVWindow *window = this->GetWindow();
 
  this->ConeSource->SetRadius(this->RadiusEntry->GetValueAsFloat());
  this->ConeSource->SetHeight(this->HeightEntry->GetValueAsFloat());
  this->ConeSource->SetResolution(this->ResolutionEntry->GetValueAsInt());

  if (this->GetPVData() == NULL)
    { // This is the first time, initialize data.  
    pvd = vtkPVPolyData::New();
    pvd->Clone(pvApp);
    a = vtkPVAssignment::New();
    a->Clone(pvApp);
  
    pvd->SetAssignment(a);
    this->SetOutput(pvd);
  
    this->CreateDataPage();
  
    ac = this->GetPVData()->GetActorComposite();
    window->GetMainView()->AddComposite(ac);
    }
  window->GetMainView()->SetSelectedComposite(this);
  this->GetView()->Render();
}

//----------------------------------------------------------------------------
vtkPVApplication* vtkPVConeSource::GetPVApplication()
{
  if (this->Application == NULL)
    {
    return NULL;
    }
  
  if (this->Application->IsA("vtkPVApplication"))
    {  
    return (vtkPVApplication*)(this->Application);
    }
  else
    {
    vtkErrorMacro("Bad typecast");
    return NULL;
    } 
}


//----------------------------------------------------------------------------
void vtkPVConeSource::SetRadius(float rad)
{
  vtkPVApplication *pvApp = this->GetPVApplication();
  
  this->ConeSource->SetRadius(rad);
  
  if (pvApp && pvApp->GetController()->GetNumberOfProcesses() == 0)
    {
    pvApp->BroadcastScript("%s SetRadius %f", this->GetTclName(), rad);
    }
}

//----------------------------------------------------------------------------
void vtkPVConeSource::SetHeight(float height)
{
  vtkPVApplication *pvApp = this->GetPVApplication();
  
  this->ConeSource->SetHeight(height);
  
  if (pvApp && pvApp->GetController()->GetNumberOfProcesses() == 0)
    {
    pvApp->BroadcastScript("%s SetHeight %f", this->GetTclName(), height);
    }
}

//----------------------------------------------------------------------------
void vtkPVConeSource::SetResolution(int res)
{
  vtkPVApplication *pvApp = this->GetPVApplication();
  
  this->ConeSource->SetResolution(res);
  
  if (pvApp && pvApp->GetController()->GetNumberOfProcesses() == 0)
    {
    pvApp->BroadcastScript("%s SetResolution %d", this->GetTclName(), res);
    }
}

