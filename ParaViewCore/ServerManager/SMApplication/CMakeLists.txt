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
  vtkInitializationHelper)

set(private_headers
  vtkPVInitializer.h)

vtk_module_add_module(ParaView::ServerManagerApplication
  CLASSES ${classes}
  PRIVATE_HEADERS ${private_headers})
vtk_module_link(ParaView::ServerManagerApplication
  PRIVATE
    paraview_client_server
    paraview_server_manager)
vtk_module_client_server_exclude()

set(xml_files
  Resources/3d_widgets.xml
  Resources/filters.xml
  Resources/internal_writers.xml
  Resources/readers.xml
  Resources/rendering.xml
  Resources/sources.xml
  Resources/utilities.xml
  Resources/views_and_representations.xml
  Resources/writers.xml)

if (TARGET VTK::mpi)
  list(APPEND xml_files
    Resources/proxies_mpi.xml)
endif ()

if (TARGET VTK::IOXdmf2)
  list(APPEND xml_files
    Resources/proxies_xdmf2.xml)
endif ()

if (TARGET VTK::IOXdmf3)
  list(APPEND xml_files
    Resources/proxies_xdmf3.xml)
endif ()

if (TARGET VTK::IOGDAL)
  list(APPEND xml_files
    Resources/proxies_gdal.xml)
endif ()

if (TARGET VTK::IOLAS)
  list(APPEND xml_files
    Resources/proxies_las.xml)
endif ()

if (TARGET VTK::IOPDAL)
  list(APPEND xml_files
    Resources/proxies_pdal.xml)
endif ()

if (TARGET VTK::IOCityGML)
  list(APPEND xml_files
    Resources/proxies_citygml.xml)
endif ()

if (TARGET VTK::IOSegY)
  list(APPEND xml_files
    Resources/proxies_segy.xml)
endif ()

if (TARGET VTK::IOADIOS2)
  list(APPEND xml_files
    Resources/proxies_adios2_core.xml)
  if(PARAVIEW_USE_MPI)
    list(APPEND xml_files
      Resources/proxies_adios2_vtx.xml)
  endif()
endif ()

if (TARGET VTK::IOMotionFX)
  list(APPEND xml_files
    Resources/proxies_motionfx.xml)
endif ()

if (TARGET VTK::FiltersOpenTURNS)
  list(APPEND xml_files
    Resources/proxies_openturns.xml)
endif ()

if (TARGET VTK::MomentInvariants)
  list(APPEND xml_files
    Resources/MomentInvariants.xml)
endif ()

if (TARGET VTK::ParallelMomentInvariants)
  list(APPEND xml_files
    Resources/ParallelMomentInvariants.xml)
endif ()


if (TARGET VTK::PythonInterpreter)
  _vtk_module_set_module_property(ParaView::ServerManagerApplication APPEND
    PROPERTY  "forward_link"
    VALUE     VTK::PythonUsed)
  list(APPEND xml_files
    Resources/pythonfilter.xml
    Resources/pythonviews.xml)
endif ()

paraview_add_server_manager_xmls(
  XMLS  ${xml_files})
