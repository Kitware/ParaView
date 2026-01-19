// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-License-Identifier: BSD-3-Clause
/**
 * @class   vtkSMVRJoystickCameraStyleProxy
 * @brief   interactor style with a controller with joysticks
 *
 * vtkSMVRJoystickCameraStyleProxy is an interaction style that binds a controller
 * to enable movement of the camera with the joysticks. The input device must contains
 * 2 joysticks with valuators going from -1 to 1 in X and Y to function correctly (a
 * `vrpn_Joylin` or a `vrpn_Joywin32` for example with VRPN). In the CAVE, this interaction
 * style won't have any effect unless the off-axis projection is turned off in the
 * CAVE configuration.
 */
#ifndef vtkSMVRJoystickCameraStyleProxy_h
#define vtkSMVRJoystickCameraStyleProxy_h

#include "vtkPVIncubatorCAVEInteractionStylesModule.h" // for export macro
#include "vtkSMVRInteractorStyleProxy.h"

class vtkCamera;
class vtkSMRenderViewProxy;
class vtkSMDoubleVectorProperty;
class vtkSMIntVectorProperty;
struct vtkVREvent;

class VTKPVINCUBATORCAVEINTERACTIONSTYLES_EXPORT vtkSMVRJoystickCameraStyleProxy
  : public vtkSMVRInteractorStyleProxy
{
public:
  static vtkSMVRJoystickCameraStyleProxy* New();
  vtkTypeMacro(vtkSMVRJoystickCameraStyleProxy, vtkSMVRInteractorStyleProxy);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  /**
   * Override this function to automatically set the controlled property
   */
  int GetControlledPropertySize() override { return 0; }

  ///@{
  /**
   * Set/Get the sensitivity when rotating the camera (0.01 by default).
   */
  vtkSetMacro(LookRotationSensitivity, double);
  vtkGetMacro(LookRotationSensitivity, double);
  ///@}

  ///@{
  /**
   * Set the sensitivity when moving the camera (0.1 by default).
   */
  vtkSetMacro(MoveCameraSensitivity, double);
  vtkGetMacro(MoveCameraSensitivity, double);
  ///@}

  ///@{
  /**
   * Set/Get the inversion of the rotation axis of the camera (false by default).
   */
  vtkSetMacro(InvertXAxis, bool);
  vtkGetMacro(InvertXAxis, bool);
  vtkSetMacro(InvertYAxis, bool);
  vtkGetMacro(InvertYAxis, bool);
  ///@}

  /**
   * Enum axis to retrieve the forward, the right and the up axis of the scene
   */
  enum Axis
  {
    X_AXIS = 0,
    Y_AXIS,
    Z_AXIS
  };

  ///@{
  /**
   * Set/Get the up axis of the scene to define axis controlled by pitch and yaw (Y Axis by
   * default).
   */
  vtkSetMacro(UpAxis, Axis);
  vtkGetMacro(UpAxis, Axis);
  ///@}

  /**
   * Overridden to defer expensive calculations and update vtk objects
   */
  bool Update() override;

  /**
   * Overridden to support retrieving the move speed, the joystick sensitivity and the up axis
   * from the UI.
   */
  void UpdateVTKObjects() override;

protected:
  vtkSMVRJoystickCameraStyleProxy();
  ~vtkSMVRJoystickCameraStyleProxy() override = default;

  void HandleValuator(const vtkVREvent& event) override;

private:
  vtkSMVRJoystickCameraStyleProxy(const vtkSMVRJoystickCameraStyleProxy&) = delete;
  void operator=(const vtkSMVRJoystickCameraStyleProxy&) = delete;

  double OrientationX = 0;
  bool InvertXAxis = false;
  double OrientationY = 0;
  bool InvertYAxis = false;

  double MoveForward = 0;
  double MoveRight = 0;

  Axis UpAxis = Y_AXIS;

  double LookRotationSensitivity = 0.01;
  double MoveCameraSensitivity = 0.1;
};

#endif //  vtkSMVRJoystickCameraStyleProxy_h
