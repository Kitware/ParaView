SUBDIRS(Tcl)
SUBDIRS(Cxx)

IF (VTK_WRAP_PYTHON)
  SUBDIRS(Python)
ENDIF (VTK_WRAP_PYTHON)



IF(PYTHON_EXECUTABLE)
  ADD_TEST(HeaderTesting-Widgets ${PYTHON_EXECUTABLE}
           ${VTK_SOURCE_DIR}/Common/Testing/HeaderTesting.py
	   "${Widgets_SOURCE_DIR}"
           KitwareLogo.h
           bitmaps.h
           icons.h
           kwinit.h
           kwappicon.h
           vtkKWEvent.h
           vtkKWWidgets.h
           vtkbwidgets.h
           )
ENDIF(PYTHON_EXECUTABLE)

