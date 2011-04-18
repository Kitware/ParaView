cmake_minimum_required(VERSION 2.8)

INCLUDE_DIRECTORIES(../..)

create_test_sourcelist(STREAMINGTESTS
  StreamingCxxTests.cxx
  Source.cxx
  Harness.cxx
  Iteration.cxx
  Prioritization.cxx
  Refinement.cxx
)

ADD_EXECUTABLE( StreamingCxxTests ${STREAMINGTESTS} )
set (TestsToRun ${STREAMINGTESTS})
remove (TestsToRun StreamingCxxTests.cxx)
foreach (test ${TestsToRun})
  get_filename_component(TName ${test} NAME_WE)
  add_test(Streaming-Test${TName} ${EXECUTABLE_OUTPUT_PATH}/StreamingCxxTests ${TName}
    -T ${EXECUTABLE_OUTPUT_PATH}/../Testing/Temporary
    -V ${PARAVIEW_DATA_ROOT}/Baseline/StreamingTest${TName}.png
    )
endforeach()
TARGET_LINK_LIBRARIES( StreamingCxxTests
  vtkStreaming
  vtkIO vtkRendering vtkFiltering vtkGraphics vtkCommon vtkImaging)
