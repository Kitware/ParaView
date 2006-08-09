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
  DataAnalysisPointId
  DataAnalysisCellIdTemporal
  DefaultAnimation
  Delete
  DeleteSource
  DualSphereAnimation
  EnSight
  Extrusion
  ExtractGrid
#  ExtractPart
  Flow
  fractal2d
  fractal3d
  Glyph
#  Group
  Transform
  ImageReader
  Mandelbrot
  MouseInteraction
  OrientationWidget
  ProbeLine
  ProbePoint
  ReadPVD
  RecordAnimation
  ScalarBar
  ScalarRangeLabel
  ScalarSelection
  ScalarsSphereClip
  SPCTH
  TestDemo
  TestLookmarks
  TestMBPlot3D
  TestPHT
  TestPackage
  Threshold
  ThresholdAttrMode
  Trace
  VectorTextSpace
  Write
  Tessellate
  TestExodus
  TestComparativeVis
  TestPolyDataIO
  TestUnstructuredGridIO
  TestRectilinearGridIO
  TestImageDataIO
#  VolumeGroup
  XdmfRead
  XdmfReadImageData
  XdmfReadImageDataCollection
  XdmfReadStructuredGrid
  XdmfReadStructuredGridCollection
  XdmfReadRectilinearGrid
  XdmfReadRectilinearGridCollection
  )

# some tests require MPI because the classes do not get built otherwise
IF(VTK_USE_MPI)
  SET(PROJECT_TESTS ${PROJECT_TESTS} D3)
ENDIF(VTK_USE_MPI)


# Some of the following tests require some extra variable (hidden from the user)
# to be set: these variable are:
# VTK_MPI_CLIENT_PREFLAGS
# VTK_MPI_SERVER_PREFLAGS   #typically -nolocal for mpich implementation (so the machinefile will be properly read)
# VTK_MPI_CLIENT_POSTFLAGS  #typically -tdx=2 -tdy=1 --host=amber5
# VTK_MPI_SERVER_POSTFLAGS  
# Then basically the command line is:
# If this is the client:
# ${VTK_MPIRUN_EXE} ${VTK_MPI_NUMPROC_FLAG} 2 ${VTK_MPI_PREFLAGS} ${VTK_MPI_CLIENT_PREFLAGS} executable --client ${VTK_MPI_POSTFLAGS} ${VTK_MPI_CLIENT_POSTFLAGS}
# If this is the server:
# ${VTK_MPIRUN_EXE} ${VTK_MPI_NUMPROC_FLAG} 2 ${VTK_MPI_PREFLAGS} ${VTK_MPI_SERVER_PREFLAGS} executable --server ${VTK_MPI_POSTFLAGS} ${VTK_MPI_SERVER_POSTFLAGS}
# For more information you can also check for VTK_MPI_PREFLAGS / VTK_MPI_POSTFLAGS in VTK/CMakeLists.txt
# 
# For tiled display tests, another set of similar variables can be
# defined
# VTK_MPI_CLIENT_TD_PREFLAGS
# VTK_MPI_CLIENT_TD_POSTFLAGS
# VTK_MPI_SERVER_TD_PREFLAGS
# VTK_MPI_SERVER_TD_POSTFLAGS


# Foreach test add the following:
# 1. the test (possibly with mpirun)
# 2. the test with compositing and possibly mpi
# 3. the test in client server mode
# 4. the test in client server render server mode
# 

