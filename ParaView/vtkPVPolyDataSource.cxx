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
#include "vtkPVApplication.h"
#include "vtkPVPolyData.h"
#include "vtkPVAssignment.h"
#include "vtkPVWindow.h"
#include "vtkPVActorComposite.h"
#include "vtkPVScalarBar.h"

int vtkPVPolyDataSourceCommand(ClientData cd, Tcl_Interp *interp,
			   int argc, char *argv[]);

//----------------------------------------------------------------------------
vtkPVPolyDataSource::vtkPVPolyDataSource()
{
  this->CommandFunction = vtkPVPolyDataSourceCommand;
}

//----------------------------------------------------------------------------
vtkPVPolyDataSource* vtkPVPolyDataSource::New()
{
  return new vtkPVPolyDataSource();
}

//----------------------------------------------------------------------------
void vtkPVPolyDataSource::SetPVOutput(vtkPVPolyData *pvd)
{
  vtkPVApplication *pvApp = this->GetPVApplication();
  
  if (pvApp && pvApp->GetController()->GetLocalProcessId() == 0)
    {
    pvApp->BroadcastScript("%s SetPVOutput %s", this->GetTclName(), 
			   pvd->GetTclName());
    }

  this->vtkPVSource::SetPVOutput(pvd);
  pvd->SetData(this->GetVTKPolyDataSource()->GetOutput());
}

//----------------------------------------------------------------------------
vtkPVPolyData *vtkPVPolyDataSource::GetPVOutput()
{
  return vtkPVPolyData::SafeDownCast(this->PVOutput);
}

//----------------------------------------------------------------------------
void vtkPVPolyDataSource::InitializePVOutput()
{
  vtkPVData *input;
  vtkPVPolyData *output;
  vtkPVAssignment *assignment;
  
  input = this->vtkPVSource::GetNthPVInput(0);
  output = vtkPVPolyData::New();
  output->Clone(this->GetPVApplication());
  this->SetPVOutput(output);
  
  if (input != NULL)
    {
    assignment = input->GetAssignment();
    }
  else
    {
    assignment = vtkPVAssignment::New();
    assignment->Clone(this->GetPVApplication());
    }
  
  output->SetAssignment(assignment);
}

//----------------------------------------------------------------------------
void vtkPVPolyDataSource::SelectInputSource()
{
  this->GetPVOutput()->GetActorComposite()->VisibilityOff();
  this->GetPVOutput()->GetScalarBar()->VisibilityOff();
  this->GetWindow()->GetMainView()->
    SetSelectedComposite(this->GetPVInput()->GetPVSource());
  this->GetPVInput()->GetActorComposite()->VisibilityOn();
  this->GetPVInput()->GetScalarBar()->VisibilityOn();
  this->GetView()->Render();
  this->GetWindow()->GetMainView()->ResetCamera();
}

//----------------------------------------------------------------------------
vtkPolyDataSource *vtkPVPolyDataSource::GetVTKPolyDataSource()
{
  vtkPolyDataSource *pds = NULL;

  if (this->VTKSource)
    {
    pds = vtkPolyDataSource::SafeDownCast(this->VTKSource);
    }
  if (pds == NULL)
    {
    vtkWarningMacro("Could not get the vtkPolyDataSource.");
    }
  return pds;
}

//---------------------------------------------------------------------------
vtkPVPolyData *vtkPVPolyDataSource::GetPVInput()
{
  return (vtkPVPolyData *)this->vtkPVSource::GetNthPVInput(0);
}
