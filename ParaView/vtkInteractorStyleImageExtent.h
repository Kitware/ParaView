/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkInteractorStyleImageExtent.h
  Language:  C++
  Date:      $Date$
  Version:   $Revision$


Copyright (c) 1993-2000 Ken Martin, Will Schroeder, Bill Lorensen 
All rights reserved.

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are met:

 * Redistributions of source code must retain the above copyright notice,
   this list of conditions and the following disclaimer.

 * Redistributions in binary form must reproduce the above copyright notice,
   this list of conditions and the following disclaimer in the documentation
   and/or other materials provided with the distribution.

 * Neither name of Ken Martin, Will Schroeder, or Bill Lorensen nor the names
   of any contributors may be used to endorse or promote products derived
   from this software without specific prior written permission.

 * Modified source versions must be plainly marked as such, and must not be
   misrepresented as being the original software.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS ``AS IS''
AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
ARE DISCLAIMED. IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE FOR
ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

=========================================================================*/

// .NAME vtkInteractorStyleImageExtent - Manipulate the extent of an image.
// .SECTION Description


#ifndef __vtkInteractorStyleImageExtent_h
#define __vtkInteractorStyleImageExtent_h

#include "vtkInteractorStyleExtent.h"
#include "vtkImageData.h"

class VTK_EXPORT vtkInteractorStyleImageExtent : public vtkInteractorStyleExtent 
{
public:
  // Description:
  // This class must be supplied with a vtkRenderWindowInteractor wrapper or
  // parent. This class should not normally be instantiated by application
  // programmers.
  static vtkInteractorStyleImageExtent *New();

  vtkTypeMacro(vtkInteractorStyleImageExtent,vtkInteractorStyleExtent);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  void SetImageData(vtkImageData *image);
  vtkGetObjectMacro(ImageData, vtkImageData);

  // Description:
  // set flag for whether to constrain the spheres to the corners of the
  // image slice when one of the axes is collapsed
  vtkSetMacro(ConstrainSpheres, int);
  vtkGetMacro(ConstrainSpheres, int);
  vtkBooleanMacro(ConstrainSpheres, int);
  
  // only overriding the functionality of the middle mouse button
  virtual void OnMouseMove  (int ctrl, int shift, int X, int Y);
  virtual void OnMiddleButtonDown(int ctrl, int shift, int X, int Y);
  virtual void OnMiddleButtonUp  (int ctrl, int shift, int X, int Y);
  
protected:

  vtkInteractorStyleImageExtent();
  ~vtkInteractorStyleImageExtent();
  vtkInteractorStyleImageExtent(const vtkInteractorStyleImageExtent&) {};
  void operator=(const vtkInteractorStyleImageExtent&) {};

  vtkImageData *ImageData;
  // Constrain spheres to the corners of the extent?
  int ConstrainSpheres;
  // Which axis to translate along in TranslateZ
  int TranslateAxis;
  
  void GetWorldSpot(int spotId, float spot[3]); 
  void GetSpotAxes(int spotId, double *v0, 
                   double *v1, double *v2);
  int *GetWholeExtent();
  void GetTranslateAxis();
  void TranslateZ(int dx, int dy);
};

#endif
