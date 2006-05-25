/*=========================================================================

  Program:   ParaView
  Module:    vtkSMNew3DWidgetProxy.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkSMNew3DWidgetProxy - proxy for new style 3d widgets
// .SECTION Description
// vtkSMNew3DWidgetProxy supports the new style widgets. These widgets
// have two components: a widget and a representation. The widget handles
// the events and is only instantiated on the client. The representation
// handles rendering and picking and is rendered on both client and the
// server. The XML for a vtkSMNew3DWidgetProxy should contain two sub-
// proxies: Prop and Widget. The Prop sub-proxy is the representation
// and the Widget sub-proxy is the widget. All exposed properties of the
// the vtkSMNew3DWidgetProxy are linked to their information property
// (if any). The information properties are updated when an 
// EndInteractionEvent occurs. This is also propagated to the properties
// through the link. The GUI controlling the vtkSMNew3DWidgetProxy should
// listen to the PropertyModifiedEvent to update when the 3D widget is
// changed.

#ifndef __vtkSMNew3DWidgetProxy_h
#define __vtkSMNew3DWidgetProxy_h

#include "vtkSMDisplayProxy.h"

class vtkSMNew3DWidgetObserver;
class vtkAbstractWidget;
//BTX
struct vtkSMNew3DWidgetProxyInternals;
//ETX

class VTK_EXPORT vtkSMNew3DWidgetProxy : public vtkSMDisplayProxy
{
public:
  static vtkSMNew3DWidgetProxy* New();
  vtkTypeRevisionMacro(vtkSMNew3DWidgetProxy, vtkSMDisplayProxy);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Called when the display is added/removed to/from a RenderModule.
  // Chains to superclass and connects/disconnects the 3d widget
  // to the interactor.
  virtual void AddToRenderModule(vtkSMRenderModuleProxy*);
  virtual void RemoveFromRenderModule(vtkSMRenderModuleProxy*);

  // Description:
  // Break reference loop that are due to links.
  virtual void UnRegister(vtkObjectBase* o);
 
  // Description:
  // Calls set enabled on the WidgetProxy.
  void SetEnabled(int enable);
protected:
  vtkSMNew3DWidgetProxy();
  ~vtkSMNew3DWidgetProxy();

  virtual void CreateVTKObjects(int numObjects);

  vtkSMProxy* RepresentationProxy;
  vtkSMProxy* WidgetProxy;
  vtkAbstractWidget* Widget;
  vtkSMNew3DWidgetObserver* Observer;
  vtkSMNew3DWidgetProxyInternals* Internal;

//BTX
  friend class vtkSMNew3DWidgetObserver;
//ETX

private:
  void ExecuteEvent(unsigned long event);

  vtkSMNew3DWidgetProxy(const vtkSMNew3DWidgetProxy&); // Not implemented.
  void operator=(const vtkSMNew3DWidgetProxy&); // Not implemented.
};



#endif

