SUBDIRS(Tcl)

IF (VTK_WRAP_PYTHON)
  SUBDIRS(Python)
ENDIF (VTK_WRAP_PYTHON)

IF(PYTHON_EXECUTABLE)
  ADD_TEST(HeaderTesting-PVPackages ${PYTHON_EXECUTABLE}
           ${PARAVIEW_SOURCE_DIR}/KWCommon/Testing/HeaderTesting.py
	   "${PVPackages_SOURCE_DIR}"
           vtkAltIOHeader.h
           )
ENDIF(PYTHON_EXECUTABLE)

