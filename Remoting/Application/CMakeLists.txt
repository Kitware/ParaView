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

vtk_module_add_module(ParaView::RemotingApplication
  CLASSES ${classes}
  PRIVATE_HEADERS ${private_headers})
vtk_module_link(ParaView::RemotingApplication
  PRIVATE
    paraview_client_server
    paraview_server_manager)
vtk_module_client_server_exclude()

set(xml_files
  Resources/extractors.xml
  Resources/internal_writers.xml
  Resources/utilities.xml)

if (TARGET VTK::CommonComputationalGeometry)
  list(APPEND xml_files
    Resources/proxies_computationalgeometry.xml)
endif ()

if (TARGET VTK::DomainsChemistry)
  list(APPEND xml_files
    Resources/filters_domainschemistry.xml
    Resources/readers_domainschemistry.xml)
endif ()

if (TARGET VTK::FiltersAMR)
  list(APPEND xml_files
    Resources/filters_filtersamr.xml
    Resources/sources_filtersamr.xml)
endif ()

if (TARGET VTK::FiltersCore)
  list(APPEND xml_files
    Resources/filters_filterscore.xml
    Resources/sources_filterscore.xml)
endif ()

if (TARGET VTK::FiltersExtraction)
  list(APPEND xml_files
    Resources/filters_filtersextraction.xml)
endif ()

if (TARGET VTK::FiltersFlowPaths)
  list(APPEND xml_files
    Resources/filters_filtersflowpaths.xml)
endif ()

if (TARGET VTK::FiltersGeneral)
  list(APPEND xml_files
    Resources/filters_filtersgeneral.xml
    Resources/sources_filtersgeneral.xml)
endif ()

if (TARGET VTK::FiltersGeneral AND
    TARGET VTK::FiltersGeometry AND
    TARGET ParaView::VTKExtensionsFiltersGeneral)
  list(APPEND xml_files
    Resources/filters_plotonintersectioncurves.xml)
endif ()

if (TARGET ParaView::VTKExtensionsFiltersGeneral AND
    TARGET VTK::FiltersGeneral AND
    TARGET VTK::FiltersParallel)
  list(APPEND xml_files
    Resources/filters_normalglyphs.xml)
endif()

if (TARGET VTK::FiltersGeneric)
  list(APPEND xml_files
    Resources/filters_filtersgeneric.xml)
endif ()

if (TARGET VTK::FiltersGeometry)
  list(APPEND xml_files
    Resources/filters_filtersgeometry.xml)
endif ()

if (TARGET VTK::FiltersParallel)
  list(APPEND xml_files
    Resources/filters_filtersparallel.xml)
endif ()

if (TARGET VTK::FiltersParallelMPI)
  list(APPEND xml_files
    Resources/filters_filtersparallelmpi.xml)
endif ()

if (TARGET VTK::FiltersHybrid)
  list(APPEND xml_files
    Resources/filters_filtershybrid.xml
    Resources/readers_filtershybrid.xml)
endif ()

if (TARGET VTK::FiltersHyperTree)
  list(APPEND xml_files
    Resources/filters_filtershypertree.xml)
endif ()

if (TARGET VTK::FiltersModeling)
  list(APPEND xml_files
    Resources/filters_filtersmodeling.xml)
endif ()

if (TARGET VTK::FiltersSources)
  list(APPEND xml_files
    Resources/sources_filterssources.xml)
endif ()

if (TARGET VTK::FiltersStatistics)
  list(APPEND xml_files
    Resources/filters_filtersstatistics.xml)
endif ()

if (TARGET VTK::ImagingCore)
  list(APPEND xml_files
    Resources/filters_imagingcore.xml
    Resources/sources_imagingcore.xml)
endif ()

if (TARGET VTK::ImagingFourier)
  list(APPEND xml_files
    Resources/filters_imagingfourier.xml)
endif ()

if (TARGET VTK::ImagingGeneral)
  list(APPEND xml_files
    Resources/filters_imaginggeneral.xml)
endif ()

if (TARGET VTK::ImagingHybrid)
  list(APPEND xml_files
    Resources/filters_imaginghybrid.xml)
endif ()

if (TARGET VTK::ImagingSources)
  list(APPEND xml_files
    Resources/sources_imagingsources.xml)
