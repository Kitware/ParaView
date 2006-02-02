PROJECT(WidgetsTour)

# If we are not in the KWWidgets source tree, make sure we can find KWWidgets
# as an external package, and use it. If you are using this CMakeLists.txt 
# file to create your own application based on KWWidgets, you only need the
# FIND_PACKAGE(...) and INCLUDE(...) commands. 

IF (NOT KWWidgets_SOURCE_DIR)
  FIND_PACKAGE(KWWidgets REQUIRED)
  INCLUDE(${KWWidgets_USE_FILE})
ENDIF (NOT KWWidgets_SOURCE_DIR)

# The name of our targets (scripts, mainly) will simply be based
# on the project name, with an extra prefix and suffix.

SET(TARGET_BASE_NAME "KW${PROJECT_NAME}Example")

# The name of our executable script (and additional helper scripts if any).

SET(EXE_NAME "${TARGET_BASE_NAME}")
SET(EXE_SRCS "${EXE_NAME}.tcl")

# Install the scripts in the KWWidgets examples section. If you are
# using this CMakeLists.txt file to create your own application based on
# KWWidgets, you can most likely omit this section, unless you want to 
# install your scripts right where KWWidgets is already installed.

IF(BUILD_EXAMPLES)
  INSTALL_FILES(
    "${KWWidgets_INSTALL_DATA_DIR}/Examples/Tcl/${PROJECT_NAME}"
    .tcl ${EXE_SRCS})
ENDIF(BUILD_EXAMPLES)

# Install all individual widgets examples. They are used by the executable
# to display the C++ and Tcl source code side-by-side.

IF(BUILD_EXAMPLES)
  FILE(GLOB WIDGETS_SRCS "${CMAKE_CURRENT_SOURCE_DIR}/Widgets/*.tcl")
  INSTALL_FILES(
    "${KWWidgets_INSTALL_DATA_DIR}/Examples/Tcl/${PROJECT_NAME}/Widgets" 
    .tcl ${WIDGETS_SRCS})
ENDIF(BUILD_EXAMPLES)

# Register this example as a test. Our executable script supports a --test
# configuration option so that it can be run non-interactively as a test.
# If you are using this CMakeLists.txt file to create your own application
# based on KWWidgets, you can most likely omit this section too, unless
# your application supports that feature too.

IF(BUILD_TESTING AND KWWidgets_BUILD_SHARED_LIBS)
  IF (VTK_WRAP_TCL AND TCL_TCLSH)
    ADD_TEST(Test${EXE_NAME}Tcl 
      ${TCL_TCLSH} "${CMAKE_CURRENT_SOURCE_DIR}/${EXE_NAME}.tcl" --test)
  ENDIF (VTK_WRAP_TCL AND TCL_TCLSH)
ENDIF(BUILD_TESTING AND KWWidgets_BUILD_SHARED_LIBS)
