/*=========================================================================

  Program:   ParaView
  Module:    vtkSMPickPointWidgetProxy.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkSMPickPointWidgetProxy - Proxy with picking extensions.
// .SECTION Description
// .SECTION See Also
// vtkPickPointWidget


#ifndef __vtkSMPickPointWidgetProxy_h
#define __vtkSMPickPointWidgetProxy_h

#include "vtkSMPointWidgetProxy.h"

class vtkCallbackCommand;
class vtkRenderWindowInteractor;

class VTK_EXPORT vtkSMPickPointWidgetProxy : public vtkSMPointWidgetProxy
{
public:
  static vtkSMPickPointWidgetProxy* New();
  vtkTypeRevisionMacro(vtkSMPickPointWidgetProxy, vtkSMPointWidgetProxy);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Called when the display is added/removed to/from a RenderModule.
  // Overridden to bind OnChar event on the Interactor (for picking).
  virtual void AddToRenderModule(vtkSMRenderModuleProxy*);
  virtual void RemoveFromRenderModule(vtkSMRenderModuleProxy*);
protected:
  vtkSMPickPointWidgetProxy();
  ~vtkSMPickPointWidgetProxy();

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

private:
  vtkSMPickPointWidgetProxy(const vtkSMPickPointWidgetProxy&); // Not implemented.
  void operator=(const vtkSMPickPointWidgetProxy&); // Not implemented.
    
};



#endif
