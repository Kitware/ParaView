SET(ServersServerManager_SRCS
  ServersServerManagerPrintSelf
  )

FOREACH(name ${ServersServerManager_SRCS})
  ADD_EXECUTABLE(${name} ${name}.cxx)
  ADD_TEST(${name} ${CXX_TEST_PATH}/${name} ${name}
        -D ${VTK_DATA_ROOT})
  TARGET_LINK_LIBRARIES(${name} vtkPVServerManager)
ENDFOREACH(name)
