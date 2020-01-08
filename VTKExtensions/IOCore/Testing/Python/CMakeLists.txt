###############################################################################
# For python scripts for testing.
#
# Add python script names here.
set(PY_TESTS
  PVDWriter.py,NO_VALID
  )

#set(SMSTATE_FILE_DIR "${CMAKE_CURRENT_SOURCE_DIR}")
#set(vtkPVVTKExtensionsDefault_ARGS
#  -S "${SMSTATE_FILE_DIR}")
paraview_add_test_python(
  JUST_VALID
  ${PY_TESTS}
  )
