/*=========================================================================

  Program:   ParaView
  Module:    vtkPickLineWidget.h

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



#ifndef __vtkPickLineWidget_h
#define __vtkPickLineWidget_h

#include "vtkLineWidget.h"


class vtkPVRenderModule;


class VTK_EXPORT vtkPickLineWidget : public vtkLineWidget
{
public:
  static vtkPickLineWidget* New();
  vtkTypeRevisionMacro(vtkPickLineWidget, vtkLineWidget);

  void PrintSelf(ostream& os, vtkIndent indent);
    
  // Description:
  // The render module is for picking.
  void SetRenderModule(vtkPVRenderModule* rm);
  vtkPVRenderModule* GetRenderModule() { return this->RenderModule;}

  // Description:
  // We have to look for key press events too.
  virtual void SetEnabled(int);

protected:
  vtkPickLineWidget();
  ~vtkPickLineWidget();

  // For picking.  Use a proxy in the future.
  vtkPVRenderModule* RenderModule;

  virtual void OnChar();

  // Handles the events
  static void ProcessEvents(vtkObject* object, 
                            unsigned long event,
                            void* clientdata, 
                            void* calldata);

  // For toggling the pick between the two end points.
  int LastPicked;

private:
  vtkPickLineWidget(const vtkPickLineWidget&); // Not implemented
  void operator=(const vtkPickLineWidget&); // Not implemented

  int LastY;
};

#endif
