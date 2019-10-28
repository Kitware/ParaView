include_directories(
  ${TestDriver_SOURCE_DIR})

function (_set_standard_test_properties test_name)
  set(_vtk_fail_regex "(\n|^)ERROR: " "instance(s)? still around")
  set_tests_properties("${test_name}"
    PROPERTIES
      LABELS "${_vtk_build_test_labels}"
      FAIL_REGULAR_EXPRESSION "${_vtk_fail_regex}"
      # This must match VTK_SKIP_RETURN_CODE in vtkTestingObjectFactory.h"
      SKIP_RETURN_CODE 125
    )
endfunction ()

#------------------------------------------------------------------------------
vtk_module_test_executable(CoProcessingPythonScriptExample
  PythonScriptCoProcessingExample.cxx
  vtkPVCustomTestDriver.cxx)

#------------------------------------------------------------------------------
# Generate the image compare tool.
# below is for doing image comparisons
vtk_module_test_executable(CoProcessingCompareImagesTester CompareImages.cxx)

#------------------------------------------------------------------------------
# a simple test to see if the input is changing, i.e. that the initial
# pipeline is having it's trivial producer updated with a new grid
add_test(NAME CoProcessingTestInput
  COMMAND pvbatch -sym ${CMAKE_CURRENT_SOURCE_DIR}/waveletdriver.py
  ${CMAKE_CURRENT_SOURCE_DIR}/TestInput.py 4
  )
_set_standard_test_properties(CoProcessingTestInput)



# the CoProcessingTestPythonScript needs to be run with ${MPIEXEC_EXECUTABLE} if
# CoProcessingPythonScriptExample was built with MPI because certain
# machines only allow running MPI programs with the proper ${MPIEXEC_EXECUTABLE}

# just test if a given Catalyst Python script can import another script
if (NOT PARAVIEW_USE_MPI)
  add_test(NAME CoProcessingImport
    COMMAND ${CMAKE_COMMAND}
    -DCOPROCESSING_TEST_DRIVER:FILEPATH=$<TARGET_FILE:CoProcessingPythonScriptExample>
    -DCOPROCESSING_TEST_DIR:PATH=${CMAKE_BINARY_DIR}/Testing/Temporary
    -DCOPROCESSING_TEST_SCRIPT=${CMAKE_CURRENT_SOURCE_DIR}/cpimport.py
    -P ${CMAKE_CURRENT_SOURCE_DIR}/CoProcessingTestPythonScript.cmake)
else()
  add_test(NAME CoProcessingImport
    COMMAND ${CMAKE_COMMAND}
    -DCOPROCESSING_TEST_DRIVER:FILEPATH=$<TARGET_FILE:CoProcessingPythonScriptExample>
    -DCOPROCESSING_TEST_DIR:PATH=${CMAKE_BINARY_DIR}/Testing/Temporary
    -DCOPROCESSING_TEST_SCRIPT=${CMAKE_CURRENT_SOURCE_DIR}/cpimport.py
    -DUSE_MPI:BOOL=TRUE
    -DMPIEXEC:FILEPATH=${MPIEXEC_EXECUTABLE}
    -DMPIEXEC_NUMPROC_FLAG:STRING=${MPIEXEC_NUMPROC_FLAG}
    -DMPIEXEC_NUMPROCS=2
    -DMPIEXEC_PREFLAGS:STRING=${MPIEXEC_PREFLAGS}
    -DVTK_MPI_POSTFLAGS:STRING=${VTK_MPI_POSTFLAGS}
    -P ${CMAKE_CURRENT_SOURCE_DIR}/CoProcessingTestPythonScript.cmake)
endif()

set_tests_properties(CoProcessingImport PROPERTIES PASS_REGULAR_EXPRESSION "in DoCoProcessing")
_set_standard_test_properties(CoProcessingImport)

# test if we can use a Python programmable filter in a Catalyst Python script
if (NOT PARAVIEW_USE_MPI)
  add_test(NAME CoProcessingProgrammableFilter
    COMMAND ${CMAKE_COMMAND}
    -DCOPROCESSING_TEST_DRIVER:FILEPATH=$<TARGET_FILE:CoProcessingPythonScriptExample>
    -DCOPROCESSING_TEST_DIR:PATH=${CMAKE_BINARY_DIR}/Testing/Temporary
    -DCOPROCESSING_TEST_SCRIPT=${CMAKE_CURRENT_SOURCE_DIR}/cpprogrammablefilter.py
    -P ${CMAKE_CURRENT_SOURCE_DIR}/CoProcessingTestPythonScript.cmake)
