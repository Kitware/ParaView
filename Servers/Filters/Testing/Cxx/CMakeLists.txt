INCLUDE_DIRECTORIES(
  ${ParaView_SOURCE_DIR}/VTK/Common/Testing/Cxx/
  ${ParaView_SOURCE_DIR}/VTK/Rendering/Testing/Cxx/
  )

SET(ServersFilters_SRCS
  TestContinuousClose3D
  ServersFiltersPrintSelf
  TestCTH
  TestPVFilters
  TestMPI
  )

IF(VTK_USE_MPI)
  SET(ServersFilters_SRCS
    ${ServersFilters_SRCS}
    TestCave
    )
ENDIF(VTK_USE_MPI)

SET(ServersFiltersImage_SRCS
  TestProjectedTetrahedra
  TestSort
)

FOREACH(name ${ServersFilters_SRCS})
  ADD_EXECUTABLE(${name} ${name}.cxx)
  ADD_TEST(${name} ${CXX_TEST_PATH}/${name} ${name}
    -D ${VTK_DATA_ROOT}
    )
  TARGET_LINK_LIBRARIES(${name} vtkPVFilters)
ENDFOREACH(name)

FOREACH(name ${ServersFiltersImage_SRCS})
  ADD_EXECUTABLE(${name} ${name}.cxx)
  ADD_TEST(${name} ${CXX_TEST_PATH}/${name} ${name}
    -D ${VTK_DATA_ROOT}
    -T ${ParaView_BINARY_DIR}/Testing/Temporary
    -V ${ParaView_SOURCE_DIR}/Data/Baseline/${name}.png
    )
  TARGET_LINK_LIBRARIES(${name} vtkPVFilters)
ENDFOREACH(name)
