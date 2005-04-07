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
// .NAME vtkPickLineWidget - A point widget with pick ability.
// .SECTION Description
// This is a subclass of vtkLineWidget that has key bindings for
// picking the ends of the line widget.  I plan to have the first 'p' pick
// the end closest to the picked point, subsequent 'p' will toggle between
// the two ends.  We could make a new widget that would continue to
// add line segments.

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

  void OnChar();

protected:
  vtkSMPickLineWidgetProxy();
  ~vtkSMPickLineWidgetProxy();

  virtual void SetInteractor(vtkSMProxy* iren);

  static void ProcessEvents(vtkObject* vtkNotUsed(object), 
                                          unsigned long event,
                                          void* clientdata, 
                                          void* vtkNotUsed(calldata));
  unsigned long EventTag;
  vtkCallbackCommand* EventCallbackCommand;
  vtkRenderWindowInteractor* Interactor;
  int LastPicked;

private:
  vtkSMPickLineWidgetProxy(const vtkSMPickLineWidgetProxy&); // Not implemented.
  void operator=(const vtkSMPickLineWidgetProxy&); // Not implemented.
};

#endif
