// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-License-Identifier: BSD-3-Clause

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

  ///@{
  /**
   * set/get the current physical scale
   */
  double GetPhysicalScale();
  void SetPhysicalScale(double val);
  ///@}

  ///@{
  /**
   * set/get the current physical translation
   */
  double* GetPhysicalTranslation();
  void SetPhysicalTranslation(double* val) { this->SetPhysicalTranslation(val[0], val[1], val[2]); }
  void SetPhysicalTranslation(double, double, double);
  ///@}

  ///@{
  /**
   * set/get the current physical translation
   */
  double* GetPhysicalViewDirection();
  void SetPhysicalViewDirection(double* val)
  {
    this->SetPhysicalViewDirection(val[0], val[1], val[2]);
  }
  void SetPhysicalViewDirection(double, double, double);
  ///@}

  ///@{
  /**
   * physical view up
   */
  double* GetPhysicalViewUp();
  void SetPhysicalViewUp(double* val) { this->SetPhysicalViewUp(val[0], val[1], val[2]); }
  void SetPhysicalViewUp(double, double, double);
  ///@}

  vtkVRCamera::Pose* GetPhysicalPose() { return this->PhysicalPose; }

  void SetPose(vtkVRCamera::Pose* thePose, vtkOpenGLRenderer* ren, vtkOpenGLRenderWindow* renWin);

  void ApplyPose(vtkVRCamera::Pose* thePose, vtkOpenGLRenderer* ren, vtkOpenGLRenderWindow* renWin);

protected:
  vtkXRInterfacePolyfill();
  ~vtkXRInterfacePolyfill() override;

  vtkVRCamera::Pose* PhysicalPose = nullptr;
  vtkVRRenderWindow* VRRenderWindow = nullptr;
  vtkOpenGLRenderWindow* RenderWindow = nullptr;
  double ID;

private:
  vtkXRInterfacePolyfill(const vtkXRInterfacePolyfill&) = delete;
  void operator=(const vtkXRInterfacePolyfill&) = delete;
};

#endif
