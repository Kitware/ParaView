SUBDIRS(Tcl Cxx)

IF (VTK_WRAP_PYTHON)
  SUBDIRS(Python)
ENDIF (VTK_WRAP_PYTHON)

IF(PYTHON_EXECUTABLE AND VTK_HEADER_TESTING_PY)
  ADD_TEST(HeaderTesting-PVDevelopment ${PYTHON_EXECUTABLE}
           ${VTK_HEADER_TESTING_PY}
	   "${Development_SOURCE_DIR}"
           vtkApplyFilterCommandInternal.h           
           vtkHierarchicalDataSetInternal.h
           vtkHierarchicalBoxDataSetInternal.h
           vtkMultiBlockDataSetInternal.h
           vtkAMRBox.h
           )
ENDIF(PYTHON_EXECUTABLE AND VTK_HEADER_TESTING_PY)

