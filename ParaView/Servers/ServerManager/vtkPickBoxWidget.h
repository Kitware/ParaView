/*=========================================================================

  Program:   ParaView
  Module:    vtkPickBoxWidget.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkPickBoxWidget - A point widget with pick ability.
// .SECTION Description
// This is a subclass of vtkBoxWidget that has different bindings 
// for paraview.  Shift left causes to pick using a z buffer.
// Right causes moving in and out of the window.

// What I want in the future is:
// Left pick new center moves point in view plane.
// Left pick on axis (away from center) moves constrained to axis.
// Shift left causes pick with zbuffer.



#ifndef __vtkPickBoxWidget_h
#define __vtkPickBoxWidget_h

#include "vtkBoxWidget.h"


class vtkSMRenderModuleProxy;


class VTK_EXPORT vtkPickBoxWidget : public vtkBoxWidget
{
public:
  static vtkPickBoxWidget* New();
  vtkTypeRevisionMacro(vtkPickBoxWidget, vtkBoxWidget);

  void PrintSelf(ostream& os, vtkIndent indent);
    
  // Description:
  // The render module is for picking.
  void SetRenderModuleProxy(vtkSMRenderModuleProxy* rm)
    { this->RenderModuleProxy = rm; }
  vtkGetObjectMacro(RenderModuleProxy, vtkSMRenderModuleProxy);

  virtual void PlaceWidget(double bounds[6]);
  void PlaceWidget()
    {this->Superclass::PlaceWidget();}
  void PlaceWidget(double xmin, double xmax, double ymin, double ymax, 
                   double zmin, double zmax)
    {this->Superclass::PlaceWidget(xmin,xmax,ymin,ymax,zmin,zmax);}

  // Description:
  // We have to look for key press events too.
  virtual void SetEnabled(int);

  vtkSetMacro(MouseControlToggle,int);
  vtkGetMacro(MouseControlToggle,int);

protected:
  vtkPickBoxWidget();
  ~vtkPickBoxWidget();

  // For picking.  Use a proxy in the future.
  vtkSMRenderModuleProxy* RenderModuleProxy;

  virtual void OnChar();
  virtual void OnMouseMove();
  virtual void OnLeftButtonDown();
  virtual void OnRightButtonDown();

  void PickInternal(int x, int y);

  // Handles the events
  static void ProcessEvents(vtkObject* object, 
                            unsigned long event,
                            void* clientdata, 
                            void* calldata);


  double PrevPickedPoint[4];

  int MouseControlToggle;

private:
  vtkPickBoxWidget(const vtkPickBoxWidget&); // Not implemented
  void operator=(const vtkPickBoxWidget&); // Not implemented

  int LastY;
};

#endif
