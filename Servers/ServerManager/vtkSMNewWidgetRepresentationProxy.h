/*=========================================================================

  Program:   ParaView
  Module:    vtkSMNewWidgetRepresentationProxy.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkSMNewWidgetRepresentationProxy - representation that can be used to
// show a 3D surface in a render view.
// .SECTION Description
// vtkSMNewWidgetRepresentationProxy is a concrete representation that can be used
// to render the surface in a vtkSMRenderViewProxy. It uses a
// vtkPVGeometryFilter to convert non-polydata input to polydata that can be
// rendered. It supports rendering the data as a surface, wireframe or points.

#ifndef __vtkSMNewWidgetRepresentationProxy_h
#define __vtkSMNewWidgetRepresentationProxy_h

#include "vtkSMRepresentationProxy.h"
class vtkSMViewProxy;
class vtkSMNewWidgetRepresentationObserver;
class vtkAbstractWidget;
//BTX
struct vtkSMNewWidgetRepresentationInternals;
//ETX

class VTK_EXPORT vtkSMNewWidgetRepresentationProxy : 
  public vtkSMRepresentationProxy
{
public:
  static vtkSMNewWidgetRepresentationProxy* New();
  vtkTypeRevisionMacro(vtkSMNewWidgetRepresentationProxy, vtkSMRepresentationProxy);
  void PrintSelf(ostream& os, vtkIndent indent);
 
  // Description:
  // Break reference loop that are due to links.
  virtual void UnRegister(vtkObjectBase* o);
 
  // Description:
  // Calls set enabled on the WidgetProxy.
  virtual void SetEnabled(int enable);

//BTX
protected:
  vtkSMNewWidgetRepresentationProxy();
  ~vtkSMNewWidgetRepresentationProxy();

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

  // Description:
  // Updates the widget's enabled state.
  void UpdateEnabled();

  vtkSMProxy* RepresentationProxy;
  vtkSMProxy* WidgetProxy;
  vtkAbstractWidget* Widget;
  vtkSMNewWidgetRepresentationObserver* Observer;
  vtkSMNewWidgetRepresentationInternals* Internal;

//BTX
  friend class vtkSMNewWidgetRepresentationObserver;
//ETX

  int Enabled;
private:
  void ExecuteEvent(unsigned long event);

  vtkSMNewWidgetRepresentationProxy(const vtkSMNewWidgetRepresentationProxy&); // Not implemented
  void operator=(const vtkSMNewWidgetRepresentationProxy&); // Not implemented
//ETX
};

#endif

