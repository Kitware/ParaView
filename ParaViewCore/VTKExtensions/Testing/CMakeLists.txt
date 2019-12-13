INCLUDE_DIRECTORIES(
  ${ParaView_SOURCE_DIR}/VTK/Common/Testing/Cxx/
  ${ParaView_SOURCE_DIR}/VTK/Rendering/Testing/Cxx/
  )

set(vtk-module VTKExtensions)
set(${vtk-module}_TEST_LABELS PARAVIEW)

vtk_add_test_cxx(${vtk-modules}ServerFilterTests tests
  NO_VALID NO_OUTPUT
  ParaViewCoreVTKExtensionsPrintSelf.cxx,NO_DATA
  TestExtractHistogram.cxx,NO_DATA
  TestExtractScatterPlot.cxx,NO_DATA
  TestTilesHelper.cxx,NO_DATA
  TestSortingTable.cxx,NO_DATA
  TestContinuousClose3D.cxx
  TestPVFilters.cxx
  TestSpyPlotTracers.cxx
  TestPVAMRDualContour.cxx
  )

if (PARAVIEW_USE_MPI)
  set_property(SOURCE ParaViewCoreVTKExtensionsPrintSelf.cxx APPEND
    PROPERTY
      COMPILE_DEFINITIONS PARAVIEW_USE_MPI
                          PARAVIEW_USE_ICE_T)
endif ()

vtk_test_cxx_executable(${vtk-modules}ServerFilterTests tests)
target_link_libraries(${vtk-modules}ServerFilterTests
  vtkPVVTKExtensions
  vtkImagingMorphological
  vtkInteractionImage)

set (_MPI_TEST_PATH ${CXX_TEST_PATH})
# CMAKE_CONFIGURATION_TYPES is set for generators that support multiple
# configurations e.g. Visual Studio. In that case we update the _MPI_TEST_PATH to
# include the configuration type (CTEST_CONFIGURATION_TYPE)
  if (CMAKE_CONFIGURATION_TYPES)
set (_MPI_TEST_PATH ${CXX_TEST_PATH}/\${CTEST_CONFIGURATION_TYPE})
endif ()


