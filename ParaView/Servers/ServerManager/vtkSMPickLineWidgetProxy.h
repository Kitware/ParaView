/*=========================================================================

  Program:   ParaView
  Module:    vtkSMPickLineWidgetProxy.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkSMPickLineWidget - Proxy with picking extensions.
// .SECTION Description
// This class is based on vtkPickLineWidget.
// Instead of creating a client instance on vtkPickLineWidget, the 
// functionality of picking has been moved to the proxy, since, picking
// needs RenderModuleProxy, and hence typically, never reside on the
// Servers.
// This is a subclass of vtkSMLineWidgetProxy that has key bindings for
// picking the ends of the line widget.  I plan to have the first 'p' pick
// the end closest to the picked point, subsequent 'p' will toggle between
// the two ends.  We could make a new widget that would continue to
// add line segments.
// .SECTION See Also
// vtkPickLineWidget

#ifndef __vtkSMPickLineWidgetProxy_h
#define __vtkSMPickLineWidgetProxy_h

#include "vtkSMLineWidgetProxy.h"

class vtkCallbackCommand;
class vtkRenderWindowInteractor;

class VTK_EXPORT vtkSMPickLineWidgetProxy : public vtkSMLineWidgetProxy
{
public:
  static vtkSMPickLineWidgetProxy* New();
  vtkTypeRevisionMacro(vtkSMPickLineWidgetProxy, vtkSMLineWidgetProxy);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Called when the display is added/removed to/from a RenderModule.
  // Overridden to bind OnChar event on the Interactor (for picking).
  virtual void AddToRenderModule(vtkSMRenderModuleProxy*);
  virtual void RemoveFromRenderModule(vtkSMRenderModuleProxy*);
protected:
  vtkSMPickLineWidgetProxy();
  ~vtkSMPickLineWidgetProxy();

  // Determines the position of the widget end poins based on the 
  // pointer positions.
  void OnChar();

  // Static method for vtkCallbackCommand.
  static void ProcessEvents(vtkObject* vtkNotUsed(object), 
                                          unsigned long event,
                                          void* clientdata, 
                                          void* vtkNotUsed(calldata));
  
  unsigned long EventTag;
  vtkCallbackCommand* EventCallbackCommand;
  vtkRenderWindowInteractor* Interactor;
  int LastPicked; // identifier for the last picked line end point.

private:
  vtkSMPickLineWidgetProxy(const vtkSMPickLineWidgetProxy&); // Not implemented.
  void operator=(const vtkSMPickLineWidgetProxy&); // Not implemented.
};

#endif
