SET(ServersFilters_SRCS
  TestContinuousClose3D
  ServersFiltersPrintSelf
  TestCTH
  )

FOREACH(name ${ServersFilters_SRCS})
  ADD_EXECUTABLE(${name} ${name}.cxx)
  ADD_TEST(${name} ${CXX_TEST_PATH}/${name})
 
  TARGET_LINK_LIBRARIES(${name} vtkPVFilters)
ENDFOREACH(name)
