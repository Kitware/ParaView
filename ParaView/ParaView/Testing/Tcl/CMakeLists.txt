
ADD_TEST(Arrow ${PARAVIEW_EXECUTABLE}
         ${PARAVIEW_SOURCE_DIR}/ParaView/Testing/Tcl/Arrow.tcl 
         -D ${PARAVIEW_SOURCE_DIR}/Data)

ADD_TEST(Axes ${PARAVIEW_EXECUTABLE}
         ${PARAVIEW_SOURCE_DIR}/ParaView/Testing/Tcl/Axes.tcl 
         -D ${PARAVIEW_SOURCE_DIR}/Data)

ADD_TEST(BlowTest ${PARAVIEW_EXECUTABLE}
         ${PARAVIEW_SOURCE_DIR}/ParaView/Testing/Tcl/BlowTest.tcl 
         -D ${PARAVIEW_SOURCE_DIR}/Data)

ADD_TEST(CalcInput ${PARAVIEW_EXECUTABLE}
         ${PARAVIEW_SOURCE_DIR}/ParaView/Testing/Tcl/CalcInput.tcl 
         -D ${PARAVIEW_SOURCE_DIR}/Data)

ADD_TEST(Calculator ${PARAVIEW_EXECUTABLE}
         ${PARAVIEW_SOURCE_DIR}/ParaView/Testing/Tcl/Calculator.tcl 
         -D ${PARAVIEW_SOURCE_DIR}/Data)

ADD_TEST(ClipCut ${PARAVIEW_EXECUTABLE}
         ${PARAVIEW_SOURCE_DIR}/ParaView/Testing/Tcl/ClipCut.tcl 
         -D ${PARAVIEW_SOURCE_DIR}/Data)

ADD_TEST(CombustorTest ${PARAVIEW_EXECUTABLE}
         ${PARAVIEW_SOURCE_DIR}/ParaView/Testing/Tcl/CombustorTest.tcl 
         -D ${PARAVIEW_SOURCE_DIR}/Data)

ADD_TEST(ConeCubeCylSQ ${PARAVIEW_EXECUTABLE}
         ${PARAVIEW_SOURCE_DIR}/ParaView/Testing/Tcl/ConeCubeCylSQ.tcl 
         -D ${PARAVIEW_SOURCE_DIR}/Data)

ADD_TEST(CutContours ${PARAVIEW_EXECUTABLE}
         ${PARAVIEW_SOURCE_DIR}/ParaView/Testing/Tcl/CutContours.tcl 
         -D ${PARAVIEW_SOURCE_DIR}/Data)

ADD_TEST(Delete ${PARAVIEW_EXECUTABLE}
         ${PARAVIEW_SOURCE_DIR}/ParaView/Testing/Tcl/Delete.tcl         
         -D ${PARAVIEW_SOURCE_DIR}/Data)

ADD_TEST(EnSight ${PARAVIEW_EXECUTABLE}
         ${PARAVIEW_SOURCE_DIR}/ParaView/Testing/Tcl/EnSight.tcl 
         -D ${PARAVIEW_SOURCE_DIR}/Data)

ADD_TEST(Extrusion ${PARAVIEW_EXECUTABLE}
         ${PARAVIEW_SOURCE_DIR}/ParaView/Testing/Tcl/Extrusion.tcl 
         -D ${PARAVIEW_SOURCE_DIR}/Data)

ADD_TEST(Glyph ${PARAVIEW_EXECUTABLE}
         ${PARAVIEW_SOURCE_DIR}/ParaView/Testing/Tcl/Glyph.tcl 
         -D ${PARAVIEW_SOURCE_DIR}/Data)

ADD_TEST(ImageReader ${PARAVIEW_EXECUTABLE}
         ${PARAVIEW_SOURCE_DIR}/ParaView/Testing/Tcl/ImageReader.tcl 
         -D ${PARAVIEW_SOURCE_DIR}/Data)

ADD_TEST(Mandelbrot ${PARAVIEW_EXECUTABLE}
         ${PARAVIEW_SOURCE_DIR}/ParaView/Testing/Tcl/Mandelbrot.tcl 
         -D ${PARAVIEW_SOURCE_DIR}/Data)

ADD_TEST(ProbeLine ${PARAVIEW_EXECUTABLE}
         ${PARAVIEW_SOURCE_DIR}/ParaView/Testing/Tcl/ProbeLine.tcl 
         -D ${PARAVIEW_SOURCE_DIR}/Data)

ADD_TEST(ProbePoint ${PARAVIEW_EXECUTABLE}
         ${PARAVIEW_SOURCE_DIR}/ParaView/Testing/Tcl/ProbePoint.tcl 
         -D ${PARAVIEW_SOURCE_DIR}/Data)

ADD_TEST(RotateTranslate ${PARAVIEW_EXECUTABLE}
         ${PARAVIEW_SOURCE_DIR}/ParaView/Testing/Tcl/RotateTranslate.tcl 
         -D ${PARAVIEW_SOURCE_DIR}/Data)

ADD_TEST(ScalarSelection ${PARAVIEW_EXECUTABLE}
         ${PARAVIEW_SOURCE_DIR}/ParaView/Testing/Tcl/ScalarSelection.tcl 
         -D ${PARAVIEW_SOURCE_DIR}/Data)

ADD_TEST(SphereTest ${PARAVIEW_EXECUTABLE}
         ${PARAVIEW_SOURCE_DIR}/ParaView/Testing/Tcl/SphereTest.tcl 
         -D ${PARAVIEW_SOURCE_DIR}/Data)

ADD_TEST(Threshold ${PARAVIEW_EXECUTABLE}
         ${PARAVIEW_SOURCE_DIR}/ParaView/Testing/Tcl/Threshold.tcl 
         -D ${PARAVIEW_SOURCE_DIR}/Data)

ADD_TEST(ThresholdAttrMode ${PARAVIEW_EXECUTABLE}
         ${PARAVIEW_SOURCE_DIR}/ParaView/Testing/Tcl/ThresholdAttrMode.tcl 
         -D ${PARAVIEW_SOURCE_DIR}/Data)

ADD_TEST(Trace ${PARAVIEW_EXECUTABLE}
         ${PARAVIEW_SOURCE_DIR}/ParaView/Testing/Tcl/Trace.tcl 
         -D ${PARAVIEW_SOURCE_DIR}/Data -D ${PARAVIEW_SOURCE_DIR}/Data)

ADD_TEST(VectorTextSpace ${PARAVIEW_EXECUTABLE}
         ${PARAVIEW_SOURCE_DIR}/ParaView/Testing/Tcl/VectorTextSpace.tcl 
         -D ${PARAVIEW_SOURCE_DIR}/Data)
