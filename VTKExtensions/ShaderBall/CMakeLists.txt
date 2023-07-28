# SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
# SPDX-FileCopyrightText: Copyright (c) Sandia Corporation
# SPDX-License-Identifier: BSD-3-Clause

# HDRi created by Andreas Mischok, shared under CC-0, from: https://polyhaven.com/a/spiaggia_di_mondello
vtk_encode_string(
  INPUT         "ShaderBallSkybox.jpg"
  HEADER_OUTPUT skybox_header
  SOURCE_OUTPUT skybox_source
  BINARY)

vtk_encode_string(
  INPUT         "ShaderBallTexture.png"
  HEADER_OUTPUT shaderBallTexture_header
  SOURCE_OUTPUT shaderBallTexture_source
  BINARY)

vtk_module_add_module(ParaView::VTKExtensionsShaderBall
  NOWRAP_CLASSES vtkShaderBallScene
  CLASSES ${textures}
  SOURCES "${skybox_source}" "${shaderBallTexture_source}"
  PRIVATE_HEADERS "${skybox_header}" "${shaderBallTexture_header}")

if (NOT paraview_use_materialeditor)
  message(FATAL_ERROR "ShadeBall required the MaterialEditor")
endif()
