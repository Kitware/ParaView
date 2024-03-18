# SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
# SPDX-FileCopyrightText: Copyright (c) Sandia Corporation
# SPDX-License-Identifier: BSD-3-Clause
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

function (_paraview_remoting_application_optional_xml)
  cmake_parse_arguments(PARSE_ARGV 0 _paraview_optional_xml
    ""
    ""
    "TARGETS;FILES")

  foreach (_paraview_optional_xml_target IN LISTS _paraview_optional_xml_TARGETS)
    _vtk_module_optional_dependency_exists("${_paraview_optional_xml_target}"
      SATISFIED_VAR _have_dep_module)
    if (NOT _have_dep_module)
      return ()
    endif ()
  endforeach ()

  set(xml_files "${xml_files}" ${_paraview_optional_xml_FILES} PARENT_SCOPE)
endfunction ()

_paraview_remoting_application_optional_xml(
  TARGETS VTK::CommonComputationalGeometry
  FILES   Resources/proxies_computationalgeometry.xml)

_paraview_remoting_application_optional_xml(
  TARGETS VTK::DomainsChemistry
  FILES   Resources/filters_domainschemistry.xml)

_paraview_remoting_application_optional_xml(
  TARGETS VTK::IOChemistry
  FILES   Resources/readers_iochemistry.xml)

_paraview_remoting_application_optional_xml(
  TARGETS VTK::FiltersAMR
  FILES   Resources/filters_filtersamr.xml
          Resources/sources_filtersamr.xml)

_paraview_remoting_application_optional_xml(
  TARGETS VTK::FiltersCellGrid
  FILES   Resources/filters_filterscellgrid.xml)

_paraview_remoting_application_optional_xml(
  TARGETS VTK::FiltersCore
  FILES   Resources/filters_filterscore.xml
          Resources/sources_filterscore.xml)

_paraview_remoting_application_optional_xml(
  TARGETS VTK::FiltersPoints
  FILES   Resources/filters_filterspoints.xml)

_paraview_remoting_application_optional_xml(
  TARGETS VTK::FiltersExtraction
  FILES   Resources/filters_filtersextraction.xml)

_paraview_remoting_application_optional_xml(
  TARGETS VTK::FiltersFlowPaths
  FILES   Resources/filters_filtersflowpaths.xml)

_paraview_remoting_application_optional_xml(
  TARGETS VTK::FiltersGeneral
  FILES   Resources/filters_filtersgeneral.xml
          Resources/sources_filtersgeneral.xml)

_paraview_remoting_application_optional_xml(
  TARGETS VTK::FiltersGeneral
          VTK::FiltersGeometry
          ParaView::VTKExtensionsFiltersGeneral
  FILES   Resources/filters_plotonintersectioncurves.xml)

_paraview_remoting_application_optional_xml(
  TARGETS VTK::FiltersGeneral
          VTK::FiltersParallel
          ParaView::VTKExtensionsFiltersGeneral
  FILES   Resources/filters_normalglyphs.xml)

_paraview_remoting_application_optional_xml(
  TARGETS VTK::FiltersGeneric
  FILES   Resources/filters_filtersgeneric.xml)

_paraview_remoting_application_optional_xml(
  TARGETS VTK::FiltersGeometry
  FILES   Resources/filters_filtersgeometry.xml)

_paraview_remoting_application_optional_xml(
  TARGETS VTK::FiltersParallel
  FILES   Resources/filters_filtersparallel.xml)

_paraview_remoting_application_optional_xml(
  TARGETS VTK::FiltersParallelMPI
  FILES   Resources/filters_filtersparallelmpi.xml)

_paraview_remoting_application_optional_xml(
  TARGETS VTK::FiltersHybrid
  FILES   Resources/filters_filtershybrid.xml
          Resources/readers_filtershybrid.xml)

_paraview_remoting_application_optional_xml(
  TARGETS VTK::FiltersHyperTree
  FILES   Resources/filters_filtershypertree.xml)

_paraview_remoting_application_optional_xml(
  TARGETS VTK::FiltersModeling
  FILES   Resources/filters_filtersmodeling.xml)

_paraview_remoting_application_optional_xml(
  TARGETS VTK::FiltersSources
  FILES   Resources/sources_filterssources.xml)

_paraview_remoting_application_optional_xml(
  TARGETS VTK::FiltersStatistics
  FILES   Resources/filters_filtersstatistics.xml)

_paraview_remoting_application_optional_xml(
  TARGETS VTK::ImagingCore
  FILES   Resources/filters_imagingcore.xml
          Resources/sources_imagingcore.xml)

