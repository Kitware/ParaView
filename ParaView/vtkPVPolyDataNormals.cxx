/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkPVPolyDataNormals.cxx
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

#include "vtkPVPolyDataNormals.h"
#include "vtkPVApplication.h"
#include "vtkKWView.h"
#include "vtkKWRenderView.h"
#include "vtkPVPolyData.h"
#include "vtkPVImage.h"
#include "vtkPVWindow.h"
#include "vtkPVActorComposite.h"

int vtkPVPolyDataNormalsCommand(ClientData cd, Tcl_Interp *interp,
				int argc, char *argv[]);

//----------------------------------------------------------------------------
vtkPVPolyDataNormals::vtkPVPolyDataNormals()
{
  this->CommandFunction = vtkPVPolyDataNormalsCommand;
  
  this->Accept = vtkKWPushButton::New();
  this->Accept->SetParent(this->Properties);
  
  this->SourceButton = vtkKWPushButton::New();
  this->SourceButton->SetParent(this->Properties);
  
  this->Splitting = vtkKWCheckButton::New();
  this->Splitting->SetParent(this->Properties);

  this->FeatureAngleEntry = vtkKWLabeledEntry::New();
  this->FeatureAngleEntry->SetParent(this->Properties);
  
  this->PolyDataNormals = vtkPolyDataNormals::New();
}

//----------------------------------------------------------------------------
vtkPVPolyDataNormals::~vtkPVPolyDataNormals()
{
  this->Accept->Delete();
  this->Accept = NULL;
  
  this->SourceButton->Delete();
  this->SourceButton = NULL;
  
  this->Splitting->Delete();
  this->Splitting = NULL;
  
  this->FeatureAngleEntry->Delete();
  this->FeatureAngleEntry = NULL;
  
  this->PolyDataNormals->Delete();
  this->PolyDataNormals = NULL;
}

//----------------------------------------------------------------------------
vtkPVPolyDataNormals* vtkPVPolyDataNormals::New()
{
  return new vtkPVPolyDataNormals();
}

//----------------------------------------------------------------------------
void vtkPVPolyDataNormals::CreateProperties()
{ 
  // must set the application
  this->vtkPVSource::CreateProperties();
  
  this->SourceButton->Create(this->Application, "-text GetSource");
  this->SourceButton->SetCommand(this, "GetSource");
  
  this->Accept->Create(this->Application, "-text Accept");
  this->Accept->SetCommand(this, "NormalsParameterChanged");

  this->Splitting->Create(this->Application, "-text Splitting");
  this->Splitting->SetState(0);
  
  this->FeatureAngleEntry->Create(this->Application);
  this->FeatureAngleEntry->SetLabel("Feature Angle:");
  this->FeatureAngleEntry->SetValue(this->GetPolyDataNormals()->GetFeatureAngle());
  
  this->Script("pack %s %s %s %s",
	       this->SourceButton->GetWidgetName(),
	       this->Accept->GetWidgetName(),
	       this->Splitting->GetWidgetName(),
	       this->FeatureAngleEntry->GetWidgetName());
}

//----------------------------------------------------------------------------
void vtkPVPolyDataNormals::SetInput(vtkPVData *pvData)
{
  vtkPVApplication *pvApp = this->GetPVApplication();
  
  if (pvApp && pvApp->GetController()->GetLocalProcessId() == 0)
    {
    pvApp->BroadcastScript("%s SetInput %s", this->GetTclName(),
			   pvData->GetTclName());
    }  
  
  this->GetPolyDataNormals()->SetInput(vtkPolyData::SafeDownCast(pvData->GetData()));
  this->Input = pvData;
}

//----------------------------------------------------------------------------
void vtkPVPolyDataNormals::SetOutput(vtkPVPolyData *pvd)
{
  vtkPVApplication *pvApp = this->GetPVApplication();
  
  if (pvApp && pvApp->GetController()->GetLocalProcessId() == 0)
    {
    pvApp->BroadcastScript("%s SetOutput %s", this->GetTclName(),
			   pvd->GetTclName());
    }  
  
  this->SetPVData(pvd);  
  pvd->SetData(this->GetPolyDataNormals()->GetOutput());
}

//----------------------------------------------------------------------------
vtkPVPolyData *vtkPVPolyDataNormals::GetOutput()
{
  return vtkPVPolyData::SafeDownCast(this->Output);
}

//----------------------------------------------------------------------------
// Functions to update the progress bar
void StartNormalsProgress(void *arg)
{
  vtkPVPolyDataNormals *me = (vtkPVPolyDataNormals*)arg;
  me->GetWindow()->SetStatusText("Processing PolyData Normals");
}

//----------------------------------------------------------------------------
void NormalsProgress(void *arg)
{
  vtkPVPolyDataNormals *me = (vtkPVPolyDataNormals*)arg;
  me->GetWindow()->GetProgressGauge()->SetValue((int)(me->GetPolyDataNormals()->GetProgress() * 100));
}

//----------------------------------------------------------------------------
void EndNormalsProgress(void *arg)
{
  vtkPVPolyDataNormals *me = (vtkPVPolyDataNormals*)arg;
  me->GetWindow()->SetStatusText("");
  me->GetWindow()->GetProgressGauge()->SetValue(0);
}

//----------------------------------------------------------------------------
void vtkPVPolyDataNormals::NormalsParameterChanged()
{
  vtkPVApplication *pvApp = this->GetPVApplication();
  vtkPVWindow *window = this->GetWindow();
  vtkPVPolyData *pvd;
  vtkPVAssignment *a;
  vtkPVActorComposite *ac;
  
  this->SetSplitting(this->Splitting->GetState());
  this->SetFeatureAngle(this->FeatureAngleEntry->GetValueAsFloat());
  
  if (this->GetPVData() == NULL)
    { // This is the first time.  Create the data.
    this->GetPolyDataNormals()->SetStartMethod(StartNormalsProgress, this);
    this->GetPolyDataNormals()->SetProgressMethod(NormalsProgress, this);
    this->GetPolyDataNormals()->SetEndMethod(EndNormalsProgress, this);
    pvd = vtkPVPolyData::New();
    pvd->Clone(pvApp);
    this->SetOutput(pvd);
    a = this->GetInput()->GetAssignment();
    pvd->SetAssignment(a);
    this->GetInput()->GetActorComposite()->VisibilityOff();
    this->CreateDataPage();
    ac = this->GetPVData()->GetActorComposite();
    window->GetMainView()->AddComposite(ac);
    }
  
  window->GetMainView()->SetSelectedComposite(this);
  this->GetView()->Render();
  window->GetMainView()->ResetCamera();
}

//----------------------------------------------------------------------------
void vtkPVPolyDataNormals::SetSplitting(int split)
{
  vtkPVApplication *pvApp = this->GetPVApplication();
  
  if (pvApp && pvApp->GetController()->GetLocalProcessId() == 0)
    {
    pvApp->BroadcastScript("%s SetSplitting %d", this->GetTclName(),
			   split);
    }  
  
  this->GetPolyDataNormals()->SetSplitting(split);
}

//----------------------------------------------------------------------------
void vtkPVPolyDataNormals::SetFeatureAngle(float angle)
{
  vtkPVApplication *pvApp = this->GetPVApplication();
  
  if (pvApp && pvApp->GetController()->GetLocalProcessId() == 0)
    {
    pvApp->BroadcastScript("%s SetFeatureAngle %f", this->GetTclName(),
			   angle);
    }
  
  this->GetPolyDataNormals()->SetFeatureAngle(angle);
}
