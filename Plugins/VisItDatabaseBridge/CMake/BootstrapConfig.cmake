# +---------------------------------------------------------------------------+
# |                                                                           |
# |                            Bootstrap Config                               |
# |                                                                           |
# +---------------------------------------------------------------------------+

configure_file(
  "${PROJECT_SOURCE_DIR}/BootstrapConfigure.h.in"
  "${PROJECT_BINARY_DIR}/BootstrapConfigure.h"
  @ONLY)

set(BOOTSTRAP_CONFIGURE_EXLUDES "${PROJECT_SOURCE_DIR}/PVDatabaseExcludes.txt")
link_directories(${VISIT_LIB_PATH})
include_directories(${VISIT_INCLUDE_PATH} ${PROJECT_BINARY_DIR})
add_executable(BootstrapConfigure BootstrapConfigure.cpp)
target_link_libraries(BootstrapConfigure ${VISIT_LIBS} vtkCommon vtkFiltering vtkIO vtkGraphics vtkRendering)

if (UNIX OR CYGWIN)
  set_target_properties(BootstrapConfigure PROPERTIES LINK_FLAGS -Wl,--rpath,${VISIT_LOCAL})
endif (UNIX OR CYGWIN)

set(_configOut
  "${PROJECT_BINARY_DIR}/vtkVisItDatabaseBridgeServerManager.xml"
  "${PROJECT_BINARY_DIR}/vtkVisItDatabaseBridgeClient.xml"
  "${PROJECT_BINARY_DIR}/CMake/pqVisItDatabaseBridgePanel.cmake")

set(_configIn
  "${PROJECT_SOURCE_DIR}/vtkVisItDatabaseBridgeServerManager.xml.in"
  "${PROJECT_SOURCE_DIR}/vtkVisItDatabaseBridgeClient.xml.in"
  "${PROJECT_SOURCE_DIR}/CMake/pqVisItDatabaseBridgePanel.cmake.in")

set(VISIT_RUN_TIME 
  ${VISIT_LOCAL} 
  CACHE FILEPATH
  "Path that is used by the PluginManager to load database plugins at runtime.")

add_custom_command(
  OUTPUT ${_configOut}
  DEPENDS BootstrapConfigure ${VISIT_LIBS} vtkCommon vtkFiltering vtkIO vtkGraphics vtkRendering
  COMMAND BootstrapConfigure
  ARGS
    ${VISIT_LOCAL}
    ${VISIT_RUN_TIME}
    ${_configIn}
    ${_configOut}
    ${BOOTSTRAP_CONFIGURE_EXLUDES}
  COMMENT "Configuring vtkVisItDatabaseBridge build.")

set_property(
  DIRECTORY ${PROJECT_SOURCE_DIR}
  PROPERTY ADDITIONAL_MAKE_CLEAN_FILES ${_configOut})

add_custom_target(configure ALL DEPENDS ${_configOut})
