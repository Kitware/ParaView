/*=========================================================================

  Program:   ParaView
  Module:    vtkPVInteractorStyle.h
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
// .NAME vtkPVInteractorStyle - interactive manipulation of the camera
// .SECTION Description
// vtkPVInteractorStyle allows the user to interactively
// manipulate the camera, the viewpoint of the scene.
// The left button is for rotation; shift + left button is for rolling;
// the right button is for panning; and shift + right button is for zooming.

#ifndef __vtkPVInteractorStyle_h
#define __vtkPVInteractorStyle_h

#include "vtkInteractorStyle.h"

class vtkPVCameraManipulator;
class vtkCollection;

class VTK_EXPORT vtkPVInteractorStyle : public vtkInteractorStyle
{
public:
  static vtkPVInteractorStyle *New();
  vtkTypeRevisionMacro(vtkPVInteractorStyle, vtkInteractorStyle);
  void PrintSelf(ostream& os, vtkIndent indent);
  
  // Description:
  // Event bindings controlling the effects of pressing mouse buttons
  // or moving the mouse.
  virtual void OnMouseMove();
  virtual void OnLeftButtonDown();
  virtual void OnLeftButtonUp();
  virtual void OnMiddleButtonDown();
  virtual void OnMiddleButtonUp();
  virtual void OnRightButtonDown();
  virtual void OnRightButtonUp();
  
  // Description:
  // Access to adding or removing manipulators.
  void AddManipulator(vtkPVCameraManipulator *m);

  // Description:
  // Accessor for the collection of camera manipulators.
  vtkGetObjectMacro(CameraManipulators, vtkCollection);

  // Description:
  // Propagates the center to the manipulators.
  void SetCenterOfRotation(float x, float y, float z);

protected:
  vtkPVInteractorStyle();
  ~vtkPVInteractorStyle();

  vtkPVCameraManipulator *Current;

  // The CameraInteractors also store there button and modifier.
  vtkCollection *CameraManipulators;

  void OnButtonDown(int button, int shift, int control);
  void OnButtonUp(int button);
  void ResetLights();

  vtkPVInteractorStyle(const vtkPVInteractorStyle&); // Not implemented
  void operator=(const vtkPVInteractorStyle&); // Not implemented
};

#endif