FOREACH (tfile ${PROJECT_TESTS})
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
  # There is a special hidden mode where the testing is being done remotely
  # Basically it ssh onto another machine start server, once server is started,
  # start the client on localhost. You need to set:
  # A ssh command to log onto another machine (use ssh-key rather than passwd)  
  # - PV_SSH_FLAGS              ( ssh -l kitware dash8 )
  # a shell script that would initialize the server
  # - PV_SETUP_SCRIPT           (eval ${PARAVIEW_SETUP_SCRIPT} ; eval  ${PARAVIEW_EXECUTABLE} --server --port=11111)
  # with echo ${PARAVIEW_SETUP_SCRIPT}
  # . ~/Software/paraview_env.sh
  # and cat paraview_env.sh
  # > export DISPLAY=:0.0
  # > export PARAVIEW_EXECUTABLE="/home/kitware/Dashboard/MyTests/ParaView-gcc/bin/paraview"
  # > echo "ParaView Environment Established"
  # and paraview flags to connect to specific server:
  # - VTK_MPI_CLIENT_POSTFLAGS  (--user=kitware --host=dash8)
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
  # repeat test in tiling mode if test tiling is on
  IF (PARAVIEW_TEST_TILED_DISPLAY)
    ADD_TEST(${tfile}-ParaView-TILED-MPI 
      ${PARAVIEW_CS_EXECUTABLE}     
      --test-tiled
      ${TIMEOUT}
      --disable-registry 
      ${PVClientTests_SOURCE_DIR}/Tcl/${tfile}.pvs
      -UC
      -D ${PVClientTestData}
      -C ${PVCompareImages}
      -T ${ParaView_BINARY_DIR}/Testing/Temporary
      -V ${PVClientTestData}/Baseline/${tfile}-TILED.png
      -FrontBuffer
      -NoRerender
      )
  ENDIF (PARAVIEW_TEST_TILED_DISPLAY)
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

# Make sure that load cth - extract cth - array calc (cell data)
# works. This is a bug that keeps resurfacing.
ADD_TEST(TestCTHAndCalc-ParaView
  ${PARAVIEW_EXECUTABLE} 
  ${TIMEOUT}
  --disable-registry
  ${PVClientTests_SOURCE_DIR}/Tcl/TestCTHAndCalc.pvs
  -D ${PVClientTestData}
  -C ${PVCompareImages}
  -T ${ParaView_BINARY_DIR}/Testing/Temporary
  -V ${PVClientTestData}/Baseline/TestCTHAndCalc.png
  )

# remove the following for the batch tests
SET (PROJECT_TESTS_NO_BATCH 
  DataAnalysisCellIdTemporal
  DefaultAnimation
  DeleteSource
  DualSphereAnimation
  fractal2d
  fractal3d
  MouseInteraction
  OrientationWidget
  RecordAnimation
  TestDemo
  TestPackage
  Write
  TestComparativeVis
)
REMOVE (PROJECT_TESTS ${PROJECT_TESTS_NO_BATCH})

ADD_TEST(TestEmptyInput-ParaView
  ${PARAVIEW_EXECUTABLE} 
  ${TIMEOUT}
  --disable-registry
  ${PVClientTests_SOURCE_DIR}/Tcl/TestEmptyInput.pvs
  )

# for each test add a batch test that will possibly run in mpi mode
FOREACH (tfile ${PROJECT_TESTS})
  # test batch
  ADD_TEST(${tfile}-batch-ParaView 
    ${PARAVIEW_EXECUTABLE} 
    ${TIMEOUT}
    --disable-registry
    ${PVClientTests_SOURCE_DIR}/Tcl/${tfile}.pvs
    -D ${PVClientTestData}
    -C ${PVCompareImages}
    -T ${ParaView_BINARY_DIR}/Testing/Temporary
    -V ${PVClientTestData}/Baseline/${tfile}.png
    -BT ${PVClientTests_SOURCE_DIR}/Tcl/TestBatch.tcl
    -BC ${PVClientTests_SOURCE_DIR}/Tcl/ComparePNGs.tcl
    -B ${ParaView_BINARY_DIR}/Testing/Temporary/${tfile}batch
#    -BV ${ParaView_BINARY_DIR}/Testing/Temporary/${tfile}batch.png
    )
ENDFOREACH (tfile)

# run the non batch tests just in normal mode
FOREACH (tfile ${PROJECT_TESTS_NO_BATCH})
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
  ADD_TEST(${test}-PVBatch 
    ${PARAVIEW_EXECUTABLE} --test-batch ${PARAVIEW_MPI_RUN_ONE}
    ${PVClientTests_SOURCE_DIR}/Tcl/${test}.pvb
    )
