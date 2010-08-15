INCLUDE_DIRECTORIES(
  ${ParaView_SOURCE_DIR}/VTK/Common/Testing/Cxx/
  ${ParaView_SOURCE_DIR}/VTK/Rendering/Testing/Cxx/
  )


SET(ServersFilters_SRCS
  ServersFiltersPrintSelf
  TestExtractHistogram
  TestExtractScatterPlot
  TestMPI
  TestTilesHelper
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

IF(PARAVIEW_USE_ICE_T AND VTK_USE_MPI AND VTK_USE_DISPLAY AND
   PARAVIEW_DATA_ROOT)
  ADD_EXECUTABLE(TestIceTCompositePass TestIceTCompositePass.cxx)
  TARGET_LINK_LIBRARIES(TestIceTCompositePass vtkPVFilters)

  ADD_TEST(TestIceTCompositePassWithBlurAndOrderedCompositing
            ${VTK_MPIRUN_EXE} ${VTK_MPI_PRENUMPROC_FLAGS} ${VTK_MPI_NUMPROC_FLAG} 2 ${VTK_MPI_PREFLAGS}
            ${CXX_TEST_PATH}/\${CTEST_CONFIGURATION_TYPE}/TestIceTCompositePass
            --tdx 2
            --blur
            --use-ordered-compositing
            --use-depth-peeling
            -D ${PARAVIEW_DATA_ROOT}
            -T ${ParaView_BINARY_DIR}/Testing/Temporary
            -V Baseline/TestIceTCompositePassWithBlurAndOrderedCompositing.png
            ${VTK_MPI_POSTFLAGS})

  ADD_TEST(TestIceTCompositePassWithSobel
            ${VTK_MPIRUN_EXE} ${VTK_MPI_PRENUMPROC_FLAGS} ${VTK_MPI_NUMPROC_FLAG} 2 ${VTK_MPI_PREFLAGS}
            ${CXX_TEST_PATH}/\${CTEST_CONFIGURATION_TYPE}/TestIceTCompositePass
            --sobel
            -D ${PARAVIEW_DATA_ROOT}
            -T ${ParaView_BINARY_DIR}/Testing/Temporary
            -V Baseline/TestIceTCompositePassWithSobel.png
            ${VTK_MPI_POSTFLAGS})

  ADD_TEST(TestIceTCompositePassDepthOnly
            ${VTK_MPIRUN_EXE} ${VTK_MPI_PRENUMPROC_FLAGS} ${VTK_MPI_NUMPROC_FLAG} 2 ${VTK_MPI_PREFLAGS}
            ${CXX_TEST_PATH}/\${CTEST_CONFIGURATION_TYPE}/TestIceTCompositePass
            --tdx 2
            --depth-only
            -D ${PARAVIEW_DATA_ROOT}
            -T ${ParaView_BINARY_DIR}/Testing/Temporary
            -V Baseline/TestIceTCompositePassDepthOnly.png
            ${VTK_MPI_POSTFLAGS})

  ADD_EXECUTABLE(TestSimpleIceTCompositePass TestSimpleIceTCompositePass.cxx)
  TARGET_LINK_LIBRARIES(TestSimpleIceTCompositePass vtkPVFilters)
  ADD_TEST(TestSimpleIceTCompositePass
            ${VTK_MPIRUN_EXE} ${VTK_MPI_PRENUMPROC_FLAGS} ${VTK_MPI_NUMPROC_FLAG} 2 ${VTK_MPI_PREFLAGS}
            ${CXX_TEST_PATH}/\${CTEST_CONFIGURATION_TYPE}/TestSimpleIceTCompositePass
            -D ${PARAVIEW_DATA_ROOT}
            -T ${ParaView_BINARY_DIR}/Testing/Temporary
            -V Baseline/TestSimpleIceTCompositePass.png
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
ENDIF()


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
