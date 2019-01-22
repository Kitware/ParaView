#==========================================================================
#
#     Program: ParaView
#
#     Copyright (c) 2005-2008 Sandia Corporation, Kitware Inc.
#     All rights reserved.
#
#     ParaView is a free software; you can redistribute it and/or modify it
#     under the terms of the ParaView license version 1.2.
#
#     See License_v1.2.txt for the full ParaView license.
#     A copy of this license can be obtained by contacting
#     Kitware Inc.
#     28 Corporate Drive
#     Clifton Park, NY 12065
#     USA
#
#  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
#  ``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
#  LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
#  A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE AUTHORS OR
#  CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
#  EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
#  PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
#  PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
#  LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
#  NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
#  SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
#
#==========================================================================
set(classes
  vtkAnimationPlayer
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
  vtkSMAnimationSceneProxy
  vtkSMAnimationSceneWriter
  vtkSMSaveAnimationProxy
  vtkSequenceAnimationPlayer
  vtkTimestepsAnimationPlayer
  vtkXMLPVAnimationWriter)

if (TARGET VTK::PythonInterpreter)
  # TODO: Split these into a separate module so that there isn't a public
  # optional dependency.
  list(APPEND classes
    vtkPythonAnimationCue)
endif ()

vtk_module_add_module(ParaView::Animation
  CLASSES ${classes})

set(xml_files
  animation.xml)

if (WIN32)
  list(APPEND xml_files
    animation_win.xml)
endif ()

if (TARGET VTK::IOFFMPEG)
  list(APPEND xml_files
    animation_ffmpeg.xml)
endif ()

if (TARGET VTK::IOOggTheora)
  list(APPEND xml_files
    animation_oggtheora.xml)
endif ()

paraview_add_server_manager_xmls(
  XMLS  ${xml_files})
