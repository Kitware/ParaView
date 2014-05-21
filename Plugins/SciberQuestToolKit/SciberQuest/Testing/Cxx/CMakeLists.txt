include_directories(
  ${ParaView_SOURCE_DIR}/VTK/Common/Testing/Cxx/
  ${ParaView_SOURCE_DIR}/VTK/Rendering/Testing/Cxx/
  ${SCIBERQUEST_INCLUDE_DIRS}
  )

vtk_module_load(vtksys)
vtk_module_load(vtkSciberQuest)

include(ParaViewTestingMacros)

include_directories(${vtksys_INCLUDE_DIRS})
include_directories(${vtkSciberQuest_INCLUDE_DIRS})

if(PARAVIEW_USE_MPI)
  include(vtkMPI)
endif()

if (PARAVIEW_USE_MPI AND (NOT WIN32))
  add_definitions(-DPARAVIEW_USE_MPI)
endif ()

paraview_test_load_data_dirs(SciberQuestToolKit
  SciberQuestToolKit
  SciberQuestToolKit/Asym2D
  SciberQuestToolKit/Gyres
  SciberQuestToolKit/MagneticIslands
  SciberQuestToolKit/Rankine
  SciberQuestToolKit/SmallVector
  )
paraview_test_data_target(SciberQuestToolKit)

if (PARAVIEW_USE_MPI AND (NOT WIN32))
  # parallel invocations, these could run with any number of
  # processes.
  set(${vtk-module}Cxx-MPI_NUMPROCS 4)
  paraview_add_test_mpi(${vtk-module}Cxx-MPI tests
    TESTING_DATA
    CUSTOM_BASELINES
    TestBOVIO.cxx
    TestKernelConvolution.cxx
    TestVortexFilter.cxx
    TestFieldTopologyMapper.cxx
    TestPoincareMapper.cxx
    TestFTLE.cxx
    TestFieldTracer.cxx
    TestPlaneSource.cxx
    )
  vtk_test_mpi_executable(${vtk-module}Cxx-MPI tests
    TestUtils.cxx
    )

  # Increase timeout for long tests (600 = 10m)
  set_tests_properties(${vtk-module}Cxx-MPI-TestVortexFilter PROPERTIES TIMEOUT 600)
  set_tests_properties(${vtk-module}Cxx-MPI-TestVortexFilter PROPERTIES RUN_SERIAL ON)
  set_tests_properties(${vtk-module}Cxx-MPI-TestPoincareMapper PROPERTIES TIMEOUT 600)
  set_tests_properties(${vtk-module}Cxx-MPI-TestPoincareMapper PROPERTIES RUN_SERIAL ON)
else()
  # serial invocations, our reader needs mpi these tests use
  # an alternate reader when invoked serially.
  paraview_add_test_cxx(${vtk-module}CxxTests tests
    CUSTOM_BASELINES
    TestKernelConvolution.cxx
    TestVortexFilter.cxx
    )
  vtk_test_cxx_executable(${vtk-module}CxxTests tests
    TestUtils.cxx
    )

  # Increase timeout for long tests (1200 = 20m)
  set_tests_properties(${vtk-module}Cxx-TestVortexFilter PROPERTIES TIMEOUT 1200)
  set_tests_properties(${vtk-module}Cxx-TestVortexFilter PROPERTIES RUN_SERIAL ON)
endif()
