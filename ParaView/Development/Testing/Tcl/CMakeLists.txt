SET(TEST_PATH "Development")
SET(RUN_WITH_MPI 0)
IF (VTK_USE_MPI)
  IF (UNIX)
    IF (VTK_MPIRUN_EXE)
      SET(RUN_WITH_MPI 1)
    ENDIF (VTK_MPIRUN_EXE)
  ENDIF (UNIX)
ENDIF (VTK_USE_MPI)

#                  TestStructuredGridIO
SET(PROJECT_TESTS )

IF(RUN_WITH_MPI)
  FOREACH (tfile ${PROJECT_TESTS})
    ADD_TEST(${tfile}-MPI ${VTK_MPIRUN_EXE} ${VTK_MPI_PREFLAGS} ${PARAVIEW_EXECUTABLE}
      --disable-registry
      ${PARAVIEW_SOURCE_DIR}/${TEST_PATH}/Testing/Tcl/${tfile}.pvs
      -D ${PARAVIEW_SOURCE_DIR}/Data 
      -C ${PARAVIEW_SOURCE_DIR}/Widgets/Testing/Tcl/CompareImages.tcl
      -V ${PARAVIEW_SOURCE_DIR}/Data/Baseline/${tfile}.png
      ${VTK_MPI_POSTFLAGS})
  ENDFOREACH(tfile)
ENDIF(RUN_WITH_MPI)
FOREACH (tfile ${PROJECT_TESTS})
  ADD_TEST(${tfile} ${PARAVIEW_EXECUTABLE} --disable-registry
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


