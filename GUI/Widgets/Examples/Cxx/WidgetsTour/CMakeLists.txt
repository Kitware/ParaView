PROJECT(WidgetsTour)

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

IF(WIN32)
  IF(NOT BORLAND)
    IF(NOT CYGWIN)
      INCLUDE_DIRECTORIES(${VTK_TK_RESOURCES_DIR})
      SET(RES_FILE "${CMAKE_CURRENT_BINARY_DIR}/${EXE_NAME}.rc")
      SET(KWWidgets_RESOURCE_APPLICATION_NAME "${EXE_NAME}")
      SET(KWWidgets_RESOURCE_FILE_NAME "${EXE_NAME}")
      CONFIGURE_FILE(${KWWidgets_RESOURCES_DIR}/KWWidgets.rc.in ${RES_FILE})
    ENDIF(NOT CYGWIN)
  ENDIF(NOT BORLAND)
ENDIF(WIN32)

# This example uses some files from the KWWidgets distribution tree.
# Let's configure KWWidgets's vtkKWWidgetsPaths.h.in into our
# own header file so that we can find the paths to KWWidgets files.

INCLUDE_DIRECTORIES(${CMAKE_CURRENT_BINARY_DIR})
CONFIGURE_FILE(
  ${KWWidgets_TEMPLATES_DIR}/vtkKWWidgetsPaths.h.in 
  ${CMAKE_CURRENT_BINARY_DIR}/vtkKWWidgetsPaths.h)

# Now this gets tricky. We are going to iterate over all examples file
# in the Widgets sub-directory, and create a header file with the expected
# entry points for each one of them. This allows us to just drop examples
# in the Widgets sub-directory without having to modify an extra header
# file or a CMakeLists.txt file.

FILE(GLOB WIDGETS_SRCS "${CMAKE_CURRENT_SOURCE_DIR}/Widgets/*.cxx")
FOREACH(file ${WIDGETS_SRCS})
  GET_FILENAME_COMPONENT(filename ${file} NAME)
  GET_FILENAME_COMPONENT(WIDGET_NAME ${file} NAME_WE)
  SET(WIDGET_ENTRY_POINT "${WIDGET_NAME}EntryPoint")
  SET(WIDGETS_FORWARD_DECL
    "${WIDGETS_FORWARD_DECL}
         extern KWWidgetsTourItem* ${WIDGET_ENTRY_POINT}(vtkKWWidget *parent, vtkKWWindow *win);")
  SET(WIDGETS_NODES
    "${WIDGETS_NODES}
         {\"${WIDGET_NAME}\", ${WIDGET_ENTRY_POINT}},")
ENDFOREACH(file)

CONFIGURE_FILE(
  ${CMAKE_CURRENT_SOURCE_DIR}/KWWidgetsTourExampleEntryPoints.h.in
  ${CMAKE_CURRENT_BINARY_DIR}/KWWidgetsTourExampleEntryPoints.h
  IMMEDIATE)
INCLUDE_DIRECTORIES(${CMAKE_CURRENT_SOURCE_DIR} ${CMAKE_CURRENT_BINARY_DIR})

# Create the executable, and link it against the KWWidgets libraries.

ADD_EXECUTABLE(${EXE_NAME} WIN32 ${EXE_SRCS} ${WIDGETS_SRCS} ${RES_FILE})
TARGET_LINK_LIBRARIES(${EXE_NAME} ${KWWidgets_LIBRARIES})

# If needed, copy the Tcl/Tk support files required at run-time 
# to initialize Tcl/Tk. This is only triggered if VTK was built
# against a Tcl/Tk static library.

IF(NOT KWWidgets_SOURCE_DIR)
  IF(VTK_TCL_TK_COPY_SUPPORT_LIBRARY)
    IF(VTK_TCL_SUPPORT_LIBRARY_PATH AND VTK_TK_SUPPORT_LIBRARY_PATH)
      INCLUDE(${VTK_TCL_TK_MACROS_MODULE})
      VTK_COPY_TCL_TK_SUPPORT_FILES_TO_DIR(
        ${VTK_TCL_SUPPORT_LIBRARY_PATH} ${VTK_TK_SUPPORT_LIBRARY_PATH}
        "${PROJECT_BINARY_DIR}/lib")
    ENDIF(VTK_TCL_SUPPORT_LIBRARY_PATH AND VTK_TK_SUPPORT_LIBRARY_PATH)
  ENDIF(VTK_TCL_TK_COPY_SUPPORT_LIBRARY)
ENDIF(NOT KWWidgets_SOURCE_DIR)

# Install the example target. If you are using this CMakeLists.txt file
# to create your own application based on KWWidgets, you can most likely 
# omit this section or change the installation directory, unless you want
# to install your application right where KWWidgets is already installed.

IF(BUILD_EXAMPLES)
  INSTALL_TARGETS(${KWWidgets_INSTALL_BIN_DIR} ${EXE_NAME})
ENDIF(BUILD_EXAMPLES)

# Install all individual widgets examples. They are used by the executable
# to display the C++ and Tcl source code side-by-side.

IF(BUILD_EXAMPLES)
  INSTALL_FILES(
    "${KWWidgets_INSTALL_DATA_DIR}/Examples/Cxx/${PROJECT_NAME}/Widgets"
    .cxx ${WIDGETS_SRCS})
ENDIF(BUILD_EXAMPLES)

# Register this example as a test. Our executable supports a --test
# configuration option so that it can be run non-interactively as a test.
# If you are using this CMakeLists.txt file to create your own application
# based on KWWidgets, you can most likely omit this section too, unless
# your application supports that feature too.

IF(BUILD_TESTING)
  ADD_TEST(Test${EXE_NAME} ${EXECUTABLE_OUTPUT_PATH}/${EXE_NAME} --test)
  IF(KWWidgets_SOURCE_DIR)
    ADD_TEST(Test${EXE_NAME}OutOfSource ${CMAKE_CTEST_COMMAND}
      --build-and-test 
      "${CMAKE_CURRENT_SOURCE_DIR}" "${CMAKE_CURRENT_BINARY_DIR}OutOfSource"
      --build-generator ${CMAKE_GENERATOR}
      --build-makeprogram ${CMAKE_MAKE_PROGRAM}
      --build-project ${PROJECT_NAME}
      --build-options "-DKWWidgets_DIR:PATH=${KWWidgets_BINARY_DIR}"
      --test-command ${EXE_NAME} --test)
  ENDIF(KWWidgets_SOURCE_DIR)
ENDIF(BUILD_TESTING)
