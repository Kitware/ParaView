paraview_test_load_data(""
  can.ex2
  )

###############################################################################
# For python scripts for testing.
#
# Add python script names here.
set(PY_TESTS
  PVDWriter.py,NO_VALID
  )

include(FindPythonModules)

#set(SMSTATE_FILE_DIR "${CMAKE_CURRENT_SOURCE_DIR}")
#set(${vtk-module}_ARGS
#  -S "${SMSTATE_FILE_DIR}")
paraview_add_test_python(
  JUST_VALID
  ${PY_TESTS}
  )
