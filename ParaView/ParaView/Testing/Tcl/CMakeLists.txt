IF (VTK_USE_MPI)
  IF (UNIX)
    IF (MPIRUN)
      ADD_TEST(ArrayMenuChangeInput-MPI ${MPIRUN} ${MPI_PREFLAGS} ${PARAVIEW_EXECUTABLE} 
              ${PARAVIEW_SOURCE_DIR}/ParaView/Testing/Tcl/ArrayMenuChangeInput.pvs 
              -D ${PARAVIEW_SOURCE_DIR}/Data ${MPI_POSTFLAGS})

      ADD_TEST(Arrow-MPI ${MPIRUN} ${MPI_PREFLAGS} ${PARAVIEW_EXECUTABLE} 
              ${PARAVIEW_SOURCE_DIR}/ParaView/Testing/Tcl/Arrow.pvs 
              -D ${PARAVIEW_SOURCE_DIR}/Data ${MPI_POSTFLAGS})

      ADD_TEST(Axes-MPI ${MPIRUN} ${MPI_PREFLAGS} ${PARAVIEW_EXECUTABLE} 
              ${PARAVIEW_SOURCE_DIR}/ParaView/Testing/Tcl/Axes.pvs 
              -D ${PARAVIEW_SOURCE_DIR}/Data ${MPI_POSTFLAGS})

      ADD_TEST(BlowTest-MPI ${MPIRUN} ${MPI_PREFLAGS} ${PARAVIEW_EXECUTABLE} 
              ${PARAVIEW_SOURCE_DIR}/ParaView/Testing/Tcl/BlowTest.pvs 
              -D ${PARAVIEW_SOURCE_DIR}/Data ${MPI_POSTFLAGS})

      ADD_TEST(BoundsDisplay-MPI ${MPIRUN} ${MPI_PREFLAGS} ${PARAVIEW_EXECUTABLE} 
              ${PARAVIEW_SOURCE_DIR}/ParaView/Testing/Tcl/BoundsDisplay.pvs 
              -D ${PARAVIEW_SOURCE_DIR}/Data ${MPI_POSTFLAGS})

      ADD_TEST(CalcInput-MPI ${MPIRUN} ${MPI_PREFLAGS} ${PARAVIEW_EXECUTABLE} 
              ${PARAVIEW_SOURCE_DIR}/ParaView/Testing/Tcl/CalcInput.pvs 
              -D ${PARAVIEW_SOURCE_DIR}/Data ${MPI_POSTFLAGS})

      ADD_TEST(Calculator-MPI ${MPIRUN} ${MPI_PREFLAGS} ${PARAVIEW_EXECUTABLE} 
              ${PARAVIEW_SOURCE_DIR}/ParaView/Testing/Tcl/Calculator.pvs 
              -D ${PARAVIEW_SOURCE_DIR}/Data ${MPI_POSTFLAGS})

      ADD_TEST(ClipCut-MPI ${MPIRUN} ${MPI_PREFLAGS} ${PARAVIEW_EXECUTABLE} 
              ${PARAVIEW_SOURCE_DIR}/ParaView/Testing/Tcl/ClipCut.pvs 
              -D ${PARAVIEW_SOURCE_DIR}/Data ${MPI_POSTFLAGS})

      ADD_TEST(CombustorTest-MPI ${MPIRUN} ${MPI_PREFLAGS} ${PARAVIEW_EXECUTABLE} 
              ${PARAVIEW_SOURCE_DIR}/ParaView/Testing/Tcl/CombustorTest.pvs 
              -D ${PARAVIEW_SOURCE_DIR}/Data ${MPI_POSTFLAGS})

      ADD_TEST(ConeCubeCylSQ-MPI ${MPIRUN} ${MPI_PREFLAGS} ${PARAVIEW_EXECUTABLE} 
              ${PARAVIEW_SOURCE_DIR}/ParaView/Testing/Tcl/ConeCubeCylSQ.pvs 
              -D ${PARAVIEW_SOURCE_DIR}/Data ${MPI_POSTFLAGS})

      ADD_TEST(CutContours-MPI ${MPIRUN} ${MPI_PREFLAGS} ${PARAVIEW_EXECUTABLE} 
              ${PARAVIEW_SOURCE_DIR}/ParaView/Testing/Tcl/CutContours.pvs 
              -D ${PARAVIEW_SOURCE_DIR}/Data ${MPI_POSTFLAGS})

      ADD_TEST(Delete-MPI ${MPIRUN} ${MPI_PREFLAGS} ${PARAVIEW_EXECUTABLE} 
              ${PARAVIEW_SOURCE_DIR}/ParaView/Testing/Tcl/Delete.pvs         
              -D ${PARAVIEW_SOURCE_DIR}/Data ${MPI_POSTFLAGS})

      ADD_TEST(EnSight-MPI ${MPIRUN} ${MPI_PREFLAGS} ${PARAVIEW_EXECUTABLE} 
              ${PARAVIEW_SOURCE_DIR}/ParaView/Testing/Tcl/EnSight.pvs 
              -D ${PARAVIEW_SOURCE_DIR}/Data ${MPI_POSTFLAGS})

      ADD_TEST(Extrusion-MPI ${MPIRUN} ${MPI_PREFLAGS} ${PARAVIEW_EXECUTABLE} 
              ${PARAVIEW_SOURCE_DIR}/ParaView/Testing/Tcl/Extrusion.pvs 
              -D ${PARAVIEW_SOURCE_DIR}/Data ${MPI_POSTFLAGS})

      ADD_TEST(Glyph-MPI ${MPIRUN} ${MPI_PREFLAGS} ${PARAVIEW_EXECUTABLE} 
              ${PARAVIEW_SOURCE_DIR}/ParaView/Testing/Tcl/Glyph.pvs 
              -D ${PARAVIEW_SOURCE_DIR}/Data ${MPI_POSTFLAGS})

      ADD_TEST(ImageReader-MPI ${MPIRUN} ${MPI_PREFLAGS} ${PARAVIEW_EXECUTABLE} 
              ${PARAVIEW_SOURCE_DIR}/ParaView/Testing/Tcl/ImageReader.pvs 
              -D ${PARAVIEW_SOURCE_DIR}/Data ${MPI_POSTFLAGS})

      ADD_TEST(Mandelbrot-MPI ${MPIRUN} ${MPI_PREFLAGS} ${PARAVIEW_EXECUTABLE} 
              ${PARAVIEW_SOURCE_DIR}/ParaView/Testing/Tcl/Mandelbrot.pvs 
              -D ${PARAVIEW_SOURCE_DIR}/Data ${MPI_POSTFLAGS})

      ADD_TEST(ProbeLine-MPI ${MPIRUN} ${MPI_PREFLAGS} ${PARAVIEW_EXECUTABLE} 
              ${PARAVIEW_SOURCE_DIR}/ParaView/Testing/Tcl/ProbeLine.pvs 
              -D ${PARAVIEW_SOURCE_DIR}/Data ${MPI_POSTFLAGS})

      ADD_TEST(ProbePoint-MPI ${MPIRUN} ${MPI_PREFLAGS} ${PARAVIEW_EXECUTABLE} 
              ${PARAVIEW_SOURCE_DIR}/ParaView/Testing/Tcl/ProbePoint.pvs 
              -D ${PARAVIEW_SOURCE_DIR}/Data ${MPI_POSTFLAGS})

      ADD_TEST(RotateTranslate-MPI ${MPIRUN} ${MPI_PREFLAGS} ${PARAVIEW_EXECUTABLE} 
              ${PARAVIEW_SOURCE_DIR}/ParaView/Testing/Tcl/RotateTranslate.pvs 
              -D ${PARAVIEW_SOURCE_DIR}/Data ${MPI_POSTFLAGS})

      ADD_TEST(ScalarSelection-MPI ${MPIRUN} ${MPI_PREFLAGS} ${PARAVIEW_EXECUTABLE} 
              ${PARAVIEW_SOURCE_DIR}/ParaView/Testing/Tcl/ScalarSelection.pvs 
              -D ${PARAVIEW_SOURCE_DIR}/Data ${MPI_POSTFLAGS})

      ADD_TEST(SphereTest-MPI ${MPIRUN} ${MPI_PREFLAGS} ${PARAVIEW_EXECUTABLE} 
              ${PARAVIEW_SOURCE_DIR}/ParaView/Testing/Tcl/SphereTest.pvs 
              -D ${PARAVIEW_SOURCE_DIR}/Data ${MPI_POSTFLAGS})

      ADD_TEST(Threshold-MPI ${MPIRUN} ${MPI_PREFLAGS} ${PARAVIEW_EXECUTABLE} 
              ${PARAVIEW_SOURCE_DIR}/ParaView/Testing/Tcl/Threshold.pvs 
              -D ${PARAVIEW_SOURCE_DIR}/Data ${MPI_POSTFLAGS})

      ADD_TEST(ThresholdAttrMode-MPI ${MPIRUN} ${MPI_PREFLAGS} ${PARAVIEW_EXECUTABLE} 
              ${PARAVIEW_SOURCE_DIR}/ParaView/Testing/Tcl/ThresholdAttrMode.pvs 
              -D ${PARAVIEW_SOURCE_DIR}/Data ${MPI_POSTFLAGS})

      ADD_TEST(Trace-MPI ${MPIRUN} ${MPI_PREFLAGS} ${PARAVIEW_EXECUTABLE} 
              ${PARAVIEW_SOURCE_DIR}/ParaView/Testing/Tcl/Trace.pvs 
              -D ${PARAVIEW_SOURCE_DIR}/Data -D ${PARAVIEW_SOURCE_DIR}/Data ${MPI_POSTFLAGS})

      ADD_TEST(VectorTextSpace-MPI ${MPIRUN} ${MPI_PREFLAGS} ${PARAVIEW_EXECUTABLE} 
              ${PARAVIEW_SOURCE_DIR}/ParaView/Testing/Tcl/VectorTextSpace.pvs 
              -D ${PARAVIEW_SOURCE_DIR}/Data ${MPI_POSTFLAGS})

    ENDIF (MPIRUN)
  ENDIF (UNIX)
