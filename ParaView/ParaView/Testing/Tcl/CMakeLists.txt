CONFIGURE_FILE(
${PARAVIEW_SOURCE_DIR}/ParaView/Testing/Tcl/BlowTest.tcl.in
${PARAVIEW_BINARY_DIR}/ParaView/Testing/Tcl/BlowTest.tcl)

CONFIGURE_FILE(
${PARAVIEW_SOURCE_DIR}/ParaView/Testing/Tcl/SphereTest.tcl.in
${PARAVIEW_BINARY_DIR}/ParaView/Testing/Tcl/SphereTest.tcl)

CONFIGURE_FILE(
${PARAVIEW_SOURCE_DIR}/ParaView/Testing/Tcl/CombustorTest.tcl.in
${PARAVIEW_BINARY_DIR}/ParaView/Testing/Tcl/CombustorTest.tcl)

ADD_TEST(BlowTest ${PARAVIEW_EXECUTABLE}
                  ${PARAVIEW_BINARY_DIR}/ParaView/Testing/Tcl/BlowTest.tcl)

ADD_TEST(SphereTest ${PARAVIEW_EXECUTABLE}
                    ${PARAVIEW_BINARY_DIR}/ParaView/Testing/Tcl/SphereTest.tcl)

ADD_TEST(CombustorTest ${PARAVIEW_EXECUTABLE}
                    ${PARAVIEW_BINARY_DIR}/ParaView/Testing/Tcl/CombustorTest.tcl)
