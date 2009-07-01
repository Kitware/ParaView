# +---------------------------------------------------------------------------+
# |                                                                           |
# |                           vtk VisIt Database                              |
# |                                                                           |
# +---------------------------------------------------------------------------+
# Database Target
link_directories(vtkVisItDatabase ${VISIT_LIB_PATH})
include_directories(vtkVisItDatabase ${VISIT_INCLUDE_PATH})
add_library(vtkVisItDatabase SHARED vtkVisItDatabase.cxx PrintUtils.cxx)
target_link_libraries(vtkVisItDatabase ${VISIT_LIBS})
target_link_libraries(vtkVisItDatabase vtkCommon vtkFiltering vtkGraphics vtkParallel)
# Platform
if (UNIX OR CYGWIN)
  message(STATUS "Configuring vtkVisItDatabase for use on Linux.")
  set_source_files_properties(vtkVisItDatabase COMPILE_FLAGS ${COMPILE_FLAGS} "-DUNIX")
  #set_target_properties(vtkVisItDatabase PROPERTIES LINK_FLAGS -Wl,--rpath,${VISIT_LIB_PATH})
  #set_target_properties(vtkVisItDatabase PROPERTIES LINK_FLAGS -Wl,--export-dynamic)
else (UNIX OR CYGWIN)
  message(STATUS "Configuring  vtkVisItDatabase for use on Windows.")
endif (UNIX OR CYGWIN)
# MPI
if (VISIT_WITH_MPI)
  message(STATUS "Configure vtkVisItDatabase for VisIt built with MPI.")
  set_source_files_properties(vtkVisItDatabase COMPILE_FLAGS ${COMPILE_FLAGS} "-DMPI")
endif (VISIT_WITH_MPI)

install(TARGETS vtkVisItDatabase
  RUNTIME DESTINATION ${PV_INSTALL_BIN_DIR} COMPONENT Runtime
  LIBRARY DESTINATION ${PV_INSTALL_LIB_DIR} COMPONENT Runtime
  ARCHIVE DESTINATION ${PV_INSTALL_LIB_DIR} COMPONENT Development)
