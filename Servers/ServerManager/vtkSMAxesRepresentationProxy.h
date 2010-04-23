/*=========================================================================

  Program:   ParaView
  Module:    vtkSMAxesRepresentationProxy.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkSMAxesRepresentationProxy - representation that can be used to
// show a 3D surface in a render view.
// .SECTION Description
// vtkSMAxesRepresentationProxy is a concrete representation that can be used
// to render the surface in a vtkSMRenderViewProxy. It uses a
// vtkPVGeometryFilter to convert non-polydata input to polydata that can be
// rendered. It supports rendering the data as a surface, wireframe or points.

#ifndef __vtkSMAxesRepresentationProxy_h
#define __vtkSMAxesRepresentationProxy_h

#include "vtkSMRepresentationProxy.h"
class vtkSMViewProxy;

class VTK_EXPORT vtkSMAxesRepresentationProxy : 
  public vtkSMRepresentationProxy
{
public:
  static vtkSMAxesRepresentationProxy* New();
  vtkTypeMacro(vtkSMAxesRepresentationProxy, vtkSMRepresentationProxy);
  void PrintSelf(ostream& os, vtkIndent indent);
 
//BTX
protected:
  vtkSMAxesRepresentationProxy();
  ~vtkSMAxesRepresentationProxy();

  // Description:
  // Overridden from vtkSMProxy to call BeginCreateVTKObjects() and
  // EndCreateVTKObjects().
  virtual void CreateVTKObjects();

  // Description:
  // Called when a representation is added to a view. 
  // Returns true on success.
  // Currently a representation can be added to only one view.
  virtual bool AddToView(vtkSMViewProxy* view);

  // Description:
  // Called to remove a representation from a view.
  // Returns true on success.
  // Currently a representation can be added to only one view.
  virtual bool RemoveFromView(vtkSMViewProxy* view);

private:
  vtkSMAxesRepresentationProxy(const vtkSMAxesRepresentationProxy&); // Not implemented
  void operator=(const vtkSMAxesRepresentationProxy&); // Not implemented
//ETX
};

#endif

