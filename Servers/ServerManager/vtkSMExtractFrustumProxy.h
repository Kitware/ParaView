/*=========================================================================

  Program:   ParaView
  Module:    vtkSMExtractFrustumProxy.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkSMExtractFrustumProxy - proxy for extract frustum filters. 
// .SECTION Description
// vtkSMExtractFrustumProxy has a subproxy which is the proxy for the 
// selection.

#ifndef __vtkSMExtractFrustumProxy_h
#define __vtkSMExtractFrustumProxy_h

#include "vtkSMSourceProxy.h"

class vtkDoubleArray;

class VTK_EXPORT vtkSMExtractFrustumProxy : public vtkSMSourceProxy
{
public:
  static vtkSMExtractFrustumProxy* New();
  vtkTypeRevisionMacro(vtkSMExtractFrustumProxy, vtkSMSourceProxy);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Update the VTK object on the server by pushing the values of
  // all modifed properties (un-modified properties are ignored).
  // If the object has not been created, it will be created first.
  virtual void UpdateVTKObjects();

  // Description:
  // Add a range to the selection.
  void SetFrustum(double *vertices);
  void RemoveAllValues();

//BTX
protected:
  vtkSMExtractFrustumProxy();
  ~vtkSMExtractFrustumProxy();

  virtual void CreateVTKObjects();

private:
  vtkSMExtractFrustumProxy(const vtkSMExtractFrustumProxy&); // Not implemented.
  void operator=(const vtkSMExtractFrustumProxy&); // Not implemented.

  vtkDoubleArray* Frustum;

//ETX
};

#endif

