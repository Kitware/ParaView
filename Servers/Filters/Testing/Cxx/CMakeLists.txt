INCLUDE_DIRECTORIES(
  ${ParaView_SOURCE_DIR}/VTK/Common/Testing/Cxx/
  ${ParaView_SOURCE_DIR}/VTK/Rendering/Testing/Cxx/
  )


SET(ServersFilters_SRCS
  ServersFiltersPrintSelf
  TestCTH
  TestMPI
  TestSort
  )

IF (VTK_DATA_ROOT)
  SET(ServersFilters_SRCS
    ${ServersFilters_SRCS}
    TestContinuousClose3D
    TestPVFilters
    )
ENDIF (VTK_DATA_ROOT)


IF(VTK_USE_MPI)
  SET(ServersFilters_SRCS
    ${ServersFilters_SRCS}
    TestCave
    )
ENDIF(VTK_USE_MPI)


FOREACH(name ${ServersFilters_SRCS})
  ADD_EXECUTABLE(${name} ${name}.cxx)
  ADD_TEST(${name} ${CXX_TEST_PATH}/${name} ${name}
    -D ${VTK_DATA_ROOT}
    )
  TARGET_LINK_LIBRARIES(${name} vtkPVFilters)
ENDFOREACH(name)



IF (VTK_USE_DISPLAY)
  SET(ServersFiltersImage_SRCS
    )
  IF (VTK_DATA_ROOT)
    SET(ServersFiltersImage_SRCS ${ServersFiltersImage_SRCS}
      TestProjectedTetrahedra
      )
  ENDIF (VTK_DATA_ROOT)
  FOREACH(name ${ServersFiltersImage_SRCS})
    ADD_EXECUTABLE(${name} ${name}.cxx)
    ADD_TEST(${name} ${CXX_TEST_PATH}/${name} ${name}
      -D ${VTK_DATA_ROOT}
      -T ${ParaView_BINARY_DIR}/Testing/Temporary
      -V ${ParaView_SOURCE_DIR}/Data/Baseline/${name}.png
      )
    TARGET_LINK_LIBRARIES(${name} vtkPVFilters)
  ENDFOREACH(name)
ENDIF (VTK_USE_DISPLAY)
