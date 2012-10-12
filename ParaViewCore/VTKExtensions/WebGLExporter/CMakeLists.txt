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



# This plugin bring the ParaViewWeb WebGL exporter to regular ParaView
# as a plugin.
# That plugin will behave as any other ParaView exporter (VRML, X3D, POV...)
# but will generate several types of files. The main one is the scene graph
# description define as a JSON object with all the corresponding binary+base64
# pieces that come along with it. But also with it come a single standalone HTML
# file that can directly be used to see the data in a browser without any plugin.
#
# This code base should be cleaned up to follow VTK standard and even be
# integrated into VTK itself. But for now it is provided as is.


set(Module_SRCS
  vtkPVWebGLExporter.cxx
  vtkWebGLDataSet.cxx
  vtkWebGLExporter.cxx
  vtkWebGLObject.cxx
  vtkWebGLPolyData.cxx
  vtkWebGLWidget.cxx
  md5.cxx
)

set_source_files_properties(md5.cxx WRAP_EXCLUDE)

set(JAVASCRIPT_SRCS
  webglRenderer
  glMatrix
)


foreach(file ${JAVASCRIPT_SRCS})
  set(src ${CMAKE_CURRENT_SOURCE_DIR}/${file}.js)
  set(res ${CMAKE_CURRENT_BINARY_DIR}/${file}.h)
  add_custom_command(
    OUTPUT ${res}
    DEPENDS ${src}
    COMMAND vtkEncodeString
    ARGS ${res} ${src} ${file}
    )
  list(APPEND Module_SRCS ${res})
  set_source_files_properties(${res} WRAP_EXCLUDE)
endforeach()

vtk_module_library(vtkPVVTKExtensionsWebGLExporter ${Module_SRCS})
