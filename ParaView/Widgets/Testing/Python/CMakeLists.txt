ADD_TEST(HeaderTesting-Widgets ${PYTHON_EXECUTABLE}
         ${VTK_SOURCE_DIR}/Common/Testing/Python/HeaderTesting.py
	 "${Widgets_SOURCE_DIR}"
         vtkbwidgets.h)
