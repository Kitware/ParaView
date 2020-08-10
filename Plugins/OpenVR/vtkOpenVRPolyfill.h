/*=========================================================================

  Program:   Visualization Toolkit

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#ifndef vtkOpenVRPolyfill_h
#define vtkOpenVRPolyfill_h

#include "vtkObject.h"

class vtkOpenGLRenderer;
class vtkOpenGLRenderWindow;
class vtkOpenVRCameraPose;
class vtkOpenVRRenderer;
class vtkOpenVRRenderWindow;

class vtkOpenVRPolyfill : public vtkObject
{
public:
  static vtkOpenVRPolyfill* New();
  vtkTypeMacro(vtkOpenVRPolyfill, vtkObject);

  void SetRenderWindow(vtkOpenGLRenderWindow*);

  // get the current physical scale
  double GetPhysicalScale();
  void SetPhysicalScale(double val);

  // get the current physical translation
  double* GetPhysicalTranslation();
  void SetPhysicalTranslation(double* val) { this->SetPhysicalTranslation(val[0], val[1], val[2]); }
  void SetPhysicalTranslation(double, double, double);

  // get the current physical translation
  double* GetPhysicalViewDirection();
  void SetPhysicalViewDirection(double* val)
  {
    this->SetPhysicalViewDirection(val[0], val[1], val[2]);
  }
  void SetPhysicalViewDirection(double, double, double);

  // physcial view up
  double* GetPhysicalViewUp();
  void SetPhysicalViewUp(double* val) { this->SetPhysicalViewUp(val[0], val[1], val[2]); }
  void SetPhysicalViewUp(double, double, double);

  vtkOpenVRCameraPose* GetPhysicalPose() { return this->PhysicalPose; }

  void SetPose(vtkOpenVRCameraPose* thePose, vtkOpenGLRenderer* ren, vtkOpenGLRenderWindow* renWin);

  void ApplyPose(
    vtkOpenVRCameraPose* thePose, vtkOpenGLRenderer* ren, vtkOpenGLRenderWindow* renWin);

protected:
  vtkOpenVRPolyfill();
  ~vtkOpenVRPolyfill() override;

  vtkOpenVRCameraPose* PhysicalPose;
  vtkOpenVRRenderWindow* OpenVRRenderWindow;
  vtkOpenGLRenderWindow* RenderWindow;
  double ID;

private:
  vtkOpenVRPolyfill(const vtkOpenVRPolyfill&) = delete;
  void operator=(const vtkOpenVRPolyfill&) = delete;
};

#endif
