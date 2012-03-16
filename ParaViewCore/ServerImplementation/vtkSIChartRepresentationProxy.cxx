/*=========================================================================

  Program:   ParaView
  Module:    vtkSIChartRepresentationProxy.cxx

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkSIChartRepresentationProxy.h"

#include "vtkChartRepresentation.h"
#include "vtkChartNamedOptions.h"
#include "vtkObjectFactory.h"

vtkStandardNewMacro(vtkSIChartRepresentationProxy);
//----------------------------------------------------------------------------
vtkSIChartRepresentationProxy::vtkSIChartRepresentationProxy()
{
}

//----------------------------------------------------------------------------
vtkSIChartRepresentationProxy::~vtkSIChartRepresentationProxy()
{
}

//----------------------------------------------------------------------------
void vtkSIChartRepresentationProxy::OnCreateVTKObjects()
{
  vtkSIProxy* optionsProxy = this->GetSubSIProxy("PlotOptions");
  if (optionsProxy)
    {
    vtkChartNamedOptions* options = vtkChartNamedOptions::SafeDownCast(
      optionsProxy->GetVTKObject());
    vtkChartRepresentation* repr = vtkChartRepresentation::SafeDownCast(
      this->GetVTKObject());
    repr->SetOptions(options);
    }
}

//----------------------------------------------------------------------------
void vtkSIChartRepresentationProxy::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}
