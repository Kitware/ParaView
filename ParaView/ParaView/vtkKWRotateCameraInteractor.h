/*=========================================================================

  Program:   ParaView
  Module:    vtkKWRotateCameraInteractor.h
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
// .NAME vtkKWRotateCameraInteractor
// .SECTION Description
// This widget gets displayed when rotate mode is selected.

#ifndef __vtkKWRotateCameraInteractor_h
#define __vtkKWRotateCameraInteractor_h

#include "vtkKWInteractor.h"
#include "vtkCameraInteractor.h"
class vtkKWCenterOfRotation;


class VTK_EXPORT vtkKWRotateCameraInteractor : public vtkKWInteractor
{
public:
  static vtkKWRotateCameraInteractor* New();
  vtkTypeMacro(vtkKWRotateCameraInteractor,vtkKWInteractor);

  // Description:
  // Create a Tk widget
  virtual void Create(vtkKWApplication *app, char *args);

  // Description:
  // When the active interactor is changed, these methods allow
  // it to change its state.  This may similar to a composite.
  void Select();
  void Deselect();

  // When camera changes, we need to recompute the center of rotation.
  void CameraMovedNotify();

  // Description:
  // We need to tell the center of rotation widget the render view.
  void SetRenderView(vtkPVRenderView *view);

  void AButtonPress(int num, int x, int y);
  void AButtonRelease(int num, int x, int y);
  void Button1Motion(int x, int y);
  void Button3Motion(int x, int y);

  void MotionCallback(int x, int y);

  // Description:
  // Get rid of all references so circular references will not
  // keep objects from being deleted.
  void PrepareForDelete();

  // Description:
  // Set the center of rotation manually.
  void SetCenter(double x, double y, double z);

protected: 
  vtkKWRotateCameraInteractor();
  ~vtkKWRotateCameraInteractor();
  vtkKWRotateCameraInteractor(const vtkKWRotateCameraInteractor&) {};
  void operator=(const vtkKWRotateCameraInteractor&) {};

  vtkKWCenterOfRotation *CenterUI;

  // The vtk object which manipulates the camera.
  // Maybe this should go in the RenderView instead of having a different one
  // for each interactor.
  vtkCameraInteractor *Interactor;

  // This is here to get around thae fact that we do not have a good Roll cursor.
  void UpdateRollCursor(double px, double py);

  int CursorState;
};


#endif


