# SurfaceLIC tests

SET(PVBATCH_TESTS
  SurfaceLIC-ShuttleZoom1
  SurfaceLIC-ShuttleZoom2
  SurfaceLIC-ShuttleAll
  SurfaceLIC-OfficeHSlice
  SurfaceLIC-OfficeVSlice
  SurfaceLIC-OfficeSlices
  SurfaceLIC-OfficeContour
  )

IF (PARAVIEW_DATA_ROOT)
  FOREACH (tfile ${PVBATCH_TESTS})
    IF (VTK_MPIRUN_EXE)
      # run the tests in parallel
      ADD_TEST(NAME ${tfile}-Batch
        COMMAND ${VTK_MPIRUN_EXE} ${VTK_MPI_PRENUMPROC_FLAGS} ${VTK_MPI_NUMPROC_FLAG} 4 ${VTK_MPI_PREFLAGS}
          $<TARGET_FILE:pvbatch>
          ${CMAKE_CURRENT_SOURCE_DIR}/SurfaceLICTestDriver.py
          -D ${PARAVIEW_DATA_ROOT}
          -T ${ParaView_BINARY_DIR}/Testing/Temporary
          -V ${PARAVIEW_DATA_ROOT}/Baseline/SurfaceLICPlugin/${tfile}.png
          --state ${CMAKE_CURRENT_SOURCE_DIR}/${tfile}.pvsm)
    ELSE()
      # run the test serially
      ADD_TEST(NAME ${tfile}-Batch
        COMMAND $<TARGET_FILE:pvbatch>
          ${CMAKE_CURRENT_SOURCE_DIR}/SurfaceLICTestDriver.py
          -D ${PARAVIEW_DATA_ROOT}
          -T ${ParaView_BINARY_DIR}/Testing/Temporary
          -V ${PARAVIEW_DATA_ROOT}/Baseline/SurfaceLICPlugin/${tfile}.png
          --state ${CMAKE_CURRENT_SOURCE_DIR}/${tfile}.pvsm)
    ENDIF()
  ENDFOREACH (tfile)
ENDIF()
