/*=========================================================================

  Program:   ParaView
  Module:    vtkSMTextWidgetDisplayProxy.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkSMTextWidgetDisplayProxy - Proxy for vtkSMTextWidgetDisplay.
// .SECTION Description
// vtkSMTextWidgetDisplayProxy is the proxy for vtkSMTextWidgetDisplay.

#ifndef __vtkSMTextWidgetDisplayProxy_h
#define __vtkSMTextWidgetDisplayProxy_h

#include "vtkSMNew3DWidgetProxy.h"

class VTK_EXPORT vtkSMTextWidgetDisplayProxy : public vtkSMNew3DWidgetProxy
{
public:
  static vtkSMTextWidgetDisplayProxy* New();
  vtkTypeRevisionMacro(vtkSMTextWidgetDisplayProxy, vtkSMNew3DWidgetProxy);
  void PrintSelf(ostream& os, vtkIndent indent);
  
  // Description:
  // Called when the display is added/removed to/from a RenderModule.
  virtual void AddToRenderModule(vtkSMRenderModuleProxy*);
  virtual void RemoveFromRenderModule(vtkSMRenderModuleProxy*);

protected:
//BTX
  vtkSMTextWidgetDisplayProxy();
  ~vtkSMTextWidgetDisplayProxy();
  
  virtual void CreateVTKObjects();

  int Visibility;

  vtkSMProxy* TextActorProxy;
  vtkSMProxy* TextPropertyProxy;
  
  vtkSMRenderModuleProxy* RenderModuleProxy;
private:
  vtkSMTextWidgetDisplayProxy(const vtkSMTextWidgetDisplayProxy&); // Not implemented
  void operator=(const vtkSMTextWidgetDisplayProxy&); // Not implemented
//ETX
};

#endif

