/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkInteractorStyleExtent.h
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

// .NAME vtkInteractorStyleExtent - Manipulate the extent of a structured data object.

// .SECTION Description

#ifndef __vtkInteractorStyleExtent_h
#define __vtkInteractorStyleExtent_h

#include "vtkInteractorStyle.h"
#include "vtkRenderWindowInteractor.h"
#include "vtkActor.h"
#include "vtkPolyData.h"
#include "vtkSphereSource.h"

class vtkPolyDataMapper;
class vtkOutlineSource;

class VTK_EXPORT vtkInteractorStyleExtent : public vtkInteractorStyle 
{
public:
  static vtkInteractorStyleExtent* New();
  vtkTypeMacro(vtkInteractorStyleExtent,vtkInteractorStyle);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Generic event bindings must be overridden in subclasses
  virtual void OnMouseMove  (int ctrl, int shift, int X, int Y);
  virtual void OnLeftButtonDown(int ctrl, int shift, int X, int Y);
  virtual void OnLeftButtonUp  (int ctrl, int shift, int X, int Y);
  virtual void OnMiddleButtonDown(int ctrl, int shift, int X, int Y);
  virtual void OnMiddleButtonUp  (int ctrl, int shift, int X, int Y);
  virtual void OnRightButtonDown(int ctrl, int shift, int X, int Y);
  virtual void OnRightButtonUp  (int ctrl, int shift, int X, int Y);

  // Description:
  void SetExtent(int min0, int max0, int min1, int max1, 
                 int min2, int max2);
  void SetExtent(int *e) 
    {this->SetExtent(e[0],e[1],e[2],e[3],e[4],e[5]);}
  vtkGetVector6Macro(Extent, int);

  // Description:
  // Specify function to be called when a significant event occurs.
  // the instance variable "CallbackType" will be set to one of the 
  // following strings: "InteractiveChange" "ButtonRelease"
  void SetCallbackMethod(void (*f)(void *), void *arg);
  vtkGetStringMacro(CallbackType);

  // Description:
  // Set the arg delete method. This is used to free user memory.
  void SetCallbackMethodArgDelete(void (*f)(void *));

  // Description:
  // The CallbackMethod
  void DefaultCallback(char *type);

protected:
  vtkInteractorStyleExtent();
  ~vtkInteractorStyleExtent();
  vtkInteractorStyleExtent(const vtkInteractorStyleExtent&) {};
  void operator=(const vtkInteractorStyleExtent&) {};

  int Extent[6];
  // For controlling the extent.
  int *ExtentPtr0;
  int *ExtentPtr1;
  int *ExtentPtr2;
  double ExtentRemainder0;
  double ExtentRemainder1;
  double ExtentRemainder2;
  double DisplayToExtentMatrixRow0[2];
  double DisplayToExtentMatrixRow1[2];
  double *DisplayToExtentMatrix[2];
  int DisplayToExtentPermutation0;
  int DisplayToExtentPermutation1;

  int CurrentSpotId;
  int OldSpotId;
  float CurrentSpot[3];

  vtkSphereSource    *SphereSource;
  vtkPolyDataMapper  *SphereMapper;
  vtkActor           *SphereActor;

  // Which mouse button is currently pressed (-1 => None).
  int Button;
  // Indicates which corner is highlighted.
  int ActiveCorner;

  void HandleIndicator(int x, int y); 
  void TranslateXY(int dx, int dy);
  void ComputeDisplayToExtentMapping();

  // methods implemented by the subclass
  virtual void GetWorldSpot(int spotId, float spot[3]) {}; 
  virtual void GetSpotAxes(int spotId, double *v0, 
                           double *v1, double *v2) {};
  virtual int *GetWholeExtent() {return NULL;}; 

  // Description:
  // The CallbackMethod
  void (*CallbackMethod)(void *);
  void *CallbackMethodArg;
  char *CallbackType;
  void (*CallbackMethodArgDelete)(void *);

  // Ideally we would make the CallbackType a 
  // parameter of the Callback method.
  vtkSetStringMacro(CallbackType);


};

#endif
