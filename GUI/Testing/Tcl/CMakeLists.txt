SET(TEST_PATH "ParaView")
# initialize these to emtpy
SET(PARAVIEW_MPI_RUN )
SET(PARAVIEW_MPI_POST_FLAGS )

IF (VTK_USE_MPI)
  SEPARATE_ARGUMENTS(VTK_MPI_PREFLAGS)
  SEPARATE_ARGUMENTS(VTK_MPI_POSTFLAGS)
  IF (VTK_MPIRUN_EXE)
    SET(PARAVIEW_MPI_RUN 
      ${VTK_MPIRUN_EXE} ${VTK_MPI_NUMPROC_FLAG} ${VTK_MPI_MAX_NUMPROCS} ${VTK_MPI_PREFLAGS})
    SET(PARAVIEW_MPI_RUN_ONE
      ${VTK_MPIRUN_EXE} ${VTK_MPI_NUMPROC_FLAG} 1 ${VTK_MPI_PREFLAGS})
    SET(PARAVIEW_MPI_POST_FLAGS ${VTK_MPI_POSTFLAGS})
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

# Foreach test add the following:
# 1. the test (possibly with mpirun)
# 2. the test with compositing and possibly mpi
# 3. the test in client server mode
# 4. the test in client server render server mode
# 

FOREACH (tfile ${PROJECT_TESTS})
  # add the test use MPI if it is available
  # PARAVIEW_MPI_RUN will be empty if there is no MPI run specified, same
  # goes for PARAVIEW_MPI_POST_FLAGS.  So the test will be run MPI
  # if MPI is available but will be called the same thing
  ADD_TEST(${tfile}-ParaView 
    ${PARAVIEW_MPI_RUN}
    ${PARAVIEW_EXECUTABLE} --disable-registry
    ${PVClientTests_SOURCE_DIR}/Tcl/${tfile}.pvs
    -D ${PVClientTestData}
    -C ${PVCompareImages}
    -T ${ParaView_BINARY_DIR}/Testing/Temporary
    -V ${PVClientTestData}/Baseline/${tfile}.png
    ${PARAVIEW_MPI_POST_FLAGS}
    )
  # repeat test in composite mode if test composite is on
  IF (PARAVIEW_TEST_COMPOSITING)
    ADD_TEST(${tfile}-ParaView-COMPOSITE-MPI 
      ${PARAVIEW_MPI_RUN}
      ${PARAVIEW_EXECUTABLE} --disable-registry 
      ${PVClientTests_SOURCE_DIR}/Tcl/${tfile}.pvs
      -UC
      -D ${PVClientTestData}
      -C ${PVCompareImages}
      -T ${ParaView_BINARY_DIR}/Testing/Temporary
      -V ${PVClientTestData}/Baseline/${tfile}.png
      ${PARAVIEW_MPI_POST_FLAGS})
  ENDIF (PARAVIEW_TEST_COMPOSITING)
  # repeat the test with the client server driver
  ADD_TEST("${tfile}ClientServer-ParaView" 
    ${PARAVIEW_CS_EXECUTABLE}
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
    --disable-registry
    ${PVClientTests_SOURCE_DIR}/Tcl/${tfile}.pvs
    -D ${PVClientTestData}
    -C ${PVCompareImages}
    -T ${ParaView_BINARY_DIR}/Testing/Temporary
    -V ${PVClientTestData}/Baseline/${tfile}.png)
ENDFOREACH (tfile)

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
    ${PARAVIEW_MPI_RUN_ONE}
    ${PARAVIEW_EXECUTABLE} --disable-registry
    ${PVClientTests_SOURCE_DIR}/Tcl/${tfile}.pvs
    -D ${PVClientTestData}
    -T ${ParaView_BINARY_DIR}/Testing/Temporary
    -BT ${PVClientTests_SOURCE_DIR}/Tcl/TestBatch.tcl
    -BC ${PVClientTests_SOURCE_DIR}/Tcl/ComparePNGs.tcl
    -B ${ParaView_BINARY_DIR}/Testing/Temporary/${tfile}batch
    -V ${ParaView_BINARY_DIR}/Testing/Temporary/${tfile}batch.png
    ${PARAVIEW_MPI_POST_FLAGS}
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
    ${PARAVIEW_MPI_RUN_ONE}
    ${PARAVIEW_EXECUTABLE} --batch
    ${PVClientTests_SOURCE_DIR}/Tcl/${test}.pvb
    ${PARAVIEW_MPI_POST_FLAGS}
    )
ENDFOREACH(test)

ADD_TEST(OtherTests-ParaView 
  ${PARAVIEW_MPI_RUN_ONE}
  ${PARAVIEW_EXECUTABLE} --disable-registry --start-empty
  ${PVClientTests_SOURCE_DIR}/Tcl/Other.pvs
  -D ${PVClientTestData}
  -A ${VTK_TCL_HOME}
  ${PARAVIEW_MPI_POST_FLAGS}
  )

ADD_TEST (AnimationImages-ParaView 
  ${PARAVIEW_MPI_RUN_ONE}
  ${PARAVIEW_EXECUTABLE} --disable-registry
  ${PVClientTests_SOURCE_DIR}/Tcl/AnimationImages.pvs
  -D ${PVClientTestData}
  -T ${ParaView_BINARY_DIR}/Testing/Temporary
  -C ${PVClientTests_SOURCE_DIR}/Tcl/ComparePNGs.tcl
  -V ${PVClientTestData}/Baseline/AnimationImages.png
  ${PARAVIEW_MPI_POST_FLAGS}
  )

# Test the PVLocal plugin example.
IF(${CMAKE_MAJOR_VERSION}.${CMAKE_MINOR_VERSION} GREATER 1.8)
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
ENDIF(${CMAKE_MAJOR_VERSION}.${CMAKE_MINOR_VERSION} GREATER 1.8)
