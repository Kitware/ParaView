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

#include "vtkSMRepresentationProxy.h"
class vtkSMViewProxy;
class vtkScalarBarWidget;

class VTK_EXPORT vtkSMScalarBarWidgetRepresentationProxy : 
  public vtkSMRepresentationProxy
{
public:
  static vtkSMScalarBarWidgetRepresentationProxy* New();
  vtkTypeRevisionMacro(vtkSMScalarBarWidgetRepresentationProxy, vtkSMRepresentationProxy);
  void PrintSelf(ostream& os, vtkIndent indent);
  
  // Description:
  // Enable/Disable the scalar bar. Overridden to set the current 
  // renderer on the ScalarBarWidget. This also enables the scalar bar 
  // widget on the client.
  virtual void SetVisibility(int visible);
  virtual bool GetVisibility() { return this->Visibility? true : false; }

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
  vtkScalarBarWidget* Widget; // Widget on the client. 
  int Visibility;
  vtkSMViewProxy* ViewProxy;
  
//BTX
  friend class vtkSMScalarBarWidgetRepresentationObserver;
//ETX
  vtkSMScalarBarWidgetRepresentationObserver* Observer;

private:
  void ExecuteEvent(unsigned long event);

  vtkSMScalarBarWidgetRepresentationProxy(const vtkSMScalarBarWidgetRepresentationProxy&); // Not implemented
  void operator=(const vtkSMScalarBarWidgetRepresentationProxy&); // Not implemented
};

#endif

