# Handle out-of-source builds correctly.
#
#  1. Create a list of Python files to be installed/copied.
#  2. Copy them to KWWIDGETS_BINARY_DIR if different from KWWIDGETS_SOURCE_DIR.
#  3. Use Python's compileall to compile the copied files.
#
# *** Step 1 has to be done carefully to avoid missing out files ***

IF(PYTHON_EXECUTABLE)

# Wrapping/Python/kwwidgets.py
  SET(KWWidgets_PYTHON_FILES 
    kwwidgets
    )

# Done listing of files.

# Now copy these files if necessary.
  SET(KWWidgets_PYTHON_SOURCE_FILES)
  SET(KWWidgets_PYTHON_OUTPUT_FILES)
  FOREACH(file ${KWWidgets_PYTHON_FILES})
    SET(src "${KWWidgets_BINARY_DIR}/Wrapping/Python/${file}.py")
    SET(KWWidgets_PYTHON_SOURCE_FILES ${KWWidgets_PYTHON_SOURCE_FILES} ${src})
  ENDFOREACH(file)

  IF ("${KWWidgets_BINARY_DIR}" MATCHES "^${KWWidgets_SOURCE_DIR}$")
    #MESSAGE("In source build -- no need to copy Python files.")
  ELSE ("${KWWidgets_BINARY_DIR}" MATCHES "^${KWWidgets_SOURCE_DIR}$")
    FOREACH(file ${KWWidgets_PYTHON_FILES})
      SET(src "${KWWidgets_SOURCE_DIR}/Wrapping/Python/${file}.py")
      SET(tgt "${KWWidgets_BINARY_DIR}/Wrapping/Python/${file}.py")
      ADD_CUSTOM_COMMAND(DEPENDS ${src}
                         COMMAND ${CMAKE_COMMAND}
                         ARGS -E copy ${src} ${tgt}
                         OUTPUT ${tgt}
                         COMMENT "source copy")
    ENDFOREACH(file)
  ENDIF ("${KWWidgets_BINARY_DIR}" MATCHES "^${KWWidgets_SOURCE_DIR}$")

  # Byte compile the Python files.
  CONFIGURE_FILE(${CMAKE_CURRENT_SOURCE_DIR}/compile_all_vtk.py.in
                 ${CMAKE_CURRENT_BINARY_DIR}/compile_all_vtk.py
                 @ONLY IMMEDIATE)
  ADD_CUSTOM_COMMAND(
    COMMAND ${PYTHON_EXECUTABLE}
    ARGS ${CMAKE_CURRENT_BINARY_DIR}/compile_all_vtk.py
    DEPENDS ${KWWidgets_PYTHON_SOURCE_FILES} ${CMAKE_CURRENT_BINARY_DIR}/compile_all_vtk.py
    OUTPUT "${KWWidgets_BINARY_DIR}/Wrapping/Python/vtk_compile_complete"
    )

  # Create a target on which custom commands can depend.  When this
  # is up-to-date, it is safe to run VTK python code.  Therefore
  # this should also depend on the vtkpython executable.
  ADD_CUSTOM_TARGET(vtkpython_pyc ALL echo "..."
    DEPENDS "${KWWidgets_BINARY_DIR}/Wrapping/Python/vtk_compile_complete")

  # If no runtime is to be installed then do not install python modules.
  IF(KWWidgets_INSTALL_NO_RUNTIME)
    SET(KWWidgets_INSTALL_NO_PYTHON 1)
  ENDIF(KWWidgets_INSTALL_NO_RUNTIME)

  # Add a rule to use python distutils to install the python wrappers.
  IF(NOT KWWidgets_INSTALL_NO_PYTHON)
    SET(DOLLAR "$")

    # Create default python setup arguments if they are not set.
    IF(DEFINED KWWidgets_PYTHON_SETUP_ARGS)
    ELSE(DEFINED KWWidgets_PYTHON_SETUP_ARGS)
      SET(KWWidgets_PYTHON_SETUP_ARGS "--prefix=\"${DOLLAR}{CMAKE_INSTALL_PREFIX}\""
        CACHE STRING "Arguments passed to \"python setup.py install ...\" during installation.")
      MARK_AS_ADVANCED(KWWidgets_PYTHON_SETUP_ARGS)
    ENDIF(DEFINED KWWidgets_PYTHON_SETUP_ARGS)

    # If there are multiple configurations then add a BUILD_TYPE=...
    # argument to the python setup.py call.  The build type to use is set
    # in the CMake variable BUILD_TYPE while running the install script.
    IF(CMAKE_CONFIGURATION_TYPES)
      SET(KWWidgets_PYTHON_SETUP_BUILD_TYPE "BUILD_TYPE=${DOLLAR}{BUILD_TYPE}")
    ELSE(CMAKE_CONFIGURATION_TYPES)
      SET(KWWidgets_PYTHON_SETUP_BUILD_TYPE)
    ENDIF(CMAKE_CONFIGURATION_TYPES)

    # Configure the post-install script to run python on setup.py.
    CONFIGURE_FILE(${KWWidgets_SOURCE_DIR}/Wrapping/Python/PythonInstall.cmake.in
                   ${KWWidgets_BINARY_DIR}/Wrapping/Python/PythonInstall.cmake
                   @ONLY IMMEDIATE)
    SET_TARGET_PROPERTIES(vtkpython_pyc PROPERTIES POST_INSTALL_SCRIPT
      ${KWWidgets_BINARY_DIR}/Wrapping/Python/PythonInstall.cmake
      )
  ENDIF(NOT KWWidgets_INSTALL_NO_PYTHON)
ENDIF(PYTHON_EXECUTABLE)

# Create the setup.py file.
IF(CMAKE_CONFIGURATION_TYPES)
  # The build has multiple configuration types.  If CMAKE_BUILD_TYPE
  # is set use it as the default BUILD_TYPE for setup.py to install.
  SET(KWWidgets_PYTHON_HAS_CONFIG_TYPES 1)
  IF(CMAKE_BUILD_TYPE)
    SET(KWWidgets_PYTHON_BUILD_TYPE "\"${CMAKE_BUILD_TYPE}\"")
  ELSE(CMAKE_BUILD_TYPE)
    SET(KWWidgets_PYTHON_BUILD_TYPE "[]")
  ENDIF(CMAKE_BUILD_TYPE)
ELSE(CMAKE_CONFIGURATION_TYPES)
  # The build has one configuration type.  The build type does not
  # affect installation.
  SET(KWWidgets_PYTHON_HAS_CONFIG_TYPES 0)
  SET(KWWidgets_PYTHON_BUILD_TYPE "[]")
ENDIF(CMAKE_CONFIGURATION_TYPES)
CONFIGURE_FILE(${KWWidgets_SOURCE_DIR}/Wrapping/Python/setup.py.in
               ${KWWidgets_BINARY_DIR}/Wrapping/Python/setup.py @ONLY IMMEDIATE)