_paraview_remoting_application_optional_xml(
  TARGETS VTK::ImagingFourier
  FILES   Resources/filters_imagingfourier.xml)

_paraview_remoting_application_optional_xml(
  TARGETS VTK::ImagingGeneral
  FILES   Resources/filters_imaginggeneral.xml)

_paraview_remoting_application_optional_xml(
  TARGETS VTK::ImagingHybrid
  FILES   Resources/filters_imaginghybrid.xml)

_paraview_remoting_application_optional_xml(
  TARGETS VTK::ImagingSources
  FILES   Resources/sources_imagingsources.xml)

_paraview_remoting_application_optional_xml(
  TARGETS VTK::ImagingStatistics
  FILES   Resources/filters_imagingstatistics.xml)

_paraview_remoting_application_optional_xml(
  TARGETS VTK::FiltersParallelDIY2
  FILES   Resources/filters_filtersparalleldiy2.xml)

_paraview_remoting_application_optional_xml(
  TARGETS VTK::FiltersParallelVerdict
  FILES   Resources/filters_filtersparallelverdict.xml)

_paraview_remoting_application_optional_xml(
  TARGETS VTK::FiltersTensor
  FILES   Resources/filters_filterstensors.xml)

_paraview_remoting_application_optional_xml(
  TARGETS VTK::FiltersTexture
  FILES   Resources/filters_filterstexture.xml)

_paraview_remoting_application_optional_xml(
  TARGETS VTK::FiltersVerdict
  FILES   Resources/filters_filtersverdict.xml)

_paraview_remoting_application_optional_xml(
  TARGETS VTK::IOCellGrid
  FILES   Resources/readers_iocellgrid.xml)

_paraview_remoting_application_optional_xml(
  TARGETS VTK::IOXML
          VTK::IOParallelXML
  FILES   Resources/readers_ioxml.xml
          Resources/writers_ioxml.xml)

_paraview_remoting_application_optional_xml(
  TARGETS VTK::IOAlembic
  FILES   Resources/proxies_alembic.xml)

_paraview_remoting_application_optional_xml(
  TARGETS VTK::IONetCDF
  FILES   Resources/readers_ionetcdf.xml)

_paraview_remoting_application_optional_xml(
  TARGETS VTK::IOParallelNetCDF
  FILES   Resources/readers_ioparallelnetcdf.xml)

_paraview_remoting_application_optional_xml(
  TARGETS VTK::IOXdmf2
  FILES   Resources/proxies_xdmf2.xml)

_paraview_remoting_application_optional_xml(
  TARGETS VTK::IOXdmf3
  FILES   Resources/proxies_xdmf3.xml)

_paraview_remoting_application_optional_xml(
  TARGETS VTK::IOGDAL
  FILES   Resources/proxies_gdal.xml)

_paraview_remoting_application_optional_xml(
  TARGETS VTK::IOLAS
  FILES   Resources/proxies_las.xml)

_paraview_remoting_application_optional_xml(
  TARGETS VTK::IOPDAL
  FILES   Resources/proxies_pdal.xml)

_paraview_remoting_application_optional_xml(
  TARGETS VTK::IOCityGML
  FILES   Resources/proxies_citygml.xml)

_paraview_remoting_application_optional_xml(
  TARGETS VTK::IOHDF
  FILES   Resources/readers_iohdf.xml)

_paraview_remoting_application_optional_xml(
  TARGETS VTK::IOSegY
  FILES   Resources/proxies_segy.xml)

_paraview_remoting_application_optional_xml(
  TARGETS VTK::IOADIOS2
  FILES   Resources/proxies_adios2_core.xml
          Resources/proxies_adios2_vtx.xml)

_paraview_remoting_application_optional_xml(
  TARGETS VTK::IOFides
  FILES   Resources/proxies_fides.xml)

_paraview_remoting_application_optional_xml(
  TARGETS VTK::IOOMF
  FILES   Resources/proxies_omf.xml)

_paraview_remoting_application_optional_xml(
  TARGETS VTK::IOMotionFX
  FILES   Resources/proxies_motionfx.xml)

_paraview_remoting_application_optional_xml(
  TARGETS VTK::IOLegacy
  FILES   Resources/writers_iolegacy.xml)

_paraview_remoting_application_optional_xml(
  TARGETS VTK::IOGeometry
  FILES   Resources/readers_iogeometry.xml
          Resources/writers_iogeometry.xml)

_paraview_remoting_application_optional_xml(
  TARGETS VTK::IOOCCT
  FILES   Resources/readers_ioocct.xml)

