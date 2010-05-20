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
  vtkTypeMacro(vtkSMNewWidgetRepresentationProxy, vtkSMRepresentationProxy);
  void PrintSelf(ostream& os, vtkIndent indent);
 
  // Description:
  // Break reference loop that are due to links.
  virtual void UnRegister(vtkObjectBase* o);
 
  // Description:
  // Calls set/get enabled on the WidgetProxy.
  virtual void SetEnabled(int enable);
  vtkGetMacro(Enabled, int);
  
  // Description:
  // Get the bounds for the representation.  Returns true if successful.
  virtual bool GetBounds(double bounds[6]);

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

  // Description:
  // Updates state from an XML element. Returns 0 on failure.
  virtual int LoadState(vtkPVXMLElement* element, vtkSMProxyLocator* loader);

  vtkSMProxy* RepresentationProxy;
  vtkSMProxy* WidgetProxy;
  vtkAbstractWidget* Widget;
  vtkSMNewWidgetRepresentationObserver* Observer;
  vtkSMNewWidgetRepresentationInternals* Internal;

  friend class vtkSMNewWidgetRepresentationObserver;

  // Description:
  // Called every time the user interacts with the widget.
  virtual void ExecuteEvent(unsigned long event);

  int Enabled;
private:

  vtkSMNewWidgetRepresentationProxy(const vtkSMNewWidgetRepresentationProxy&); // Not implemented
  void operator=(const vtkSMNewWidgetRepresentationProxy&); // Not implemented

  bool StateLoaded;
//ETX
};

#endif

