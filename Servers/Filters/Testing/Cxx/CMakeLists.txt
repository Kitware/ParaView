INCLUDE_DIRECTORIES(
  ${ParaView_SOURCE_DIR}/VTK/Common/Testing/Cxx/
  )

SET(ServersFilters_SRCS
  TestContinuousClose3D
  ServersFiltersPrintSelf
  TestCTH
  TestPVFilters
  TestMPI
  TestCave
  )

FOREACH(name ${ServersFilters_SRCS})
  ADD_EXECUTABLE(${name} ${name}.cxx)
  ADD_TEST(${name} ${CXX_TEST_PATH}/${name} ${name}
        -D ${VTK_DATA_ROOT})
  TARGET_LINK_LIBRARIES(${name} vtkPVFilters)
ENDFOREACH(name)
