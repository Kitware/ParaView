

IF(PARAVIEW_DATA_ROOT)
  SET(BASELINE_DIR ${PARAVIEW_DATA_ROOT}/Baseline)
  # Reduce the number of dirs that get included on moc command line
  # since it causes issues on Windows 2000.
  GET_DIRECTORY_PROPERTY(include_dirs_tmp INCLUDE_DIRECTORIES)
  SET_DIRECTORY_PROPERTIES(PROPERTIES INCLUDE_DIRECTORIES "${MOC_INCLUDE_DIRS}")
  QT4_WRAP_CPP(MOC_SRCS BasicApp.h)
  SET_DIRECTORY_PROPERTIES(PROPERTIES INCLUDE_DIRECTORIES "${include_dirs_tmp}")
  ADD_EXECUTABLE(pqCoreBasicApp BasicApp.cxx BasicApp.h ${MOC_SRCS})
  TARGET_LINK_LIBRARIES(pqCoreBasicApp pqCore QtTesting ${QT_LIBRARIES} )
  ADD_TEST(pqCoreBasicApp "${EXECUTABLE_OUTPUT_PATH}/pqCoreBasicApp" -dr "--test-directory=${PARAVIEW_TEST_DIR}"
           "--test-baseline=${BASELINE_DIR}/pqCoreBasicApp.png" --exit)
ENDIF(PARAVIEW_DATA_ROOT)

################################################################################
IF(QT_TESTING_WITH_PYTHON)

  # Reduce the number of dirs that get included on moc command line
  # since it causes issues on Windows 2000.
  GET_DIRECTORY_PROPERTY(include_dirs_tmp INCLUDE_DIRECTORIES)
  SET_DIRECTORY_PROPERTIES(PROPERTIES INCLUDE_DIRECTORIES "${MOC_INCLUDE_DIRS}")
  SET (MOC_SRCS)
  QT4_WRAP_CPP(MOC_SRCS pqFileDialogTest.h)
  SET_DIRECTORY_PROPERTIES(PROPERTIES INCLUDE_DIRECTORIES "${include_dirs_tmp}")
  ADD_EXECUTABLE(pqFileDialogTest
                 pqFileDialogTest.cxx pqFileDialogTest.h 
                 ${MOC_SRCS})
  TARGET_LINK_LIBRARIES(pqFileDialogTest pqCore QtTesting ${QT_LIBRARIES} )
  ADD_TEST(pqFileDialogTest "${EXECUTABLE_OUTPUT_PATH}/pqFileDialogTest" -dr
           "--test-directory=${PARAVIEW_TEST_DIR}"
           "--test-script=${CMAKE_CURRENT_SOURCE_DIR}/pqFileDialogTest.py"
           --exit)


ENDIF(QT_TESTING_WITH_PYTHON)
