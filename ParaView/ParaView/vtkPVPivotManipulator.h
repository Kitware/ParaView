/*=========================================================================

  Program:   ParaView
  Module:    vtkPVPivotManipulator.h
  Language:  C++
  Date:      $Date$
  Version:   $Revision$
  
Copyright (c) 2000-2001 Kitware Inc. 469 Clifton Corporate Parkway,
Clifton Park, NY, 12065, USA.
All rights reserved.

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are met:

 * Redistributions of source code must retain the above copyright notice,
   this list of conditions and the following disclaimer.

 * Redistributions in binary form must reproduce the above copyright notice,
   this list of conditions and the following disclaimer in the documentation
   and/or other materials provided with the distribution.

 * Neither the name of Kitware nor the names of any contributors may be used
   to endorse or promote products derived from this software without specific 
   prior written permission.

 * Modified source versions must be plainly marked as such, and must not be
   misrepresented as being the original software.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS ``AS IS''
AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
ARE DISCLAIMED. IN NO EVENT SHALL THE AUTHORS OR CONTRIBUTORS BE LIABLE FOR
ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

=========================================================================*/
// .NAME vtkPVPivotManipulator - Fly camera towards or away from the object
// .SECTION Description
// vtkPVPivotManipulator allows the user to interactively manipulate the
// camera, the viewpoint of the scene.

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

protected:
  vtkPVPivotManipulator();
  ~vtkPVPivotManipulator();

  // Description:
  // Set the center of rotation.
  void SetCenterOfRotation(float x, float y, float z);

  // Description:
  // Pick at position x, y
  void Pick(vtkRenderer*, int x, int y);

  vtkPVWorldPointPicker *Picker;

  vtkPVPivotManipulator(const vtkPVPivotManipulator&); // Not implemented
  void operator=(const vtkPVPivotManipulator&); // Not implemented
};

#endif
