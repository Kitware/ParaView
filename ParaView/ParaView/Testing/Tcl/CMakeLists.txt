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
               RotateTranslate
               ScalarRangeLabel
               ScalarSelection
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
         RotateTranslate
         ScalarRangeLabel
         ScalarSelection
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

