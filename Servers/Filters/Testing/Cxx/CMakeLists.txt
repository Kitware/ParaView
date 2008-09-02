INCLUDE_DIRECTORIES(
  ${ParaView_SOURCE_DIR}/VTK/Common/Testing/Cxx/
  ${ParaView_SOURCE_DIR}/VTK/Rendering/Testing/Cxx/
  )


SET(ServersFilters_SRCS
  ServersFiltersPrintSelf
  TestExtractHistogram
  TestExtractScatterPlot
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

IF (PARAVIEW_DATA_ROOT)
  #
  SET (PARALLEL_TESTS
    TestCTHFragmentConnect)
  #
  FOREACH (name ${PARALLEL_TESTS})
    ADD_EXECUTABLE(${name} ${name}.cxx)
    IF (VTK_MPIRUN_EXE AND VTK_MPI_MAX_NUMPROCS GREATER 1)
      ADD_TEST(${name}
        ${VTK_MPIRUN_EXE} ${VTK_MPI_NUMPROC_FLAG} 2 ${VTK_MPI_PREFLAGS}
        ${CXX_TEST_PATH}/${name}
        -D ${PARAVIEW_DATA_ROOT}
        -T ${ParaView_BINARY_DIR}/Testing/Temporary)
    ELSE (VTK_MPIRUN_EXE AND VTK_MPI_MAX_NUMPROCS GREATER 1)
      ADD_TEST(${name}
        ${CXX_TEST_PATH}/${name}
        -D ${PARAVIEW_DATA_ROOT}
        -T ${ParaView_BINARY_DIR}/Testing/Temporary)
    ENDIF (VTK_MPIRUN_EXE AND VTK_MPI_MAX_NUMPROCS GREATER 1)
    TARGET_LINK_LIBRARIES(${name} vtkPVFilters)
  ENDFOREACH (name)
  #
ENDIF (PARAVIEW_DATA_ROOT)



IF (VTK_USE_DISPLAY AND VTK_DATA_ROOT AND PARAVIEW_DATA_ROOT)
  SET(ServersFiltersImage_SRCS
    TestTransferFunctionEditor
    TestTransferFunctionEditor2
    )
  FOREACH(name ${ServersFiltersImage_SRCS})
    ADD_EXECUTABLE(${name} ${name}.cxx)
    ADD_TEST(${name} ${CXX_TEST_PATH}/${name} ${name}
      -D ${VTK_DATA_ROOT}
      -T ${ParaView_BINARY_DIR}/Testing/Temporary
      -V ${PARAVIEW_DATA_ROOT}/Baseline/${name}.png
      )
    TARGET_LINK_LIBRARIES(${name} vtkPVFilters)
  ENDFOREACH(name)
ENDIF (VTK_USE_DISPLAY AND VTK_DATA_ROOT AND PARAVIEW_DATA_ROOT)

