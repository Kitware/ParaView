SUBDIRS(Cxx)
SUBDIRS(Tcl)

IF (VTK_WRAP_PYTHON)
  SUBDIRS(Python)
ENDIF (VTK_WRAP_PYTHON)

IF(PYTHON_EXECUTABLE AND VTK_HEADER_TESTING_PY)
  ADD_TEST(HeaderTesting-ParaView ${PYTHON_EXECUTABLE}
           ${VTK_HEADER_TESTING_PY}
	   "${PARAVIEW_SOURCE_DIR}/ParaView"
           vtkPVDefaultModules.h
           ParaViewRC.h
           )
ENDIF(PYTHON_EXECUTABLE AND VTK_HEADER_TESTING_PY)

# Test the "make install" procedure on ParaView.
IF(UNIX)
  IF("x${CMAKE_INSTALL_PREFIX}" MATCHES "^x${PARAVIEW_BINARY_DIR}/ParaView/Testing/TestInstall")
    CONFIGURE_FILE(
      ${PARAVIEW_SOURCE_DIR}/ParaView/Testing/TestInstall.sh.in
      ${PARAVIEW_BINARY_DIR}/ParaView/Testing/TestInstall.sh
      @ONLY IMMEDIATE
      )
    ADD_TEST(Install /bin/sh ${PARAVIEW_BINARY_DIR}/ParaView/Testing/TestInstall.sh)
  ENDIF("x${CMAKE_INSTALL_PREFIX}" MATCHES "^x${PARAVIEW_BINARY_DIR}/ParaView/Testing/TestInstall")
ENDIF(UNIX)
