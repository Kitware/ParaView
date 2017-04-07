include(ParaViewTestingMacros)

if("${VTK_RENDERING_BACKEND}" STREQUAL "OpenGL2")
  add_definitions(-DVTKGL2)
endif()

paraview_add_test_cxx(${vtk-module}CxxTests tests
  NO_DATA NO_VALID NO_OUTPUT
  ParaViewCoreClientServerCorePrintSelf.cxx
  TestPVArrayInformation.cxx
  TestPartialArraysInformation.cxx
  TestSpecialDirectories.cxx
  TestSystemCaps.cxx
  )
if (PARAVIEW_USE_MPI)
  vtk_add_test_mpi(${vtk-module}CxxTests mpi_tests
    NO_DATA NO_VALID NO_OUTPUT
    TestMPI.cxx)
  list(APPEND tests
    ${mpi_tests})

  set_property(SOURCE TestSystemCaps.cxx
    APPEND PROPERTY COMPILE_DEFINITIONS TEST_MPI_CAPS)
else ()
  vtk_add_test_cxx(${vtk-module}CxxTests no_mpi_tests
    NO_DATA NO_VALID NO_OUTPUT
    TestMPI.cxx)
  list(APPEND tests
    ${no_mpi_tests})
endif()

if (PARAVIEW_ENABLE_PYTHON)
  set_property(SOURCE TestSystemCaps.cxx
    APPEND PROPERTY COMPILE_DEFINITIONS TEST_PY_CAPS)
endif()

vtk_test_cxx_executable(${vtk-module}CxxTests tests)

include(vtkOpenGL)
vtk_opengl_link(${vtk-module}CxxTests)

if (PARAVIEW_USE_MPI)
  vtk_mpi_link(${vtk-module}CxxTests)
endif()
