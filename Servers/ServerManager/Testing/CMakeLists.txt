###############################################################################
# Set the pvsm files to load for state tests.
# Don't forget to replace the Data dir paths in the the state files
# with ${DataDir}. All state loader tests, replace every occurence of
# ${DataDir} with the actual Data directory.
SET (SMSTATE_FILES
  Arrow
  Axes
  #  BlowTest
  CalcInput
  ExtractGrid
  TestPHT
  SPCTH2
  )

# Path to the state files.
SET (SMSTATE_FILE_DIR
  ${CMAKE_CURRENT_SOURCE_DIR}/SMStates)

# State files for Cxx tests.
SET(CXX_SMSTATE_FILES
  ${SMSTATE_FILES})

# State files for Python tests.
SET (PYTHON_SMSTATE_FILES
  ${SMSTATE_FILES})

###############################################################################

ADD_SUBDIRECTORY(Cxx)

IF (PARAVIEW_ENABLE_PYTHON)
  ADD_SUBDIRECTORY(Python)
ENDIF (PARAVIEW_ENABLE_PYTHON)


