SUBDIRS(Tcl)
SUBDIRS(Cxx)

IF (VTK_WRAP_PYTHON)
  SUBDIRS(Python)
ENDIF (VTK_WRAP_PYTHON)

IF(PYTHON_EXECUTABLE AND VTK_HEADER_TESTING_PY)
  ADD_TEST(HeaderTesting-KWWidgets ${PYTHON_EXECUTABLE}
           ${VTK_HEADER_TESTING_PY}
	       "${KWWidgets_SOURCE_DIR}"
           vtkKWEvent.h
           vtkKWWidgets.h
           vtkKWRenderWidgetCallbackCommand.h
           )
ENDIF(PYTHON_EXECUTABLE AND VTK_HEADER_TESTING_PY)

