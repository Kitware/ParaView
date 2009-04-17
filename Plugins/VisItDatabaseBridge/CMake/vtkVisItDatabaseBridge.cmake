# +---------------------------------------------------------------------------+
# |                                                                           |
# |                          vtk VisIt Database Bridge                        |
# |                                                                           |
# +---------------------------------------------------------------------------+

# Pipeline Bridge Target 
link_directories(vtkVisItDatabaseBridge ${VISIT_LIB_PATH})
include_directories(vtkVisItDatabaseBridge ${VISIT_INCLUDE_PATH})
SET(PLUGIN_SRCS vtkVisItDatabaseBridge.cxx)
ADD_PARAVIEW_PLUGIN(
  vtkVisItDatabaseBridge "1.0"
  SERVER_MANAGER_SOURCES vtkVisItDatabaseBridge.cxx
  SERVER_MANAGER_XML vtkVisItDatabaseBridgeServerManager.xml
  GUI_RESOURCE_FILES vtkVisItDatabaseBridgeClient.xml)
#add_dependencies(vtkVisItDatabaseBridge xml)
if (UNIX OR CYGWIN)
  message(STATUS "Configuring vtkVisItDatabaseBridge for use on Linux.")
  #set_target_properties(vtkVisItDatabaseBridge PROPERTIES LINK_FLAGS -Wl,--rpath,${VISIT_LIB_PATH})
  #set_target_properties(vtkVisItDatabaseBridge PROPERTIES LINK_FLAGS -Wl,--export-dynamic)
else (UNIX OR CYGWIN)
  message(STATUS "Configuring  vtkVisItDatabaseBridge for use on Windows.")
endif (UNIX OR CYGWIN)
target_link_libraries(vtkVisItDatabaseBridge vtkVisItDatabase)
target_link_libraries(vtkVisItDatabaseBridge ${VISIT_LIBS})

install(TARGETS vtkVisItDatabaseBridge DESTINATION ${CMAKE_INSTALL_PREFIX})
