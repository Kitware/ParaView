SUBDIRS(Tcl)

IF (VTK_WRAP_PYTHON)
  SUBDIRS(Python)
ENDIF (VTK_WRAP_PYTHON)

IF(PYTHON_EXECUTABLE)
  ADD_TEST(HeaderTesting-ParaView ${PYTHON_EXECUTABLE}
           ${VTK_SOURCE_DIR}/Common/Testing/HeaderTesting.py
	   "${PARAVIEW_SOURCE_DIR}/ParaView"
           vtkPVDefaultModules.h
           )
ENDIF(PYTHON_EXECUTABLE)
