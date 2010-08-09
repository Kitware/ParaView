INCLUDE_DIRECTORIES(
  ${ParaView_SOURCE_DIR}/VTK/Common/Testing/Cxx/
  ${ParaView_SOURCE_DIR}/VTK/Rendering/Testing/Cxx/
  )


SET(ServersFilters_SRCS
  ServersFiltersPrintSelf
  TestExtractHistogram
  TestExtractScatterPlot
  TestMPI
  )

IF (VTK_DATA_ROOT)
  SET(ServersFilters_SRCS
    ${ServersFilters_SRCS}
    TestContinuousClose3D
    TestPVFilters
    TestSpyPlotTracers
    )
ENDIF (VTK_DATA_ROOT)


FOREACH(name ${ServersFilters_SRCS})
  ADD_EXECUTABLE(${name} ${name}.cxx)
  ADD_TEST(${name} ${CXX_TEST_PATH}/${name} ${name}
    -D ${VTK_DATA_ROOT}
    )
  TARGET_LINK_LIBRARIES(${name} vtkPVFilters)
ENDFOREACH(name)

ADD_EXECUTABLE(TestIceTCompositePass TestIceTCompositePass.cxx)
    TARGET_LINK_LIBRARIES(TestIceTCompositePass vtkPVFilters)

 ADD_TEST(TestIceTCompositePass-image
            ${VTK_MPIRUN_EXE} ${VTK_MPI_PRENUMPROC_FLAGS} ${VTK_MPI_NUMPROC_FLAG} 2 ${VTK_MPI_PREFLAGS}
            ${CXX_TEST_PATH}/\${CTEST_CONFIGURATION_TYPE}/TestIceTCompositePass
            --tdx 2
            -D ${PARAVIEW_DATA_ROOT}
            -T ${ParaView_BINARY_DIR}/Testing/Temporary
            -V Baseline/TestIceTCompositePass.cxx.png
            ${VTK_MPI_POSTFLAGS})

 ADD_TEST(TestIceTCompositePassDepthOnly-image
            ${VTK_MPIRUN_EXE} ${VTK_MPI_PRENUMPROC_FLAGS} ${VTK_MPI_NUMPROC_FLAG} 2 ${VTK_MPI_PREFLAGS}
            ${CXX_TEST_PATH}/\${CTEST_CONFIGURATION_TYPE}/TestIceTCompositePass
            --tdx 2
            --depth-only
            -D ${PARAVIEW_DATA_ROOT}
            -T ${ParaView_BINARY_DIR}/Testing/Temporary
            -V Baseline/TestIceTCompositePassDepthOnly.cxx.png
            ${VTK_MPI_POSTFLAGS})


ADD_EXECUTABLE(TestIceTShadowMapPass TestIceTShadowMapPass.cxx)
    TARGET_LINK_LIBRARIES(TestIceTShadowMapPass vtkPVFilters)

 ADD_TEST(TestIceTShadowMapPass-image
            ${VTK_MPIRUN_EXE} ${VTK_MPI_PRENUMPROC_FLAGS} ${VTK_MPI_NUMPROC_FLAG} 2 ${VTK_MPI_PREFLAGS}
            ${CXX_TEST_PATH}/\${CTEST_CONFIGURATION_TYPE}/TestIceTShadowMapPass
            --tdx 2
            -D ${PARAVIEW_DATA_ROOT}
            -T ${ParaView_BINARY_DIR}/Testing/Temporary
            -V Baseline/TestIceTShadowMapPass.cxx.png
            ${VTK_MPI_POSTFLAGS})

IF (VTK_USE_DISPLAY AND VTK_DATA_ROOT AND PARAVIEW_DATA_ROOT)
  SET(ServersFiltersImage_SRCS
# Enable these after the transfer function can take the vtkTable histograms.
#TestTransferFunctionEditor
#TestTransferFunctionEditor2
TestPVAMRDualClip
    )
  FOREACH(name ${ServersFiltersImage_SRCS})
    ADD_EXECUTABLE(${name} ${name}.cxx)
    ADD_TEST(${name} ${CXX_TEST_PATH}/${name} ${name}
      -D ${PARAVIEW_DATA_ROOT}
      -T ${ParaView_BINARY_DIR}/Testing/Temporary
      -V ${PARAVIEW_DATA_ROOT}/Baseline/${name}.png
      )
    TARGET_LINK_LIBRARIES(${name} vtkPVFilters)
  ENDFOREACH(name)
ENDIF (VTK_USE_DISPLAY AND VTK_DATA_ROOT AND PARAVIEW_DATA_ROOT)
