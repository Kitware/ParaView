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


int vtkPVPolyDataToPolyDataFilterCommand(ClientData cd, Tcl_Interp *interp,
			   int argc, char *argv[]);

//----------------------------------------------------------------------------
vtkPVPolyDataToPolyDataFilter::vtkPVPolyDataToPolyDataFilter()
{
  this->CommandFunction = vtkPVPolyDataToPolyDataFilterCommand;
}

//----------------------------------------------------------------------------
vtkPVPolyDataToPolyDataFilter* vtkPVPolyDataToPolyDataFilter::New()
{
  return new vtkPVPolyDataToPolyDataFilter();
}

//----------------------------------------------------------------------------
void vtkPVPolyDataToPolyDataFilter::SetInput(vtkPVPolyData *pvData)
{
  vtkPolyDataToPolyDataFilter *f;
  
  // Set the input of the VTK filter.
  f = vtkPolyDataToPolyDataFilter::SafeDownCast(this->GetVTKSource());
  f->SetInput(pvData->GetPolyData());

  this->vtkPVSource::SetNthInput(0, pvData);
}


