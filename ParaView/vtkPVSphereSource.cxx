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
  
  this->Accept = vtkKWPushButton::New();
  this->Accept->SetParent(this->Properties);

  vtkSphereSource *s = vtkSphereSource::New();  
  this->SetPolyDataSource(s);
  s->Delete();
}

//----------------------------------------------------------------------------
vtkPVSphereSource::~vtkPVSphereSource()
{

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

  this->Accept->Create(this->Application, "-text Accept");
  this->Accept->SetCommand(this, "SphereParameterChanged");
  this->Script("pack %s", this->Accept->GetWidgetName());
  
  this->AddLabeledEntry("Radius:", "SetRadius", "GetRadius");
  this->AddXYZEntry("Center", "SetCenter", "GetCenter");
  this->AddLabeledEntry("Phi Resolution:", "SetPhiResolution", "GetPhiResolution");
  this->AddLabeledEntry("Theta Resolution:", "SetThetaResolution", "GetThetaResolution");
  this->AddLabeledEntry("Start Theta:", "SetStartTheta", "GetStartTheta");
  this->AddLabeledEntry("End Theta:", "SetEndTheta", "GetEndTheta");
  this->AddLabeledEntry("Start Phi:", "SetStartPhi", "GetStartPhi");
  this->AddLabeledEntry("End Phi:", "SetEndPhi", "GetEndPhi");


}

//----------------------------------------------------------------------------
// Functions to update the progress bar
void StartSphereSourceProgress(void *arg)
{
  vtkPVSphereSource *me = (vtkPVSphereSource*)arg;
  me->GetWindow()->SetStatusText("Processing SphereSource");
}

//----------------------------------------------------------------------------
void SphereSourceProgress(void *arg)
{
  vtkPVSphereSource *me = (vtkPVSphereSource*)arg;
  me->GetWindow()->GetProgressGauge()->SetValue((int)(me->GetSphereSource()->GetProgress() * 100));
}

//----------------------------------------------------------------------------
void EndSphereSourceProgress(void *arg)
{
  vtkPVSphereSource *me = (vtkPVSphereSource*)arg;
  me->GetWindow()->SetStatusText("");
  me->GetWindow()->GetProgressGauge()->SetValue(0);
}

//----------------------------------------------------------------------------
void vtkPVSphereSource::SphereParameterChanged()
{
  vtkPVApplication *pvApp = this->GetPVApplication();
  vtkPVWindow *window = this->GetWindow();
 
  if (this->GetPVData() == NULL)
    { // This is the first time, initialize data.
    this->GetSphereSource()->SetStartMethod(StartSphereSourceProgress, this);
    this->GetSphereSource()->SetProgressMethod(SphereSourceProgress, this);
    this->GetSphereSource()->SetEndMethod(EndSphereSourceProgress, this);
    this->InitializeData();
    }
  
  if (window->GetPreviousSource() != NULL)
    {
    window->GetPreviousSource()->GetPVData()->GetActorComposite()->VisibilityOff();
    }
  
  window->GetMainView()->SetSelectedComposite(this);

  // ####
  int i;
  for (i = 0; i < this->NumberOfAcceptCommands; ++i)
    {
    this->Script(this->AcceptCommands[i]);
    }
  // ####

  window->GetMainView()->ResetCamera();
  this->GetView()->Render();
  window->GetSourceList()->Update();
}


//----------------------------------------------------------------------------
vtkSphereSource *vtkPVSphereSource::GetSphereSource()
{
  return vtkSphereSource::SafeDownCast(this->PolyDataSource);
}














