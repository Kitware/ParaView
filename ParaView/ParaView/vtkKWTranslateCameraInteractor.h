/*=========================================================================

  Program:   ParaView
  Module:    vtkKWTranslateCameraInteractor.h
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
// .NAME vtkKWTranslateCameraInteractor
// .SECTION Description
// This is not much of a widget, but it works with a panel
// to enable the user to xy translate the camera as well as zoom.
// It zooms when the top third or bottom third
// of the screen is first selected.  The middle third pans xy.

#ifndef __vtkKWTranslateCameraInteractor_h
#define __vtkKWTranslateCameraInteractor_h

#include "vtkKWInteractor.h"
#include "vtkCameraInteractor.h"

class vtkPVRenderView;

class VTK_EXPORT vtkKWTranslateCameraInteractor : public vtkKWInteractor
{
public:
  static vtkKWTranslateCameraInteractor* New();
  vtkTypeMacro(vtkKWTranslateCameraInteractor,vtkKWInteractor);

  // Description:
  // Create a Tk widget
  virtual void Create(vtkKWApplication *app, char *args);

  // Description:
  // When the active interactor is changed, these methods allow
  // it to change its state.  This may similar to a composite.
  void Select();
  void Deselect();

  void AButtonPress(int num, int x, int y);
  void AButtonRelease(int num, int x, int y);
  void Button1Motion(int x, int y);
  void Button3Motion(int x, int y);

  // Changes the cursor based on mouse position.
  void MotionCallback(int x, int y);

protected: 
  vtkKWTranslateCameraInteractor();
  ~vtkKWTranslateCameraInteractor();
  vtkKWTranslateCameraInteractor(const vtkKWTranslateCameraInteractor&) {};
  void operator=(const vtkKWTranslateCameraInteractor&) {};

  vtkKWWidget *Label;

  // The vtk object which manipulates the camera.
  vtkCameraInteractor *Helper;

  void InitializeCursors();
  char *PanCursorName;
  char *ZoomCursorName;
  vtkSetStringMacro(PanCursorName);
  vtkSetStringMacro(ZoomCursorName);
  int CursorState;
  
  virtual int InitializeTrace();
};


#endif
