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
  
  this->Accept = vtkKWPushButton::New();
  this->Accept->SetParent(this->Properties);
  this->SourceButton = vtkKWPushButton::New();
  this->SourceButton->SetParent(this->Properties);
  this->ShrinkFactorScale = vtkKWScale::New();
  this->ShrinkFactorScale->SetParent(this->Properties);

  vtkShrinkPolyData *shrink = vtkShrinkPolyData::New();
  this->SetPolyDataSource(shrink);
  shrink->Delete();
}

//----------------------------------------------------------------------------
vtkPVShrinkPolyData::~vtkPVShrinkPolyData()
{ 
  this->Accept->Delete();
  this->Accept = NULL;
  this->SourceButton->Delete();
  this->SourceButton = NULL;
  
  this->ShrinkFactorScale->Delete();
  this->ShrinkFactorScale = NULL;
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
  
  this->SourceButton->Create(this->Application, "-text GetSource");
  this->SourceButton->SetCommand(this, "SelectInputSource");
  this->Accept->Create(this->Application, "-text Accept");
  this->Accept->SetCommand(this, "ShrinkFactorChanged");
  this->Script("pack %s %s %s",
	       this->SourceButton->GetWidgetName(),
	       this->Accept->GetWidgetName(),
	       this->ShrinkFactorScale->GetWidgetName());
}

//----------------------------------------------------------------------------
// Functions to update the progress bar
void StartShrinkPolyDataProgress(void *arg)
{
  vtkPVShrinkPolyData *me = (vtkPVShrinkPolyData*)arg;
  me->GetWindow()->SetStatusText("Processing Shrink");
}

//----------------------------------------------------------------------------
void ShrinkPolyDataProgress(void *arg)
{
  vtkPVShrinkPolyData *me = (vtkPVShrinkPolyData*)arg;
  me->GetWindow()->GetProgressGauge()->SetValue((int)(me->GetShrink()->GetProgress() * 100));
}

//----------------------------------------------------------------------------
void EndShrinkPolyDataProgress(void *arg)
{
  vtkPVShrinkPolyData *me = (vtkPVShrinkPolyData*)arg;
  me->GetWindow()->SetStatusText("");
  me->GetWindow()->GetProgressGauge()->SetValue(0);
}

//----------------------------------------------------------------------------
void vtkPVShrinkPolyData::ShrinkFactorChanged()
{
  vtkPVApplication *pvApp = (vtkPVApplication *)this->Application;
  vtkPVWindow *window = this->GetWindow();
  
  this->SetShrinkFactor(this->ShrinkFactorScale->GetValue());

  if (this->GetPVData() == NULL)
    { // This is the first time. Create the data.
    this->GetShrink()->SetStartMethod(StartShrinkPolyDataProgress, this);
    this->GetShrink()->SetProgressMethod(ShrinkPolyDataProgress, this);
    this->GetShrink()->SetEndMethod(EndShrinkPolyDataProgress, this);
    this->InitializeData();
    window->GetSourceList()->Update();
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
  
  this->GetShrink()->SetShrinkFactor(factor);
}

//----------------------------------------------------------------------------
vtkShrinkPolyData *vtkPVShrinkPolyData::GetShrink()
{
  return vtkShrinkPolyData::SafeDownCast(this->PolyDataSource);
}
