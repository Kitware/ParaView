/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkPVTubeFilter.cxx
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

#include "vtkPVTubeFilter.h"
#include "vtkPVApplication.h"
#include "vtkPVRenderView.h"
#include "vtkPVPolyData.h"
#include "vtkPVWindow.h"
#include "vtkPVActorComposite.h"

int vtkPVTubeFilterCommand(ClientData cd, Tcl_Interp *interp,
			   int argc, char *argv[]);

//----------------------------------------------------------------------------
vtkPVTubeFilter::vtkPVTubeFilter()
{
  this->CommandFunction = vtkPVTubeFilterCommand;
  
  this->Accept = vtkKWPushButton::New();
  this->Accept->SetParent(this->Properties);
  this->SourceButton = vtkKWPushButton::New();
  this->SourceButton->SetParent(this->Properties);
  this->RadiusEntry = vtkKWLabeledEntry::New();
  this->RadiusEntry->SetParent(this->Properties);
  this->SidesEntry = vtkKWLabeledEntry::New();
  this->SidesEntry->SetParent(this->Properties);
  
  vtkTubeFilter *tube = vtkTubeFilter::New();
  this->SetVTKSource(tube);
  tube->Delete();
}

//----------------------------------------------------------------------------
vtkPVTubeFilter::~vtkPVTubeFilter()
{ 
  this->Accept->Delete();
  this->Accept = NULL;
  this->SourceButton->Delete();
  this->SourceButton = NULL;
  
  this->RadiusEntry->Delete();
  this->RadiusEntry = NULL;
  this->SidesEntry->Delete();
  this->SidesEntry = NULL;
}

//----------------------------------------------------------------------------
vtkPVTubeFilter* vtkPVTubeFilter::New()
{
  return new vtkPVTubeFilter();
}

//----------------------------------------------------------------------------
void vtkPVTubeFilter::CreateProperties()
{  
  // must set the application
  this->vtkPVSource::CreateProperties();
  
  this->RadiusEntry->Create(this->Application);
  this->RadiusEntry->SetLabel("Radius: ");
  this->RadiusEntry->SetValue(this->GetTubeFilter()->GetRadius(), 2);
  this->SidesEntry->Create(this->Application);
  this->SidesEntry->SetLabel("Number of Sides: ");
  this->SidesEntry->SetValue(this->GetTubeFilter()->GetNumberOfSides());
  
  this->SourceButton->Create(this->Application, "-text GetSource");
  this->SourceButton->SetCommand(this, "SelectInputSource");
  this->Accept->Create(this->Application, "-text Accept");
  this->Accept->SetCommand(this, "TubeFilterChanged");
  this->Script("pack %s %s %s %s",
	       this->SourceButton->GetWidgetName(),
	       this->Accept->GetWidgetName(),
	       this->RadiusEntry->GetWidgetName(),
	       this->SidesEntry->GetWidgetName());
}

//----------------------------------------------------------------------------
// Functions to update the progress bar
void StartTubeFilterProgress(void *arg)
{
  vtkPVTubeFilter *me = (vtkPVTubeFilter*)arg;
  me->GetWindow()->SetStatusText("Processing Tube Filter");
}

//----------------------------------------------------------------------------
void TubeFilterProgress(void *arg)
{
  vtkPVTubeFilter *me = (vtkPVTubeFilter*)arg;
  me->GetWindow()->GetProgressGauge()->SetValue((int)(me->GetTubeFilter()->GetProgress() * 100));
}

//----------------------------------------------------------------------------
void EndTubeFilterProgress(void *arg)
{
  vtkPVTubeFilter *me = (vtkPVTubeFilter*)arg;
  me->GetWindow()->SetStatusText("");
  me->GetWindow()->GetProgressGauge()->SetValue(0);
}

//----------------------------------------------------------------------------
void vtkPVTubeFilter::TubeFilterChanged()
{
  vtkPVApplication *pvApp = (vtkPVApplication *)this->Application;
  vtkPVWindow *window = this->GetWindow();
  
  this->SetRadius(this->RadiusEntry->GetValueAsFloat());
  this->SetNumberOfSides(this->SidesEntry->GetValueAsInt());

  if (this->GetPVData() == NULL)
    { // This is the first time. Create the data.
    this->GetTubeFilter()->SetStartMethod(StartTubeFilterProgress, this);
    this->GetTubeFilter()->SetProgressMethod(TubeFilterProgress, this);
    this->GetTubeFilter()->SetEndMethod(EndTubeFilterProgress, this);

    //this->InitializeData();
    window->GetSourceList()->Update();
    }

  window->GetMainView()->SetSelectedComposite(this);
  this->GetView()->Render();
}

//----------------------------------------------------------------------------
void vtkPVTubeFilter::SetRadius(float radius)
{
  vtkPVApplication *pvApp = this->GetPVApplication();
  if (pvApp && pvApp->GetController()->GetLocalProcessId() == 0)
    {
    pvApp->BroadcastScript("%s SetRadius %f", this->GetTclName(), 
			   radius);
    }
  
  this->GetTubeFilter()->SetRadius(radius);
}

//----------------------------------------------------------------------------
void vtkPVTubeFilter::SetNumberOfSides(int sides)
{
  vtkPVApplication *pvApp = this->GetPVApplication();
  if (pvApp && pvApp->GetController()->GetLocalProcessId() == 0)
    {
    pvApp->BroadcastScript("%s SetNumberOfSides %d", this->GetTclName(),
			   sides);
    }
  
  this->GetTubeFilter()->SetNumberOfSides(sides);
}

//----------------------------------------------------------------------------
vtkTubeFilter *vtkPVTubeFilter::GetTubeFilter()
{
  return vtkTubeFilter::SafeDownCast(this->GetVTKSource());
}


