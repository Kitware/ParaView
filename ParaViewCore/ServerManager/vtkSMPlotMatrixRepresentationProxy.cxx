/*=========================================================================

  Program:   ParaView
  Module:    vtkSMPlotMatrixRepresentationProxy.cxx

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkSMPlotMatrixRepresentationProxy.h"

#include "vtkObjectFactory.h"

vtkStandardNewMacro(vtkSMPlotMatrixRepresentationProxy);

//----------------------------------------------------------------------------
vtkSMPlotMatrixRepresentationProxy::vtkSMPlotMatrixRepresentationProxy()
{
}

//----------------------------------------------------------------------------
vtkSMPlotMatrixRepresentationProxy::~vtkSMPlotMatrixRepresentationProxy()
{
}

//----------------------------------------------------------------------------
void vtkSMPlotMatrixRepresentationProxy::PrintSelf(ostream& os, vtkIndent indent)
{
  vtkSMChartRepresentationProxy::PrintSelf(os, indent);
}
