set(CXX_SOURCES
  vtkSQHemisphereSourceConfigurationWriter.cxx
  vtkSQHemisphereSourceConfigurationReader.cxx
  vtkSQPlaneSourceConfigurationWriter.cxx
  vtkSQPlaneSourceConfigurationReader.cxx
  vtkSQVolumeSourceConfigurationWriter.cxx
  vtkSQVolumeSourceConfigurationReader.cxx
  )

if(PARAVIEW_BUILD_QT_GUI)
  vtk_module_load(pqComponents)
  include_directories(${pqComponents_INCLUDE_DIRS})

  #message(STATUS "SQTK Including custom panels.")
  set(_IFACES)
  set(_IFACE_SRCS)

  macro(sciberquest_wrap_qt name)
    if (PARAVIEW_QT_VERSION VERSION_GREATER "4")
      qt5_wrap_cpp(MOC_SRCS "${name}.h")
    else ()
      qt4_wrap_cpp(MOC_SRCS "${name}.h")
    endif ()
    if (EXISTS "${CMAKE_CURRENT_LIST_DIR}/${name}Form.ui")
      if (PARAVIEW_QT_VERSION VERSION_GREATER "4")
        qt5_wrap_ui(UI_SRCS "${name}Form.ui")
      else ()
        qt4_wrap_ui(UI_SRCS "${name}Form.ui")
      endif ()
    endif ()
  endmacro()

  macro(sciberquest_wrap_panel group name)
    sciberquest_wrap_qt(${name})
    add_paraview_object_panel(
      IFACES IFACE_SRCS
      CLASS_NAME ${name}
      XML_NAME ${name} ${ARGN}
      XML_GROUP ${group})
    list(APPEND _IFACES ${IFACES})
    list(APPEND _IFACE_SRCS ${IFACE_SRCS})
  endmacro()

  # Qt custom panels
  sciberquest_wrap_panel(sources pqSQHemisphereSource)
  sciberquest_wrap_panel(sources pqSQPlaneSource)
  sciberquest_wrap_panel(sources pqSQVolumeSource)
  sciberquest_wrap_panel(filters pqSQFieldTracer
    vtkSQFieldTracerRK45
    vtkSQFieldTopologyMapper
    vtkSQFieldTopologyMapperRK45
    vtkSQPoincareMapper
    vtkSQPoincareMapper45
    vtkSQDisplacementMapper
    vtkSQDisplacementMapperRK45
    )
  sciberquest_wrap_panel(filters pqSQImageGhosts)
  sciberquest_wrap_panel(filters pqSQTensorGlyph)

  sciberquest_wrap_qt(pqSQTranslateDialog)

  list(APPEND CXX_SOURCES
    ${MOC_SRCS}
    ${UI_SRCS}
    ${_IFACE_SRCS}
    pqSQFieldTracer.cxx
    pqSQHemisphereSource.cxx
    pqSQImageGhosts.cxx
    pqSQPlaneSource.cxx
    pqSQTensorGlyph.cxx
    pqSQTranslateDialog.cxx
    pqSQVolumeSource.cxx
    )
endif (PARAVIEW_BUILD_QT_GUI)

include("${CMAKE_CURRENT_SOURCE_DIR}/GetDate.cmake")
# identify the build
set(SQTK_RELEASE "20120904")
set(SQTK_BUILD_DATE)
GetDate(SQTK_BUILD_DATE)
set (SQTK_VERSION "SQTK-${SQTK_RELEASE} ${CMAKE_SYSTEM} ${CMAKE_CXX_COMPILER} ${SQTK_BUILD_DATE}")
#message(STATUS "SQTK version ${SQTK_VERSION}")

set (SQTK_XML
  SciberQuestToolKitSources.xml
  SciberQuestToolKitFilters.xml)

set (SQTK_SM_XML
  SciberQuestToolKitSMReaders.xml
  SciberQuestToolKitSMWriters.xml
  SciberQuestToolKitSMSources.xml
  SciberQuestToolKitSMFilters.xml)

include_directories(${VTK_INCLUDE_DIRS} ${MPI_CXX_INCLUDE_PATH})

# Generate the plugin, servermanager and wrapped language bindings.
add_paraview_plugin(
  SciberQuestToolKit "${SQTK_VERSION}"
  GUI_INTERFACES ${_IFACES}
  SERVER_MANAGER_SOURCES ${SM_CXX_SOURCES}
  SERVER_MANAGER_XML ${SQTK_SM_XML}
  GUI_RESOURCE_FILES ${SQTK_XML}
  SOURCES ${CXX_SOURCES})

target_link_libraries(SciberQuestToolKit
  LINK_PUBLIC vtkFiltersFlowPaths vtkSciberQuest vtkPVServerManagerDefault
  LINK_PRIVATE vtksys)
if(PARAVIEW_BUILD_QT_GUI)
  target_link_libraries(SciberQuestToolKit
    LINK_PRIVATE pqComponents)
endif()

install(TARGETS SciberQuestToolKit
  ARCHIVE DESTINATION ${VTK_INSTALL_ARCHIVE_DIR}
  LIBRARY DESTINATION ${VTK_INSTALL_LIBRARY_DIR})

if(PARAVIEW_USE_MPI)
  include(vtkMPI)
  vtk_mpi_link(SciberQuestToolKit)
endif()
