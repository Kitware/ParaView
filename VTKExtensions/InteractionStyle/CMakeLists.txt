# SPDX-FileCopyrightText: Copyright (c) Sandia Corporation, Kitware Inc.
# SPDX-License-Identifier: BSD-3-CLAUSE
set(classes
  vtkCameraManipulator
  vtkCameraManipulatorGUIHelper
  vtkPVInteractorStyle
  vtkPVJoystickFly
  vtkPVJoystickFlyIn
  vtkPVJoystickFlyOut
  vtkPVTrackballMoveActor
  vtkPVTrackballMultiRotate
  vtkPVTrackballPan
  vtkPVTrackballRoll
  vtkPVTrackballRotate
  vtkPVTrackballEnvironmentRotate
  vtkPVTrackballZoom
  vtkPVTrackballZoomToMouse
  vtkTrackballPan)

vtk_module_add_module(ParaView::VTKExtensionsInteractionStyle
  CLASSES ${classes})
