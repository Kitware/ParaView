SET(TEST_PATH "ParaView")
SET(RUN_WITH_MPI 0)
IF (VTK_USE_MPI)
  IF (UNIX)
    IF (VTK_MPIRUN_EXE)
      SET(RUN_WITH_MPI 1)
    ENDIF (VTK_MPIRUN_EXE)
  ENDIF (UNIX)
  SEPARATE_ARGUMENTS(VTK_MPI_PREFLAGS)
  SEPARATE_ARGUMENTS(VTK_MPI_POSTFLAGS)
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
  #               Composite
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

IF(RUN_WITH_MPI)
  FOREACH (tfile ${PROJECT_TESTS})
    ADD_TEST(${tfile}-ParaView ${VTK_MPIRUN_EXE} ${VTK_MPI_PREFLAGS}
      ${PARAVIEW_EXECUTABLE} --disable-registry
      ${PVClientTests_SOURCE_DIR}/Tcl/${tfile}.pvs
      -D ${PVClientTestData}
      -C ${PVCompareImages}
      -T ${ParaView_BINARY_DIR}/Testing/Temporary
      -V ${PVClientTestData}/Baseline/${tfile}.png
      ${PARAVIEW_MPI_POSTFLAGS})
    IF (PARAVIEW_TEST_COMPOSITING)
      ADD_TEST(${tfile}-ParaView-COMPOSITE-MPI ${VTK_MPIRUN_EXE} ${VTK_MPI_PREFLAGS}
        ${PARAVIEW_EXECUTABLE} --disable-registry
        ${PVClientTests_SOURCE_DIR}/Tcl/${tfile}.pvs
        -UC
        -D ${PVClientTestData}
        -C ${PVCompareImages}
        -T ${ParaView_BINARY_DIR}/Testing/Temporary
        -V ${PVClientTestData}/Baseline/${tfile}.png
        ${PARAVIEW_MPI_POSTFLAGS})
    ENDIF (PARAVIEW_TEST_COMPOSITING)
  ENDFOREACH(tfile)

ELSE(RUN_WITH_MPI)

  FOREACH (tfile ${PROJECT_TESTS})
    ADD_TEST(${tfile}-ParaView ${PARAVIEW_EXECUTABLE} --disable-registry
      ${PVClientTests_SOURCE_DIR}/Tcl/${tfile}.pvs
      -D ${PVClientTestData}
      -C ${PVCompareImages}
      -T ${ParaView_BINARY_DIR}/Testing/Temporary
      -V ${PVClientTestData}/Baseline/${tfile}.png)
    # repeat the test with the client server driver
    ADD_TEST("${tfile}ClientServer-ParaView" ${PARAVIEW_CS_EXECUTABLE}
      --disable-registry
      ${PVClientTests_SOURCE_DIR}/Tcl/${tfile}.pvs
      -D ${PVClientTestData}
      -C ${PVCompareImages}
      -T ${ParaView_BINARY_DIR}/Testing/Temporary
      -V ${PVClientTestData}/Baseline/${tfile}.png)
    # repeat the test with the client server driver in render server mode
    ADD_TEST("${tfile}RenderServer-ParaView" ${PARAVIEW_CS_EXECUTABLE}
      --test-render-server
      --disable-registry
      ${PVClientTests_SOURCE_DIR}/Tcl/${tfile}.pvs
      -D ${PVClientTestData}
      -C ${PVCompareImages}
      -T ${ParaView_BINARY_DIR}/Testing/Temporary
      -V ${PVClientTestData}/Baseline/${tfile}.png)
  ENDFOREACH (tfile)


  IF(VTK_PRINT_SELF_CHECK_TCL)
    ADD_TEST(PrintSelf-${TEST_PATH} ${TCL_TCLSH}
      ${VTK_PRINT_SELF_CHECK_TCL}
      ${PVGUIClientDir})
  ENDIF(VTK_PRINT_SELF_CHECK_TCL)
  
  IF(VTK_FIND_STRING_TCL)
    ADD_TEST(TestSetObjectMacro-${TEST_PATH} ${TCL_TCLSH}
      ${VTK_FIND_STRING_TCL}
      "${PVGUIClientDir}/vtk\\\\*.h"
      "vtkSetObjectMacro")
  ENDIF(VTK_FIND_STRING_TCL)
  
  FOREACH(test otherPrint TestSetGet)
    ADD_TEST(${test}-ParaView ${PARAVIEW_EXECUTABLE} --batch
      ${PVClientTests_SOURCE_DIR}/Tcl/${test}.pvb)
  ENDFOREACH(test)
  
  ADD_TEST(OtherTests-ParaView ${PARAVIEW_EXECUTABLE} --disable-registry --start-empty
    ${PVClientTests_SOURCE_DIR}/Tcl/Other.pvs
    -D ${PVClientTestData}
    -A ${VTK_TCL_HOME}
    )
  
  ADD_TEST (AnimationImages-ParaView ${PARAVIEW_EXECUTABLE} --disable-registry
    ${PVClientTests_SOURCE_DIR}/Tcl/AnimationImages.pvs
    -D ${PVClientTestData}
    -T ${ParaView_BINARY_DIR}/Testing/Temporary
    -C ${PVClientTests_SOURCE_DIR}/Tcl/ComparePNGs.tcl
    -V ${PVClientTestData}/Baseline/AnimationImages.png)
  
ENDIF(RUN_WITH_MPI)
