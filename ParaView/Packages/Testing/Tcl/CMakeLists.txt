IF (EXECUTABLE_OUTPUT_PATH)
  SET (VTK_EXECUTABLE ${EXECUTABLE_OUTPUT_PATH}/vtk)
ELSE (EXECUTABLE_OUTPUT_PATH)
  SET (VTK_EXECUTABLE ${VTK_BINARY_DIR}/Wrapping/Tcl/${CMAKE_CFG_INTDIR}/vtk)
ENDIF (EXECUTABLE_OUTPUT_PATH)

IF (VTK_USE_MPI)
  IF (UNIX)
    IF (MPIRUN)
      FOREACH (tfile
	      SimplePackage
               )

         ADD_TEST(${tfile}-MPI ${MPIRUN} ${MPI_PREFLAGS} ${PARAVIEW_EXECUTABLE}
                  ${PARAVIEW_SOURCE_DIR}/Packages/Testing/Tcl/${tfile}.pvs
                  -D ${PARAVIEW_SOURCE_DIR}/Data 
                  -C ${PARAVIEW_SOURCE_DIR}/Widgets/Testing/Tcl/CompareImages.tcl
                  -V ${PARAVIEW_SOURCE_DIR}/Data/Baseline/${tfile}.png
                  ${MPI_POSTFLAGS})
      ENDFOREACH (tfile)


    ENDIF (MPIRUN)
  ENDIF (UNIX)
ENDIF (VTK_USE_MPI)

FOREACH (tfile
	SimplePackage
         )

   ADD_TEST(${tfile} ${PARAVIEW_EXECUTABLE}
            ${PARAVIEW_SOURCE_DIR}/Packages/Testing/Tcl/${tfile}.pvs
            -D ${PARAVIEW_SOURCE_DIR}/Data 
            -C ${PARAVIEW_SOURCE_DIR}/Widgets/Testing/Tcl/CompareImages.tcl
            -V ${PARAVIEW_SOURCE_DIR}/Data/Baseline/${tfile}.png)
ENDFOREACH (tfile)

ADD_TEST(PrintSelf-Packages ${TCL_TCLSH}
         ${VTK_SOURCE_DIR}/Common/Testing/Tcl/PrintSelfCheck.tcl  
         ${PARAVIEW_SOURCE_DIR}/ParaView)

#ADD_TEST(TestWrongArgument ${PARAVIEW_EXECUTABLE} -bla)

ADD_TEST(TestSetObjectMacro-Packages ${TCL_TCLSH}
         ${KWCommon_SOURCE_DIR}/Testing/Tcl/FindString.tcl
         "${PARAVIEW_SOURCE_DIR}/ParaView/vtk\\\\*.h"
         "vtkSetObjectMacro")

