vtk_add_test_cxx(vtkRemotingApplicationCxxTests tests
  NO_DATA NO_VALID NO_OUTPUT
  TestExecutableRunner.cxx
  TestStandardPaths.cxx
  )

set(test_name vtkRemotingApplicationCxxTests)
vtk_test_cxx_executable("${test_name}" tests)

set (test_env)
if (WIN32)
  set (test_env "COMMON_APPDATA=C:\\test\\syspath")
  list (APPEND test_env "APPDATA=D:\\test\\appdata")
else()
  set (test_env "XDG_DATA_DIRS=/test/syspath:/test/othersyspath")
  list (APPEND test_env "XDG_CONFIG_HOME=/test/config")
endif()

set_tests_properties("ParaView::RemotingApplicationCxx-TestStandardPaths"
  PROPERTIES
    ENVIRONMENT "${test_env}")
