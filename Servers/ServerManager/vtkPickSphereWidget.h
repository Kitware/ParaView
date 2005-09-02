/*=========================================================================

  Program:   ParaView
  Module:    vtkPickSphereWidget.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkPickSphereWidget - A point widget with pick ability.
// .SECTION Description
// This is a subclass of vtkBoxWidget that has different bindings 
// for paraview.  Shift left causes to pick using a z buffer.
// Right causes moving in and out of the window.

// What I want in the future is:
// Left pick new center moves point in view plane.
// Left pick on axis (away from center) moves constrained to axis.
// Shift left causes pick with zbuffer.



#ifndef __vtkPickSphereWidget_h
#define __vtkPickSphereWidget_h

#include "vtkSphereWidget.h"


class vtkSMRenderModuleProxy;


class VTK_EXPORT vtkPickSphereWidget : public vtkSphereWidget
{
public:
  static vtkPickSphereWidget* New();
  vtkTypeRevisionMacro(vtkPickSphereWidget, vtkSphereWidget);

  void PrintSelf(ostream& os, vtkIndent indent);
    
  virtual void PlaceWidget(double bounds[6]);
  void PlaceWidget()
    {this->Superclass::PlaceWidget();}
  void PlaceWidget(double xmin, double xmax, double ymin, double ymax, 
                   double zmin, double zmax)
    {this->Superclass::PlaceWidget(xmin,xmax,ymin,ymax,zmin,zmax);}

  // Description:
  // The render module is for picking.
  void SetRenderModuleProxy(vtkSMRenderModuleProxy* rm)
    { this->RenderModuleProxy = rm; }
  vtkGetObjectMacro(RenderModuleProxy, vtkSMRenderModuleProxy);

  // Description:
  // We have to look for key press events too.
  virtual void SetEnabled(int);

  vtkSetMacro(MouseControlToggle,int);
  vtkGetMacro(MouseControlToggle,int);

protected:
  vtkPickSphereWidget();
  ~vtkPickSphereWidget();

  // For picking.  Use a proxy in the future.
  vtkSMRenderModuleProxy* RenderModuleProxy;

  virtual void OnChar();
  virtual void OnMouseMove();
  virtual void OnRightButtonDown();
  virtual void OnLeftButtonDown();

  void PickInternal(int x, int y);

  // Handles the events
  static void ProcessEvents(vtkObject* object, 
                            unsigned long event,
                            void* clientdata, 
                            void* calldata);

  double PrevPickedPoint[4];

  int MouseControlToggle;

private:
  vtkPickSphereWidget(const vtkPickSphereWidget&); // Not implemented
  void operator=(const vtkPickSphereWidget&); // Not implemented

  int LastY;
};

#endif
