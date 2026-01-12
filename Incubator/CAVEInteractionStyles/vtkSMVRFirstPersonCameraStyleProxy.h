// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-License-Identifier: BSD-3-Clause
/**
 * @class   vtkSMVRFirstPersonCameraStyleProxy
 * @brief   interactor style with mouse and keyboard like a first person flight
 *
 * vtkSMVRFirstPersonCameraStyleProxy is an interaction style that binds the mouse
 * and the keyboard to enable movement like a first person flight by changing
 * the view direction of the camera with the mouse and by using four keys of
 * the keyboard to move it in the space. The input device must return the absolute
 * position of the mouse on the screen as valuators (a valuator for X position and
 * a valuator for Y position) to function correctly (a `vrpn_Mouse` for example with
 * VRPN). In the CAVE, this interaction style won't have any effect unless the off-axis
 * projection is turned off in the CAVE configuration.
 */
#ifndef vtkSMVRFirstPersonCameraStyleProxy_h
#define vtkSMVRFirstPersonCameraStyleProxy_h

#include "vtkPVIncubatorCAVEInteractionStylesModule.h" // for export macro
#include "vtkSMVRInteractorStyleProxy.h"

class vtkCamera;
class vtkSMRenderViewProxy;
class vtkSMDoubleVectorProperty;
class vtkSMIntVectorProperty;
struct vtkVREvent;

class VTKPVINCUBATORCAVEINTERACTIONSTYLES_EXPORT vtkSMVRFirstPersonCameraStyleProxy
  : public vtkSMVRInteractorStyleProxy
{
public:
  static vtkSMVRFirstPersonCameraStyleProxy* New();
  vtkTypeMacro(vtkSMVRFirstPersonCameraStyleProxy, vtkSMVRInteractorStyleProxy);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  /**
   * Override this function to automatically set the controlled property
   */
  int GetControlledPropertySize() override { return 0; }

  ///@{
  /**
   * Set/Get the mouse sensitivity values (by default : 6.28 for X and 3.14 for Y).
   */
  vtkSetMacro(MouseSensitivityX, double);
  vtkGetMacro(MouseSensitivityX, double);
  vtkSetMacro(MouseSensitivityY, double);
  vtkGetMacro(MouseSensitivityY, double);
  ///@}

  ///@{
  /**
   * Set/Get the move speed values (by default : 1.0 for forward speed and 0.5 for right speed).
   */
  vtkSetMacro(MoveSpeedForward, double);
  vtkGetMacro(MoveSpeedForward, double);
  vtkSetMacro(MoveSpeedRight, double);
  vtkGetMacro(MoveSpeedRight, double);
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
   * Overridden to support retrieving the move speed, the mouse sensitivity, the inverted axis and
   * the up axis from the UI.
   */
  void UpdateVTKObjects() override;

protected:
  vtkSMVRFirstPersonCameraStyleProxy();
  ~vtkSMVRFirstPersonCameraStyleProxy() override = default;

  void HandleButton(const vtkVREvent& event) override;
  void HandleValuator(const vtkVREvent& event) override;

private:
  vtkSMVRFirstPersonCameraStyleProxy(const vtkSMVRFirstPersonCameraStyleProxy&) = delete;
  void operator=(const vtkSMVRFirstPersonCameraStyleProxy&) = delete;

  double MousePosX = 0;
  bool InvertXAxis = false;
  double MousePosY = 0;
  bool InvertYAxis = false;
  int MoveForward = 0; // -1 for moving backward and 1 for moving forward
  int MoveRight = 0;   // -1 for moving left and 1 for moving right

  Axis UpAxis = Y_AXIS;

  double MouseSensitivityX = 6.28;
  double MouseSensitivityY = 3.14;
  double MoveSpeedRight = 0.5;
  double MoveSpeedForward = 1;
};

#endif //  vtkSMVRFirstPersonCameraStyleProxy_h
