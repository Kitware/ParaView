SET(TEST_PATH "ParaView")
SET(RUN_WITH_MPI 0)
IF (VTK_USE_MPI)
  IF (UNIX)
    IF (VTK_MPIRUN_EXE)
      SET(RUN_WITH_MPI 1)
    ENDIF (VTK_MPIRUN_EXE)
  ENDIF (UNIX)
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
  MouseInteraction
  OrientationWidget
  ProbeLine
  ProbePoint
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
  )

IF (NOT PARAVIEW_MPI_PREFLAGS)
  SET (PARAVIEW_MPI_PREFLAGS ${VTK_MPI_PREFLAGS})
ENDIF (NOT PARAVIEW_MPI_PREFLAGS)

IF (NOT PARAVIEW_MPI_POSTFLAGS)
  SET (PARAVIEW_MPI_POSTFLAGS ${VTK_MPI_POSTFLAGS})
ENDIF (NOT PARAVIEW_MPI_POSTFLAGS)

IF(RUN_WITH_MPI)
  FOREACH (tfile ${PROJECT_TESTS})
    ADD_TEST(${tfile}-MPI ${VTK_MPIRUN_EXE} ${PARAVIEW_MPI_PREFLAGS}
      ${PARAVIEW_EXECUTABLE} --disable-registry
      ${PARAVIEW_SOURCE_DIR}/${TEST_PATH}/Testing/Tcl/${tfile}.pvs
      -D ${PARAVIEW_SOURCE_DIR}/Data
      -C ${PARAVIEW_SOURCE_DIR}/Widgets/Testing/Tcl/CompareImages.tcl
      -V ${PARAVIEW_SOURCE_DIR}/Data/Baseline/${tfile}.png
      ${PARAVIEW_MPI_POSTFLAGS})
    IF (PARAVIEW_TEST_COMPOSITING)
      ADD_TEST(${tfile}-COMPOSITE-MPI ${VTK_MPIRUN_EXE} ${PARAVIEW_MPI_PREFLAGS}
        ${PARAVIEW_EXECUTABLE} --disable-registry
        ${PARAVIEW_SOURCE_DIR}/${TEST_PATH}/Testing/Tcl/${tfile}.pvs
        -UC
        -D ${PARAVIEW_SOURCE_DIR}/Data
        -C ${PARAVIEW_SOURCE_DIR}/Widgets/Testing/Tcl/CompareImages.tcl
        -V ${PARAVIEW_SOURCE_DIR}/Data/Baseline/${tfile}.png
        ${PARAVIEW_MPI_POSTFLAGS})
    ENDIF (PARAVIEW_TEST_COMPOSITING)
  ENDFOREACH(tfile)
ENDIF(RUN_WITH_MPI)

FOREACH (tfile ${PROJECT_TESTS})
  ADD_TEST(${tfile} ${PARAVIEW_EXECUTABLE} --disable-registry
    ${PARAVIEW_SOURCE_DIR}/${TEST_PATH}/Testing/Tcl/${tfile}.pvs
    -D ${PARAVIEW_SOURCE_DIR}/Data
    -C ${PARAVIEW_SOURCE_DIR}/Widgets/Testing/Tcl/CompareImages.tcl
    -V ${PARAVIEW_SOURCE_DIR}/Data/Baseline/${tfile}.png)
  # repeat the test with the client server driver
  ADD_TEST("${tfile}ClientServer" ${PARAVIEW_CS_EXECUTABLE}
    --disable-registry
    ${PARAVIEW_SOURCE_DIR}/${TEST_PATH}/Testing/Tcl/${tfile}.pvs
    -D ${PARAVIEW_SOURCE_DIR}/Data
    -C ${PARAVIEW_SOURCE_DIR}/Widgets/Testing/Tcl/CompareImages.tcl
    -V ${PARAVIEW_SOURCE_DIR}/Data/Baseline/${tfile}.png)
ENDFOREACH (tfile)

IF(VTK_PRINT_SELF_CHECK_TCL)
  ADD_TEST(PrintSelf-${TEST_PATH} ${TCL_TCLSH}
    ${VTK_PRINT_SELF_CHECK_TCL}
    ${PARAVIEW_SOURCE_DIR}/${TEST_PATH})
ENDIF(VTK_PRINT_SELF_CHECK_TCL)

IF(VTK_FIND_STRING_TCL)
  ADD_TEST(TestSetObjectMacro-${TEST_PATH} ${TCL_TCLSH}
    ${VTK_FIND_STRING_TCL}
    "${PARAVIEW_SOURCE_DIR}/${TEST_PATH}/vtk\\\\*.h"
    "vtkSetObjectMacro")
ENDIF(VTK_FIND_STRING_TCL)

FOREACH(test otherPrint TestSetGet)
  ADD_TEST(${test}-ParaView ${PARAVIEW_EXECUTABLE} --batch
    ${PARAVIEW_SOURCE_DIR}/ParaView/Testing/Tcl/${test}.pvb)
ENDFOREACH(test)

ADD_TEST(OtherTests-ParaView ${PARAVIEW_EXECUTABLE} --disable-registry --start-empty
  ${PARAVIEW_SOURCE_DIR}/ParaView/Testing/Tcl/Other.pvs
  -D ${PARAVIEW_SOURCE_DIR}/Data
  -A ${VTK_TCL_HOME}
  )

ADD_TEST (AnimationImages ${PARAVIEW_EXECUTABLE} --disable-registry
  ${PARAVIEW_SOURCE_DIR}/ParaView/Testing/Tcl/AnimationImages.pvs
  -D ${PARAVIEW_SOURCE_DIR}/Data
  -C ${PARAVIEW_SOURCE_DIR}/ParaView/Testing/Tcl/ComparePNGs.tcl
  -V ${PARAVIEW_SOURCE_DIR}/Data/Baseline/AnimationImages.png)
