if (PARAVIEW_DATA_ROOT)
  # only enable TestPythonAnnotationFilter test if numpy is available
  include(TestNumPy)
  if (FOUND_NUMPY)
    # Add pvpython tests
    add_test(NAME pvpython.TestPythonAnnotationFilter
        COMMAND pvpython
        --enable-bt
        --data=${PARAVIEW_DATA_ROOT}/Data/can.ex2
        ${CMAKE_CURRENT_SOURCE_DIR}/TestPythonAnnotationFilter.py)
    add_test(NAME pvpython.TestPythonAnnotationFilterNoMerge
        COMMAND pvpython
        --enable-bt
        --data=${PARAVIEW_DATA_ROOT}/Data/can.ex2
        ${CMAKE_CURRENT_SOURCE_DIR}/TestPythonAnnotationFilterNoMerge.py)

    set_tests_properties(
      pvpython.TestPythonAnnotationFilter
      pvpython.TestPythonAnnotationFilterNoMerge
      PROPERTIES LABELS "PARAVIEW")

    if (PARAVIEW_ENABLE_MATPLOTLIB AND PYTHON_MODULE_matplotlib_FOUND)
      add_test(NAME pvpython.TestPythonView
          COMMAND pvpython
          --enable-bt
          ${CMAKE_CURRENT_SOURCE_DIR}/TestPythonView.py)
      set_tests_properties(
        pvpython.TestPythonView
        PROPERTIES LABELS "PARAVIEW")
    endif()

  endif()

  # these tests could run safely in serial and
  # in parallel.
  set(PVBATCH_TESTS
    StructuredGridVolumeRendering
    )
  if (PARAVIEW_USE_MPI AND VTK_MPIRUN_EXE)
    # run the tests in parallel
    set(NP 3)
    foreach (tfile ${PVBATCH_TESTS})
      add_test(NAME Test${tfile}-Batch
        COMMAND ${VTK_MPIRUN_EXE} ${VTK_MPI_PRENUMPROC_FLAGS} ${VTK_MPI_NUMPROC_FLAG} ${NP} ${VTK_MPI_PREFLAGS}
          $<TARGET_FILE:pvbatch>
          --enable-bt
          ${CMAKE_CURRENT_SOURCE_DIR}/${tfile}.py
          -D ${PARAVIEW_DATA_ROOT}
          -T ${ParaView_BINARY_DIR}/Testing/Temporary
          -V ${PARAVIEW_DATA_ROOT}/Baseline//${tfile}.png
          --state ${CMAKE_CURRENT_SOURCE_DIR}/${tfile}.pvsm)
      set_tests_properties(Test${tfile}-Batch PROPERTIES LABELS "PARAVIEW")
    endforeach()
  else()
    # run the test serially
    foreach (tfile ${PVBATCH_TESTS})
      add_test(NAME Test${tfile}-Batch
        COMMAND $<TARGET_FILE:pvbatch>
          --enable-bt
          ${CMAKE_CURRENT_SOURCE_DIR}/${tfile}.py
          -D ${PARAVIEW_DATA_ROOT}
          -T ${ParaView_BINARY_DIR}/Testing/Temporary
          -V ${PARAVIEW_DATA_ROOT}/Baseline/${tfile}.png
          --state ${CMAKE_CURRENT_SOURCE_DIR}/${tfile}.pvsm)
      set_tests_properties(Test${tfile}-Batch PROPERTIES LABELS "PARAVIEW")
    endforeach()
  endif()
endif()

# Python Multi-servers test
# => Only for shared build as we dynamically load plugins
if(BUILD_SHARED_LIBS)
  add_test(NAME py-multi-servers.TestMultiServersConfig
    COMMAND smTestDriver
    --enable-bt
    --test-multi-servers 2
    --server $<TARGET_FILE:pvserver>
    --client $<TARGET_FILE:pvpython>
    ${CMAKE_CURRENT_SOURCE_DIR}/TestMultiServersConfig.py
  )

  add_test(NAME py-multi-servers.TestMultiServersRemoteProxy
    COMMAND smTestDriver
    --enable-bt
    --test-multi-servers 3
    --server $<TARGET_FILE:pvserver>
    --client $<TARGET_FILE:pvpython>
    ${CMAKE_CURRENT_SOURCE_DIR}/TestMultiServersRemoteProxy.py
  )

  add_test(NAME py-multi-servers.TestRemoteProgrammableFilter
    COMMAND smTestDriver
    --enable-bt
    --server $<TARGET_FILE:pvserver>
    --client $<TARGET_FILE:pvpython>
    ${CMAKE_CURRENT_SOURCE_DIR}/TestRemoteProgrammableFilter.py
  )

  set_tests_properties(
    py-multi-servers.TestMultiServersConfig
    py-multi-servers.TestMultiServersRemoteProxy
    py-multi-servers.TestRemoteProgrammableFilter
    PROPERTIES LABELS "PARAVIEW")
endif(BUILD_SHARED_LIBS)
