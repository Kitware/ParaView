SET(TEST_PATH "ParaView")
# initialize these to emtpy
SET(PARAVIEW_MPI_RUN_ONE )
SET(PARAVIEW_CS_EXECUTABLE ${EXECUTABLE_OUTPUT_PATH}/pvTestDriver)
# always use pvTestDriver so that per test timeouts can be done
SET(PARAVIEW_EXECUTABLE ${EXECUTABLE_OUTPUT_PATH}/pvTestDriver)
# the default timeout to use
SET (TIMEOUT)
#SET (TIMEOUT --timeout 100)
IF (VTK_USE_MPI)
  SEPARATE_ARGUMENTS(VTK_MPI_PREFLAGS)
  SEPARATE_ARGUMENTS(VTK_MPI_POSTFLAGS)
  IF (VTK_MPIRUN_EXE)
    SET(PARAVIEW_MPI_POST_FLAGS ${VTK_MPI_POSTFLAGS})
    SET(PARAVIEW_EXECUTABLE ${EXECUTABLE_OUTPUT_PATH}/pvTestDriver)
    # some runs of paraview must never be run with more than one mpi
    # process.  This is a special option to pvTestDriver that forces
    # it to use -np 1
    SET(PARAVIEW_MPI_RUN_ONE "--one-mpi-np")
  ELSE(VTK_MPIRUN_EXE)
    MESSAGE("ParaView has been built with MPI, but VTK_MPIRUN_EXE has not been set, tests may not run correctly")
  ENDIF (VTK_MPIRUN_EXE)
ENDIF (VTK_USE_MPI)

SET(PROJECT_TESTS
  ArrayMenuChangeInput
  Arrow
  Axes
  BlowTest
  BoundsDisplay
  CalcInput
  Calculator
  ClipCut
  ColorMap
  CombustorTest
  #Composite
  ConeCubeCylSQ
  CTH
  CutContours
  Data
  Delete
  DeleteSource
  EnSight
  Extrusion
  ExtractGrid
  ExtractPart
  Glyph
  Group
  Transform
  ImageReader
  Mandelbrot
  Maze
  MouseInteraction
  OrientationWidget
  ProbeLine
  ProbePoint
  ReadPVD
  ScalarBar
  ScalarRangeLabel
  ScalarSelection
  ScalarsSphereClip
  SphereTest
  TestDemo
  TestPackage
  Threshold
  ThresholdAttrMode
  Trace
  VectorTextSpace
  Write
  TestPolyDataIO
  TestUnstructuredGridIO
  TestRectilinearGridIO
  TestImageDataIO
  VolumeGroup
  XdmfRead
  )

# some tests require MPI because the classes do not get built otherwise
IF(VTK_USE_MPI)
  SET(PROJECT_TESTS ${PROJECT_TESTS} D3)
ENDIF(VTK_USE_MPI)


# Foreach test add the following:
# 1. the test (possibly with mpirun)
# 2. the test with compositing and possibly mpi
# 3. the test in client server mode
# 4. the test in client server render server mode
# 

FOREACH (tfile ${PROJECT_TESTS})
  ADD_TEST(${tfile}-ParaView 
    ${PARAVIEW_EXECUTABLE} 
    ${TIMEOUT}
    --disable-registry
    ${PVClientTests_SOURCE_DIR}/Tcl/${tfile}.pvs
    -D ${PVClientTestData}
    -C ${PVCompareImages}
    -T ${ParaView_BINARY_DIR}/Testing/Temporary
    -V ${PVClientTestData}/Baseline/${tfile}.png
    )
  # repeat test in composite mode if test composite is on
  IF (PARAVIEW_TEST_COMPOSITING)
    ADD_TEST(${tfile}-ParaView-COMPOSITE-MPI 
      ${PARAVIEW_EXECUTABLE}     
      ${TIMEOUT}
      --disable-registry 
      ${PVClientTests_SOURCE_DIR}/Tcl/${tfile}.pvs
      -UC
      -D ${PVClientTestData}
      -C ${PVCompareImages}
      -T ${ParaView_BINARY_DIR}/Testing/Temporary
      -V ${PVClientTestData}/Baseline/${tfile}.png
      )
  ENDIF (PARAVIEW_TEST_COMPOSITING)
  # repeat the test with the client server driver
  ADD_TEST("${tfile}ClientServer-ParaView" 
    ${PARAVIEW_CS_EXECUTABLE} 
    --test-server
    ${TIMEOUT}
    --disable-registry
    ${PVClientTests_SOURCE_DIR}/Tcl/${tfile}.pvs
    -D ${PVClientTestData}
    -C ${PVCompareImages}
    -T ${ParaView_BINARY_DIR}/Testing/Temporary
    -V ${PVClientTestData}/Baseline/${tfile}.png)
  # repeat the test with the client server driver in render server mode
  ADD_TEST("${tfile}RenderServer-ParaView" 
    ${PARAVIEW_CS_EXECUTABLE}
    --test-render-server
    ${TIMEOUT}
    --disable-registry
    ${PVClientTests_SOURCE_DIR}/Tcl/${tfile}.pvs
    -D ${PVClientTestData}
    -C ${PVCompareImages}
    -T ${ParaView_BINARY_DIR}/Testing/Temporary
    -V ${PVClientTestData}/Baseline/${tfile}.png)
