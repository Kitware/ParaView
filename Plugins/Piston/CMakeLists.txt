if (ParaView_SOURCE_DIR)
  include_directories(
    ${VTK_INCLUDE_DIRS}
  )
else ()
  find_package(ParaView REQUIRED)
  include(${PARAVIEW_USE_FILE})
endif()

add_paraview_plugin(
  PistonPlugin "1.0"
  REQUIRED_ON_SERVER
  REQUIRED_ON_CLIENT
  DOCUMENTATION_DIR "${CMAKE_CURRENT_SOURCE_DIR}/doc"
  SERVER_MANAGER_SOURCES
    vtkPistonRepresentation.cxx
    vtkPistonInformationHelper.cxx
    vtkPVDataSetToPiston.cxx
  SERVER_MANAGER_XML
    PistonSM.xml
)
