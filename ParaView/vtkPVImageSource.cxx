/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkPVImageSource.cxx
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

#include "vtkPVImageSource.h"
#include "vtkImageSource.h"
#include "vtkPVApplication.h"
#include "vtkPVImageData.h"
#include "vtkPVActorComposite.h"
#include "vtkPVWindow.h"
#include "vtkPVAssignment.h"

int vtkPVPolyDataSourceCommand(ClientData cd, Tcl_Interp *interp,
			   int argc, char *argv[]);

//----------------------------------------------------------------------------
vtkPVImageSource::vtkPVImageSource()
{
  this->CommandFunction = vtkPVPolyDataSourceCommand;
}

//----------------------------------------------------------------------------
vtkPVImageSource* vtkPVImageSource::New()
{
  return new vtkPVImageSource();
}

//----------------------------------------------------------------------------
void vtkPVImageSource::SetPVOutput(vtkPVImageData *pvi)
{
  vtkPVApplication *pvApp = this->GetPVApplication();

  if (pvApp && pvApp->GetController()->GetLocalProcessId() == 0)
    {
    pvApp->BroadcastScript("%s SetPVOutput %s", this->GetTclName(), 
			   pvi->GetTclName());
    }

  this->SetPVData(pvi);
  pvi->SetData(this->GetVTKImageSource()->GetOutput());  
}

//----------------------------------------------------------------------------
vtkPVImageData *vtkPVImageSource::GetPVOutput()
{
  return vtkPVImageData::SafeDownCast(this->PVOutput);
}

//----------------------------------------------------------------------------
void vtkPVImageSource::InitializeOutput()
{
  vtkPVImageData *output;
  vtkPVData *input;
  vtkPVAssignment *assignment;  

  output = vtkPVImageData::New();
  output->Clone(this->GetPVApplication());
  this->SetPVOutput(output);

  input = this->Input;  
  if (input != NULL)
    {
    assignment = input->GetAssignment();
    }
  else
    {
    assignment = vtkPVAssignment::New();
    assignment->Clone(this->GetPVApplication());
    assignment->SetOriginalImage(this->GetPVOutput());
    }
  
  output->SetAssignment(assignment);
}

//----------------------------------------------------------------------------
vtkImageSource *vtkPVImageSource::GetVTKImageSource()
{
  vtkImageSource *imageSource = NULL;

  if (this->VTKSource)
    {
    imageSource = vtkImageSource::SafeDownCast(this->VTKSource);
    }
  if (imageSource == NULL)
    {
    vtkWarningMacro("Could not get the vtkImageSource.");
    }
  return imageSource;
}
