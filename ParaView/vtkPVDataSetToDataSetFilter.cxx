/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkPVDataSetToDataSetFilter.cxx
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

#include "vtkPVDataSetToDataSetFilter.h"
#include "vtkPVApplication.h"
#include "vtkPVPolyData.h"
#include "vtkPVImageData.h"
#include "vtkPVActorComposite.h"
#include "vtkPVWindow.h"

int vtkPVDataSetToDataSetFilterCommand(ClientData cd, Tcl_Interp *interp,
				int argc, char *argv[]);

//----------------------------------------------------------------------------
vtkPVDataSetToDataSetFilter::vtkPVDataSetToDataSetFilter()
{
  this->CommandFunction = vtkPVDataSetToDataSetFilterCommand;
}


//----------------------------------------------------------------------------
vtkPVDataSetToDataSetFilter* vtkPVDataSetToDataSetFilter::New()
{
  return new vtkPVDataSetToDataSetFilter();
}


//----------------------------------------------------------------------------
void vtkPVDataSetToDataSetFilter::InitializeOutput()
{
  vtkPVData *input;
  vtkPVData *output;
  vtkPVAssignment *assignment;
  
  input = this->GetInput();
  if (input == NULL)
    {
    vtkErrorMacro("Input not set.");
    return;
    }
  if (input->IsA("vtkPVPolyData"))
    {
    output = vtkPVPolyData::New();
    }
  else if (input->IsA("vtkPVImageData"))
    {
    output = vtkPVImageData::New();
    }
  else
    {
    vtkErrorMacro("Cannot determine type of input: " << input->GetClassName());
    return;
    }
  output->Clone(this->GetPVApplication());
  this->SetPVOutput(output);
  
  assignment = input->GetAssignment();
  output->SetAssignment(assignment);
}

//----------------------------------------------------------------------------
void vtkPVDataSetToDataSetFilter::SetPVOutput(vtkPVData *pvd)
{
  vtkPVApplication *pvApp = this->GetPVApplication();
  vtkDataSetToDataSetFilter *f = this->GetVTKDataSetToDataSetFilter();

  if (pvApp && pvApp->GetController()->GetLocalProcessId() == 0)
    {
    pvApp->BroadcastScript("%s SetPVOutput %s", this->GetTclName(), 
			   pvd->GetTclName());
    }
  // This calls just does reference counting.
  this->SetPVData(pvd);  
  pvd->SetData(f->GetOutput());
}

//----------------------------------------------------------------------------
vtkPVData *vtkPVDataSetToDataSetFilter::GetPVOutput()
{
  return this->PVOutput;
}

//----------------------------------------------------------------------------
vtkPVPolyData *vtkPVDataSetToDataSetFilter::GetPVPolyDataOutput()
{
  return vtkPVPolyData::SafeDownCast(this->PVOutput);
}

//----------------------------------------------------------------------------
vtkPVImageData *vtkPVDataSetToDataSetFilter::GetPVImageDataOutput()
{
  return vtkPVImageData::SafeDownCast(this->PVOutput);
}


//----------------------------------------------------------------------------
void vtkPVDataSetToDataSetFilter::SetInput(vtkPVData *pvData)
{
  vtkDataSetToDataSetFilter *f;
  
  f = vtkDataSetToDataSetFilter::SafeDownCast(this->GetVTKSource());
  if (f == NULL)
    {
    vtkErrorMacro("Could not get source as vtkDataSetToDataSetFilter. "
		  << "Did you choose the correct superclass?");
    return;
    }
  
  f->SetInput(pvData->GetData());

  this->vtkPVSource::SetNthInput(0, pvData);
  if (pvData)
    {
    this->Inputs[0]->AddPVSourceToUsers(this);
    }
}

//----------------------------------------------------------------------------
vtkDataSetToDataSetFilter*
vtkPVDataSetToDataSetFilter::GetVTKDataSetToDataSetFilter()
{
  vtkDataSetToDataSetFilter *f = NULL;

  if (this->VTKSource)
    {
    f = vtkDataSetToDataSetFilter::SafeDownCast(this->VTKSource);
    }
  if (f == NULL)
    {
    vtkWarningMacro("Could not get the vtkDataSetToDataSetFilter.");
    }
  return f;
}

//--------------------------------------------------------------------------
vtkPVData *vtkPVDataSetToDataSetFilter::GetInput()
{
  return (vtkPVData *)(this->vtkPVSource::GetNthInput(0));
}
