CONFIGURE_FILE(
${PARAVIEW_SOURCE_DIR}/ParaView/Testing/Tcl/Arrow.tcl.in
${PARAVIEW_BINARY_DIR}/ParaView/Testing/Tcl/Arrow.tcl)

CONFIGURE_FILE(
${PARAVIEW_SOURCE_DIR}/ParaView/Testing/Tcl/Axes.tcl.in
${PARAVIEW_BINARY_DIR}/ParaView/Testing/Tcl/Axes.tcl)

CONFIGURE_FILE(
${PARAVIEW_SOURCE_DIR}/ParaView/Testing/Tcl/BlowTest.tcl.in
${PARAVIEW_BINARY_DIR}/ParaView/Testing/Tcl/BlowTest.tcl)

CONFIGURE_FILE(
${PARAVIEW_SOURCE_DIR}/ParaView/Testing/Tcl/Calculator.tcl.in
${PARAVIEW_BINARY_DIR}/ParaView/Testing/Tcl/Calculator.tcl)

CONFIGURE_FILE(
${PARAVIEW_SOURCE_DIR}/ParaView/Testing/Tcl/ClipCut.tcl.in
${PARAVIEW_BINARY_DIR}/ParaView/Testing/Tcl/ClipCut.tcl)

CONFIGURE_FILE(
${PARAVIEW_SOURCE_DIR}/ParaView/Testing/Tcl/CombustorTest.tcl.in
${PARAVIEW_BINARY_DIR}/ParaView/Testing/Tcl/CombustorTest.tcl)

CONFIGURE_FILE(
${PARAVIEW_SOURCE_DIR}/ParaView/Testing/Tcl/ConeCubeCylSQ.tcl.in
${PARAVIEW_BINARY_DIR}/ParaView/Testing/Tcl/ConeCubeCylSQ.tcl)

CONFIGURE_FILE(
${PARAVIEW_SOURCE_DIR}/ParaView/Testing/Tcl/EnSight.tcl.in
${PARAVIEW_BINARY_DIR}/ParaView/Testing/Tcl/EnSight.tcl)

CONFIGURE_FILE(
${PARAVIEW_SOURCE_DIR}/ParaView/Testing/Tcl/Extrusion.tcl.in
${PARAVIEW_BINARY_DIR}/ParaView/Testing/Tcl/Extrusion.tcl)

CONFIGURE_FILE(
${PARAVIEW_SOURCE_DIR}/ParaView/Testing/Tcl/Glyph.tcl.in
${PARAVIEW_BINARY_DIR}/ParaView/Testing/Tcl/Glyph.tcl)

CONFIGURE_FILE(
${PARAVIEW_SOURCE_DIR}/ParaView/Testing/Tcl/ImageReader.tcl.in
${PARAVIEW_BINARY_DIR}/ParaView/Testing/Tcl/ImageReader.tcl)

CONFIGURE_FILE(
${PARAVIEW_SOURCE_DIR}/ParaView/Testing/Tcl/Mandelbrot.tcl.in
${PARAVIEW_BINARY_DIR}/ParaView/Testing/Tcl/Mandelbrot.tcl)

CONFIGURE_FILE(
${PARAVIEW_SOURCE_DIR}/ParaView/Testing/Tcl/SphereTest.tcl.in
${PARAVIEW_BINARY_DIR}/ParaView/Testing/Tcl/SphereTest.tcl)

CONFIGURE_FILE(
${PARAVIEW_SOURCE_DIR}/ParaView/Testing/Tcl/Threshold.tcl.in
${PARAVIEW_BINARY_DIR}/ParaView/Testing/Tcl/Threshold.tcl)

CONFIGURE_FILE(
${PARAVIEW_SOURCE_DIR}/ParaView/Testing/Tcl/ThresholdAttrMode.tcl.in
${PARAVIEW_BINARY_DIR}/ParaView/Testing/Tcl/ThresholdAttrMode.tcl)

ADD_TEST(Arrow ${PARAVIEW_EXECUTABLE}
                    ${PARAVIEW_BINARY_DIR}/ParaView/Testing/Tcl/Arrow.tcl)

ADD_TEST(Axes ${PARAVIEW_EXECUTABLE}
                    ${PARAVIEW_BINARY_DIR}/ParaView/Testing/Tcl/Axes.tcl)

ADD_TEST(BlowTest ${PARAVIEW_EXECUTABLE}
                  ${PARAVIEW_BINARY_DIR}/ParaView/Testing/Tcl/BlowTest.tcl)

ADD_TEST(Calculator ${PARAVIEW_EXECUTABLE}
                  ${PARAVIEW_BINARY_DIR}/ParaView/Testing/Tcl/Calculator.tcl)

ADD_TEST(ClipCut ${PARAVIEW_EXECUTABLE}
                  ${PARAVIEW_BINARY_DIR}/ParaView/Testing/Tcl/ClipCut.tcl)

ADD_TEST(CombustorTest ${PARAVIEW_EXECUTABLE}
                    ${PARAVIEW_BINARY_DIR}/ParaView/Testing/Tcl/CombustorTest.tcl)

ADD_TEST(ConeCubeCylSQ ${PARAVIEW_EXECUTABLE}
                 ${PARAVIEW_BINARY_DIR}/ParaView/Testing/Tcl/ConeCubeCylSQ.tcl)

ADD_TEST(EnSight ${PARAVIEW_EXECUTABLE}
                   ${PARAVIEW_BINARY_DIR}/ParaView/Testing/Tcl/EnSight.tcl)

ADD_TEST(Extrusion ${PARAVIEW_EXECUTABLE}
                    ${PARAVIEW_BINARY_DIR}/ParaView/Testing/Tcl/Extrusion.tcl)

ADD_TEST(Glyph ${PARAVIEW_EXECUTABLE}
                    ${PARAVIEW_BINARY_DIR}/ParaView/Testing/Tcl/Glyph.tcl)

ADD_TEST(ImageReader ${PARAVIEW_EXECUTABLE}
                   ${PARAVIEW_BINARY_DIR}/ParaView/Testing/Tcl/ImageReader.tcl)

ADD_TEST(Mandelbrot ${PARAVIEW_EXECUTABLE}
                    ${PARAVIEW_BINARY_DIR}/ParaView/Testing/Tcl/Mandelbrot.tcl)

ADD_TEST(SphereTest ${PARAVIEW_EXECUTABLE}
                    ${PARAVIEW_BINARY_DIR}/ParaView/Testing/Tcl/SphereTest.tcl)

ADD_TEST(Threshold ${PARAVIEW_EXECUTABLE}
                    ${PARAVIEW_BINARY_DIR}/ParaView/Testing/Tcl/Threshold.tcl)

ADD_TEST(ThresholdAttrMode ${PARAVIEW_EXECUTABLE}
             ${PARAVIEW_BINARY_DIR}/ParaView/Testing/Tcl/ThresholdAttrMode.tcl)