ENDIF (VTK_USE_MPI)

ADD_TEST(ArrayMenuChangeInput ${PARAVIEW_EXECUTABLE}
         ${PARAVIEW_SOURCE_DIR}/ParaView/Testing/Tcl/ArrayMenuChangeInput.pvs 
         -D ${PARAVIEW_SOURCE_DIR}/Data)

ADD_TEST(Arrow ${PARAVIEW_EXECUTABLE}
         ${PARAVIEW_SOURCE_DIR}/ParaView/Testing/Tcl/Arrow.pvs 
         -D ${PARAVIEW_SOURCE_DIR}/Data)

ADD_TEST(Axes ${PARAVIEW_EXECUTABLE}
         ${PARAVIEW_SOURCE_DIR}/ParaView/Testing/Tcl/Axes.pvs 
         -D ${PARAVIEW_SOURCE_DIR}/Data)

ADD_TEST(BlowTest ${PARAVIEW_EXECUTABLE}
         ${PARAVIEW_SOURCE_DIR}/ParaView/Testing/Tcl/BlowTest.pvs 
         -D ${PARAVIEW_SOURCE_DIR}/Data)

ADD_TEST(BoundsDisplay ${PARAVIEW_EXECUTABLE}
         ${PARAVIEW_SOURCE_DIR}/ParaView/Testing/Tcl/BoundsDisplay.pvs 
         -D ${PARAVIEW_SOURCE_DIR}/Data)

