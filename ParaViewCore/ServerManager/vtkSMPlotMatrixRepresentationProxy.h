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
#ifndef _vtkSMPlotMatrixRepresentationProxy_h
#define _vtkSMPlotMatrixRepresentationProxy_h

#include "vtkSMChartRepresentationProxy.h"

class VTK_EXPORT vtkSMPlotMatrixRepresentationProxy : public vtkSMChartRepresentationProxy
{
public:
  static vtkSMPlotMatrixRepresentationProxy* New();
  vtkTypeMacro(vtkSMPlotMatrixRepresentationProxy, vtkSMChartRepresentationProxy);
  void PrintSelf(ostream &os, vtkIndent indent);

protected:
  vtkSMPlotMatrixRepresentationProxy();
  ~vtkSMPlotMatrixRepresentationProxy();
};

#endif
