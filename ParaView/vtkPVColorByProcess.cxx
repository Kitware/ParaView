/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkPVColorByProcess.cxx
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

#include "vtkPVColorByProcess.h"
#include "vtkPVApplication.h"
#include "vtkKWView.h"
#include "vtkKWRenderView.h"
#include "vtkPVPolyData.h"
#include "vtkPVImageData.h"
#include "vtkPVWindow.h"

int vtkPVColorByProcessCommand(ClientData cd, Tcl_Interp *interp,
				int argc, char *argv[]);

//----------------------------------------------------------------------------
vtkPVColorByProcess::vtkPVColorByProcess()
{
  this->CommandFunction = vtkPVColorByProcessCommand;
  
  this->Accept = vtkKWPushButton::New();
  this->Accept->SetParent(this->Properties);
  
  this->SourceButton = vtkKWPushButton::New();
  this->SourceButton->SetParent(this->Properties);
  
  vtkColorByProcess *f = vtkColorByProcess::New();
  this->SetVTKSource(f);
  f->Delete();
}

//----------------------------------------------------------------------------
vtkPVColorByProcess::~vtkPVColorByProcess()
{
  this->Accept->Delete();
  this->Accept = NULL;
  
  this->SourceButton->Delete();
  this->SourceButton = NULL;
}

//----------------------------------------------------------------------------
vtkPVColorByProcess* vtkPVColorByProcess::New()
{
  return new vtkPVColorByProcess();
}

//----------------------------------------------------------------------------
vtkColorByProcess* vtkPVColorByProcess::GetFilter()
{
  return vtkColorByProcess::SafeDownCast(this->GetVTKSource());
}

//----------------------------------------------------------------------------
void vtkPVColorByProcess::SetApplication(vtkKWApplication *app)
{
  vtkPVApplication *pvApp = vtkPVApplication::SafeDownCast(app);

  this->GetFilter()->SetController(pvApp->GetController());
  this->vtkPVDataSetToDataSetFilter::SetApplication(app);
}

//----------------------------------------------------------------------------
void vtkPVColorByProcess::CreateProperties()
{  
  // must set the application
  this->vtkPVSource::CreateProperties();
  
  this->SourceButton->Create(this->Application, "-text GetSource");
  this->SourceButton->SetCommand(this, "GetSource");
  this->Script("pack %s", this->SourceButton->GetWidgetName());
  
  this->Accept->Create(this->Application, "-text Accept");
  this->Accept->SetCommand(this, "ParameterChanged");
  this->Script("pack %s", this->Accept->GetWidgetName());
}

//----------------------------------------------------------------------------
// Functions to update the progress bar
void StartColorByProcessProgress(void *arg)
{
  vtkPVColorByProcess *me = (vtkPVColorByProcess*)arg;
  me->GetWindow()->SetStatusText("Processing ColorByProcess");
}

//----------------------------------------------------------------------------
void ColorByProcessProgress(void *arg)
{
  vtkPVColorByProcess *me = (vtkPVColorByProcess*)arg;
  me->GetWindow()->GetProgressGauge()->SetValue((int)(me->GetFilter()->GetProgress() * 100));
}

//----------------------------------------------------------------------------
void EndColorByProcessProgress(void *arg)
{
  vtkPVColorByProcess *me = (vtkPVColorByProcess*)arg;
  me->GetWindow()->SetStatusText("");
  me->GetWindow()->GetProgressGauge()->SetValue(0);
}

//----------------------------------------------------------------------------
void vtkPVColorByProcess::ParameterChanged()
{
  vtkPVWindow *window = this->GetWindow();
  
  if (this->GetPVData() == NULL)
    { // This is the first time. Initialize data.
    this->GetFilter()->SetStartMethod(StartColorByProcessProgress, this);
    this->GetFilter()->SetProgressMethod(ColorByProcessProgress, this);
    this->GetFilter()->SetEndMethod(EndColorByProcessProgress, this);
    //this->InitializeData();
    window->GetSourceList()->Update();
    }
  
  this->GetView()->Render();
  window->GetMainView()->SetSelectedComposite(this);
}
