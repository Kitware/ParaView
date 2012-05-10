#############
### FIXME ###
#############

if(Module_vtkRenderingOpenGL)
  IF(VTK_USE_ADIOS)
    SET(ConditionalTests
      ${ConditionalTests}
      TestAdiosReader.cxx
      )

    INCLUDE_DIRECTORIES(${ADIOS_INCLUDE_PATH})

    ADD_EXECUTABLE(TestAdiosReaderPixie TestAdiosReaderPixie.cxx)
    TARGET_LINK_LIBRARIES(TestAdiosReaderPixie vtkParallel vtkIO vtkGraphics vtkImaging vtkRendering ${ADIOS_READ_LIBRARY})

    ADD_EXECUTABLE(TestAdiosInternals TestAdiosInternals.cxx)
    TARGET_LINK_LIBRARIES(TestAdiosInternals vtkParallel vtkIO vtkGraphics vtkImaging vtkRendering ${ADIOS_READ_LIBRARY})
  ENDIF(VTK_USE_ADIOS)

endif()


IF (VTK_DATA_ROOT)
  if(Module_vtkRenderingOpenGL)
    IF(VTK_USE_ADIOS)
      ADD_TEST(TestAdiosReader ${CXX_TEST_PATH}/${KIT}CxxTests
           TestAdiosReader -D ${VTK_DATA_ROOT}
           -T ${VTK_BINARY_DIR}/Testing/Temporary
           -V ${VTK_DATA_ROOT}/Baseline/${KIT}/TestAdiosReader.png)
    ENDIF(VTK_USE_ADIOS)
  endif()
ENDIF (VTK_DATA_ROOT)
