# SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
# SPDX-FileCopyrightText: Copyright (c) Sandia Corporation
# SPDX-License-Identifier: BSD-3-Clause
set(classes
  vtkAnimationPlayer
  vtkCameraInterpolator2
  vtkCompositeAnimationPlayer
  vtkPVAnimationCue
  vtkPVBooleanKeyFrame
  vtkPVCameraAnimationCue
  vtkPVCameraCueManipulator
  vtkPVCameraKeyFrame
  vtkPVCompositeKeyFrame
  vtkPVCueManipulator
  vtkPVExponentialKeyFrame
  vtkPVKeyFrame
  vtkPVKeyFrameAnimationCue
  vtkPVKeyFrameAnimationCueForProxies
  vtkPVKeyFrameCueManipulator
  vtkPVRampKeyFrame
  vtkPVRepresentationAnimationHelper
  vtkPVSinusoidKeyFrame
  vtkRealtimeAnimationPlayer
  vtkSIXMLAnimationWriterRepresentationProperty
  vtkSMAnimationFrameWindowDomain
  vtkSMAnimationScene
  vtkSMAnimationSceneGeometryWriter
  vtkSMAnimationSceneWebWriter
  vtkSMAnimationSceneProxy
  vtkSMAnimationSceneWriter
  vtkSMSaveAnimationExtractsProxy
  vtkSMSaveAnimationProxy
  vtkSequenceAnimationPlayer
  vtkTimestepsAnimationPlayer
  vtkXMLPVAnimationWriter)

set(xml_files
  Resources/animation.xml
  Resources/writers_animation.xml)

if (TARGET VTK::PythonInterpreter)
  # TODO: Split these into a separate module so that there isn't a public
  # optional dependency.
  list(APPEND classes
    vtkPythonAnimationCue)
  list(APPEND xml_files
    Resources/animation_python.xml)
endif ()

vtk_module_add_module(ParaView::RemotingAnimation
  CLASSES ${classes})


if (WIN32)
  list(APPEND xml_files
    Resources/animation_win.xml)
endif ()

if (TARGET VTK::IOFFMPEG)
  list(APPEND xml_files
    Resources/animation_ffmpeg.xml)
endif ()

if (TARGET VTK::IOOggTheora)
  list(APPEND xml_files
    Resources/animation_oggtheora.xml)
endif ()

paraview_add_server_manager_xmls(
  XMLS  ${xml_files})
