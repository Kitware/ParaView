/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkPVPolyDataSource.cxx
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

#include "vtkPVPolyDataSource.h"
#include "vtkKWApplication.h"
#include "vtkKWView.h"
#include "vtkKWRenderView.h"
#include "vtkPVPolyData.h"
#include "vtkPVApplication.h"
#include "vtkPVAssignment.h"
#include "vtkPVWindow.h"
#include "vtkPVActorComposite.h"

int vtkPVPolyDataSourceCommand(ClientData cd, Tcl_Interp *interp,
			   int argc, char *argv[]);

//----------------------------------------------------------------------------
vtkPVPolyDataSource::vtkPVPolyDataSource()
{
  this->CommandFunction = vtkPVPolyDataSourceCommand;
  
  this->PolyDataSource = NULL;  
}

//----------------------------------------------------------------------------
vtkPVPolyDataSource* vtkPVPolyDataSource::New()
{
  return new vtkPVPolyDataSource();
}

//----------------------------------------------------------------------------
void vtkPVPolyDataSource::SetOutput(vtkPVPolyData *pd)
{
  vtkPVApplication *pvApp = this->GetPVApplication();

  this->SetPVData(pd);  
  pd->SetData(this->PolyDataSource->GetOutput());
  
  if (pvApp && pvApp->GetController()->GetLocalProcessId() == 0)
    {
    pvApp->BroadcastScript("%s SetOutput %s", this->GetTclName(), 
			   pd->GetTclName());
    }
}

//----------------------------------------------------------------------------
vtkPVPolyData *vtkPVPolyDataSource::GetOutput()
{
  return vtkPVPolyData::SafeDownCast(this->Output);
}

//----------------------------------------------------------------------------
// Functions to update the progress bar
void vtkPVSourceStartProgress(void *arg)
{
  vtkPVPolyDataSource *me = (vtkPVPolyDataSource*)arg;
  vtkPolyDataSource *vtkSource = me->GetPolyDataSource();
  static char str[200];
  
  if (vtkSource)
    {
    sprintf(str, "Processing %s", vtkSource->GetClassName());
    me->GetWindow()->SetStatusText(str);
    }
}

//----------------------------------------------------------------------------
void vtkPVSourceReportProgress(void *arg)
{
  vtkPVPolyDataSource *me = (vtkPVPolyDataSource*)arg;
  vtkPolyDataSource *vtkSource = me->GetPolyDataSource();

  me->GetWindow()->GetProgressGauge()->SetValue((int)(vtkSource->GetProgress() * 100));
}

//----------------------------------------------------------------------------
void vtkPVSourceEndProgress(void *arg)
{
  vtkPVPolyDataSource *me = (vtkPVPolyDataSource*)arg;
  me->GetWindow()->SetStatusText("");
  me->GetWindow()->GetProgressGauge()->SetValue(0);
}

//----------------------------------------------------------------------------
void vtkPVPolyDataSource::AcceptCallback()
{
  vtkPVWindow *window = this->GetWindow();
 
  this->vtkPVSource::AcceptCallback();
  
  if (this->PolyDataSource == NULL)
    {
    vtkErrorMacro("Source has not been set.");
    }

  if (this->GetPVData() == NULL)
    { // This is the first time, initialize data.  
    vtkPVApplication *pvApp = this->GetPVApplication();
    vtkPVPolyData *pvd;
    vtkPVAssignment *a;
    vtkPVActorComposite *ac;

    this->PolyDataSource->SetStartMethod(vtkPVSourceStartProgress, this);
    this->PolyDataSource->SetProgressMethod(vtkPVSourceReportProgress, this);
    this->PolyDataSource->SetEndMethod(vtkPVSourceEndProgress, this);
  
    pvd = vtkPVPolyData::New();
    pvd->Clone(pvApp);
    a = vtkPVAssignment::New();
    a->Clone(pvApp);
    
    pvd->SetAssignment(a);
    this->SetOutput(pvd);
    
    this->CreateDataPage();
  
    ac = this->GetPVData()->GetActorComposite();
    window->GetMainView()->AddComposite(ac);
    window->GetMainView()->ResetCamera();
    }

  window->GetMainView()->SetSelectedComposite(this);  
  this->GetView()->Render();
  window->GetSourceList()->Update();
}

//----------------------------------------------------------------------------
void vtkPVPolyDataSource::SelectInputSource()
{
  this->GetPVData()->GetActorComposite()->VisibilityOff();
  this->GetWindow()->GetMainView()->
    SetSelectedComposite(this->GetInput()->GetPVSource());
  this->GetInput()->GetActorComposite()->VisibilityOn();
  this->GetView()->Render();
  this->GetWindow()->GetMainView()->ResetCamera();
}















