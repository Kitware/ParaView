ADD_TEST(HeaderTesting-ParaView ${PYTHON_EXECUTABLE}
         ${VTK_SOURCE_DIR}/Common/Testing/Python/HeaderTesting.py
	 "${PARAVIEW_SOURCE_DIR}/ParaView"
         vtkPVDefaultModules.h
         )
