/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkPVContourFilter.cxx
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

#include "vtkPVContourFilter.h"
#include "vtkKitwareContourFilter.h"


int vtkPVContourFilterCommand(ClientData cd, Tcl_Interp *interp,
			      int argc, char *argv[]);

//----------------------------------------------------------------------------
vtkPVContourFilter::vtkPVContourFilter()
{
  this->CommandFunction = vtkPVContourFilterCommand;
    
  vtkKitwareContourFilter *c = vtkKitwareContourFilter::New();
  this->SetVTKSource(c);
  c->Delete();
}

//----------------------------------------------------------------------------
vtkPVContourFilter* vtkPVContourFilter::New()
{
  return new vtkPVContourFilter();
}

//----------------------------------------------------------------------------
void vtkPVContourFilter::CreateProperties()
{
  this->vtkPVSource::CreateProperties();
  
  this->AddLabeledEntry("Value:","SetValue","GetValue",this);

  this->UpdateParameterWidgets();
}


//----------------------------------------------------------------------------
vtkKitwareContourFilter *vtkPVContourFilter::GetContour()
{
  return vtkKitwareContourFilter::SafeDownCast(this->GetVTKSource());
}

//----------------------------------------------------------------------------
void vtkPVContourFilter::SetValue(float val)
{  
  this->GetContour()->SetValue(0, val);
}

//----------------------------------------------------------------------------
float vtkPVContourFilter::GetValue()
{  
  return this->GetContour()->GetValue(0);
}

