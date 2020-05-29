###############################################################################
#
# Add python script names here.
set(PY_TESTS
  LockScalarRangeBackwardsCompatibility.py,NO_VALID
  SpreadSheetViewBlockNames.py,NO_VALID
  SpreadSheetViewPartialArrays.py,NO_VALID
  )

paraview_add_test_python(
  JUST_VALID
  ${PY_TESTS}
  )
