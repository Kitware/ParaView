/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkPVPolyDataToPolyDataFilter.cxx
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

#include "vtkPVPolyDataToPolyDataFilter.h"
#include "vtkPVApplication.h"
#include "vtkPVPolyData.h"
#include "vtkPVActorComposite.h"
#include "vtkPVWindow.h"


int vtkPVPolyDataToPolyDataFilterCommand(ClientData cd, Tcl_Interp *interp,
			   int argc, char *argv[]);

//----------------------------------------------------------------------------
vtkPVPolyDataToPolyDataFilter::vtkPVPolyDataToPolyDataFilter()
{
  this->CommandFunction = vtkPVPolyDataToPolyDataFilterCommand;
}

//----------------------------------------------------------------------------
vtkPVPolyDataToPolyDataFilter::~vtkPVPolyDataToPolyDataFilter()
{ 
}

//----------------------------------------------------------------------------
vtkPVPolyDataToPolyDataFilter* vtkPVPolyDataToPolyDataFilter::New()
{
  return new vtkPVPolyDataToPolyDataFilter();
}

//----------------------------------------------------------------------------
void vtkPVPolyDataToPolyDataFilter::SetInput(vtkPVPolyData *pvData)
{
  vtkPVApplication *pvApp = this->GetPVApplication();
  vtkPolyDataToPolyDataFilter *f;
  
  f = vtkPolyDataToPolyDataFilter::SafeDownCast(this->PolyDataSource);
  
  if (pvApp && pvApp->GetController()->GetLocalProcessId() == 0)
    {
    pvApp->BroadcastScript("%s SetInput %s", this->GetTclName(),
			   pvData->GetTclName());
    }  
  
  f->SetInput(pvData->GetPolyData());
  this->Input = pvData;
}

//----------------------------------------------------------------------------
void vtkPVPolyDataToPolyDataFilter::InitializeData()
{
  // Right now, this only deals with polydata.  This needs to be changed.

  vtkPVApplication *pvApp = (vtkPVApplication *)this->Application;
  vtkPVPolyData *newData;
  vtkPVAssignment *a;
  vtkPVWindow *window = this->GetWindow();
  vtkPVActorComposite *ac;

  newData = vtkPVPolyData::New();
  newData->Clone(pvApp);
  this->SetOutput(newData);
  a = this->GetInput()->GetAssignment();
  newData->SetAssignment(a);
  this->GetInput()->GetActorComposite()->VisibilityOff();
  this->CreateDataPage();
  ac = this->GetPVData()->GetActorComposite();
  window->GetMainView()->AddComposite(ac);

  newData->Delete();
}