ENDFOREACH (tfile)

# add some tests for the various ways that connections 
# can be made

# test render server with a reverse connection
# between the servers and the client.  The client
# will start first and wait for the servers to connect to it.
ADD_TEST("ArrowRenderServer-ParaView-rc" 
  ${PARAVIEW_CS_EXECUTABLE}
  --test-render-server
  --test-rc
  ${TIMEOUT}
  --disable-registry
  ${PVClientTests_SOURCE_DIR}/Tcl/Arrow.pvs
  -D ${PVClientTestData}
  -C ${PVCompareImages}
  -T ${ParaView_BINARY_DIR}/Testing/Temporary
  -V ${PVClientTestData}/Baseline/Arrow.png)

# test the render server where the render server
# connects to the data server, the default is for the
# data server to connect to the render server
ADD_TEST("ArrowRenderServer-ParaView-r2d" 
  ${PARAVIEW_CS_EXECUTABLE}
  --test-r2d
  ${TIMEOUT}
  --disable-registry
  ${PVClientTests_SOURCE_DIR}/Tcl/Arrow.pvs
  -D ${PVClientTestData}
  -C ${PVCompareImages}
  -T ${ParaView_BINARY_DIR}/Testing/Temporary
  -V ${PVClientTestData}/Baseline/Arrow.png)

# same test as -r2d, but this time do a reverse
# connection between the servers and the client like
# the -rc test
ADD_TEST("ArrowRenderServer-ParaView-r2d-rc" 
  ${PARAVIEW_CS_EXECUTABLE}
  --test-r2d 
  --test-rc
  ${TIMEOUT}
  --disable-registry
  ${PVClientTests_SOURCE_DIR}/Tcl/Arrow.pvs
  -D ${PVClientTestData}
  -C ${PVCompareImages}
  -T ${ParaView_BINARY_DIR}/Testing/Temporary
  -V ${PVClientTestData}/Baseline/Arrow.png)

# test the reverse connection for one server
# and no render server
ADD_TEST("ArrowServer-ParaView-rc" 
  ${PARAVIEW_CS_EXECUTABLE}
  --test-server
  --test-rc
  ${TIMEOUT}
  --disable-registry
  ${PVClientTests_SOURCE_DIR}/Tcl/Arrow.pvs
  -D ${PVClientTestData}
  -C ${PVCompareImages}
  -T ${ParaView_BINARY_DIR}/Testing/Temporary
  -V ${PVClientTestData}/Baseline/Arrow.png)

# remove the following for the batch tests
REMOVE (PROJECT_TESTS
  MouseInteraction
  TestDemo
  TestPackage
  Write
  XdmfRead
)

# for each test add a batch test that will possibly run in mpi mode
FOREACH (tfile ${PROJECT_TESTS})
  # test batch
  ADD_TEST(${tfile}-batch-ParaView 
    ${PARAVIEW_EXECUTABLE}     
    ${TIMEOUT}
    ${PARAVIEW_MPI_RUN_ONE} 
    --disable-registry
    ${PVClientTests_SOURCE_DIR}/Tcl/${tfile}.pvs
    -D ${PVClientTestData}
    -T ${ParaView_BINARY_DIR}/Testing/Temporary
    -BT ${PVClientTests_SOURCE_DIR}/Tcl/TestBatch.tcl
    -BC ${PVClientTests_SOURCE_DIR}/Tcl/ComparePNGs.tcl
    -B ${ParaView_BINARY_DIR}/Testing/Temporary/${tfile}batch
    -V ${ParaView_BINARY_DIR}/Testing/Temporary/${tfile}batch.png
    )
