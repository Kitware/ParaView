/*=========================================================================

  Program:   ParaView
  Module:    vtkPVCameraManipulator.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkPVCameraManipulator - Abstraction of style away from button.
// .SECTION Description
// vtkPVCameraManipulator is a subclass pf vtkCameraManipulator for those
// manipulators that neeed vtkPVApplication.

#ifndef __vtkPVCameraManipulator_h
#define __vtkPVCameraManipulator_h

#include "vtkCameraManipulator.h"

class vtkRenderer;
class vtkRenderWindowInteractor;
class vtkPVApplication;

class VTK_EXPORT vtkPVCameraManipulator : public vtkCameraManipulator
{
public:
  static vtkPVCameraManipulator *New();
  vtkTypeRevisionMacro(vtkPVCameraManipulator, vtkCameraManipulator);
  void PrintSelf(ostream& os, vtkIndent indent);
  
  // Description:
  // In order to make calls on the application, we need a pointer to
  // it.
  void SetApplication(vtkPVApplication*);
  vtkGetObjectMacro(Application, vtkPVApplication);

protected:
  vtkPVCameraManipulator();
  ~vtkPVCameraManipulator();

  vtkPVApplication *Application;

private:
  vtkPVCameraManipulator(const vtkPVCameraManipulator&); // Not implemented
  void operator=(const vtkPVCameraManipulator&); // Not implemented
};

#endif
