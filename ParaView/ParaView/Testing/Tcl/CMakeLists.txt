IF (EXECUTABLE_OUTPUT_PATH)
  SET (VTK_EXECUTABLE ${EXECUTABLE_OUTPUT_PATH}/vtk)
ELSE (EXECUTABLE_OUTPUT_PATH)
  SET (VTK_EXECUTABLE ${VTK_BINARY_DIR}/Wrapping/Tcl/${CMAKE_CFG_INTDIR}/vtk)
ENDIF (EXECUTABLE_OUTPUT_PATH)

IF (VTK_USE_MPI)
  IF (UNIX)
    IF (MPIRUN)
      FOREACH (tfile
               ArrayMenuChangeInput
               Arrow
               Axes
               BlowTest
               BoundsDisplay
               CalcInput
               Calculator
               ClipCut
               CombustorTest
               ConeCubeCylSQ
               CutContours
	       Data
               Delete
               DeleteSource
               EnSight
               Extrusion
               Glyph
               ImageReader
               Mandelbrot
               ProbeLine
               ProbePoint
#               RotateTranslate
               ScalarRangeLabel
               ScalarSelection
               ScalarsSphereClip
               SphereTest
               Threshold
               ThresholdAttrMode
               Trace
               VectorTextSpace)

         ADD_TEST(${tfile}-MPI ${MPIRUN} ${MPI_PREFLAGS} ${PARAVIEW_EXECUTABLE}
                  ${PARAVIEW_SOURCE_DIR}/ParaView/Testing/Tcl/${tfile}.pvs
                  -D ${PARAVIEW_SOURCE_DIR}/Data 
                  -C ${PARAVIEW_SOURCE_DIR}/Widgets/Testing/Tcl/CompareImages.tcl
                  -V ${PARAVIEW_SOURCE_DIR}/Data/Baseline/${tfile}.png
                  ${MPI_POSTFLAGS})
      ENDFOREACH (tfile)


    ENDIF (MPIRUN)
  ENDIF (UNIX)
ENDIF (VTK_USE_MPI)

FOREACH (tfile
         ArrayMenuChangeInput
         Arrow
         Axes
         BlowTest
         BoundsDisplay
         CalcInput
         Calculator
         ClipCut
         CombustorTest
         ConeCubeCylSQ
         CutContours
	 Data
         Delete
         DeleteSource
         EnSight
         Extrusion
         Glyph
         ImageReader
         Mandelbrot
         ProbeLine
         ProbePoint
#         RotateTranslate
         ScalarRangeLabel
         ScalarSelection
         ScalarsSphereClip
         SphereTest
         Threshold
         ThresholdAttrMode
         Trace
         VectorTextSpace)

   ADD_TEST(${tfile} ${PARAVIEW_EXECUTABLE}
            ${PARAVIEW_SOURCE_DIR}/ParaView/Testing/Tcl/${tfile}.pvs
            -D ${PARAVIEW_SOURCE_DIR}/Data 
            -C ${PARAVIEW_SOURCE_DIR}/Widgets/Testing/Tcl/CompareImages.tcl
            -V ${PARAVIEW_SOURCE_DIR}/Data/Baseline/${tfile}.png)
ENDFOREACH (tfile)

ADD_TEST(PrintSelf-Widgets ${TCL_TCLSH}
         ${VTK_SOURCE_DIR}/Common/Testing/Tcl/PrintSelfCheck.tcl  
         ${PARAVIEW_SOURCE_DIR}/ParaView)

ADD_TEST(TestSetObjectMacro-Widgets ${TCL_TCLSH}
         ${KWCommon_SOURCE_DIR}/Testing/Tcl/FindString.tcl
	 "${PARAVIEW_SOURCE_DIR}/ParaView/vtk\\\\*.h"
	 "vtkSetObjectMacro")
