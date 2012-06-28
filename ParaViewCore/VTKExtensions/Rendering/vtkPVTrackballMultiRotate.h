// -*- c++ -*-
/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkPVTrackballMultiRotate.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

/*-------------------------------------------------------------------------
  Copyright 2009 Sandia Corporation.
  Under the terms of Contract DE-AC04-94AL85000 with Sandia Corporation,
  the U.S. Government retains certain rights in this software.
-------------------------------------------------------------------------*/

// .NAME vtkPVTrackballMultiRotate
//
// .SECTION Description
//
// This camera manipulator combines the vtkPVTrackballRotate and
// vtkPVTrackballRoll manipulators in one.  Think of there being an invisible
// sphere in the middle of the screen.  If you grab that sphere and move the
// mouse, you will rotate that sphere.  However, if you grab outside that sphere
// and move the mouse, you will roll the view.
//

#ifndef __vtkPVTrackballMultiRotate_h
#define __vtkPVTrackballMultiRotate_h

#include "vtkCameraManipulator.h"

class vtkCameraManipulator;
class vtkPVTrackballRoll;
class vtkPVTrackballRotate;

class VTK_EXPORT vtkPVTrackballMultiRotate : public vtkCameraManipulator
{
public:
  vtkTypeMacro(vtkPVTrackballMultiRotate, vtkCameraManipulator);
  static vtkPVTrackballMultiRotate *New();
  virtual void PrintSelf(ostream &os, vtkIndent indent);

  // Description:
  // Event bindings controlling the effects of pressing mouse buttons
  // or moving the mouse.
  virtual void OnMouseMove(int x, int y, vtkRenderer *ren,
                           vtkRenderWindowInteractor *rwi);
  virtual void OnButtonDown(int x, int y, vtkRenderer *ren,
                            vtkRenderWindowInteractor *rwi);
  virtual void OnButtonUp(int x, int y, vtkRenderer *ren,
                          vtkRenderWindowInteractor *rwi);

protected:
  vtkPVTrackballMultiRotate();
  ~vtkPVTrackballMultiRotate();

  vtkPVTrackballRotate *RotateManipulator;
  vtkPVTrackballRoll   *RollManipulator;

  vtkCameraManipulator *CurrentManipulator;

private:
  vtkPVTrackballMultiRotate(const vtkPVTrackballMultiRotate &); // Not implemented
  void operator=(const vtkPVTrackballMultiRotate &); // Not implemented
};

#endif //__vtkPVTrackballMultiRotate_h
