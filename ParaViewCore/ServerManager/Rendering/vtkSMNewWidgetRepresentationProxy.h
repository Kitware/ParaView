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

#include "vtkSMProxy.h"
class vtkSMViewProxy;
class vtkSMNewWidgetRepresentationObserver;
class vtkAbstractWidget;
//BTX
struct vtkSMNewWidgetRepresentationInternals;
//ETX

class VTK_EXPORT vtkSMNewWidgetRepresentationProxy : public vtkSMProxy
{
public:
  static vtkSMNewWidgetRepresentationProxy* New();
  vtkTypeMacro(vtkSMNewWidgetRepresentationProxy, vtkSMProxy);
  void PrintSelf(ostream& os, vtkIndent indent);
 
  // Description:
  // Break reference loop that are due to links.
  virtual void UnRegister(vtkObjectBase* o);

  // Description:
  // Get the widget for the representation.
  vtkGetObjectMacro(Widget, vtkAbstractWidget);

  // Description:
  // Get Representation Proxy.
  vtkGetObjectMacro(RepresentationProxy, vtkSMProxy);

//BTX
protected:
  vtkSMNewWidgetRepresentationProxy();
  ~vtkSMNewWidgetRepresentationProxy();

  // Description:
  // Overridden from vtkSMProxy to call BeginCreateVTKObjects() and
  // EndCreateVTKObjects().
  virtual void CreateVTKObjects();

  vtkSMProxy* RepresentationProxy;
  vtkSMProxy* WidgetProxy;
  vtkAbstractWidget* Widget;
  vtkSMNewWidgetRepresentationObserver* Observer;
  vtkSMNewWidgetRepresentationInternals* Internal;

  friend class vtkSMNewWidgetRepresentationObserver;

  // Description:
  // Called every time the user interacts with the widget.
  virtual void ExecuteEvent(unsigned long event);

private:

  vtkSMNewWidgetRepresentationProxy(const vtkSMNewWidgetRepresentationProxy&); // Not implemented
  void operator=(const vtkSMNewWidgetRepresentationProxy&); // Not implemented  
//ETX
};

#endif

