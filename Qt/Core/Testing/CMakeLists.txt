
SET(BASELINE_DIR ${PARAVIEW_DATA_ROOT}/Baseline)

MACRO(GET_TEST_BINARY in out)
  IF(APPLE)
    SET(${out} "${EXECUTABLE_OUTPUT_PATH}/${in}.app/Contents/MacOS/${in}")
  ELSE(APPLE)
    SET(${out} "${EXECUTABLE_OUTPUT_PATH}/${in}")
  ENDIF(APPLE)
ENDMACRO(GET_TEST_BINARY)


ADD_EXECUTABLE(pqCoreBasicApp MACOSX_BUNDLE BasicApp.cxx)
TARGET_LINK_LIBRARIES(pqCoreBasicApp pqCore QtTesting ${QT_LIBRARIES} )
GET_TEST_BINARY(pqCoreBasicApp TEST_BINARY)
ADD_TEST(pqCoreBasicApp "${TEST_BINARY}" -dr "--test-directory=${PARAVIEW_TEST_DIR}"
         "--compare-view=${BASELINE_DIR}/pqCoreBasicApp.png" --exit)


IF(QT_TESTING_WITH_PYTHON)

  INCLUDE_DIRECTORIES(${QtPython_SOURCE_DIR})
  # Reduce the number of dirs that get included on moc command line
  # since it causes issues on Windows 2000.
  GET_DIRECTORY_PROPERTY(include_dirs_tmp INCLUDE_DIRECTORIES)
  SET_DIRECTORY_PROPERTIES(PROPERTIES INCLUDE_DIRECTORIES "${MOC_INCLUDE_DIRS}")
  QT4_WRAP_CPP(MOC_SRCS pqFileDialogTest.h)
  SET_DIRECTORY_PROPERTIES(PROPERTIES INCLUDE_DIRECTORIES "${include_dirs_tmp}")
  ADD_EXECUTABLE(pqFileDialogTest MACOSX_BUNDLE 
                 pqFileDialogTest.cxx pqFileDialogTest.h 
                 pqFileDialogTest.py
                 ${MOC_SRCS})
  TARGET_LINK_LIBRARIES(pqFileDialogTest pqCore QtPython QtTesting ${QT_LIBRARIES} )
  GET_TEST_BINARY(pqFileDialogTest TEST_BINARY)
  ADD_TEST(pqFileDialogTest "${TEST_BINARY}" -dr
           "--test-directory=${PARAVIEW_TEST_DIR}"
           "--run-test=${CMAKE_CURRENT_SOURCE_DIR}/pqFileDialogTest.py"
           --exit)


ENDIF(QT_TESTING_WITH_PYTHON)
