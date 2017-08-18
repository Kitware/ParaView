paraview_test_load_baselines_dirs(""
  SurfaceLICPlugin
  )
paraview_test_load_data_dirs(SurfaceLIC
  shuttle-surf
  )

set(data_files shuttle-surf.vtm)
if(NOT PARAVIEW_BUILD_QT_GUI)
    list(APPEND data_files office.binary.vtk)
endif()
paraview_test_load_data(SurfaceLIC ${data_files})

# Surface LIC Plugin testing
# these tests could run safely in serial and
# in parallel.
SET(PVBATCH_TESTS
  SurfaceLIC-ShuttleZoom1
  SurfaceLIC-ShuttleZoom2
  SurfaceLIC-ShuttleAll
  )
IF (PARAVIEW_USE_MPI AND VTK_MPIRUN_EXE)
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
        -V DATA{${PARAVIEW_TEST_BASELINE_DIR}/SurfaceLICPlugin/${tfile}.png}
        --threshold 30
        --state ${CMAKE_CURRENT_SOURCE_DIR}/${tfile}.pvsm)
    SET_TESTS_PROPERTIES(${tfile}-Batch PROPERTIES LABELS "PARAVIEW")
  ENDFOREACH()
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
        -V DATA{${PARAVIEW_TEST_BASELINE_DIR}/SurfaceLICPlugin/${tfile}.png}
        --threshold 30
        --state ${CMAKE_CURRENT_SOURCE_DIR}/${tfile}.pvsm)
    SET_TESTS_PROPERTIES(${tfile}-Batch PROPERTIES LABELS "PARAVIEW")
  ENDFOREACH()
ENDIF()
