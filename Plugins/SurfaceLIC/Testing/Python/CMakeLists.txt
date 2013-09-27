# Surface LIC Plugin testing
IF (PARAVIEW_DATA_ROOT)
  # these tests could run safely in serial and
  # in parallel.
  SET(PVBATCH_TESTS
    SurfaceLIC-ShuttleZoom1
    SurfaceLIC-ShuttleZoom2
    SurfaceLIC-ShuttleAll
    )
  # don't attempt to load plugin if linked static
  SET(STATIC_LINK 1)
  IF (BUILD_SHARED_LIBS)
    set(STATIC_LINK 0)
  ENDIF()
  IF (PARAVIEW_USE_MPI AND VTK_MPIRUN_EXE)
    # run the tests in parallel
    SET(NP 3)
    # these depend on the D3 filter and
    # can't run serially
    SET(PVBATCH_TESTS
      ${PVBATCH_TESTS}
      SurfaceLIC-OfficeHSlice
      SurfaceLIC-OfficeVSlice
      SurfaceLIC-OfficeSlices
      SurfaceLIC-OfficeContour
      )
    FOREACH (tfile ${PVBATCH_TESTS})
      ADD_TEST(NAME ${tfile}-Batch
        COMMAND ${VTK_MPIRUN_EXE} ${VTK_MPI_PRENUMPROC_FLAGS} ${VTK_MPI_NUMPROC_FLAG} ${NP} ${VTK_MPI_PREFLAGS}
          $<TARGET_FILE:pvbatch>
          --enable-bt
          ${CMAKE_CURRENT_SOURCE_DIR}/SurfaceLICTestDriver.py
          -D ${PARAVIEW_DATA_ROOT}
          -T ${ParaView_BINARY_DIR}/Testing/Temporary
          -V ${PARAVIEW_DATA_ROOT}/Baseline/SurfaceLICPlugin/${tfile}.png
          -S ${STATIC_LINK}
          --state ${CMAKE_CURRENT_SOURCE_DIR}/${tfile}.pvsm)
      SET_TESTS_PROPERTIES(${tfile}-Batch PROPERTIES LABELS "PARAVIEW")
    ENDFOREACH()
  ELSE()
    # run the test serially
    FOREACH (tfile ${PVBATCH_TESTS})
      ADD_TEST(NAME ${tfile}-Batch
        COMMAND $<TARGET_FILE:pvbatch>
          --enable-bt
          ${CMAKE_CURRENT_SOURCE_DIR}/SurfaceLICTestDriver.py
          -D ${PARAVIEW_DATA_ROOT}
          -T ${ParaView_BINARY_DIR}/Testing/Temporary
          -V ${PARAVIEW_DATA_ROOT}/Baseline/SurfaceLICPlugin/${tfile}.png
          -S ${STATIC_LINK}
          --state ${CMAKE_CURRENT_SOURCE_DIR}/${tfile}.pvsm)
      SET_TESTS_PROPERTIES(${tfile}-Batch PROPERTIES LABELS "PARAVIEW")
    ENDFOREACH()
  ENDIF()
ENDIF()
