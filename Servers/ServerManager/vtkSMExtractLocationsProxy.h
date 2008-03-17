/*=========================================================================

  Program:   ParaView
  Module:    vtkSMExtractLocationsProxy.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkSMExtractLocationsProxy - proxy for vtkPExtractArraysOverTime
// filter. It is used to extract cell and point variables at a given location
// over time.
// .SECTION Description
// vtkSMExtractLocationsProxy has a subproxy which is the proxy for the 
// selection.

#ifndef __vtkSMExtractLocationsProxy_h
#define __vtkSMExtractLocationsProxy_h

#include "vtkSMSourceProxy.h"

class VTK_EXPORT vtkSMExtractLocationsProxy : public vtkSMSourceProxy
{
public:
  static vtkSMExtractLocationsProxy* New();
  vtkTypeRevisionMacro(vtkSMExtractLocationsProxy, vtkSMSourceProxy);
  void PrintSelf(ostream& os, vtkIndent indent);

//BTX
protected:
  vtkSMExtractLocationsProxy();
  ~vtkSMExtractLocationsProxy();

  // Create the VTK objects. Overloaded to set default values for the sub-proxy.
  virtual void CreateVTKObjects();

private:
  vtkSMExtractLocationsProxy(const vtkSMExtractLocationsProxy&); // Not implemented.
  void operator=(const vtkSMExtractLocationsProxy&); // Not implemented.

//ETX
};

#endif

