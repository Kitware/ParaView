ADD_EXECUTABLE( TestHashTable TestHashTable )
ADD_EXECUTABLE( TestPointerArray TestPointerArray )
ADD_EXECUTABLE( TestRegistery TestRegistery )

ADD_TEST(TestHashTable ${CXX_TEST_PATH}/TestHashTable)
ADD_TEST(TestPointerArray ${CXX_TEST_PATH}/TestPointerArray)
ADD_TEST(TestRegistery ${CXX_TEST_PATH}/TestRegistery)

# add MPI
IF (MPI_LIBRARY)
  LINK_LIBRARIES (${MPI_LIBRARY})
ENDIF (MPI_LIBRARY)
IF (MPI_EXTRA_LIBRARY)
  LINK_LIBRARIES (${MPI_EXTRA_LIBRARY})
ENDIF (MPI_EXTRA_LIBRARY)

# sockets
IF (WIN32)
  LINK_LIBRARIES( wsock32 )
ENDIF (WIN32)

LINK_LIBRARIES( 
    vtkKWWidgetsTCL
    vtkCommon
    vtkjpeg
    vtkpng
    vtkzlib
)
