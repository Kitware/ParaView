/*=========================================================================

  Program:   ParaView
  Module:    vtkPVPivotManipulator.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkPVPivotManipulator - Change the center of manipulation.
// .SECTION Description
// vtkPVPivotManipulator allows the user to interactively manipulate
// the center of manipulation. If the user clicks on the object, the
// point on the object will be selected.

#ifndef __vtkPVPivotManipulator_h
#define __vtkPVPivotManipulator_h

#include "vtkPVCameraManipulator.h"

class vtkRenderer;
class vtkPVWorldPointPicker;

class VTK_EXPORT vtkPVPivotManipulator : public vtkPVCameraManipulator
{
public:
  static vtkPVPivotManipulator *New();
  vtkTypeRevisionMacro(vtkPVPivotManipulator, vtkPVCameraManipulator);
  void PrintSelf(ostream& os, vtkIndent indent);
  
  // Description:
  // Event bindings controlling the effects of pressing mouse buttons
  // or moving the mouse.
  virtual void OnMouseMove(int x, int y, vtkRenderer *ren,
                           vtkRenderWindowInteractor *rwi);
  virtual void OnButtonDown(int x, int y, vtkRenderer *ren,
                            vtkRenderWindowInteractor *rwi);
  virtual void OnButtonUp(int x, int y, vtkRenderer *ren,
                          vtkRenderWindowInteractor *rwi);

  // Description:
  // This method is called by the control when the reset button is
  // pressed. The reason for the name starting with set is because the
  // same mechanism is used as for other widgets and they do Set/Get
  void SetResetCenterOfRotation();

  void SetCenterOfRotation(float x, float y, float z);
  void SetCenterOfRotation(float* x);
  vtkGetVector3Macro(CenterOfRotation, float);

protected:
  vtkPVPivotManipulator();
  ~vtkPVPivotManipulator();

  // Description:
  // Set the center of rotation.
  void SetCenterOfRotationInternal(float x, float y, float z);

  // Description:
  // Pick at position x, y
  void Pick(vtkRenderer*, int x, int y);

  vtkPVWorldPointPicker *Picker;
  float CenterOfRotation[3];

  vtkPVPivotManipulator(const vtkPVPivotManipulator&); // Not implemented
  void operator=(const vtkPVPivotManipulator&); // Not implemented
};

#endif
