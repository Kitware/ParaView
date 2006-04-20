PROJECT(Internationalization)

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

# Create the executable, and link it against the KWWidgets libraries.

ADD_EXECUTABLE(${EXE_NAME} WIN32 ${EXE_SRCS} ${RES_FILE})
TARGET_LINK_LIBRARIES(${EXE_NAME} ${KWWidgets_LIBRARIES})

# Internationalization

SET(install_locale_dir "/locale")
IF(KWWidgets_SOURCE_DIR)
  SET(install_locale_dir "${KWWidgets_INSTALL_DATA_DIR}${install_locale_dir}")
ENDIF(KWWidgets_SOURCE_DIR)

IF(KWWidgets_USE_INTERNATIONALIZATION)
  FIND_PACKAGE(Gettext REQUIRED)
  INCLUDE("${KWWidgets_CMAKE_DIR}/KWWidgetsInternationalizationMacros.cmake")
  INCLUDE_DIRECTORIES(${GETTEXT_INCLUDE_DIR})
  KWWidgets_CREATE_GETTEXT_TARGETS(
    DOMAIN_NAME ${EXE_NAME} 
    LOCALE_LIST "fr"
    COPYRIGHT_HOLDER "Kitware, Inc."
    SOURCES "${EXE_SRCS}"
    PO_DIR "${CMAKE_CURRENT_SOURCE_DIR}/po"
    POT_BUILD_DIR "${CMAKE_CURRENT_BINARY_DIR}/po"
    PO_BUILD_DIR "${CMAKE_CURRENT_BINARY_DIR}/po"
    MO_INSTALL_DIR "${install_locale_dir}"
    ADD_MO_TARGET_TO_ALL)
ENDIF(KWWidgets_USE_INTERNATIONALIZATION)


# If we are building this example as a standalone external project:
# - Generate a few small scripts (.bat, .sh, .csh) that can be sourced to set
# the various environments variables (PATH, TCLLIBPATH, LD_LIBRARY_PATH, etc.) 
# required by this executable and its known third-party dependencies (VTK, ITK,
# SOV, KWWidgets, etc.).
# - Generate a lightweight C launcher for this *specific* executable: It sets
# the above environment variables before launching the executable itself.

IF(NOT KWWidgets_SOURCE_DIR)
  INCLUDE("${KWWidgets_CMAKE_DIR}/KWWidgetsPathsMacros.cmake")
  KWWidgets_GENERATE_SETUP_PATHS_SCRIPTS("${CMAKE_CURRENT_BINARY_DIR}")
  SET(LAUNCHER_EXE_NAME "${EXE_NAME}Launcher")
  KWWidgets_GENERATE_SETUP_PATHS_LAUNCHER(
    "${CMAKE_CURRENT_BINARY_DIR}" "${LAUNCHER_EXE_NAME}" "" "${EXE_NAME}")
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
# based on KWWidgets, you should omit this section, unless your application
# supports that feature too and you checked how the macro is working.

IF(BUILD_TESTING)
  INCLUDE("${KWWidgets_CMAKE_DIR}/KWWidgetsTestingMacros.cmake")
  KWWidgets_ADD_TEST_FROM_C_EXAMPLE(TestKW${PROJECT_NAME} ${EXE_NAME})
ENDIF(BUILD_TESTING)