else()
  add_test(NAME CoProcessingProgrammableFilter
    COMMAND ${CMAKE_COMMAND}
    -DCOPROCESSING_TEST_DRIVER:FILEPATH=$<TARGET_FILE:CoProcessingPythonScriptExample>
    -DCOPROCESSING_TEST_DIR:PATH=${CMAKE_BINARY_DIR}/Testing/Temporary
    -DCOPROCESSING_TEST_SCRIPT=${CMAKE_CURRENT_SOURCE_DIR}/cpprogrammablefilter.py
    -DUSE_MPI:BOOL=TRUE
    -DMPIEXEC:FILEPATH=${MPIEXEC_EXECUTABLE}
    -DMPIEXEC_NUMPROC_FLAG:STRING=${MPIEXEC_NUMPROC_FLAG}
    -DMPIEXEC_NUMPROCS=2
    -DMPIEXEC_PREFLAGS:STRING=${MPIEXEC_PREFLAGS}
    -DVTK_MPI_POSTFLAGS:STRING=${VTK_MPI_POSTFLAGS}
    -P ${CMAKE_CURRENT_SOURCE_DIR}/CoProcessingTestPythonScript.cmake)
endif()

set_tests_properties(CoProcessingProgrammableFilter PROPERTIES PASS_REGULAR_EXPRESSION "in DoCoProcessing")
_set_standard_test_properties(CoProcessingProgrammableFilter)

#-  add_test(NAME CoProcessingProgrammableFilter
#-    COMMAND CoProcessingProgrammableFilter
#-    ${CMAKE_CURRENT_SOURCE_DIR}/cpprogrammablefilter.py)

if (NOT PARAVIEW_USE_MPI)
  add_test(NAME CoProcessingTestPythonScript
    COMMAND ${CMAKE_COMMAND}
    -DCOPROCESSING_TEST_DRIVER:FILEPATH=$<TARGET_FILE:CoProcessingPythonScriptExample>
    -DCOPROCESSING_TEST_DIR:PATH=${CMAKE_BINARY_DIR}/Testing/Temporary
    -DCOPROCESSING_IMAGE_TESTER:FILEPATH=$<TARGET_FILE:CoProcessingCompareImagesTester>
    -DCOPROCESSING_DATA_DIR:PATH=${paraview_test_data_directory_output}/Testing/Data/Baseline
    -DCOPROCESSING_TEST_SCRIPT=${CMAKE_CURRENT_SOURCE_DIR}/PythonScriptTest.py
    -P ${CMAKE_CURRENT_SOURCE_DIR}/CoProcessingTestPythonScript.cmake)
else()
  add_test(NAME CoProcessingTestPythonScript
    COMMAND ${CMAKE_COMMAND}
    -DCOPROCESSING_TEST_DRIVER:FILEPATH=$<TARGET_FILE:CoProcessingPythonScriptExample>
    -DCOPROCESSING_TEST_DIR:PATH=${CMAKE_BINARY_DIR}/Testing/Temporary
    -DCOPROCESSING_IMAGE_TESTER:FILEPATH=$<TARGET_FILE:CoProcessingCompareImagesTester>
    -DCOPROCESSING_DATA_DIR:PATH=${paraview_test_data_directory_output}/Testing/Data/Baseline
    -DCOPROCESSING_TEST_SCRIPT=${CMAKE_CURRENT_SOURCE_DIR}/PythonScriptTest.py
    -DUSE_MPI:BOOL=TRUE
    -DMPIEXEC:FILEPATH=${MPIEXEC_EXECUTABLE}
    -DMPIEXEC_NUMPROC_FLAG:STRING=${MPIEXEC_NUMPROC_FLAG}
    -DMPIEXEC_NUMPROCS=1
    -DMPIEXEC_PREFLAGS:STRING=${MPIEXEC_PREFLAGS}
    -DVTK_MPI_POSTFLAGS:STRING=${VTK_MPI_POSTFLAGS}
    -P ${CMAKE_CURRENT_SOURCE_DIR}/CoProcessingTestPythonScript.cmake)
endif()

set_tests_properties(CoProcessingTestPythonScript PROPERTIES PASS_REGULAR_EXPRESSION "in DoCoProcessing")
_set_standard_test_properties(CoProcessingTestPythonScript)

if (PARAVIEW_USE_MPI)
  add_test(NAME PCoProcessingTestPythonScript
          COMMAND ${CMAKE_COMMAND}
          -DCOPROCESSING_TEST_DRIVER:FILEPATH=$<TARGET_FILE:CoProcessingPythonScriptExample>
          -DCOPROCESSING_TEST_DIR:PATH=${CMAKE_BINARY_DIR}/Testing/Temporary
          -DCOPROCESSING_IMAGE_TESTER:FILEPATH=$<TARGET_FILE:CoProcessingCompareImagesTester>
          -DCOPROCESSING_DATA_DIR:PATH=${paraview_test_data_directory_output}/Testing/Data/Baseline
          -DCOPROCESSING_TEST_SCRIPT=${CMAKE_CURRENT_SOURCE_DIR}/PythonScriptTest.py
          -DUSE_MPI:BOOL=TRUE
          -DMPIEXEC:FILEPATH=${MPIEXEC_EXECUTABLE}
          -DMPIEXEC_NUMPROC_FLAG:STRING=${MPIEXEC_NUMPROC_FLAG}
          -DMPIEXEC_NUMPROCS=2
          -DMPIEXEC_PREFLAGS:STRING=${MPIEXEC_PREFLAGS}
          -DVTK_MPI_POSTFLAGS:STRING=${VTK_MPI_POSTFLAGS}
          -P ${CMAKE_CURRENT_SOURCE_DIR}/CoProcessingTestPythonScript.cmake)
  # both the parallel and the serial tests create the same output images
  # so we run this one serially so that they don't interfere.
  set_tests_properties(PCoProcessingTestPythonScript PROPERTIES RUN_SERIAL ON)
  set_tests_properties(PCoProcessingTestPythonScript PROPERTIES PASS_REGULAR_EXPRESSION "in DoCoProcessing")
  _set_standard_test_properties(PCoProcessingTestPythonScript)
