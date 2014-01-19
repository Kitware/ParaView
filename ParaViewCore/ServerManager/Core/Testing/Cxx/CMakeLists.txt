set (tests
  TestParaViewPipelineController
)

foreach (test ${tests})
  vtk_module_test_executable(${test} ${test}.cxx)
  add_test(NAME ${test} COMMAND ${test})
  set_tests_properties(${test} PROPERTIES LABELS "PARAVIEW")
endforeach()
