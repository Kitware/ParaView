/*=========================================================================
  
  Program:   ParaView
  Module:    vtkInteractor.h
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
// .NAME vtkInteractor - Super class for camera and part interactors.
// .SECTION Description
// vtkInteractor provides methods neede by all interactors.  It provides
// to cordinate system of the camera in the form of three ortho-normal
// axes, computes the near and far range of actors, and provides a transform.
// It also managers the renderer.

// .SECTION see also
// vtkPartsInteractor vtkCameraInteractor

#ifndef __vtkInteractor_h
#define __vtkInteractor_h

#include "vtkObject.h"
class vtkRenderer;
class vtkTransform;
class vtkMatrix4x4;
class vtkCamera;


class VTK_EXPORT vtkInteractor : public vtkObject
{
 public:
  static vtkInteractor *New() {return new vtkInteractor;};
  vtkTypeMacro(vtkInteractor,vtkObject);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // The interactor needs a renderer before any methods are called.
  // The size of ther render is used to interpret xy points in display
  // coordinates, and to get the active camera.
  virtual void SetRenderer(vtkRenderer *ren);
  vtkGetObjectMacro(Renderer, vtkRenderer);

  // Description:
  // Given a unit vector placed at the world point "inPt", this
  // method returns the maximum length in view coordinates.
  // world point has 3 components (no w).
  float GetScaleAtPoint(float *inPt);
  float GetScaleAtPoint(float x, float y, float z)
    { float p[3]; p[0]=x; p[1]=y; p[2]=z; return this->GetScaleAtPoint(p);}

  // putting some of this here until I find a better place.
  // input == output OK
  void InterpolateCamera(vtkCamera *cam1, vtkCamera *cam2, 
                         float k, vtkCamera *camOut);

protected:
  vtkInteractor();
  ~vtkInteractor();
  vtkInteractor(const vtkInteractor&) {};
  void operator=(const vtkInteractor&) {};

  vtkRenderer *Renderer;
  vtkTransform *Transform;

  // xyz axes of the renderers active camera coordinate system.
  double CameraXAxis[3];
  double CameraYAxis[3];
  double CameraZAxis[3];

  void ComputeCameraAxes();
};

#endif


