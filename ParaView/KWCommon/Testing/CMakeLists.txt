SUBDIRS(Cxx)
SUBDIRS(Tcl)

IF (VTK_WRAP_PYTHON)
  SUBDIRS(Python)
ENDIF (VTK_WRAP_PYTHON)


IF(PYTHON_EXECUTABLE)
  ADD_TEST(HeaderTesting-KWCommon ${PYTHON_EXECUTABLE}
           ${VTK_SOURCE_DIR}/Common/Testing/HeaderTesting.py
	   "${KWCommon_SOURCE_DIR}"
           icons.h)
ENDIF(PYTHON_EXECUTABLE)

