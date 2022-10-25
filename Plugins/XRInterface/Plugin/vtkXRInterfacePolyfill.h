/*=========================================================================

  Program:   ParaView

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#ifndef vtkXRInterfacePolyfill_h
#define vtkXRInterfacePolyfill_h

#include "vtkObject.h"
#include "vtkVRCamera.h" // For visibility of inner Pose class

class vtkOpenGLRenderer;
class vtkOpenGLRenderWindow;
class vtkVRRenderWindow;

class vtkXRInterfacePolyfill : public vtkObject
{
public:
  static vtkXRInterfacePolyfill* New();
  vtkTypeMacro(vtkXRInterfacePolyfill, vtkObject);

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

  // physical view up
  double* GetPhysicalViewUp();
  void SetPhysicalViewUp(double* val) { this->SetPhysicalViewUp(val[0], val[1], val[2]); }
  void SetPhysicalViewUp(double, double, double);

  vtkVRCamera::Pose* GetPhysicalPose() { return this->PhysicalPose; }

  void SetPose(vtkVRCamera::Pose* thePose, vtkOpenGLRenderer* ren, vtkOpenGLRenderWindow* renWin);

  void ApplyPose(vtkVRCamera::Pose* thePose, vtkOpenGLRenderer* ren, vtkOpenGLRenderWindow* renWin);

protected:
  vtkXRInterfacePolyfill();
  ~vtkXRInterfacePolyfill() override;

  vtkVRCamera::Pose* PhysicalPose;
  vtkVRRenderWindow* VRRenderWindow;
  vtkOpenGLRenderWindow* RenderWindow;
  double ID;

private:
  vtkXRInterfacePolyfill(const vtkXRInterfacePolyfill&) = delete;
  void operator=(const vtkXRInterfacePolyfill&) = delete;
};

#endif
