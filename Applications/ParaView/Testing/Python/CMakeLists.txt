IF (PARAVIEW_DATA_ROOT)
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
endif()

ENDIF (PARAVIEW_DATA_ROOT)

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
