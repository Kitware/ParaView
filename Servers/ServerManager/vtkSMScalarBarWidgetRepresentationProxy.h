/*=========================================================================

  Program:   ParaView
  Module:    vtkSMScalarBarWidgetRepresentationProxy.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkSMScalarBarWidgetRepresentationProxy - representation that can be used to
// show a 3D surface in a render view.
// .SECTION Description
// vtkSMScalarBarWidgetRepresentationProxy is a concrete representation that can be used
// to render the surface in a vtkSMRenderViewProxy. It uses a
// vtkPVGeometryFilter to convert non-polydata input to polydata that can be
// rendered. It supports rendering the data as a surface, wireframe or points.

#ifndef __vtkSMScalarBarWidgetRepresentationProxy_h
#define __vtkSMScalarBarWidgetRepresentationProxy_h

#include "vtkSMNewWidgetRepresentationProxy.h"

class vtkSMViewProxy;

class VTK_EXPORT vtkSMScalarBarWidgetRepresentationProxy : 
  public vtkSMNewWidgetRepresentationProxy
{
public:
  static vtkSMScalarBarWidgetRepresentationProxy* New();
  vtkTypeMacro(vtkSMScalarBarWidgetRepresentationProxy,
                       vtkSMNewWidgetRepresentationProxy);
  virtual void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Calls set enabled on the WidgetProxy.
  // Overrridden to not pass the enabled state to WidgetProxy unless the
  // representation has been added to a view.
  virtual void SetEnabled(int enable);
//BTX
protected:
  vtkSMScalarBarWidgetRepresentationProxy();
  ~vtkSMScalarBarWidgetRepresentationProxy();

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

  vtkSMProxy* ActorProxy;
  vtkSMViewProxy* ViewProxy;
  int Enabled;

  // Description:
  // Called every time the user interacts with the widget.
  virtual void ExecuteEvent(unsigned long event);

private:
  vtkSMScalarBarWidgetRepresentationProxy(const vtkSMScalarBarWidgetRepresentationProxy&); // Not implemented
  void operator=(const vtkSMScalarBarWidgetRepresentationProxy&); // Not implemented
//ETX
};

#endif

