/*=========================================================================

  Program:   ParaView
  Module:    vtkSMScalarBarWidgetProxy.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkSMScalarBarWidgetProxy - Proxy for vtkSMScalarBarWidget.
// .SECTION Description
// vtkSMScalarBarWidgetProxy is the proxy for vtkScalarBarWidget.

#ifndef __vtkSMScalarBarWidgetProxy_h
#define __vtkSMScalarBarWidgetProxy_h

#include "vtkSMDisplayProxy.h"
class vtkSMScalarBarWidgetProxyObserver;
class vtkScalarBarWidget;


class VTK_EXPORT vtkSMScalarBarWidgetProxy : public vtkSMDisplayProxy
{
public:
  static vtkSMScalarBarWidgetProxy* New();
  vtkTypeRevisionMacro(vtkSMScalarBarWidgetProxy, vtkSMDisplayProxy);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Enable/Disable the scalar bar. Overridden to set the current 
  // renderer on the ScalarBarWidget. This also enables the scalar bar 
  // widget on the client.
  virtual void SetVisibility(int visible);
  vtkGetMacro(Visibility, int);
 
  virtual void SaveInBatchScript(ofstream* file);
  
  // Description:
  // Called when the display is added/removed to/from a RenderModule.
  virtual void AddToRenderModule(vtkSMRenderModuleProxy*);
  virtual void RemoveFromRenderModule(vtkSMRenderModuleProxy*);
protected:
//BTX
  vtkSMScalarBarWidgetProxy();
  ~vtkSMScalarBarWidgetProxy();

  
  virtual void CreateVTKObjects(int numObjects);
  
  void ExecuteEvent(vtkObject*obj, unsigned long event, void*p);

  int Visibility;
  vtkSMProxy* ScalarBarActorProxy;
  vtkScalarBarWidget* ScalarBarWidget; // Widget on the client. 
  
  friend class vtkSMScalarBarWidgetProxyObserver;
  vtkSMScalarBarWidgetProxyObserver* Observer;

  vtkSMRenderModuleProxy* RenderModuleProxy;
private:
  vtkSMScalarBarWidgetProxy(const vtkSMScalarBarWidgetProxy&); // Not implemented
  void operator=(const vtkSMScalarBarWidgetProxy&); // Not implemented
//ETX
};

#endif

