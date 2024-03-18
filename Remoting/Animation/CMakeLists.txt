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

_vtk_module_optional_dependency_exists(VTK::Python
  SATISFIED_VAR _have_vtk_python)
_vtk_module_optional_dependency_exists(VTK::PythonInterpreter
  SATISFIED_VAR _have_vtk_pythoninterpreter)
_vtk_module_optional_dependency_exists(VTK::WrappingPythonCore
  SATISFIED_VAR _have_vtk_wrappingpythoncore)

if (_have_vtk_python AND _have_vtk_wrappingpythoncore AND _have_vtk_pythoninterpreter)
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

_vtk_module_optional_dependency_exists(VTK::IOFFMPEG
  SATISFIED_VAR _have_vtk_ioffmpeg)
if (_have_vtk_ioffmpeg)
  list(APPEND xml_files
    Resources/animation_ffmpeg.xml)
endif ()

_vtk_module_optional_dependency_exists(VTK::IOOggTheora
  SATISFIED_VAR _have_vtk_iooggtheora)
if (_have_vtk_iooggtheora)
  list(APPEND xml_files
    Resources/animation_oggtheora.xml)
endif ()

paraview_add_server_manager_xmls(
  XMLS  ${xml_files})
