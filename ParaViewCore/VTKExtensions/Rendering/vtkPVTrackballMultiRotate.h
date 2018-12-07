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

/**
 * @class   vtkPVTrackballMultiRotate
 *
 *
 *
 * This camera manipulator combines the vtkPVTrackballRotate and
 * vtkPVTrackballRoll manipulators in one.  Think of there being an invisible
 * sphere in the middle of the screen.  If you grab that sphere and move the
 * mouse, you will rotate that sphere.  However, if you grab outside that sphere
 * and move the mouse, you will roll the view.
 *
*/

#ifndef vtkPVTrackballMultiRotate_h
#define vtkPVTrackballMultiRotate_h

#include "vtkCameraManipulator.h"
#include "vtkPVVTKExtensionsRenderingModule.h" // needed for export macro

class vtkCameraManipulator;
class vtkPVTrackballRoll;
class vtkPVTrackballRotate;

class VTKPVVTKEXTENSIONSRENDERING_EXPORT vtkPVTrackballMultiRotate : public vtkCameraManipulator
{
public:
  vtkTypeMacro(vtkPVTrackballMultiRotate, vtkCameraManipulator);
  static vtkPVTrackballMultiRotate* New();
  void PrintSelf(ostream& os, vtkIndent indent) override;

  //@{
  /**
   * Event bindings controlling the effects of pressing mouse buttons
   * or moving the mouse.
   */
  void OnMouseMove(int x, int y, vtkRenderer* ren, vtkRenderWindowInteractor* rwi) override;
  void OnButtonDown(int x, int y, vtkRenderer* ren, vtkRenderWindowInteractor* rwi) override;
  void OnButtonUp(int x, int y, vtkRenderer* ren, vtkRenderWindowInteractor* rwi) override;
  //@}

protected:
  vtkPVTrackballMultiRotate();
  ~vtkPVTrackballMultiRotate() override;

  vtkPVTrackballRotate* RotateManipulator;
  vtkPVTrackballRoll* RollManipulator;

  vtkCameraManipulator* CurrentManipulator;

private:
  vtkPVTrackballMultiRotate(const vtkPVTrackballMultiRotate&) = delete;
  void operator=(const vtkPVTrackballMultiRotate&) = delete;
};

#endif // vtkPVTrackballMultiRotate_h
