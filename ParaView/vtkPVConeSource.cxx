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
  
  this->HeightEntry = vtkKWLabeledEntry::New();
  this->HeightEntry->SetParent(this->Properties);
  this->RadiusEntry = vtkKWLabeledEntry::New();
  this->RadiusEntry->SetParent(this->Properties);
  this->ResolutionEntry = vtkKWLabeledEntry::New();
  this->ResolutionEntry->SetParent(this->Properties);
  this->Accept = vtkKWPushButton::New();
  this->Accept->SetParent(this->Properties);

  vtkConeSource *source = vtkConeSource::New();  
  this->SetPolyDataSource(source);
  source->Delete();
}

//----------------------------------------------------------------------------
vtkPVConeSource::~vtkPVConeSource()
{
  this->HeightEntry->Delete();
  this->HeightEntry = NULL;
  this->RadiusEntry->Delete();
  this->RadiusEntry = NULL;
  this->ResolutionEntry->Delete();
  this->ResolutionEntry = NULL;

  this->Accept->Delete();
  this->Accept = NULL;
    
  this->SetPolyDataSource(NULL);
}

//----------------------------------------------------------------------------
vtkPVConeSource* vtkPVConeSource::New()
{
  return new vtkPVConeSource();
}

//----------------------------------------------------------------------------
void vtkPVConeSource::CreateProperties()
{  
  this->vtkPVPolyDataSource::CreateProperties();
  
  this->RadiusEntry->Create(this->Application);
  this->RadiusEntry->SetLabel("Radius:");
  this->RadiusEntry->SetValue(this->GetConeSource()->GetRadius(), 2);
  this->HeightEntry->Create(this->Application);
  this->HeightEntry->SetLabel("Height:");
  this->HeightEntry->SetValue(this->GetConeSource()->GetHeight(), 2);
  this->ResolutionEntry->Create(this->Application);
  this->ResolutionEntry->SetLabel("Resolution:");
  this->ResolutionEntry->SetValue(this->GetConeSource()->GetResolution());
  this->Accept->Create(this->Application, "-text Accept");
  this->Accept->SetCommand(this, "ConeParameterChanged");
  this->Script("pack %s %s %s %s",
	       this->Accept->GetWidgetName(),
               this->RadiusEntry->GetWidgetName(),
               this->HeightEntry->GetWidgetName(),
               this->ResolutionEntry->GetWidgetName());
}

//----------------------------------------------------------------------------
// Functions to update the progress bar
void StartConeSourceProgress(void *arg)
{
  vtkPVConeSource *me = (vtkPVConeSource*)arg;
  me->GetWindow()->SetStatusText("Processing ConeSource");
}

//----------------------------------------------------------------------------
void ConeSourceProgress(void *arg)
{
  vtkPVConeSource *me = (vtkPVConeSource*)arg;
  me->GetWindow()->GetProgressGauge()->SetValue((int)(me->GetConeSource()->GetProgress() * 100));
}

//----------------------------------------------------------------------------
void EndConeSourceProgress(void *arg)
{
  vtkPVConeSource *me = (vtkPVConeSource*)arg;
  me->GetWindow()->SetStatusText("");
  me->GetWindow()->GetProgressGauge()->SetValue(0);
}

//----------------------------------------------------------------------------
void vtkPVConeSource::ConeParameterChanged()
{
  vtkPVApplication *pvApp = this->GetPVApplication();
  vtkPVWindow *window = this->GetWindow();
 
  this->SetRadius(this->RadiusEntry->GetValueAsFloat());
  this->SetHeight(this->HeightEntry->GetValueAsFloat());
  this->SetResolution(this->ResolutionEntry->GetValueAsInt());

  if (this->GetPVData() == NULL)
    { // This is the first time, initialize data.  
    this->GetConeSource()->SetStartMethod(StartConeSourceProgress, this);
    this->GetConeSource()->SetProgressMethod(ConeSourceProgress, this);
    this->GetConeSource()->SetEndMethod(EndConeSourceProgress, this);
    this->InitializeData();
    }
  
  if (window->GetPreviousSource() != NULL)
    {
    window->GetPreviousSource()->GetPVData()->GetActorComposite()->VisibilityOff();
    }
  
  window->GetMainView()->SetSelectedComposite(this);

  window->GetMainView()->ResetCamera();
  this->GetView()->Render();
}


//----------------------------------------------------------------------------
void vtkPVConeSource::SetRadius(float rad)
{
  vtkPVApplication *pvApp = this->GetPVApplication();
  
  this->GetConeSource()->SetRadius(rad);
  
  if (pvApp && pvApp->GetController()->GetLocalProcessId() == 0)
    {
    pvApp->BroadcastScript("%s SetRadius %f", this->GetTclName(), rad);
    }
}

//----------------------------------------------------------------------------
void vtkPVConeSource::SetHeight(float height)
{
  vtkPVApplication *pvApp = this->GetPVApplication();
  
  this->GetConeSource()->SetHeight(height);
  
  if (pvApp && pvApp->GetController()->GetLocalProcessId() == 0)
    {
    pvApp->BroadcastScript("%s SetHeight %f", this->GetTclName(), height);
    }
}

//----------------------------------------------------------------------------
void vtkPVConeSource::SetResolution(int res)
{
  vtkPVApplication *pvApp = this->GetPVApplication();
  
  this->GetConeSource()->SetResolution(res);
  
  if (pvApp && pvApp->GetController()->GetLocalProcessId() == 0)
    {
    pvApp->BroadcastScript("%s SetResolution %d", this->GetTclName(), res);
    }
}

//----------------------------------------------------------------------------
vtkConeSource *vtkPVConeSource::GetConeSource() 
{
  return vtkConeSource::SafeDownCast(this->PolyDataSource);
}


