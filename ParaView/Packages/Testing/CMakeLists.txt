SUBDIRS(Tcl)

IF (VTK_WRAP_PYTHON)
  SUBDIRS(Python)
ENDIF (VTK_WRAP_PYTHON)

IF(PYTHON_EXECUTABLE)
  ADD_TEST(HeaderTesting-PVPackages ${PYTHON_EXECUTABLE}
           ${VTK_SOURCE_DIR}/Common/Testing/HeaderTesting.py
	   "${PVPackages_SOURCE_DIR}"
           )
ENDIF(PYTHON_EXECUTABLE)

