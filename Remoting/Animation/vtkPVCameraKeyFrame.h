/*=========================================================================

  Program:   ParaView
  Module:    vtkPVCameraKeyFrame.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   vtkPVCameraKeyFrame
 *
 * Special key frame for animating Camera. Unlike typical keyframes,
 * this keyframe interpolates a camera and not a property on the camera.
*/

#ifndef vtkPVCameraKeyFrame_h
#define vtkPVCameraKeyFrame_h

#include "vtkPVKeyFrame.h"
#include "vtkRemotingAnimationModule.h" //needed for exports

class vtkCamera;
class vtkCameraInterpolator2;
class vtkPVAnimationCue;

class VTKREMOTINGANIMATION_EXPORT vtkPVCameraKeyFrame : public vtkPVKeyFrame
{
public:
  static vtkPVCameraKeyFrame* New();
  vtkTypeMacro(vtkPVCameraKeyFrame, vtkPVKeyFrame);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  /**
   * If the vtkPVCameraCueManipulator is in CAMERA mode, then this method is
   * not even called since the interpolation is done by vtkCameraInterpolator
   * maintained by vtkPVCameraCueManipulator itself. However,  in PATH mode,
   * this method is called to allow the key frame to use vtkCameraInterpolator2
   * to do path-based interpolations for the camera.
   */
  void UpdateValue(double currenttime, vtkPVAnimationCue* cue, vtkPVKeyFrame* next) override;

  // Overridden, since these methods are not supported by this class.
  void SetKeyValue(unsigned int, double) override {}
  void SetKeyValue(double) override {}
  virtual double GetKeyValue(unsigned int) { return 0; }

  //@{
  /**
   * Get the camera i.e. the key value for this key frame.
   */
  vtkGetObjectMacro(Camera, vtkCamera);
  //@}

  //@{
  /**
   * Methods to set the current camera value.
   */
  void SetPosition(double x, double y, double z);
  void SetFocalPoint(double x, double y, double z);
  void SetViewUp(double x, double y, double z);
  void SetViewAngle(double angle);
  void SetParallelScale(double scale);
  //@}

  //@{
  /**
   * Forwarded to vtkCameraInterpolator2.
   */
  void AddPositionPathPoint(double x, double y, double z);
  void ClearPositionPath();
  void AddFocalPathPoint(double x, double y, double z);
  void ClearFocalPath();
  void SetFocalPointMode(int val);
  void SetPositionMode(int val);
  void SetClosedFocalPath(bool val);
  void SetClosedPositionPath(bool val);
  //@}

protected:
  vtkPVCameraKeyFrame();
  ~vtkPVCameraKeyFrame() override;

  vtkCamera* Camera;
  vtkCameraInterpolator2* Interpolator;

private:
  vtkPVCameraKeyFrame(const vtkPVCameraKeyFrame&) = delete;
  void operator=(const vtkPVCameraKeyFrame&) = delete;
};

#endif