ENDFOREACH (tfile)

IF(VTK_PRINT_SELF_CHECK_TCL AND TCL_TCLSH)
  ADD_TEST(PrintSelf-${TEST_PATH} ${TCL_TCLSH}
    ${VTK_PRINT_SELF_CHECK_TCL}
    ${PVGUIClientDir})
ENDIF(VTK_PRINT_SELF_CHECK_TCL AND TCL_TCLSH)

IF(VTK_FIND_STRING_TCL AND TCL_TCLSH)
  ADD_TEST(TestSetObjectMacro-${TEST_PATH} ${TCL_TCLSH}
    ${VTK_FIND_STRING_TCL}
    "${PVGUIClientDir}/vtk\\\\*.h"
    "vtkSetObjectMacro")
ENDIF(VTK_FIND_STRING_TCL AND TCL_TCLSH)

# run these test with mpi 1 process if mpi is around, or just run the test
FOREACH(test otherPrint TestSetGet)
  ADD_TEST(${test}-ParaView 
    ${PARAVIEW_EXECUTABLE} ${PARAVIEW_MPI_RUN_ONE} --batch
    ${PVClientTests_SOURCE_DIR}/Tcl/${test}.pvb
    )
ENDFOREACH(test)

ADD_TEST(OtherTests-ParaView 
  ${PARAVIEW_EXECUTABLE} 
  ${TIMEOUT}
  ${PARAVIEW_MPI_RUN_ONE} --disable-registry --start-empty
  ${PVClientTests_SOURCE_DIR}/Tcl/Other.pvs
  -D ${PVClientTestData}
  -A ${VTK_TCL_HOME}
  )

ADD_TEST (AnimationImages-ParaView 
  ${PARAVIEW_EXECUTABLE} 
  ${TIMEOUT}
  ${PARAVIEW_MPI_RUN_ONE} 
  --disable-registry
  ${PVClientTests_SOURCE_DIR}/Tcl/AnimationImages.pvs
  -D ${PVClientTestData}
  -T ${ParaView_BINARY_DIR}/Testing/Temporary
  -C ${PVClientTests_SOURCE_DIR}/Tcl/ComparePNGs.tcl
  -V ${PVClientTestData}/Baseline/AnimationImages.png
  )

ADD_TEST(VolumeRenderVox8-ParaView 
  ${PARAVIEW_EXECUTABLE} 
  ${TIMEOUT}
  --disable-registry
  ${PVClientTests_SOURCE_DIR}/Tcl/VolumeRenderVox8.pvs
  -D ${PVClientTestData}
  -C ${PVCompareImages}
  -T ${ParaView_BINARY_DIR}/Testing/Temporary
  -V ${PVClientTestData}/Baseline/VolumeRenderVox8.png
  )

# Test the PVLocal plugin example.
IF(${CMAKE_MAJOR_VERSION}.${CMAKE_MINOR_VERSION} GREATER 1.8)
  IF(BUILD_SHARED_LIBS)
    ADD_TEST(PVLocal ${CMAKE_CTEST_COMMAND}
      --build-and-test
      ${ParaView_SOURCE_DIR}/Examples/PVLocal
      ${PVClientTests_BINARY_DIR}/Examples/PVLocal
      --build-two-config
      --build-generator ${CMAKE_GENERATOR}
      --build-makeprogram ${MAKEPROGRAM}
      --build-project PVLocal
      --build-options -DParaView_DIR:PATH=${ParaView_BINARY_DIR}
      --test-command
      ${PARAVIEW_EXECUTABLE} --disable-registry
      ${PVClientTests_SOURCE_DIR}/Tcl/PVLocal.pvs
      --pvlocal-dir ${PVClientTests_BINARY_DIR}/Examples/PVLocal
      -D ${PVClientTestData}
      -C ${PVCompareImages}
      -T ${ParaView_BINARY_DIR}/Testing/Temporary
      -V ${PVClientTestData}/Baseline/PVLocal.png
      )
  ENDIF(BUILD_SHARED_LIBS)    
ENDIF(${CMAKE_MAJOR_VERSION}.${CMAKE_MINOR_VERSION} GREATER 1.8)