ENDFOREACH(test)

ADD_TEST(OtherTests-ParaView 
  ${PARAVIEW_EXECUTABLE} 
  ${TIMEOUT}
  ${PARAVIEW_MPI_RUN_ONE} --disable-registry --start-empty
  ${PVClientTests_SOURCE_DIR}/Tcl/Other.pvs
  -D ${PVClientTestData}
  )

# ADD_TEST (AnimationImages-ParaView 
#   ${PARAVIEW_EXECUTABLE} 
#   ${TIMEOUT}
#   ${PARAVIEW_MPI_RUN_ONE} 
#   --disable-registry
#   ${PVClientTests_SOURCE_DIR}/Tcl/AnimationImages.pvs
#   -D ${PVClientTestData}
#   -T ${ParaView_BINARY_DIR}/Testing/Temporary
#   -C ${PVClientTests_SOURCE_DIR}/Tcl/ComparePNGs.tcl
#   -V ${PVClientTestData}/Baseline/AnimationImages.png
#   )

# # Test animation in batch.
# ADD_TEST(AnimationImages-batch-ParaView
#   ${PARAVIEW_EXECUTABLE}
#   ${TIMEOUT}
#   ${PARAVIEW_MPI_RUN_ONE}
#   --disable-registry
#   ${PVClientTests_SOURCE_DIR}/Tcl/AnimationImages.pvs
#   -D ${PVClientTestData}
#   -T ${ParaView_BINARY_DIR}/Testing/Temporary
#   -BT ${PVClientTests_SOURCE_DIR}/Tcl/TestBatch.tcl
#   -BC ${PVClientTests_SOURCE_DIR}/Tcl/ComparePNGs.tcl
#   -B ${ParaView_BINARY_DIR}/Testing/Temporary/AnimationImagesbatch
#   -V ${ParaView_BINARY_DIR}/Testing/Temporary/AnimationImagesbatch00001.png
#   )
ADD_TEST(LabelPointIds-ParaView
  ${PARAVIEW_EXECUTABLE}
  ${TIMEOUT}
  ${PARAVIEW_MPI_RUN_ONE}
   --disable-registry
  ${PVClientTests_SOURCE_DIR}/Tcl/LabelPointIds.pvs
  -D ${PVClientTestData}
  -C ${PVCompareImages}
  -T ${ParaView_BINARY_DIR}/Testing/Temporary
  -V ${PVClientTestData}/Baseline/LabelPointIds.png
  )

ADD_TEST(VolumeRenderVox8-ParaView 
  ${PARAVIEW_EXECUTABLE} 
  ${TIMEOUT}
  ${PARAVIEW_MPI_RUN_ONE} 
  --disable-registry
  ${PVClientTests_SOURCE_DIR}/Tcl/VolumeRenderVox8.pvs
  -D ${PVClientTestData}
  -C ${PVCompareImages}
  -T ${ParaView_BINARY_DIR}/Testing/Temporary
  -V ${PVClientTestData}/Baseline/VolumeRenderVox8.png
  )

ADD_TEST(VolumeRenderVox8Cell-ParaView 
  ${PARAVIEW_EXECUTABLE} 
  ${TIMEOUT}
  ${PARAVIEW_MPI_RUN_ONE} 
  --disable-registry
  ${PVClientTests_SOURCE_DIR}/Tcl/VolumeRenderVox8Cell.pvs
  -D ${PVClientTestData}
  -C ${PVCompareImages}
  -T ${ParaView_BINARY_DIR}/Testing/Temporary
  -V ${PVClientTestData}/Baseline/VolumeRenderVox8Cell.png
  )

IF (VTK_DATA_ROOT)
  ADD_TEST(ByteOrder-ParaView 
    ${PARAVIEW_EXECUTABLE} 
    ${TIMEOUT}
    ${PARAVIEW_MPI_RUN_ONE} 
    --disable-registry
    ${PVClientTests_SOURCE_DIR}/Tcl/ByteOrder.pvs
    -D ${VTK_DATA_ROOT}
    -C ${PVCompareImages}
    -T ${ParaView_BINARY_DIR}/Testing/Temporary
    -V ${PVClientTestData}/Baseline/ByteOrder.png
    )
