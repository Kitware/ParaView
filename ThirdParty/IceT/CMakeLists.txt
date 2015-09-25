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
#########################################################################
# Set the ICET MPI variables from the VTK ones.
# use a set cache internal so people don't try and use them
set(ICET_MPIRUN_EXE "${VTK_MPIRUN_EXE}" CACHE INTERNAL
  "This is set from VTK_MPIRUN_EXE.")
set(ICET_MPI_NUMPROC_FLAG
  "${VTK_MPI_PRENUMPROC_FLAGS};${VTK_MPI_NUMPROC_FLAG}"
  CACHE INTERNAL
  "This is set from a combination of VTK_MPI_PRENUMPROC_FLAGS and VTK_MPI_NUMPROC_FLAG"
)
set(ICET_MPI_PREFLAGS "${VTK_MPI_PREFLAGS}" CACHE INTERNAL
  "This is set from VTK_MPI_PREFLAGS.")
set(ICET_MPI_POSTFLAGS "${VTK_MPI_POSTFLAGS}"  CACHE INTERNAL
  "This is set from VTK_MPI_POSTFLAGS.")
set(ICET_MPI_MAX_NUMPROCS "${VTK_MPI_MAX_NUMPROCS}"  CACHE INTERNAL
  "This is set from VTK_MPI_MAX_NUMPROCS.")

# Set OpenGL variables
include(vtkOpenGL)
set(ICET_USE_OSMESA "${VTK_USE_OSMESA}" CACHE INTERNAL
  "Use Off-Screen Mesa")
set(ICET_USE_OFFSCREEN_EGL "${VTK_USE_OFFSCREEN_EGL}" CACHE INTERNAL
  "Use Off-Screen rendering through EGL")

#########################################################################
# Configure IceT
set(ICET_INSTALL_BIN_DIR ${VTK_INSTALL_RUNTIME_DIR})
set(ICET_INSTALL_LIB_DIR ${VTK_INSTALL_LIBRARY_DIR})
set(ICET_INSTALL_INCLUDE_DIR ${VTK_INSTALL_INCLUDE_DIR})
set(ICET_INSTALL_NO_DEVELOPMENT ${VTK_INSTALL_NO_DEVELOPMENT})
set(ICET_INSTALL_NO_LIBRARIES ${VTK_INSTALL_NO_LIBRARIES})
set(ICET_INSTALL_EXPORT_NAME ${VTK_INSTALL_EXPORT_NAME})

vtk_module_third_party(icet
  INCLUDE_DIRS
    ${CMAKE_CURRENT_SOURCE_DIR}/vtkicet/src/include
    ${CMAKE_CURRENT_BINARY_DIR}/vtkicet/src/include
  LIBRARIES IceTCore IceTMPI IceTGL
)

# icet exports its build dir targets to a custom IceTTargets.cmake file. We
# don't care much about it. We just ensure that we export the targets we are
# interested in here again.
vtk_target_export(IceTCore)
vtk_target_export(IceTMPI)
vtk_target_export(IceTGL)

# keep IceT variables from polluting the space.
mark_as_advanced(
  ICET_MAGIC_K ICET_MAX_IMAGE_SPLIT ICET_USE_MPI ICET_USE_OPENGL
  GLUT_cocoa_LIBRARY)
