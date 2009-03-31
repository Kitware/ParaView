/*=========================================================================

  Program:   ParaView
  Module:    vtkSMBarChartSeriesOptionsProxy.cxx

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkSMBarChartSeriesOptionsProxy.h"

#include "vtkObjectFactory.h"
#include "vtkQtBarChartSeriesOptions.h"

vtkStandardNewMacro(vtkSMBarChartSeriesOptionsProxy);
vtkCxxRevisionMacro(vtkSMBarChartSeriesOptionsProxy, "1.1");
//----------------------------------------------------------------------------
vtkSMBarChartSeriesOptionsProxy::vtkSMBarChartSeriesOptionsProxy()
{
}

//----------------------------------------------------------------------------
vtkSMBarChartSeriesOptionsProxy::~vtkSMBarChartSeriesOptionsProxy()
{
}

//----------------------------------------------------------------------------
vtkQtChartSeriesOptions* vtkSMBarChartSeriesOptionsProxy::NewOptions()
{
  return new vtkQtBarChartSeriesOptions();
}

//----------------------------------------------------------------------------
void vtkSMBarChartSeriesOptionsProxy::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}


