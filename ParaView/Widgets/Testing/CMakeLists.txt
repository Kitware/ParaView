SUBDIRS(Tcl)
SUBDIRS(Cxx)

IF (VTK_WRAP_PYTHON)
  SUBDIRS(Python)
ENDIF (VTK_WRAP_PYTHON)



IF(PYTHON_EXECUTABLE AND VTK_HEADER_TESTING_PY)
  ADD_TEST(HeaderTesting-Widgets ${PYTHON_EXECUTABLE}
           ${VTK_HEADER_TESTING_PY}
	   "${Widgets_SOURCE_DIR}"
           KitwareLogo.h
           bitmaps.h
           icons.h
           kwinit.h
           kwappicon.h
           vtkKWEvent.h
           vtkKWWidgets.h
           vtkbwidgets.h
           vtkKWRenderWidgetCallbackCommand.h
           )
ENDIF(PYTHON_EXECUTABLE AND VTK_HEADER_TESTING_PY)

