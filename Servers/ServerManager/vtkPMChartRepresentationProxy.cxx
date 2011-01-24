/*=========================================================================

  Program:   ParaView
  Module:    $RCSfile$

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkPMChartRepresentationProxy.h"

#include "vtkChartRepresentation.h"
#include "vtkContextNamedOptions.h"
#include "vtkObjectFactory.h"

vtkStandardNewMacro(vtkPMChartRepresentationProxy);
//----------------------------------------------------------------------------
vtkPMChartRepresentationProxy::vtkPMChartRepresentationProxy()
{
}

//----------------------------------------------------------------------------
vtkPMChartRepresentationProxy::~vtkPMChartRepresentationProxy()
{
}

//----------------------------------------------------------------------------
void vtkPMChartRepresentationProxy::OnCreateVTKObjects()
{
  vtkPMProxy* optionsProxy = this->GetSubProxyHelper("PlotOptions");
  if (optionsProxy)
    {
    vtkContextNamedOptions* options = vtkContextNamedOptions::SafeDownCast(
      optionsProxy->GetVTKObject());
    vtkChartRepresentation* repr = vtkChartRepresentation::SafeDownCast(
      this->GetVTKObject());
    repr->SetOptions(options);
    }
}

//----------------------------------------------------------------------------
void vtkPMChartRepresentationProxy::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}
