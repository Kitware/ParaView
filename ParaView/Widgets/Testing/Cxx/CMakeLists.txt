ADD_EXECUTABLE( TestHashTable TestHashTable )
ADD_EXECUTABLE( TestPointerArray TestPointerArray )
ADD_EXECUTABLE( TestRegistery TestRegistery )

ADD_TEST(TestHashTable ${CXX_TEST_PATH}/TestHashTable)
ADD_TEST(TestPointerArray ${CXX_TEST_PATH}/TestPointerArray)
ADD_TEST(TestRegistery ${CXX_TEST_PATH}/TestRegistery)

LINK_LIBRARIES( 
    vtkKWWidgetsTCL
    vtkCommon
)

LINK_LIBRARIES( vtkpng vtkzlib vtkjpeg )
