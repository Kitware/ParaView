SUBDIRS(Tcl)

IF (VTK_WRAP_PYTHON)
  SUBDIRS(Python)
ENDIF (VTK_WRAP_PYTHON)

IF(PYTHON_EXECUTABLE)
  ADD_TEST(HeaderTesting-ParaView ${PYTHON_EXECUTABLE}
           ${PARAVIEW_SOURCE_DIR}/KWCommon/Testing/HeaderTesting.py
	   "${PARAVIEW_SOURCE_DIR}/ParaView"
           vtkPVDefaultModules.h
           ParaViewRC.h
           )
ENDIF(PYTHON_EXECUTABLE)
