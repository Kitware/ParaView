ADD_EXECUTABLE( TestHashTable TestHashTable )
ADD_EXECUTABLE( TestPointerArray TestPointerArray )

ADD_TEST(TestHashTable ${CXX_TEST_PATH}/TestHashTable)
ADD_TEST(TestPointerArray ${CXX_TEST_PATH}/TestPointerArray)

LINK_LIBRARIES( 
    vtkKWWidgetsTcl
    vtkCommon
)