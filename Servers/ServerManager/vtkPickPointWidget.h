/*=========================================================================

  Program:   ParaView
  Module:    vtkPickPointWidget.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkPickPointWidget - A point widget with pick ability.
// .SECTION Description
// This is a subclass of vtkPointWidget that has different bindings 
// for paraview.  Shift left causes to pick using a z buffer.
// Right causes moving in and out of the window.

// What I want in the future is:
// Left pick new center moves point in view plane.
// Left pick on axis (away from center) moves constrained to axis.
// Shift left causes pick with zbuffer.



#ifndef __vtkPickPointWidget_h
#define __vtkPickPointWidget_h

#include "vtkPointWidget.h"


class vtkSMRenderModuleProxy;


class VTK_EXPORT vtkPickPointWidget : public vtkPointWidget
{
public:
  static vtkPickPointWidget* New();
  vtkTypeRevisionMacro(vtkPickPointWidget, vtkPointWidget);

  void PrintSelf(ostream& os, vtkIndent indent);
    
  // Description:
  // The render module is for picking.
  void SetRenderModuleProxy(vtkSMRenderModuleProxy* rm)
    { this->RenderModuleProxy = rm; }
  
  vtkGetObjectMacro(RenderModuleProxy, vtkSMRenderModuleProxy);

  // Description:
  // We have to look for key press events too.
  virtual void SetEnabled(int);

protected:
  vtkPickPointWidget();
  ~vtkPickPointWidget();

  // For picking.  Use a proxy in the future.
  vtkSMRenderModuleProxy* RenderModuleProxy;

  virtual void OnChar();

  // Handles the events
  static void ProcessEvents(vtkObject* object, 
                            unsigned long event,
                            void* clientdata, 
                            void* calldata);

private:
  vtkPickPointWidget(const vtkPickPointWidget&); // Not implemented
  void operator=(const vtkPickPointWidget&); // Not implemented

  int LastY;
};

#endif
