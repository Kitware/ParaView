/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkPVCameraIcon.h
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
// .NAME vtkPVCameraIcon -
// .SECTION Description

#ifndef __vtkPVCameraIcon_h
#define __vtkPVCameraIcon_h

#include "vtkKWImageLabel.h"

class vtkKWPushButton;
class vtkPVRenderView;
class vtkCamera;

class VTK_EXPORT vtkPVCameraIcon : public vtkKWImageLabel
{
public:
  static vtkPVCameraIcon* New();
  vtkTypeMacro(vtkPVCameraIcon, vtkKWImageLabel);
  void PrintSelf(ostream& os, vtkIndent indent);

  void Create(vtkKWApplication *pvApp);
  
  // Description:
  // Set the current render view.
  virtual void SetRenderView(vtkPVRenderView*);

  // Description:
  // Store the current camera from the render view.
  virtual void StoreCamera();

  // Description:
  // If the camera exists, restore the current camera to the render
  // view.
  virtual void RestoreCamera();

  // Description:
  // Get the stored camera as vtkCamera.
  vtkGetObjectMacro(Camera, vtkCamera);

protected:
  vtkPVCameraIcon();
  ~vtkPVCameraIcon();

  vtkPVRenderView* RenderView;
  vtkCamera* Camera;
  int Width;
  int Height;
  
private:
  vtkPVCameraIcon(const vtkPVCameraIcon&); // Not implemented
  void operator=(const vtkPVCameraIcon&); // Not implemented
};

#endif