ADD_TEST(CalcInput ${PARAVIEW_EXECUTABLE}
         ${PARAVIEW_SOURCE_DIR}/ParaView/Testing/Tcl/CalcInput.pvs 
         -D ${PARAVIEW_SOURCE_DIR}/Data)

ADD_TEST(Calculator ${PARAVIEW_EXECUTABLE}
         ${PARAVIEW_SOURCE_DIR}/ParaView/Testing/Tcl/Calculator.pvs 
         -D ${PARAVIEW_SOURCE_DIR}/Data)

ADD_TEST(ClipCut ${PARAVIEW_EXECUTABLE}
         ${PARAVIEW_SOURCE_DIR}/ParaView/Testing/Tcl/ClipCut.pvs 
         -D ${PARAVIEW_SOURCE_DIR}/Data)

ADD_TEST(CombustorTest ${PARAVIEW_EXECUTABLE}
         ${PARAVIEW_SOURCE_DIR}/ParaView/Testing/Tcl/CombustorTest.pvs 
         -D ${PARAVIEW_SOURCE_DIR}/Data)

ADD_TEST(ConeCubeCylSQ ${PARAVIEW_EXECUTABLE}
         ${PARAVIEW_SOURCE_DIR}/ParaView/Testing/Tcl/ConeCubeCylSQ.pvs 
         -D ${PARAVIEW_SOURCE_DIR}/Data)

