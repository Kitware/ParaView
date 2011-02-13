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
// .NAME vtkPMChartRepresentationProxy
// .SECTION Description
// vtkPMChartRepresentationProxy is the server-side helper for
// vtkSMChartRepresentationProxy. It initializes the vtkXYChartRepresentation
// instance with the vtkContextNamedOptions instance sub-proxy, if any during
// CreateVTKObjects().

#ifndef __vtkPMChartRepresentationProxy_h
#define __vtkPMChartRepresentationProxy_h

#include "vtkPMSourceProxy.h"

class VTK_EXPORT vtkPMChartRepresentationProxy : public vtkPMSourceProxy
{
public:
  static vtkPMChartRepresentationProxy* New();
  vtkTypeMacro(vtkPMChartRepresentationProxy, vtkPMSourceProxy);
  void PrintSelf(ostream& os, vtkIndent indent);

//BTX
protected:
  vtkPMChartRepresentationProxy();
  ~vtkPMChartRepresentationProxy();

  // Description;
  // Called in CreateVTKObjects() after the vtk-object has been created and
  // subproxy-information has been processed, but before the XML is parsed to
  // generate properties and initialize their values.
  virtual void OnCreateVTKObjects();

private:
  vtkPMChartRepresentationProxy(const vtkPMChartRepresentationProxy&); // Not implemented
  void operator=(const vtkPMChartRepresentationProxy&); // Not implemented
//ETX
};

#endif
