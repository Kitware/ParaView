IF (EXECUTABLE_OUTPUT_PATH)
  SET (VTK_EXECUTABLE ${EXECUTABLE_OUTPUT_PATH}/vtk)
ELSE (EXECUTABLE_OUTPUT_PATH)
  SET (VTK_EXECUTABLE ${VTK_BINARY_DIR}/Wrapping/Tcl/${CMAKE_CFG_INTDIR}/vtk)
ENDIF (EXECUTABLE_OUTPUT_PATH)

SET(TEST_PATH "Packages")
SET(RUN_WITH_MPI 0)
IF (VTK_USE_MPI)
  IF (UNIX)
    IF (MPIRUN)
      SET(RUN_WITH_MPI 1)
    ENDIF (MPIRUN)
  ENDIF (UNIX)
ENDIF (VTK_USE_MPI)

SET(PROJECT_TESTS 
		 SimplePackage TestPolyDataIO)

IF(RUN_WITH_MPI)
  FOREACH (tfile ${PROJECT_TESTS})
    ADD_TEST(${tfile}-MPI ${MPIRUN} ${MPI_PREFLAGS} ${PARAVIEW_EXECUTABLE}
             ${PARAVIEW_SOURCE_DIR}/${TEST_PATH}/Testing/Tcl/${tfile}.pvs
               -D ${PARAVIEW_SOURCE_DIR}/Data 
               -C ${PARAVIEW_SOURCE_DIR}/Widgets/Testing/Tcl/CompareImages.tcl
               -V ${PARAVIEW_SOURCE_DIR}/Data/Baseline/${tfile}.png
                  ${MPI_POSTFLAGS})
  ENDFOREACH(tfile)
ENDIF(RUN_WITH_MPI)
FOREACH (tfile ${PROJECT_TESTS})
  ADD_TEST(${tfile} ${PARAVIEW_EXECUTABLE}
           ${PARAVIEW_SOURCE_DIR}/${TEST_PATH}/Testing/Tcl/${tfile}.pvs
             -D ${PARAVIEW_SOURCE_DIR}/Data 
             -C ${PARAVIEW_SOURCE_DIR}/Widgets/Testing/Tcl/CompareImages.tcl
             -V ${PARAVIEW_SOURCE_DIR}/Data/Baseline/${tfile}.png)
ENDFOREACH (tfile)

ADD_TEST(PrintSelf-${TEST_PATH} ${TCL_TCLSH}
         ${VTK_SOURCE_DIR}/Common/Testing/Tcl/PrintSelfCheck.tcl  
         ${PARAVIEW_SOURCE_DIR}/${TEST_PATH})

ADD_TEST(TestSetObjectMacro-${TEST_PATH} ${TCL_TCLSH}
         ${KWCommon_SOURCE_DIR}/Testing/Tcl/FindString.tcl
         "${PARAVIEW_SOURCE_DIR}/${TEST_PATH}/vtk\\\\*.h"
         "vtkSetObjectMacro")
