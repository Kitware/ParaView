INCLUDE_DIRECTORIES(
  ${ParaView_SOURCE_DIR}/VTK/Common/Testing/Cxx/
  ${ParaView_SOURCE_DIR}/VTK/Rendering/Testing/Cxx/
  )


SET(ServersFilters_SRCS
  ServersFiltersPrintSelf
  TestMPI
  )

IF (VTK_DATA_ROOT)
  SET(ServersFilters_SRCS
    ${ServersFilters_SRCS}
    TestContinuousClose3D
    TestPVFilters
    )
ENDIF (VTK_DATA_ROOT)


FOREACH(name ${ServersFilters_SRCS})
  ADD_EXECUTABLE(${name} ${name}.cxx)
  ADD_TEST(${name} ${CXX_TEST_PATH}/${name} ${name}
    -D ${VTK_DATA_ROOT}
    )
  TARGET_LINK_LIBRARIES(${name} vtkPVFilters)
ENDFOREACH(name)



IF (VTK_USE_DISPLAY)
  SET(ServersFiltersImage_SRCS
    )
  FOREACH(name ${ServersFiltersImage_SRCS})
    ADD_EXECUTABLE(${name} ${name}.cxx)
    ADD_TEST(${name} ${CXX_TEST_PATH}/${name} ${name}
      -D ${VTK_DATA_ROOT}
      -T ${ParaView_BINARY_DIR}/Testing/Temporary
      -V ${ParaView_SOURCE_DIR}/Data/Baseline/${name}.png
      )
    TARGET_LINK_LIBRARIES(${name} vtkPVFilters)
  ENDFOREACH(name)


  IF(VTK_MPIRUN_EXE)
    SEPARATE_ARGUMENTS(VTK_MPI_PREFLAGS)
    SEPARATE_ARGUMENTS(VTK_MPI_POSTFLAGS)
    IF(VTK_MPI_MAX_NUMPROCS GREATER 1)
      ADD_EXECUTABLE(TestCave TestCave.cxx)
      TARGET_LINK_LIBRARIES(TestCave vtkPVFilters)
      ADD_TEST(TestCave
        ${VTK_MPIRUN_EXE} ${VTK_MPI_NUMPROC_FLAG} 2 ${VTK_MPI_PREFLAGS}
        TestCave ${CXX_TEST_PATH}/TestCave TestCave
        -D ${VTK_DATA_ROOT}
        -T ${ParaView_BINARY_DIR}/Testing/Temporary
        -V ${ParaView_SOURCE_DIR}/Data/Baseline/TestCave.png
        ${VTK_MPI_POSTFLAGS}
        )
    ENDIF(VTK_MPI_MAX_NUMPROCS GREATER 1)
  ENDIF(VTK_MPIRUN_EXE)
ENDIF (VTK_USE_DISPLAY)
