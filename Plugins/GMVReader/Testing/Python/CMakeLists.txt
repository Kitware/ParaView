# Set variables to make the testing functions.
set(_vtk_build_test "paraview")
set(${_vtk_build_test}_TEST_LABELS paraview)

#ExternalData_Expand_Arguments(ParaViewData _
#  "DATA{${CMAKE_CURRENT_SOURCE_DIR}/../Data/GMV/one_vertex.gmv}"
#  "DATA{${CMAKE_CURRENT_SOURCE_DIR}/../Data/GMV/two_vertex.gmv}"
#  )

paraview_add_test_python(
  NO_RT
  GMVReaderOneOrTwoVertices.py
)