ADD_TEST(CutContours ${PARAVIEW_EXECUTABLE}
         ${PARAVIEW_SOURCE_DIR}/ParaView/Testing/Tcl/CutContours.pvs 
         -D ${PARAVIEW_SOURCE_DIR}/Data)

ADD_TEST(Delete ${PARAVIEW_EXECUTABLE}
         ${PARAVIEW_SOURCE_DIR}/ParaView/Testing/Tcl/Delete.pvs         
         -D ${PARAVIEW_SOURCE_DIR}/Data)

ADD_TEST(EnSight ${PARAVIEW_EXECUTABLE}
         ${PARAVIEW_SOURCE_DIR}/ParaView/Testing/Tcl/EnSight.pvs 
         -D ${PARAVIEW_SOURCE_DIR}/Data)

ADD_TEST(Extrusion ${PARAVIEW_EXECUTABLE}
         ${PARAVIEW_SOURCE_DIR}/ParaView/Testing/Tcl/Extrusion.pvs 
         -D ${PARAVIEW_SOURCE_DIR}/Data)

ADD_TEST(Glyph ${PARAVIEW_EXECUTABLE}
         ${PARAVIEW_SOURCE_DIR}/ParaView/Testing/Tcl/Glyph.pvs 
         -D ${PARAVIEW_SOURCE_DIR}/Data)

ADD_TEST(ImageReader ${PARAVIEW_EXECUTABLE}
         ${PARAVIEW_SOURCE_DIR}/ParaView/Testing/Tcl/ImageReader.pvs 
         -D ${PARAVIEW_SOURCE_DIR}/Data)

ADD_TEST(Mandelbrot ${PARAVIEW_EXECUTABLE}
         ${PARAVIEW_SOURCE_DIR}/ParaView/Testing/Tcl/Mandelbrot.pvs 
         -D ${PARAVIEW_SOURCE_DIR}/Data)

ADD_TEST(ProbeLine ${PARAVIEW_EXECUTABLE}
         ${PARAVIEW_SOURCE_DIR}/ParaView/Testing/Tcl/ProbeLine.pvs 
         -D ${PARAVIEW_SOURCE_DIR}/Data)

ADD_TEST(ProbePoint ${PARAVIEW_EXECUTABLE}
         ${PARAVIEW_SOURCE_DIR}/ParaView/Testing/Tcl/ProbePoint.pvs 
         -D ${PARAVIEW_SOURCE_DIR}/Data)

ADD_TEST(RotateTranslate ${PARAVIEW_EXECUTABLE}
         ${PARAVIEW_SOURCE_DIR}/ParaView/Testing/Tcl/RotateTranslate.pvs 
         -D ${PARAVIEW_SOURCE_DIR}/Data)

ADD_TEST(ScalarSelection ${PARAVIEW_EXECUTABLE}
         ${PARAVIEW_SOURCE_DIR}/ParaView/Testing/Tcl/ScalarSelection.pvs 
         -D ${PARAVIEW_SOURCE_DIR}/Data)

ADD_TEST(SphereTest ${PARAVIEW_EXECUTABLE}
         ${PARAVIEW_SOURCE_DIR}/ParaView/Testing/Tcl/SphereTest.pvs 
         -D ${PARAVIEW_SOURCE_DIR}/Data)

ADD_TEST(Threshold ${PARAVIEW_EXECUTABLE}
         ${PARAVIEW_SOURCE_DIR}/ParaView/Testing/Tcl/Threshold.pvs 
         -D ${PARAVIEW_SOURCE_DIR}/Data)

ADD_TEST(ThresholdAttrMode ${PARAVIEW_EXECUTABLE}
         ${PARAVIEW_SOURCE_DIR}/ParaView/Testing/Tcl/ThresholdAttrMode.pvs 
         -D ${PARAVIEW_SOURCE_DIR}/Data)

ADD_TEST(Trace ${PARAVIEW_EXECUTABLE}
         ${PARAVIEW_SOURCE_DIR}/ParaView/Testing/Tcl/Trace.pvs 
         -D ${PARAVIEW_SOURCE_DIR}/Data -D ${PARAVIEW_SOURCE_DIR}/Data)

ADD_TEST(VectorTextSpace ${PARAVIEW_EXECUTABLE}
         ${PARAVIEW_SOURCE_DIR}/ParaView/Testing/Tcl/VectorTextSpace.pvs 
         -D ${PARAVIEW_SOURCE_DIR}/Data)
