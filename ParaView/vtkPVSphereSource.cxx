/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkPVSphereSource.cxx
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

#include "vtkPVSphereSource.h"
#include "vtkPVPolyData.h"
#include "vtkPVApplication.h"
#include "vtkPVAssignment.h"
#include "vtkPVWindow.h"
#include "vtkPVActorComposite.h"

int vtkPVSphereSourceCommand(ClientData cd, Tcl_Interp *interp,
			   int argc, char *argv[]);

//----------------------------------------------------------------------------
vtkPVSphereSource::vtkPVSphereSource()
{
  this->CommandFunction = vtkPVSphereSourceCommand;
  
  this->RadiusLabel = vtkKWLabel::New();
  this->RadiusLabel->SetParent(this->Properties);
  this->PhiResolutionLabel = vtkKWLabel::New();
  this->PhiResolutionLabel->SetParent(this->Properties);
  this->ThetaResolutionLabel = vtkKWLabel::New();
  this->ThetaResolutionLabel->SetParent(this->Properties);
  this->RadiusEntry = vtkKWEntry::New();
  this->RadiusEntry->SetParent(this->Properties);
  this->PhiResolutionEntry = vtkKWEntry::New();
  this->PhiResolutionEntry->SetParent(this->Properties);
  this->ThetaResolutionEntry = vtkKWEntry::New();
  this->ThetaResolutionEntry->SetParent(this->Properties);
  this->Accept = vtkKWPushButton::New();
  this->Accept->SetParent(this->Properties);

  vtkSphereSource *s = vtkSphereSource::New();  
  this->SetPolyDataSource(s);
  s->Delete();
}

//----------------------------------------------------------------------------
vtkPVSphereSource::~vtkPVSphereSource()
{
  this->RadiusLabel->Delete();
  this->RadiusLabel = NULL;
  this->PhiResolutionLabel->Delete();
  this->PhiResolutionLabel = NULL;
  this->ThetaResolutionLabel->Delete();
  this->ThetaResolutionLabel = NULL;
  
  this->RadiusEntry->Delete();
  this->RadiusEntry = NULL;
  this->PhiResolutionEntry->Delete();
  this->PhiResolutionEntry = NULL;
  this->ThetaResolutionEntry->Delete();
  this->ThetaResolutionEntry = NULL;

  this->Accept->Delete();
  this->Accept = NULL;
}

//----------------------------------------------------------------------------
vtkPVSphereSource* vtkPVSphereSource::New()
{
  return new vtkPVSphereSource();
}

//----------------------------------------------------------------------------
void vtkPVSphereSource::CreateProperties()
{  
  this->vtkPVPolyDataSource::CreateProperties();
  
  this->RadiusLabel->Create(this->Application, "");
  this->RadiusLabel->SetLabel("Radius:");
  this->PhiResolutionLabel->Create(this->Application, "");
  this->PhiResolutionLabel->SetLabel("Phi Resolution:");
  this->ThetaResolutionLabel->Create(this->Application, "");
  this->ThetaResolutionLabel->SetLabel("Theta Resolution:");
  this->RadiusEntry->Create(this->Application, "");
  this->RadiusEntry->SetValue(this->GetSphereSource()->GetRadius(), 2);
  this->PhiResolutionEntry->Create(this->Application, "");
  this->PhiResolutionEntry->SetValue(this->GetSphereSource()->GetPhiResolution());
  this->ThetaResolutionEntry->Create(this->Application, "");
  this->ThetaResolutionEntry->SetValue(this->GetSphereSource()->GetThetaResolution());
  this->Accept->Create(this->Application, "-text Accept");
  this->Accept->SetCommand(this, "SphereParameterChanged");
  this->Script("pack %s %s %s %s %s %s %s",
	       this->Accept->GetWidgetName(),
               this->RadiusLabel->GetWidgetName(),
               this->RadiusEntry->GetWidgetName(),
               this->PhiResolutionLabel->GetWidgetName(),
               this->PhiResolutionEntry->GetWidgetName(),
               this->ThetaResolutionLabel->GetWidgetName(),
               this->ThetaResolutionEntry->GetWidgetName());
}

//----------------------------------------------------------------------------
void vtkPVSphereSource::SphereParameterChanged()
{
  vtkPVApplication *pvApp = this->GetPVApplication();
  vtkPVWindow *window = this->GetWindow();
 
  this->GetSphereSource()->SetRadius(this->RadiusEntry->GetValueAsFloat());
  this->GetSphereSource()->SetPhiResolution(this->PhiResolutionEntry->GetValueAsInt());
  this->GetSphereSource()->SetThetaResolution(this->ThetaResolutionEntry->GetValueAsInt());

  if (this->GetPVData() == NULL)
    { // This is the first time, initialize data.  
    this->InitializeData();
    }
  
  if (window->GetPreviousSource() != NULL)
    {
    window->GetPreviousSource()->GetPVData()->GetActorComposite()->VisibilityOff();
    }
  
  window->GetMainView()->SetSelectedComposite(this);

  this->GetView()->Render();
  window->GetMainView()->ResetCamera();
}

//----------------------------------------------------------------------------
void vtkPVSphereSource::SetRadius(float rad)
{
  vtkPVApplication *pvApp = this->GetPVApplication();
  
  this->GetSphereSource()->SetRadius(rad);
  
  if (pvApp && pvApp->GetController()->GetLocalProcessId() == 0)
    {
    pvApp->BroadcastScript("%s SetRadius %f", this->GetTclName(), rad);
    }
}

//----------------------------------------------------------------------------
void vtkPVSphereSource::SetPhiResolution(int res)
{
  vtkPVApplication *pvApp = this->GetPVApplication();
  
  this->GetSphereSource()->SetPhiResolution(res);
  
  if (pvApp && pvApp->GetController()->GetLocalProcessId() == 0)
    {
    pvApp->BroadcastScript("%s SetPhiResolution %d", this->GetTclName(), res);
    }
}

//----------------------------------------------------------------------------
void vtkPVSphereSource::SetThetaResolution(int res)
{
  vtkPVApplication *pvApp = this->GetPVApplication();
  
  this->GetSphereSource()->SetThetaResolution(res);
  
  if (pvApp && pvApp->GetController()->GetLocalProccesId() == 0)
    {
    pvApp->BroadcastScript("%s SetThetaResolution %d", this->GetTclName(), res);
    }
}


//----------------------------------------------------------------------------
vtkSphereSource *vtkPVSphereSource::GetSphereSource()
{
  return vtkSphereSource::SafeDownCast(this->PolyDataSource);
}














