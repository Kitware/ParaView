/*=========================================================================
  
  Program:   Visualization Toolkit
  Module:    vtkInteractor.h
  Language:  C++
  Date:      $Date$
  Version:   $Revision$
  
Copyright (c) 1998-1999 Kitware Inc. 469 Clifton Corporate Parkway,
Clifton Park, NY, 12065, USA.

All rights reserved. No part of this software may be reproduced, distributed,
or modified, in any form or by any means, without permission in writing from
Kitware Inc.

IN NO EVENT SHALL THE AUTHORS OR DISTRIBUTORS BE LIABLE TO ANY PARTY FOR
DIRECT, INDIRECT, SPECIAL, INCIDENTAL, OR CONSEQUENTIAL DAMAGES ARISING OUT
OF THE USE OF THIS SOFTWARE, ITS DOCUMENTATION, OR ANY DERIVATIVES THEREOF,
EVEN IF THE AUTHORS HAVE BEEN ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

THE AUTHORS AND DISTRIBUTORS SPECIFICALLY DISCLAIM ANY WARRANTIES, INCLUDING,
BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY, FITNESS FOR A
PARTICULAR PURPOSE, AND NON-INFRINGEMENT.  THIS SOFTWARE IS PROVIDED ON AN
"AS IS" BASIS, AND THE AUTHORS AND DISTRIBUTORS HAVE NO OBLIGATION TO PROVIDE
MAINTENANCE, SUPPORT, UPDATES, ENHANCEMENTS, OR MODIFICATIONS.

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


