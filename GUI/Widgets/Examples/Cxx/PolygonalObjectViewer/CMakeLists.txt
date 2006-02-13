PROJECT(PolygonalObjectViewer)

# If we are not in the KWWidgets source tree, make sure we can find KWWidgets
# as an external package, and use it. If you are using this CMakeLists.txt 
# file to create your own application based on KWWidgets, you only need the
# FIND_PACKAGE(...) and INCLUDE(...) commands. 

IF(NOT KWWidgets_SOURCE_DIR)
  FIND_PACKAGE(KWWidgets REQUIRED)
  INCLUDE(${KWWidgets_USE_FILE})
ENDIF(NOT KWWidgets_SOURCE_DIR)

# The name of our targets (executable or libraries) will simply be based
# on the project name, with an extra prefix and suffix.

SET(TARGET_BASE_NAME "KW${PROJECT_NAME}Example")

# The name of our executable and the corresponding source file.

SET(EXE_NAME "${TARGET_BASE_NAME}")
SET(EXE_SRCS "${EXE_NAME}.cxx")

# On Win32 platforms, let's configure the KWWidgets sample resource file
# to get a nice application icon and some additional information.

IF(WIN32 AND NOT BORLAND AND NOT CYGWIN)
  INCLUDE_DIRECTORIES(${VTK_TK_RESOURCES_DIR})
  SET(RES_FILE "${CMAKE_CURRENT_BINARY_DIR}/${EXE_NAME}.rc")
  SET(KWWidgets_RESOURCE_APPLICATION_NAME "${EXE_NAME}")
  SET(KWWidgets_RESOURCE_FILE_NAME "${EXE_NAME}")
  CONFIGURE_FILE(${KWWidgets_RESOURCES_DIR}/KWWidgets.rc.in ${RES_FILE})
ENDIF(WIN32 AND NOT BORLAND AND NOT CYGWIN)

# This example uses some files from the KWWidgets distribution tree.
# Let's configure KWWidgets's vtkKWWidgetsPaths.h.in into our
# own header file so that we can find the paths to KWWidgets files.

INCLUDE_DIRECTORIES(${CMAKE_CURRENT_BINARY_DIR})
CONFIGURE_FILE(
  ${KWWidgets_TEMPLATES_DIR}/vtkKWWidgetsPaths.h.in 
  ${CMAKE_CURRENT_BINARY_DIR}/vtkKWWidgetsPaths.h)

# Create the executable, and link it against the KWWidgets libraries.

ADD_EXECUTABLE(${EXE_NAME} WIN32 ${EXE_SRCS} ${RES_FILE})
TARGET_LINK_LIBRARIES(${EXE_NAME} ${KWWidgets_LIBRARIES})

# Generate the default setup paths scripts
# If we are building this example as a standalone external project, generate
# some useful scripts that can be used to setup various environment variables
# and launch the executable properly.

IF(NOT KWWidgets_SOURCE_DIR)
  INCLUDE("${KWWidgets_CMAKE_DIR}/KWWidgetsPathsMacros.cmake")
  KWWidgets_GENERATE_DEFAULT_SETUP_PATHS("${CMAKE_CURRENT_BINARY_DIR}")
ENDIF(NOT KWWidgets_SOURCE_DIR)

# If needed, copy the Tcl/Tk support files required at run-time 
# to initialize Tcl/Tk. This is only triggered if VTK was built
# against a Tcl/Tk static library.

IF(NOT KWWidgets_SOURCE_DIR AND VTK_TCL_TK_COPY_SUPPORT_LIBRARY)
  IF(VTK_TCL_SUPPORT_LIBRARY_PATH AND VTK_TK_SUPPORT_LIBRARY_PATH)
    INCLUDE(${VTK_TCL_TK_MACROS_MODULE})
    VTK_COPY_TCL_TK_SUPPORT_FILES_TO_DIR(
      ${VTK_TCL_SUPPORT_LIBRARY_PATH} ${VTK_TK_SUPPORT_LIBRARY_PATH}
      "${PROJECT_BINARY_DIR}/lib")
  ENDIF(VTK_TCL_SUPPORT_LIBRARY_PATH AND VTK_TK_SUPPORT_LIBRARY_PATH)
ENDIF(NOT KWWidgets_SOURCE_DIR AND VTK_TCL_TK_COPY_SUPPORT_LIBRARY)

# Install the example target. If you are using this CMakeLists.txt file
# to create your own application based on KWWidgets, you can most likely 
# omit this section or change the installation directory, unless you want
# to install your application right where KWWidgets is already installed.

IF(BUILD_EXAMPLES)
  INSTALL_TARGETS(${KWWidgets_INSTALL_BIN_DIR} ${EXE_NAME})
ENDIF(BUILD_EXAMPLES)

# Register this example as a test. Our executable supports a --test
# configuration option so that it can be run non-interactively as a test.
# If you are using this CMakeLists.txt file to create your own application
# based on KWWidgets, you can most likely omit this section too, unless
# your application supports that feature too.

IF(BUILD_TESTING)
  ADD_TEST(Test${EXE_NAME} ${EXECUTABLE_OUTPUT_PATH}/${EXE_NAME} --test)
  IF(KWWidgets_SOURCE_DIR)
    INCLUDE("${KWWidgets_CMAKE_DIR}/KWWidgetsTestingMacros.cmake")
    KWWidgets_ADD_OUT_OF_SOURCE_TEST(
      Test${EXE_NAME}OutOfSource
      ${PROJECT_NAME}
      "${CMAKE_CURRENT_SOURCE_DIR}" "${CMAKE_CURRENT_BINARY_DIR}OutOfSource"
      ${EXE_NAME} --test)
  ENDIF(KWWidgets_SOURCE_DIR)
ENDIF(BUILD_TESTING)
