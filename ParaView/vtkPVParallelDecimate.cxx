/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkPVParallelDecimate.cxx
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

#include "vtkPVParallelDecimate.h"
#include "vtkPVApplication.h"
#include "vtkPVRenderView.h"
#include "vtkPVPolyData.h"
#include "vtkPVWindow.h"
#include "vtkPVActorComposite.h"

int vtkPVParallelDecimateCommand(ClientData cd, Tcl_Interp *interp,
				 int argc, char *argv[]);

//----------------------------------------------------------------------------
vtkPVParallelDecimate::vtkPVParallelDecimate()
{
  this->CommandFunction = vtkPVParallelDecimateCommand;
  
  this->Accept = vtkKWPushButton::New();
  this->Accept->SetParent(this->Properties);
  this->SourceButton = vtkKWPushButton::New();
  this->SourceButton->SetParent(this->Properties);

  vtkParallelDecimate *pd = vtkParallelDecimate::New();
  this->SetVTKSource(pd);
  pd->Delete();
}

//----------------------------------------------------------------------------
vtkPVParallelDecimate::~vtkPVParallelDecimate()
{ 
  this->Accept->Delete();
  this->Accept = NULL;
  this->SourceButton->Delete();
  this->SourceButton = NULL;
}

//----------------------------------------------------------------------------
vtkPVParallelDecimate* vtkPVParallelDecimate::New()
{
  return new vtkPVParallelDecimate();
}

//----------------------------------------------------------------------------
void vtkPVParallelDecimate::CreateProperties()
{  
  // must set the application
  this->vtkPVSource::CreateProperties();
  
  this->SourceButton->Create(this->Application, "-text GetSource");
  this->SourceButton->SetCommand(this, "SelectInputSource");
  this->Accept->Create(this->Application, "-text Accept");
  this->Accept->SetCommand(this, "ParallelDecimateChanged");
  this->Script("pack %s %s",
	       this->SourceButton->GetWidgetName(),
	       this->Accept->GetWidgetName());
}

//----------------------------------------------------------------------------
void vtkPVParallelDecimate::ParallelDecimateChanged()
{
  vtkPVApplication *pvApp = (vtkPVApplication *)this->Application;
  vtkPVWindow *window = this->GetWindow();
  
  if (this->GetPVData() == NULL)
    { // This is the first time. Create the data.
    //this->InitializeData();
    window->GetSourceList()->Update();
    }
  
  window->GetMainView()->SetSelectedComposite(this);
  
  this->GetView()->Render();
}

//----------------------------------------------------------------------------
void vtkPVParallelDecimate::SetApplication(vtkKWApplication *app)
{
  vtkPVApplication *pvApp = vtkPVApplication::SafeDownCast(app);

  this->GetParallelDecimate()->SetApplication(pvApp);
  this->vtkPVPolyDataToPolyDataFilter::SetApplication(app);
}

//----------------------------------------------------------------------------
vtkParallelDecimate *vtkPVParallelDecimate::GetParallelDecimate()
{
  return vtkParallelDecimate::SafeDownCast(this->GetVTKSource());
}
