# SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
# SPDX-FileCopyrightText: Copyright (c) Sandia Corporation
# SPDX-License-Identifier: BSD-3-Clause
set(classes
  vtkPVCameraManipulator
  vtkCameraManipulatorGUIHelper
  vtkPVInteractorStyle
  vtkPVJoystickFly
  vtkPVJoystickFlyIn
  vtkPVJoystickFlyOut
  vtkPVTrackballMoveActor
  vtkPVTrackballMultiRotate
  vtkPVTrackballPan
  vtkPVTrackballPanAxisConstrained
  vtkPVTrackballRoll
  vtkPVTrackballRotate
  vtkPVTrackballEnvironmentRotate
  vtkPVTrackballZoom
  vtkPVTrackballZoomToMouse)

vtk_module_add_module(ParaView::VTKExtensionsInteractionStyle
  CLASSES ${classes})
