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
// .NAME vtkSIChartRepresentationProxy
// .SECTION Description
// vtkSIChartRepresentationProxy is the server-side helper for
// vtkSMChartRepresentationProxy. It initializes the vtkXYChartRepresentation
// instance with the vtkContextNamedOptions instance sub-proxy, if any during
// CreateVTKObjects().

#ifndef __vtkSIChartRepresentationProxy_h
#define __vtkSIChartRepresentationProxy_h

#include "vtkSISourceProxy.h"

class VTK_EXPORT vtkSIChartRepresentationProxy : public vtkSISourceProxy
{
public:
  static vtkSIChartRepresentationProxy* New();
  vtkTypeMacro(vtkSIChartRepresentationProxy, vtkSISourceProxy);
  void PrintSelf(ostream& os, vtkIndent indent);

//BTX
protected:
  vtkSIChartRepresentationProxy();
  ~vtkSIChartRepresentationProxy();

  // Description;
  // Called in CreateVTKObjects() after the vtk-object has been created and
  // subproxy-information has been processed, but before the XML is parsed to
  // generate properties and initialize their values.
  //
  // This method is overriden here to set the VTK instance of the
  // PlotOptions subProxy to the current VTK representation.
  virtual void OnCreateVTKObjects();

private:
  vtkSIChartRepresentationProxy(const vtkSIChartRepresentationProxy&); // Not implemented
  void operator=(const vtkSIChartRepresentationProxy&); // Not implemented
//ETX
};

#endif
