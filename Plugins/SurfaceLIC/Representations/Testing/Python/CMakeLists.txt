# Surface LIC Plugin testing
# these tests could run safely in serial and
# in parallel.
SET(PVBATCH_TESTS
  SurfaceLIC-ShuttleZoom1
  SurfaceLIC-ShuttleZoom2
  SurfaceLIC-ShuttleAll
  )
if (PARAVIEW_USE_MPI)
  # run the tests in parallel
  SET(NP 3)
  # these depend on the D3 filter and
  # can't run serially
  list(APPEND PVBATCH_TESTS
    SurfaceLIC-OfficeHSlice
    SurfaceLIC-OfficeVSlice
    SurfaceLIC-OfficeSlices
    SurfaceLIC-OfficeContour
    )
  FOREACH (tfile ${PVBATCH_TESTS})
    ExternalData_add_test(ParaViewData
      NAME ${tfile}-Batch
      COMMAND ${VTK_MPIRUN_EXE} ${VTK_MPI_PRENUMPROC_FLAGS} ${VTK_MPI_NUMPROC_FLAG} ${NP} ${VTK_MPI_PREFLAGS}
        $<TARGET_FILE:pvbatch>
        --enable-bt
        ${CMAKE_CURRENT_SOURCE_DIR}/SurfaceLICTestDriver.py
        -D ${PARAVIEW_TEST_OUTPUT_DATA_DIR}
        -T ${PARAVIEW_TEST_OUTPUT_DIR}
        -V DATA{${CMAKE_CURRENT_SOURCE_DIR}/../Data/Baseline/${tfile}.png}
        --threshold 30
        --state ${CMAKE_CURRENT_SOURCE_DIR}/${tfile}.pvsm)
  ENDFOREACH()
  SET(SurfaceLIC-OfficeSlices_THRESHOLD 30)
  SET(SurfaceLIC-OfficeVSlice_THRESHOLD 30)
ELSE()
  # run the test serially
  FOREACH (tfile ${PVBATCH_TESTS})
    ExternalData_add_test(ParaViewData
      NAME ${tfile}-Batch
      COMMAND $<TARGET_FILE:pvbatch>
        --enable-bt
        ${CMAKE_CURRENT_SOURCE_DIR}/SurfaceLICTestDriver.py
        -D ${PARAVIEW_TEST_OUTPUT_DATA_DIR}
        -T ${PARAVIEW_TEST_OUTPUT_DIR}
        -V DATA{${CMAKE_CURRENT_SOURCE_DIR}/../Data/Baseline/${tfile}.png}
        --threshold 30
        --state ${CMAKE_CURRENT_SOURCE_DIR}/${tfile}.pvsm)
  ENDFOREACH()
ENDIF()

# add test that tests loading of SurfaceLIC plugin in Python and then using it.
paraview_add_test_python(TestSuraceLICPlugin.py,NO_VALID)
