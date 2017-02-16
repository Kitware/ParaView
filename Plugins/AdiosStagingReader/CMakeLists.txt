#------------------------------------------------------------------------------
# Find and Use ParaView
#------------------------------------------------------------------------------
if (ParaView_SOURCE_DIR)
  include_directories(
    ${PARAVIEW_INCLUDE_DIRS}
    ${PARAVIEW_GUI_INCLUDE_DIRS}
    ${PARAVIEW_KWSYS_INCLUDE_DIRS}
    ${VTK_INCLUDE_DIRS}
  )
else()
  find_package(ParaView REQUIRED)
  include(${PARAVIEW_USE_FILE})
endif()


find_package(MPI)
find_package(ADIOS REQUIRED)

include_directories(
  SYSTEM ${ADIOS_INCLUDE_DIRS}
  SYSTEM ${MPI_INCLUDE_PATH}
)

add_paraview_plugin(AdiosStagingReader "1.4"
  SERVER_MANAGER_XML
    AdiosStagingReader.xml
  SERVER_MANAGER_SOURCES
    vtkCartisoReader.cxx
)

target_link_libraries(AdiosStagingReader
  LINK_PRIVATE ${ADIOS_LIBRARIES} ${MPI_LIBRARIES}
)