endif ()

if (TARGET VTK::FiltersParallelDIY2)
  list(APPEND xml_files
    Resources/filters_filtersparalleldiy2.xml)
endif ()

if (TARGET VTK::FiltersParallelVerdict)
  list(APPEND xml_files
    Resources/filters_filtersparallelverdict.xml)
endif ()

if (TARGET VTK::FiltersTexture)
  list(APPEND xml_files
    Resources/filters_filterstexture.xml)
endif ()

if (TARGET VTK::FiltersVerdict)
  list(APPEND xml_files
    Resources/filters_filtersverdict.xml)
endif ()

if (TARGET VTK::IOXML AND TARGET VTK::IOParallelXML)
  list(APPEND xml_files
    Resources/readers_ioxml.xml
    Resources/writers_ioxml.xml)
endif ()

if (TARGET VTK::IONetCDF)
  list(APPEND xml_files
    Resources/readers_ionetcdf.xml)
endif ()

if (TARGET VTK::IOParallelNetCDF)
  list(APPEND xml_files
    Resources/readers_ioparallelnetcdf.xml)
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

if (TARGET VTK::IOFides)
  list(APPEND xml_files
    Resources/proxies_fides.xml)
endif()

if (TARGET VTK::IOMotionFX)
  list(APPEND xml_files
    Resources/proxies_motionfx.xml)
endif ()

if (TARGET VTK::IOLegacy)
  list(APPEND xml_files
    Resources/writers_iolegacy.xml)
endif()

if (TARGET VTK::IOGeometry)
  list(APPEND xml_files
    Resources/readers_iogeometry.xml
    Resources/writers_iogeometry.xml)
endif()
if (TARGET VTK::IOPLY)
  list(APPEND xml_files
    Resources/readers_ioply.xml)
endif()

if (TARGET VTK::IOParallel)
  list(APPEND xml_files
    Resources/readers_ioparallel.xml
    Resources/writers_ensight.xml)
endif()

if (TARGET VTK::IOParallelLSDyna)
  list(APPEND xml_files
    Resources/readers_lsdyna.xml)
endif()

if (TARGET VTK::IOInfovis)
  list(APPEND xml_files
    Resources/readers_ioinfovis.xml)
endif ()

if (TARGET VTK::IOTecplotTable)
  list(APPEND xml_files
    Resources/readers_iotecplottable.xml)
endif()

if (TARGET VTK::IOVPIC)
  list(APPEND xml_files
    Resources/readers_iovpic.xml)
endif()

if (TARGET VTK::IOTRUCHAS)
  list(APPEND xml_files
    Resources/readers_iotruchas.xml)
endif()

if (TARGET VTK::IOVeraOut)
  list(APPEND xml_files
    Resources/readers_ioveraout.xml)
endif()

if (TARGET VTK::IOH5part)
  list(APPEND xml_files
    Resources/readers_ioh5part.xml)
endif()

if (TARGET VTK::IOPIO)
  list(APPEND xml_files
    Resources/readers_iopio.xml)
endif()

if (TARGET VTK::IOCONVERGECFD)
  list(APPEND xml_files
    Resources/readers_ioconvergecfd.xml)
endif()

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
  _vtk_module_set_module_property(ParaView::RemotingApplication APPEND
    PROPERTY  "forward_link"
    VALUE     VTK::PythonUsed)
endif ()

if (TARGET VTK::RenderingFreeType)
  list(APPEND xml_files
    Resources/sources_renderingfreetype.xml)
endif ()

if (TARGET VTK::InteractionWidgets)
  list(APPEND xml_files
    Resources/proxies_interactionwidgets.xml)
endif()

if (TARGET VTK::FiltersParallel AND
    TARGET VTK::ImagingFourier AND
    TARGET ParaView::VTKExtensionsExtraction)
  list(APPEND xml_files
    Resources/filters_selectionfftovertime.xml)
endif()

if (TARGET VTK::IOIoss AND FALSE) # disabling for now
  list(APPEND xml_files
    Resources/readers_ioioss.xml)
endif()

if (PARAVIEW_USE_PYTHON)
  list(APPEND xml_files
    Resources/readers_openpmd.xml)
endif()

paraview_add_server_manager_xmls(
  XMLS  ${xml_files})
