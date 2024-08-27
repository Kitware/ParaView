###############################################################################
#

ExternalData_Expand_Arguments(ParaViewData _
  # data files
  "DATA{${paraview_test_data_directory_input}/Data/office.binary.vtk}"
  "DATA{${paraview_test_data_directory_input}/Data/SurfaceLIC-OfficeContour.pvsm}"
  "DATA{${paraview_test_data_directory_input}/Data/SurfaceLIC-OfficeHSlice.pvsm}"
  "DATA{${paraview_test_data_directory_input}/Data/SurfaceLIC-OfficeSlices.pvsm}"
  "DATA{${paraview_test_data_directory_input}/Data/SurfaceLIC-OfficeVSlice.pvsm}"
  "DATA{${paraview_test_data_directory_input}/Data/shuttle-surf.vtm}"
  "DATA{${paraview_test_data_directory_input}/Data/shuttle-surf/shuttle-surf_0_0.vtp}"
  "DATA{${paraview_test_data_directory_input}/Data/shuttle-surf/shuttle-surf_1_0.vtp}"
  "DATA{${paraview_test_data_directory_input}/Data/shuttle-surf/shuttle-surf_2_0.vtp}"
  "DATA{${paraview_test_data_directory_input}/Data/shuttle-surf/shuttle-surf_3_0.vtp}"
  "DATA{${paraview_test_data_directory_input}/Data/shuttle-surf/shuttle-surf_4_0.vtp}"
  "DATA{${paraview_test_data_directory_input}/Data/shuttle-surf/shuttle-surf_5_0.vtp}"
  "DATA{${paraview_test_data_directory_input}/Data/shuttle-surf/shuttle-surf_6_0.vtp}"
  "DATA{${paraview_test_data_directory_input}/Data/shuttle-surf/shuttle-surf_7_0.vtp}"

  # baselines
  "DATA{${CMAKE_CURRENT_SOURCE_DIR}/../Data/Baseline/SurfaceLIC-OfficeContour.png}"
  "DATA{${CMAKE_CURRENT_SOURCE_DIR}/../Data/Baseline/SurfaceLIC-OfficeContour_1.png}"
  "DATA{${CMAKE_CURRENT_SOURCE_DIR}/../Data/Baseline/SurfaceLIC-OfficeHSlice.png}"
  "DATA{${CMAKE_CURRENT_SOURCE_DIR}/../Data/Baseline/SurfaceLIC-OfficeVSlice.png}")

# Add python script names here.
set(PY_TESTS
  LockScalarRangeBackwardsCompatibility.py,NO_VALID
  SpreadSheetViewBlockNames.py,NO_VALID
  SpreadSheetViewPartialArrays.py,NO_VALID
  SpreadSheetViewSortByList.py,NO_VALID
  TransferFunctionPresets.py,NO_VALID
  TestSurfaceLIC.py,NO_VALID
  RenderViewOSPRayParameters.py,NO_VALID
  )

if (PARAVIEW_USE_MPI)
  # run the tests in parallel
  set(NP 3)
  set(VTK_MPIRUN_EXE mpiexec)
  # these depend on the D3 filter and
  # can't run serially
  set(PVBATCH_TESTS
    SurfaceLIC-OfficeHSlice
    SurfaceLIC-OfficeVSlice
    SurfaceLIC-OfficeSlices
    SurfaceLIC-OfficeContour)
  foreach (tfile ${PVBATCH_TESTS})
    ExternalData_add_test(ParaViewData
      NAME ${tfile}-Batch
      COMMAND ${MPIEXEC_EXECUTABLE} ${MPIEXEC_NUMPROC_FLAG} ${NP} ${MPIEXEC_PREFLAGS}
        $<TARGET_FILE:pvbatch>
        --enable-bt
        ${CMAKE_CURRENT_SOURCE_DIR}/SurfaceLICTestDriver.py
        -D ${CMAKE_BINARY_DIR}/ExternalData/Testing/Data
        -T ${CMAKE_BINARY_DIR}/Testing/Temporary
        -V DATA{${CMAKE_CURRENT_SOURCE_DIR}/../Data/Baseline/${tfile}.png}
        --threshold 30
        --state ${CMAKE_BINARY_DIR}/ExternalData/Testing/Data/${tfile}.pvsm)
  endforeach()
else()
  # run the test serially
  foreach (tfile ${PVBATCH_TESTS})
    ExternalData_add_test(ParaViewData
      NAME ${tfile}-Batch
      COMMAND $<TARGET_FILE:pvbatch>
        --enable-bt
        ${CMAKE_CURRENT_SOURCE_DIR}/SurfaceLICTestDriver.py
        -D ${CMAKE_BINARY_DIR}/ExternalData/Testing/Data
        -T ${CMAKE_BINARY_DIR}/Testing/Temporary
        -V DATA{${CMAKE_CURRENT_SOURCE_DIR}/../Data/Baseline/${tfile}.png}
        --threshold 30
        --state DATA{${CMAKE_CURRENT_SOURCE_DIR}/../Data/${tfile}.pvsm})
  endforeach()
endif()

paraview_add_test_python(
  JUST_VALID
  ${PY_TESTS}
  )

paraview_add_test_python(
  NO_RT
  PolarAxesAutoPoleOffTranslation.py
)