ENDIF (VTK_DATA_ROOT)

# Test timings for tiled display (Ice-T)
IF (PARAVIEW_TEST_TILED_DISPLAY)
  ADD_TEST(TiledTiming
    ${PARAVIEW_CS_EXECUTABLE}
    --test-tiled
    ${TIMEOUT}
    --disable-registry
    ${PVClientTests_SOURCE_DIR}/Tcl/TiledTiming.pvs
    -UC
    -D ${PVClientTestData}
    -C ${PVCompareImages}
    -T ${ParaView_BINARY_DIR}/Testing/Temporary
    -V ${PVClientTestData}/Baseline/${tfile}-TILED.png
    -FrontBuffer
    -NoRerender
    )
  ADD_TEST(TiledClientCollect
    ${PARAVIEW_CS_EXECUTABLE}
    --test-tiled
    ${TIMEOUT}
    --disable-registry
    ${PVClientTests_SOURCE_DIR}/Tcl/TiledClientCollect.pvs
    -UC
    -D ${PVClientTestData}
    -C ${PVCompareImages}
    -T ${ParaView_BINARY_DIR}/Testing/Temporary
    -V ${PVClientTestData}/Baseline/TiledClientCollect.png
    -FrontBuffer
    -NoRerender
    )
ENDIF (PARAVIEW_TEST_TILED_DISPLAY)

# Test the PVLocal plugin example.
IF(${CMAKE_MAJOR_VERSION}.${CMAKE_MINOR_VERSION} GREATER 1.8)
  IF(BUILD_SHARED_LIBS)
    # Default to single-configuration behavior.
    SET(PVLOCAL_TEST_CONFIG_TYPE)
    SET(PVLOCAL_TEST_RUN_DIR "${EXECUTABLE_OUTPUT_PATH}")

    # Enable multi-configuration behavior if support is available.
    IF(CMAKE_CONFIGURATION_TYPES AND VTK_TEST_WITH_CTEST)
      SET(DOLLAR "$")
      IF(${CMAKE_MAJOR_VERSION}.${CMAKE_MINOR_VERSION} GREATER 2.0)
        SET(PVLOCAL_TEST_CONFIG_TYPE -C "${DOLLAR}{CTEST_CONFIGURATION_TYPE}")
        SET(PVLOCAL_TEST_RUN_DIR "${EXECUTABLE_OUTPUT_PATH}/${DOLLAR}{CTEST_CONFIGURATION_TYPE}")
      ENDIF(${CMAKE_MAJOR_VERSION}.${CMAKE_MINOR_VERSION} GREATER 2.0)
    ENDIF(CMAKE_CONFIGURATION_TYPES AND VTK_TEST_WITH_CTEST)

    ADD_TEST(PVLocal ${CMAKE_CTEST_COMMAND}
      ${PVLOCAL_TEST_CONFIG_TYPE}
      --build-and-test
      ${ParaView_SOURCE_DIR}/Examples/PVLocal
      ${PVClientTests_BINARY_DIR}/Examples/PVLocal
      --build-two-config
      --build-generator ${CMAKE_GENERATOR}
      --build-makeprogram ${MAKEPROGRAM}
      --build-project PVLocal
      --build-run-dir ${PVLOCAL_TEST_RUN_DIR}
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

OPTION(TEST_MP2_WRITER "Test the MP2 writer used in saving animations" OFF)
MARK_AS_ADVANCED(TEST_MP2_WRITER)

IF(TEST_MP2_WRITER)
  ADD_TEST(TestMP2-ParaView
    ${PARAVIEW_EXECUTABLE}
    ${PARAVIEW_MPI_RUN_ONE}
    --disable-registry
    ${PVClientTests_SOURCE_DIR}/Tcl/TestMP2.pvs
    -T ${ParaView_BINARY_DIR}/Testing/Temporary
    )
ENDIF(TEST_MP2_WRITER)