endif()

if (PARAVIEW_BUILD_QT_GUI)
  # for now the full workflow test only works with the shared lib builds of
  # paraview since the static build will load all of the plugins automatically
  # which will add in extra properties to objects in the created script
  # which can't be used when running the 'simulation' since those same
  # plugins won't be loaded then.
  IF ( BUILD_SHARED_LIBS )
    ExternalData_add_test("${_vtk_build_TEST_DATA_TARGET}" NAME CoProcessingFullWorkflow
      COMMAND ${CMAKE_COMMAND}
      -DPARAVIEW_EXECUTABLE:FILEPATH=$<TARGET_FILE:ParaView::paraview>
      -DCOPROCESSING_TEST_DIR:PATH=${CMAKE_BINARY_DIR}/Testing/Temporary
      -DPARAVIEW_TEST_XML:FILEPATH=${ParaView_SOURCE_DIR}/Applications/ParaView/Testing/XML/CatalystGUI.xml
      -DPVBATCH_EXECUTABLE:FILEPATH=$<TARGET_FILE:ParaView::pvbatch>
      -DCOPROCESSING_DRIVER_SCRIPT:FILEPATH=${CMAKE_CURRENT_SOURCE_DIR}/waveletdriver.py
      -DCOPROCESSING_IMAGE_TESTER:FILEPATH=$<TARGET_FILE:CoProcessingCompareImagesTester>
      -DCOPROCESSING_DATA_DIR:PATH=${paraview_test_data_directory_output}/Testing/Data/Baseline
      -DCOPROCESSING_OUTPUTCHECK_SCRIPT:FILEPATH=${CMAKE_CURRENT_SOURCE_DIR}/outputcheck.py
      -DTEST_NAME:STRING=CoProcessingFullWorkflow
      -P ${CMAKE_CURRENT_SOURCE_DIR}/CoProcessingFullWorkflowTest.cmake
      DATA{${_vtk_build_TEST_INPUT_DATA_DIRECTORY}/Data/Baseline/CPFullWorkflow.png})
    set_tests_properties(CoProcessingFullWorkflow PROPERTIES RUN_SERIAL ON)
    _set_standard_test_properties(CoProcessingFullWorkflow)

    ExternalData_add_test("${_vtk_build_TEST_DATA_TARGET}" NAME CoProcessingFullWorkflowWithPlots
      COMMAND ${CMAKE_COMMAND}
      -DPARAVIEW_EXECUTABLE:FILEPATH=$<TARGET_FILE:ParaView::paraview>
      -DCOPROCESSING_TEST_DIR:PATH=${CMAKE_BINARY_DIR}/Testing/Temporary
      -DPARAVIEW_TEST_XML:FILEPATH=${ParaView_SOURCE_DIR}/Applications/ParaView/Testing/XML/CatalystGUIPlots.xml
      -DPVBATCH_EXECUTABLE:FILEPATH=$<TARGET_FILE:ParaView::pvbatch>
      -DCOPROCESSING_DRIVER_SCRIPT:FILEPATH=${CMAKE_CURRENT_SOURCE_DIR}/waveletdriver.py
      -DCOPROCESSING_IMAGE_TESTER:FILEPATH=$<TARGET_FILE:CoProcessingCompareImagesTester>
      -DCOPROCESSING_DATA_DIR:PATH=${paraview_test_data_directory_output}/Testing/Data/Baseline
      -DCOPROCESSING_OUTPUTCHECK_SCRIPT:FILEPATH=${CMAKE_CURRENT_SOURCE_DIR}/outputcheck.py
      -DTEST_NAME:STRING=CoProcessingFullWorkflowWithPlots
      -P ${CMAKE_CURRENT_SOURCE_DIR}/CoProcessingFullWorkflowTest.cmake
      DATA{${_vtk_build_TEST_INPUT_DATA_DIRECTORY}/Data/Baseline/CPFullWorkflowPlot1.png}
      DATA{${_vtk_build_TEST_INPUT_DATA_DIRECTORY}/Data/Baseline/CPFullWorkflowPlot2.png} )
    set_tests_properties(CoProcessingFullWorkflowWithPlots PROPERTIES RUN_SERIAL ON)
    _set_standard_test_properties(CoProcessingFullWorkflowWithPlots)

    find_python_module(numpy numpy_found)
    if (numpy_found AND TARGET ParaView::CinemaReader)
      add_test(NAME CoProcessingFullWorkflowCinema
        COMMAND ${CMAKE_COMMAND}
        -DPARAVIEW_EXECUTABLE:FILEPATH=$<TARGET_FILE:ParaView::paraview>
        -DCOPROCESSING_TEST_DIR:PATH=${CMAKE_BINARY_DIR}/Testing/Temporary
        -DPARAVIEW_TEST_XML:FILEPATH=${ParaView_SOURCE_DIR}/Applications/ParaView/Testing/XML/CatalystCinemaGUI.xml
        -DPVBATCH_EXECUTABLE:FILEPATH=$<TARGET_FILE:ParaView::pvbatch>
        -DCOPROCESSING_DRIVER_SCRIPT:FILEPATH=${CMAKE_CURRENT_SOURCE_DIR}/waveletdriver.py
        -DTEST_NAME:STRING=CoProcessingFullWorkflowCinema
        -P ${CMAKE_CURRENT_SOURCE_DIR}/CoProcessingFullWorkflowTest.cmake)
      set_tests_properties(CoProcessingFullWorkflowCinema PROPERTIES RUN_SERIAL ON)
      _set_standard_test_properties(CoProcessingFullWorkflowCinema)

      add_test(NAME CoProcessingFullWorkflowCinemaFilters
        COMMAND ${CMAKE_COMMAND}
        -DPARAVIEW_EXECUTABLE:FILEPATH=$<TARGET_FILE:ParaView::paraview>
        -DCOPROCESSING_TEST_DIR:PATH=${CMAKE_BINARY_DIR}/Testing/Temporary
        -DPARAVIEW_TEST_XML:FILEPATH=${ParaView_SOURCE_DIR}/Applications/ParaView/Testing/XML/CatalystCinemaFiltersGUI.xml
        -DPVBATCH_EXECUTABLE:FILEPATH=$<TARGET_FILE:ParaView::pvbatch>
        -DCOPROCESSING_DRIVER_SCRIPT:FILEPATH=${CMAKE_CURRENT_SOURCE_DIR}/waveletdriver.py
        -DTEST_NAME:STRING=CoProcessingFullWorkflowCinemaFilters
        -P ${CMAKE_CURRENT_SOURCE_DIR}/CoProcessingFullWorkflowTest.cmake)
      set_tests_properties(CoProcessingFullWorkflowCinemaFilters PROPERTIES RUN_SERIAL ON)
      _set_standard_test_properties(CoProcessingFullWorkflowCinemaFilters)

      add_test(NAME CoProcessingFullWorkflowCinemaComposite
        COMMAND ${CMAKE_COMMAND}
        -DPARAVIEW_EXECUTABLE:FILEPATH=$<TARGET_FILE:ParaView::paraview>
        -DCOPROCESSING_TEST_DIR:PATH=${CMAKE_BINARY_DIR}/Testing/Temporary
        -DPARAVIEW_TEST_XML:FILEPATH=${ParaView_SOURCE_DIR}/Applications/ParaView/Testing/XML/CatalystCinemaCompositeGUI.xml
        -DPVBATCH_EXECUTABLE:FILEPATH=$<TARGET_FILE:ParaView::pvbatch>
        -DCOPROCESSING_DRIVER_SCRIPT:FILEPATH=${CMAKE_CURRENT_SOURCE_DIR}/waveletdriver.py
        -DTEST_NAME:STRING=CoProcessingFullWorkflowCinemaComposite
        -P ${CMAKE_CURRENT_SOURCE_DIR}/CoProcessingFullWorkflowTest.cmake)
      set_tests_properties(CoProcessingFullWorkflowCinemaComposite PROPERTIES RUN_SERIAL ON)
      _set_standard_test_properties(CoProcessingFullWorkflowCinemaComposite)

      # temporarily disabling until VTK level change removes INVERTIBLE_LUT
      #
      #add_test(NAME CoProcessingFullWorkflowCinemaRecolorable
      #  COMMAND ${CMAKE_COMMAND}
      #  -DPARAVIEW_EXECUTABLE:FILEPATH=$<TARGET_FILE:ParaView::paraview>
      #  -DCOPROCESSING_TEST_DIR:PATH=${CMAKE_BINARY_DIR}/Testing/Temporary
      #  -DPARAVIEW_TEST_XML:FILEPATH=${ParaView_SOURCE_DIR}/Applications/ParaView/Testing/XML/CatalystCinemaRecolorableGUI.xml
      #  -DPVBATCH_EXECUTABLE:FILEPATH=$<TARGET_FILE:ParaView::pvbatch>
      #  -DCOPROCESSING_DRIVER_SCRIPT:FILEPATH=${CMAKE_CURRENT_SOURCE_DIR}/waveletdriver.py
      #  -DTEST_NAME:STRING=CoProcessingFullWorkflowCinemaRecolorable
      #  -P ${CMAKE_CURRENT_SOURCE_DIR}/CoProcessingFullWorkflowTest.cmake)
      #set_tests_properties(CoProcessingFullWorkflowCinemaRecolorable PROPERTIES RUN_SERIAL ON)
      #_set_standard_test_properties(CoProcessingFullWorkflowCinemaRecolorable)

      add_test(NAME CoProcessingFullWorkflowCinemaPose
        COMMAND ${CMAKE_COMMAND}
        -DPARAVIEW_EXECUTABLE:FILEPATH=$<TARGET_FILE:ParaView::paraview>
        -DCOPROCESSING_TEST_DIR:PATH=${CMAKE_BINARY_DIR}/Testing/Temporary
        -DPARAVIEW_TEST_XML:FILEPATH=${ParaView_SOURCE_DIR}/Applications/ParaView/Testing/XML/CatalystCinemaPoseGUI.xml
        -DPVBATCH_EXECUTABLE:FILEPATH=$<TARGET_FILE:ParaView::pvbatch>
        -DCOPROCESSING_DRIVER_SCRIPT:FILEPATH=${CMAKE_CURRENT_SOURCE_DIR}/waveletdriver.py
        -DTEST_NAME:STRING=CoProcessingFullWorkflowCinemaPose
        -P ${CMAKE_CURRENT_SOURCE_DIR}/CoProcessingFullWorkflowTest.cmake)
      set_tests_properties(CoProcessingFullWorkflowCinemaPose PROPERTIES RUN_SERIAL ON)
      _set_standard_test_properties(CoProcessingFullWorkflowCinemaPose)

    endif()

    add_test(NAME TemporalScriptFullWorkflow
      COMMAND ${CMAKE_COMMAND}
      -DPARAVIEW_EXECUTABLE:FILEPATH=$<TARGET_FILE:ParaView::paraview>
      -DCOPROCESSING_TEST_DIR:PATH=${CMAKE_BINARY_DIR}/Testing/Temporary
      -DCOPROCESSING_DATA_DIR:PATH=${paraview_test_data_directory_output}
      -DPARAVIEW_TEST_XML:FILEPATH=${ParaView_SOURCE_DIR}/Applications/ParaView/Testing/XML/TemporalExporterGUI.xml
      -DPVBATCH_EXECUTABLE:FILEPATH=$<TARGET_FILE:ParaView::pvbatch>
      -DCOPROCESSING_DRIVER_SCRIPT:FILEPATH=${CMAKE_BINARY_DIR}/Testing/Temporary/temporalexport.py
      -DTEST_NAME:STRING=TemporalScriptFullWorkflow
      -P ${CMAKE_CURRENT_SOURCE_DIR}/CoProcessingFullWorkflowTest.cmake)
    set_tests_properties(TemporalScriptFullWorkflow PROPERTIES RUN_SERIAL ON)
    _set_standard_test_properties(TemporalScriptFullWorkflow)

    add_test(NAME ExportNow
      COMMAND ${CMAKE_COMMAND}
      -DPARAVIEW_EXECUTABLE:FILEPATH=$<TARGET_FILE:ParaView::paraview>
      -DCOPROCESSING_TEST_DIR:PATH=${CMAKE_BINARY_DIR}/Testing/Temporary
      -DPARAVIEW_TEST_XML:FILEPATH=${ParaView_SOURCE_DIR}/Applications/ParaView/Testing/XML/ExportNowGUI.xml
      -DPVBATCH_EXECUTABLE:FILEPATH=$<TARGET_FILE:ParaView::pvbatch>
      -DTEST_NAME:STRING=ExportNow
      -P ${CMAKE_CURRENT_SOURCE_DIR}/CoProcessingFullWorkflowTest.cmake)
    set_tests_properties(ExportNow PROPERTIES RUN_SERIAL ON)
    _set_standard_test_properties(ExportNow)

    if (NOT PARAVIEW_USE_MPI)
      add_test(NAME CinemaExportGeometry
        COMMAND ${CMAKE_COMMAND}
        -DPARAVIEW_EXECUTABLE:FILEPATH=$<TARGET_FILE:ParaView::paraview>
        -DCOPROCESSING_TEST_DIR:PATH=${CMAKE_BINARY_DIR}/Testing/Temporary
        -DPARAVIEW_TEST_XML:FILEPATH=${ParaView_SOURCE_DIR}/Applications/ParaView/Testing/XML/CinemaExportGeometry.xml
        -DPVBATCH_EXECUTABLE:FILEPATH=$<TARGET_FILE:ParaView::pvbatch>
        -DPVPYTHON_EXECUTABLE:FILEPATH=$<TARGET_FILE:ParaView::pvpython>
        -DCOPROCESSING_DATA_DIR:PATH=${paraview_test_data_directory_output}
        -DCOPROCESSING_DRIVER_SCRIPT:FILEPATH=${CMAKE_SOURCE_DIR}/Examples/Catalyst/SampleScripts/filedriver.py
        -DCINEMA_DATABASE_TESTER:FILEPATH=${CMAKE_CURRENT_SOURCE_DIR}/CinemaExportCheck.py
        -DCINEMA_INPUT_DATA:FILEPATH=${CMAKE_BINARY_DIR}/ExternalData/Testing/Data/can.ex2
        -DCINEMA_BATCH_SCRIPT:FILEPATH=cinema/CinemaExportGeometry.py
        -DUSE_MPI:BOOL=FALSE
        -DTEST_NAME:STRING=CinemaExportGeometry
        -P ${CMAKE_CURRENT_SOURCE_DIR}/CinemaExportFullWorkflowTest.cmake)
      set_tests_properties(CinemaExportGeometry PROPERTIES RUN_SERIAL ON)
      _set_standard_test_properties(CinemaExportGeometry)
    
      if(NOT WIN32)
        add_test(NAME CinemaExportGeometryAndImages
          COMMAND ${CMAKE_COMMAND}
          -DPARAVIEW_EXECUTABLE:FILEPATH=$<TARGET_FILE:ParaView::paraview>
          -DCOPROCESSING_TEST_DIR:PATH=${CMAKE_BINARY_DIR}/Testing/Temporary
          -DPARAVIEW_TEST_XML:FILEPATH=${ParaView_SOURCE_DIR}/Applications/ParaView/Testing/XML/CinemaExportGeometryAndImages.xml
          -DPVBATCH_EXECUTABLE:FILEPATH=$<TARGET_FILE:ParaView::pvbatch>
          -DPVPYTHON_EXECUTABLE:FILEPATH=$<TARGET_FILE:ParaView::pvpython>
          -DCOPROCESSING_DATA_DIR:PATH=${paraview_test_data_directory_output}
          -DCOPROCESSING_DRIVER_SCRIPT:FILEPATH=${CMAKE_SOURCE_DIR}/Examples/Catalyst/SampleScripts/filedriver.py
          -DCINEMA_DATABASE_TESTER:FILEPATH=${CMAKE_CURRENT_SOURCE_DIR}/CinemaExportCheck.py
          -DCINEMA_INPUT_DATA:FILEPATH=${CMAKE_BINARY_DIR}/ExternalData/Testing/Data/can.ex2
          -DCINEMA_BATCH_SCRIPT:FILEPATH=cinema/CinemaExportGeometryAndImages.py
          -DUSE_MPI:BOOL=FALSE
          -DTEST_NAME:STRING=CinemaExportGeometryAndImages
          -P ${CMAKE_CURRENT_SOURCE_DIR}/CinemaExportFullWorkflowTest.cmake)
        set_tests_properties(CinemaExportGeometryAndImages PROPERTIES RUN_SERIAL ON)
        _set_standard_test_properties(CinemaExportGeometryAndImages)
    
        add_test(NAME CinemaExportNoTimesteps
          COMMAND ${CMAKE_COMMAND}
          -DPARAVIEW_EXECUTABLE:FILEPATH=$<TARGET_FILE:ParaView::paraview>
          -DCOPROCESSING_TEST_DIR:PATH=${CMAKE_BINARY_DIR}/Testing/Temporary
          -DPARAVIEW_TEST_XML:FILEPATH=${ParaView_SOURCE_DIR}/Applications/ParaView/Testing/XML/CinemaExportNoTimesteps.xml
          -DPVBATCH_EXECUTABLE:FILEPATH=$<TARGET_FILE:ParaView::pvbatch>
          -DPVPYTHON_EXECUTABLE:FILEPATH=$<TARGET_FILE:ParaView::pvpython>
          -DCOPROCESSING_DATA_DIR:PATH=${paraview_test_data_directory_output}
          -DCOPROCESSING_DRIVER_SCRIPT:FILEPATH=${CMAKE_SOURCE_DIR}/Examples/Catalyst/SampleScripts/filedriver.py
          -DCINEMA_DATABASE_TESTER:FILEPATH=${CMAKE_CURRENT_SOURCE_DIR}/CinemaExportCheck.py
          -DCINEMA_INPUT_DATA:FILEPATH=${CMAKE_BINARY_DIR}/ExternalData/Testing/Data/cube.vtu
          -DCINEMA_BATCH_SCRIPT:FILEPATH=cinema/CinemaExportNoTimesteps.py
          -DUSE_MPI:BOOL=FALSE
          -DTEST_NAME:STRING=CinemaExportNoTimesteps
          -P ${CMAKE_CURRENT_SOURCE_DIR}/CinemaExportFullWorkflowTest.cmake)
        set_tests_properties(CinemaExportNoTimesteps PROPERTIES RUN_SERIAL ON)
        _set_standard_test_properties(CinemaExportNoTimesteps)

        add_test(NAME CinemaExportFloatFiles
          COMMAND ${CMAKE_COMMAND}
          -DPARAVIEW_EXECUTABLE:FILEPATH=$<TARGET_FILE:ParaView::paraview>
          -DCOPROCESSING_TEST_DIR:PATH=${CMAKE_BINARY_DIR}/Testing/Temporary
          -DPARAVIEW_TEST_XML:FILEPATH=${ParaView_SOURCE_DIR}/Applications/ParaView/Testing/XML/CinemaExportFloatFiles.xml
          -DPVBATCH_EXECUTABLE:FILEPATH=$<TARGET_FILE:ParaView::pvbatch>
          -DPVPYTHON_EXECUTABLE:FILEPATH=$<TARGET_FILE:ParaView::pvpython>
          -DCOPROCESSING_DATA_DIR:PATH=${paraview_test_data_directory_output}
          -DCOPROCESSING_DRIVER_SCRIPT:FILEPATH=${CMAKE_SOURCE_DIR}/Examples/Catalyst/SampleScripts/filedriver.py
          -DCINEMA_DATABASE_TESTER:FILEPATH=${CMAKE_CURRENT_SOURCE_DIR}/CinemaExportCheck.py
          -DCINEMA_INPUT_DATA:FILEPATH=${CMAKE_BINARY_DIR}/ExternalData/Testing/Data/cube.vtu
          -DCINEMA_BATCH_SCRIPT:FILEPATH=cinema/CinemaExportFloatFiles.py
          -DUSE_MPI:BOOL=FALSE
          -DTEST_NAME:STRING=CinemaExportFloatFiles
          -P ${CMAKE_CURRENT_SOURCE_DIR}/CinemaExportFullWorkflowTest.cmake)
        set_tests_properties(CinemaExportNoTimesteps PROPERTIES RUN_SERIAL ON)
        _set_standard_test_properties(CinemaExportFloatFiles)

      endif()
    else()
      add_test(NAME CinemaExportGeometry
        COMMAND ${CMAKE_COMMAND}
        -DPARAVIEW_EXECUTABLE:FILEPATH=$<TARGET_FILE:ParaView::paraview>
        -DCOPROCESSING_TEST_DIR:PATH=${CMAKE_BINARY_DIR}/Testing/Temporary
        -DPARAVIEW_TEST_XML:FILEPATH=${ParaView_SOURCE_DIR}/Applications/ParaView/Testing/XML/CinemaExportGeometry.xml
        -DPVBATCH_EXECUTABLE:FILEPATH=$<TARGET_FILE:ParaView::pvbatch>
        -DPVPYTHON_EXECUTABLE:FILEPATH=$<TARGET_FILE:ParaView::pvpython>
        -DCOPROCESSING_DATA_DIR:PATH=${paraview_test_data_directory_output}
        -DCOPROCESSING_DRIVER_SCRIPT:FILEPATH=${CMAKE_SOURCE_DIR}/Examples/Catalyst/SampleScripts/filedriver.py
        -DCINEMA_DATABASE_TESTER:FILEPATH=${CMAKE_CURRENT_SOURCE_DIR}/CinemaExportCheck.py
        -DCINEMA_INPUT_DATA:FILEPATH=${CMAKE_BINARY_DIR}/ExternalData/Testing/Data/can.ex2
        -DCINEMA_BATCH_SCRIPT:FILEPATH=cinema/CinemaExportGeometry.py
        -DUSE_MPI:BOOL=TRUE
        -DMPIEXEC:FILEPATH=${MPIEXEC_EXECUTABLE}
        -DMPIEXEC_NUMPROC_FLAG:STRING=${MPIEXEC_NUMPROC_FLAG}
        -DMPIEXEC_NUMPROCS=1
        -DMPIEXEC_PREFLAGS:STRING=${MPIEXEC_PREFLAGS}
        -DTEST_NAME:STRING=CinemaExportGeometry
        -P ${CMAKE_CURRENT_SOURCE_DIR}/CinemaExportFullWorkflowTest.cmake)
      set_tests_properties(CinemaExportGeometry PROPERTIES RUN_SERIAL ON)
      _set_standard_test_properties(CinemaExportGeometry)
    
      if(NOT WIN32)
        add_test(NAME CinemaExportGeometryAndImages
          COMMAND ${CMAKE_COMMAND}
          -DPARAVIEW_EXECUTABLE:FILEPATH=$<TARGET_FILE:ParaView::paraview>
          -DCOPROCESSING_TEST_DIR:PATH=${CMAKE_BINARY_DIR}/Testing/Temporary
          -DPARAVIEW_TEST_XML:FILEPATH=${ParaView_SOURCE_DIR}/Applications/ParaView/Testing/XML/CinemaExportGeometryAndImages.xml
          -DPVBATCH_EXECUTABLE:FILEPATH=$<TARGET_FILE:ParaView::pvbatch>
          -DPVPYTHON_EXECUTABLE:FILEPATH=$<TARGET_FILE:ParaView::pvpython>
          -DCOPROCESSING_DATA_DIR:PATH=${paraview_test_data_directory_output}
          -DCOPROCESSING_DRIVER_SCRIPT:FILEPATH=${CMAKE_SOURCE_DIR}/Examples/Catalyst/SampleScripts/filedriver.py
          -DCINEMA_DATABASE_TESTER:FILEPATH=${CMAKE_CURRENT_SOURCE_DIR}/CinemaExportCheck.py
          -DCINEMA_INPUT_DATA:FILEPATH=${CMAKE_BINARY_DIR}/ExternalData/Testing/Data/can.ex2
          -DCINEMA_BATCH_SCRIPT:FILEPATH=cinema/CinemaExportGeometryAndImages.py
          -DUSE_MPI:BOOL=TRUE
          -DMPIEXEC:FILEPATH=${MPIEXEC_EXECUTABLE}
          -DMPIEXEC_NUMPROC_FLAG:STRING=${MPIEXEC_NUMPROC_FLAG}
          -DMPIEXEC_NUMPROCS=1
          -DMPIEXEC_PREFLAGS:STRING=${MPIEXEC_PREFLAGS}
          -DTEST_NAME:STRING=CinemaExportGeometryAndImages
          -P ${CMAKE_CURRENT_SOURCE_DIR}/CinemaExportFullWorkflowTest.cmake)
        set_tests_properties(CinemaExportGeometryAndImages PROPERTIES RUN_SERIAL ON)
        _set_standard_test_properties(CinemaExportGeometryAndImages)
    
        add_test(NAME CinemaExportNoTimesteps
          COMMAND ${CMAKE_COMMAND}
          -DPARAVIEW_EXECUTABLE:FILEPATH=$<TARGET_FILE:ParaView::paraview>
          -DCOPROCESSING_TEST_DIR:PATH=${CMAKE_BINARY_DIR}/Testing/Temporary
          -DPARAVIEW_TEST_XML:FILEPATH=${ParaView_SOURCE_DIR}/Applications/ParaView/Testing/XML/CinemaExportNoTimesteps.xml
          -DPVBATCH_EXECUTABLE:FILEPATH=$<TARGET_FILE:ParaView::pvbatch>
          -DPVPYTHON_EXECUTABLE:FILEPATH=$<TARGET_FILE:ParaView::pvpython>
          -DCOPROCESSING_DATA_DIR:PATH=${paraview_test_data_directory_output}
          -DCOPROCESSING_DRIVER_SCRIPT:FILEPATH=${CMAKE_SOURCE_DIR}/Examples/Catalyst/SampleScripts/filedriver.py
          -DCINEMA_DATABASE_TESTER:FILEPATH=${CMAKE_CURRENT_SOURCE_DIR}/CinemaExportCheck.py
          -DCINEMA_INPUT_DATA:FILEPATH=${CMAKE_BINARY_DIR}/ExternalData/Testing/Data/cube.vtu
          -DCINEMA_BATCH_SCRIPT:FILEPATH=cinema/CinemaExportNoTimesteps.py
          -DUSE_MPI:BOOL=TRUE
          -DMPIEXEC:FILEPATH=${MPIEXEC_EXECUTABLE}
          -DMPIEXEC_NUMPROC_FLAG:STRING=${MPIEXEC_NUMPROC_FLAG}
          -DMPIEXEC_NUMPROCS=1
          -DMPIEXEC_PREFLAGS:STRING=${MPIEXEC_PREFLAGS}
          -DTEST_NAME:STRING=CinemaExportNoTimesteps
          -P ${CMAKE_CURRENT_SOURCE_DIR}/CinemaExportFullWorkflowTest.cmake)
        set_tests_properties(CinemaExportNoTimesteps PROPERTIES RUN_SERIAL ON)
        _set_standard_test_properties(CinemaExportNoTimesteps)
    
        add_test(NAME CinemaExportFloatFiles
          COMMAND ${CMAKE_COMMAND}
          -DPARAVIEW_EXECUTABLE:FILEPATH=$<TARGET_FILE:ParaView::paraview>
          -DCOPROCESSING_TEST_DIR:PATH=${CMAKE_BINARY_DIR}/Testing/Temporary
          -DPARAVIEW_TEST_XML:FILEPATH=${ParaView_SOURCE_DIR}/Applications/ParaView/Testing/XML/CinemaExportFloatFiles.xml
          -DPVBATCH_EXECUTABLE:FILEPATH=$<TARGET_FILE:ParaView::pvbatch>
          -DPVPYTHON_EXECUTABLE:FILEPATH=$<TARGET_FILE:ParaView::pvpython>
          -DCOPROCESSING_DATA_DIR:PATH=${paraview_test_data_directory_output}
          -DCOPROCESSING_DRIVER_SCRIPT:FILEPATH=${CMAKE_SOURCE_DIR}/Examples/Catalyst/SampleScripts/filedriver.py
          -DCINEMA_DATABASE_TESTER:FILEPATH=${CMAKE_CURRENT_SOURCE_DIR}/CinemaExportCheck.py
          -DCINEMA_INPUT_DATA:FILEPATH=${CMAKE_BINARY_DIR}/ExternalData/Testing/Data/cube.vtu
          -DCINEMA_BATCH_SCRIPT:FILEPATH=cinema/CinemaExportFloatFiles.py
          -DUSE_MPI:BOOL=TRUE
          -DMPIEXEC:FILEPATH=${MPIEXEC_EXECUTABLE}
          -DMPIEXEC_NUMPROC_FLAG:STRING=${MPIEXEC_NUMPROC_FLAG}
          -DMPIEXEC_NUMPROCS=1
          -DMPIEXEC_PREFLAGS:STRING=${MPIEXEC_PREFLAGS}
          -DTEST_NAME:STRING=CinemaExportFloatFiles
          -P ${CMAKE_CURRENT_SOURCE_DIR}/CinemaExportFullWorkflowTest.cmake)
        set_tests_properties(CinemaExportNoTimesteps PROPERTIES RUN_SERIAL ON)
        _set_standard_test_properties(CinemaExportFloatFiles)
      endif()

    endif()

  ENDIF ()
endif()
