/*=========================================================================

  Program:   ParaView
  Module:    vtkInteractorStyleCamera.h
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

// .NAME vtkInteractorStyleCamera - Manipulation of an implicit plane.

// .SECTION Description
// vtkInteractorStyleCamera Allows interactive definition of a plane
// by manipulating a vtkPlane's parameters.  
// The center of the plane is hot.  The center provides
// rotation (left mouse button), XY translation relative to camera (middle),
// and translation along camera's view-plane normal (right).


#ifndef __vtkInteractorStyleCamera_h
#define __vtkInteractorStyleCamera_h

#include "vtkInteractorStyle.h"


#define VTK_INTERACTOR_STYLE_CAMERA_NONE    0
#define VTK_INTERACTOR_STYLE_CAMERA_ROTATE  1
#define VTK_INTERACTOR_STYLE_CAMERA_PAN     2
#define VTK_INTERACTOR_STYLE_CAMERA_ZOOM    3

class VTK_EXPORT vtkInteractorStyleCamera : public vtkInteractorStyle
{
public:
  // Description:
  // This class must be supplied with a vtkRenderWindowInteractor wrapper or
  // parent. This class should not normally be instantiated by application
  // programmers.
  static vtkInteractorStyleCamera *New();

  vtkTypeMacro(vtkInteractorStyleCamera,vtkObject);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Generic event bindings must be overridden in subclasses
  void OnMouseMove  (int ctrl, int shift, int x, int y);
  void OnLeftButtonDown(int ctrl, int shift, int x, int y);
  void OnLeftButtonUp  (int ctrl, int shift, int x, int y);
  void OnMiddleButtonDown(int ctrl, int shift, int x, int y);
  void OnMiddleButtonUp  (int ctrl, int shift, int x, int y);
  void OnRightButtonDown(int ctrl, int shift, int x, int y);
  void OnRightButtonUp  (int ctrl, int shift, int x, int y);

protected:
  vtkInteractorStyleCamera();
  ~vtkInteractorStyleCamera();
  vtkInteractorStyleCamera(const vtkInteractorStyleCamera&) {};
  void operator=(const vtkInteractorStyleCamera&) {};

  void RotateXY(int dx, int dy);
  void PanXY(int x, int y, int oldx, int oldy);
  void DollyXY(int dx, int dy);
  
  int State;
  float MotionFactor;
};

#endif