_paraview_remoting_application_optional_xml(
  TARGETS VTK::IOPLY
  FILES   Resources/readers_ioply.xml)

_paraview_remoting_application_optional_xml(
  TARGETS VTK::IOParallel
  FILES   Resources/readers_ioparallel.xml
          Resources/writers_ensight.xml)

_paraview_remoting_application_optional_xml(
  TARGETS VTK::IOParallelLSDyna
  FILES   Resources/readers_lsdyna.xml)

_paraview_remoting_application_optional_xml(
  TARGETS VTK::IOInfovis
  FILES   Resources/readers_ioinfovis.xml)

_paraview_remoting_application_optional_xml(
  TARGETS VTK::IOTecplotTable
  FILES   Resources/readers_iotecplottable.xml)

_paraview_remoting_application_optional_xml(
  TARGETS VTK::IOVPIC
  FILES   Resources/readers_iovpic.xml)

_paraview_remoting_application_optional_xml(
  TARGETS VTK::IOTRUCHAS
  FILES   Resources/readers_iotruchas.xml)

_paraview_remoting_application_optional_xml(
  TARGETS VTK::IOVeraOut
  FILES   Resources/readers_ioveraout.xml)

_paraview_remoting_application_optional_xml(
  TARGETS VTK::IOH5part
  FILES   Resources/readers_ioh5part.xml)

_paraview_remoting_application_optional_xml(
  TARGETS VTK::IOH5Rage
  FILES   Resources/readers_ioh5rage.xml)

_paraview_remoting_application_optional_xml(
  TARGETS VTK::IOPIO
  FILES   Resources/readers_iopio.xml)

_paraview_remoting_application_optional_xml(
  TARGETS VTK::IOCONVERGECFD
  FILES   Resources/readers_ioconvergecfd.xml)

_paraview_remoting_application_optional_xml(
  TARGETS VTK::IOCGNSReader
  FILES   Resources/readers_iocgns.xml)

_paraview_remoting_application_optional_xml(
  TARGETS VTK::IOFLUENTCFF
  FILES   Resources/readers_iofluentcff.xml)

_paraview_remoting_application_optional_xml(
  TARGETS ParaView::VTKExtensionsIOCGNSWriter
  FILES   Resources/writers_iocgns.xml)

_paraview_remoting_application_optional_xml(
  TARGETS VTK::FiltersOpenTURNS
  FILES   Resources/proxies_openturns.xml)

_paraview_remoting_application_optional_xml(
  TARGETS VTK::MomentInvariants
  FILES   Resources/MomentInvariants.xml)

_paraview_remoting_application_optional_xml(
  TARGETS VTK::ParallelMomentInvariants
  FILES   Resources/ParallelMomentInvariants.xml)

_paraview_remoting_application_optional_xml(
  TARGETS VTK::RenderingFreeType
  FILES   Resources/sources_renderingfreetype.xml)

_paraview_remoting_application_optional_xml(
  TARGETS VTK::InteractionWidgets
  FILES   Resources/proxies_interactionwidgets.xml)

_paraview_remoting_application_optional_xml(
  TARGETS VTK::FiltersParallel
          VTK::ImagingFourier
          ParaView::VTKExtensionsExtraction
  FILES   Resources/filters_selectionfftovertime.xml)

_paraview_remoting_application_optional_xml(
  TARGETS VTK::IOIOSS
  FILES   Resources/readers_ioioss.xml
          Resources/writers_ioioss.xml)

_paraview_remoting_application_optional_xml(
  TARGETS VTK::IOOpenVDB
  FILES   Resources/readers_ioopenvdb.xml
          Resources/writers_ioopenvdb.xml)

_paraview_remoting_application_optional_xml(
  TARGETS VTK::IOCatalystConduit
  FILES   Resources/proxies_conduit.xml)

_paraview_remoting_application_optional_xml(
  TARGETS VTK::IOFDS
  FILES   Resources/readers_iofds.xml)

_vtk_module_optional_dependency_exists(VTK::PythonInterpreter
  SATISFIED_VAR _have_vtk_pythoninterpreter)
if (TARGET _have_vtk_pythoninterpreter)
  _vtk_module_set_module_property(ParaView::RemotingApplication APPEND
    PROPERTY  "forward_link"
    VALUE     VTK::PythonUsed)
endif ()

if (PARAVIEW_USE_PYTHON)
  list(APPEND xml_files
    Resources/readers_openpmd.xml
    Resources/readers_savg.xml)
  list(APPEND xml_files
    Resources/extractors_python.xml)
endif()

paraview_add_server_manager_xmls(
  XMLS  ${xml_files})
