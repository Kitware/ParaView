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
// .NAME vtkSMExtractLocationsProxy - proxy for extract (point/cell) selection
// filters. 
// .SECTION Description
// vtkSMExtractLocationsProxy has a subproxy which is the proxy for the 
// selection.

#ifndef __vtkSMExtractLocationsProxy_h
#define __vtkSMExtractLocationsProxy_h

#include "vtkSMSourceProxy.h"

class vtkDoubleArray;

class VTK_EXPORT vtkSMExtractLocationsProxy : public vtkSMSourceProxy
{
public:
  static vtkSMExtractLocationsProxy* New();
  vtkTypeRevisionMacro(vtkSMExtractLocationsProxy, vtkSMSourceProxy);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Update the VTK object on the server by pushing the values of
  // all modifed properties (un-modified properties are ignored).
  // If the object has not been created, it will be created first.
  virtual void UpdateVTKObjects();

  // Description:
  // Add an Location to the selection.
  void AddLocation(double x, double y, double z);
  void RemoveAllLocations();

//BTX
protected:
  vtkSMExtractLocationsProxy();
  ~vtkSMExtractLocationsProxy();

  virtual void CreateVTKObjects(int numObjects);

private:
  vtkSMExtractLocationsProxy(const vtkSMExtractLocationsProxy&); // Not implemented.
  void operator=(const vtkSMExtractLocationsProxy&); // Not implemented.

  vtkDoubleArray* Locations;
//ETX
};

#endif