IF(PARAVIEW_USE_MPI)
  ADD_EXECUTABLE(TestIceTCompositePass TestIceTCompositePass.cxx)
  TARGET_LINK_LIBRARIES(TestIceTCompositePass vtkPVVTKExtensions)

  ExternalData_add_test("${_vtk_build_TEST_DATA_TARGET}"
    NAME    TestIceTCompositePassWithBlurAndOrderedCompositing
    COMMAND TestIceTCompositePassWithBlurAndOrderedCompositing
            ${VTK_MPIRUN_EXE} ${VTK_MPI_PRENUMPROC_FLAGS} ${VTK_MPI_NUMPROC_FLAG} 2 ${VTK_MPI_PREFLAGS}
            ${_MPI_TEST_PATH}/TestIceTCompositePass
            --tdx 2
            --blur
            --use-ordered-compositing
            --use-depth-peeling
            -D ${PARAVIEW_TEST_OUTPUT_DATA_DIR}
            -T ${PARAVIEW_TEST_OUTPUT_DIR}
            -V DATA{${PARAVIEW_TEST_BASELINE_DIR}/TestIceTCompositePassWithBlurAndOrderedCompositing.png}
            ${VTK_MPI_POSTFLAGS})

  ExternalData_add_test("${_vtk_build_TEST_DATA_TARGET}"
    NAME    TestIceTCompositePassWithSobel
    COMMAND TestIceTCompositePassWithSobel
            ${VTK_MPIRUN_EXE} ${VTK_MPI_PRENUMPROC_FLAGS} ${VTK_MPI_NUMPROC_FLAG} 2 ${VTK_MPI_PREFLAGS}
            ${_MPI_TEST_PATH}/TestIceTCompositePass
            --sobel
            -D ${PARAVIEW_TEST_OUTPUT_DATA_DIR}
            -T ${PARAVIEW_TEST_OUTPUT_DIR}
            -V DATA{${PARAVIEW_TEST_BASELINE_DIR}/TestIceTCompositePassWithSobel.png}
            ${VTK_MPI_POSTFLAGS})

  ExternalData_add_test("${_vtk_build_TEST_DATA_TARGET}"
    NAME    TestIceTCompositePassDepthOnly
    COMMAND TestIceTCompositePassDepthOnly
            ${VTK_MPIRUN_EXE} ${VTK_MPI_PRENUMPROC_FLAGS} ${VTK_MPI_NUMPROC_FLAG} 2 ${VTK_MPI_PREFLAGS}
            ${_MPI_TEST_PATH}/TestIceTCompositePass
            --tdx 2
            --depth-only
            -D ${PARAVIEW_TEST_OUTPUT_DATA_DIR}
            -T ${PARAVIEW_TEST_OUTPUT_DIR}
            -V DATA{${PARAVIEW_TEST_BASELINE_DIR}/TestIceTCompositePassDepthOnly.png}
            ${VTK_MPI_POSTFLAGS})

  ADD_EXECUTABLE(TestSimpleIceTCompositePass TestSimpleIceTCompositePass.cxx)
  TARGET_LINK_LIBRARIES(TestSimpleIceTCompositePass vtkPVVTKExtensions)

  ExternalData_add_test("${_vtk_build_TEST_DATA_TARGET}"
    NAME    TestSimpleIceTCompositePass
    COMMAND TestSimpleIceTCompositePass
            ${VTK_MPIRUN_EXE} ${VTK_MPI_PRENUMPROC_FLAGS} ${VTK_MPI_NUMPROC_FLAG} 2 ${VTK_MPI_PREFLAGS}
            ${_MPI_TEST_PATH}/TestSimpleIceTCompositePass
            -D ${PARAVIEW_TEST_OUTPUT_DATA_DIR}
            -T ${PARAVIEW_TEST_OUTPUT_DIR}
            -V DATA{${PARAVIEW_TEST_BASELINE_DIR}/TestSimpleIceTCompositePass.png}
            ${VTK_MPI_POSTFLAGS})

  ADD_EXECUTABLE(TestIceTShadowMapPass TestIceTShadowMapPass.cxx)
  TARGET_LINK_LIBRARIES(TestIceTShadowMapPass vtkPVVTKExtensions)

  ExternalData_add_test("${_vtk_build_TEST_DATA_TARGET}"
    NAME    TestIceTShadowMapPass-image
    COMMAND TestIceTShadowMapPass-image
            ${VTK_MPIRUN_EXE} ${VTK_MPI_PRENUMPROC_FLAGS} ${VTK_MPI_NUMPROC_FLAG} 2 ${VTK_MPI_PREFLAGS}
            ${_MPI_TEST_PATH}/TestIceTShadowMapPass
            --tdx 2
            -D ${PARAVIEW_TEST_OUTPUT_DATA_DIR}
            -T ${PARAVIEW_TEST_OUTPUT_DIR}
            -V DATA{${PARAVIEW_TEST_BASELINE_DIR}/TestIceTShadowMapPass.png}
            ${VTK_MPI_POSTFLAGS})

  set_tests_properties(
    TestIceTCompositePassWithBlurAndOrderedCompositing
    TestIceTCompositePassWithSobel
    TestIceTCompositePassDepthOnly
    TestSimpleIceTCompositePass
    TestIceTShadowMapPass-image
    PROPERTIES LABELS "PARAVIEW")
ENDIF()


if(Module_vtkRenderingOpenGL2)
  vtk_add_test_cxx(${vtk-module}CxxTests-Rendering tests
    TestPVAMRDualClip.cxx
    )
  paraview_test_cxx_executable(${vtk-module}CxxTest-Renderings tests)
  target_link_libraries(${vtk-module}CxxTest-Renderings vtkPVVTKExtensions)
endif()

IF (PARAVIEW_USE_MPI)
    IF (MPI_LIBRARY)
      SET(MPI_LIBRARIES ${MPI_LIBRARY})
    ENDIF ()
    IF (MPI_EXTRA_LIBRARY)
      SET(MPI_LIBRARIES ${MPI_LIBRARIES} "${MPI_EXTRA_LIBRARY}")
    ENDIF ()

    ADD_EXECUTABLE(DistributedSubsetSortingTable DistributedSubsetSortingTable.cxx)
    TARGET_LINK_LIBRARIES(DistributedSubsetSortingTable vtkParallelMPI vtkPVVTKExtensions)

    ExternalData_add_test("${_vtk_build_TEST_DATA_TARGET}"
      NAME    TestDistributedSubsetSortingTable
      COMMAND TestDistributedSubsetSortingTable
              ${VTK_MPIRUN_EXE} ${VTK_MPI_PRENUMPROC_FLAGS} ${VTK_MPI_NUMPROC_FLAG} 2 ${VTK_MPI_PREFLAGS}
              ${_MPI_TEST_PATH}/DistributedSubsetSortingTable
              -D ${VTK_TEST_DATA_DIR}
              -T ${PARAVIEW_TEST_OUTPUT_DIR}
              ${VTK_MPI_POSTFLAGS})
    set_tests_properties(
      TestDistributedSubsetSortingTable PROPERTIES LABELS "PARAVIEW")
ENDIF ()
