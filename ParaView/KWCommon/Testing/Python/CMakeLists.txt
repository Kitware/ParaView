ADD_TEST(HeaderTesting-KWCommon ${PYTHON_EXECUTABLE}
         ${VTK_SOURCE_DIR}/Common/Testing/Python/HeaderTesting.py
	 "${KWCommon_SOURCE_DIR}"
         icons.h)
