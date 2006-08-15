  PROJECT(VTKPythonWrapping)

  IF(NOT VTKPythonWrapping_INSTALL_LIBRARIES)
    SET(VTKPythonWrapping_INSTALL_LIBRARIES 1)
  ENDIF(NOT VTKPythonWrapping_INSTALL_LIBRARIES)
  IF(NOT VTKPythonWrapping_INSTALL_LIB_DIR)
    SET(VTKPythonWrapping_INSTALL_LIB_DIR "/lib/${PROJECT_NAME}")
  ENDIF(NOT VTKPythonWrapping_INSTALL_LIB_DIR)
  IF(NOT VTKPythonWrapping_INSTALL_BIN_DIR)
    SET(VTKPythonWrapping_INSTALL_BIN_DIR "/bin")
  ENDIF(NOT VTKPythonWrapping_INSTALL_BIN_DIR)

  # We only wrap subportion of VTK, so include only that
  INCLUDE_DIRECTORIES(
    ${VTK_SOURCE_DIR}/Wrapping
    ${VTK_SOURCE_DIR}
    ${VTK_BINARY_DIR}
    ${VTK_SOURCE_DIR}/Common
    ${VTK_SOURCE_DIR}/IO
    ${VTK_SOURCE_DIR}/Filtering
    ${VTK_BINARY_DIR}/Rendering
    ${PYTHON_INCLUDE_PATH}
    )

  # OpenGL include directories.
  IF(APPLE)
    IF(VTK_USE_X)
      SET(PVWRAP_OPEN_GL_DIRS ${PVWRAP_OPEN_GL_DIRS}
        ${OPENGL_INCLUDE_DIR})
    ENDIF(VTK_USE_X)
  ELSE(APPLE)
    SET(PVWRAP_OPEN_GL_DIRS ${PVWRAP_OPEN_GL_DIRS}
      ${OPENGL_INCLUDE_DIR})
  ENDIF(APPLE)

  IF(VTK_USE_X)
    # X include directories.
    SET(PVWRAP_OPEN_GL_DIRS ${PVWRAP_OPEN_GL_DIRS}
      ${CMAKE_Xlib_INCLUDE_PATH} ${CMAKE_Xutil_INCLUDE_PATH})
  ENDIF(VTK_USE_X)

  IF(VTK_HAVE_VP1000)
    # VolumePro VP 1000 include directory.
    SET(PVWRAP_OPEN_GL_DIRS ${PVWRAP_OPEN_GL_DIRS}
      ${VLI_INCLUDE_PATH_FOR_VP1000})
  ENDIF(VTK_HAVE_VP1000)

  IF(VTK_USE_MANGLED_MESA)
    # Mangled Mesa include directory.
    SET(PVWRAP_OPEN_GL_DIRS ${PVWRAP_OPEN_GL_DIRS}
      ${MESA_INCLUDE_PATH})
  ELSE(VTK_USE_MANGLED_MESA)
    # Off-Screen Mesa include directory.
    IF(VTK_OPENGL_HAS_OSMESA)
      IF(OSMESA_INCLUDE_DIR)
        SET(PVWRAP_OPEN_GL_DIRS ${PVWRAP_OPEN_GL_DIRS}
          ${OSMESA_INCLUDE_DIR})
      ENDIF(OSMESA_INCLUDE_DIR)
    ENDIF(VTK_OPENGL_HAS_OSMESA)
  ENDIF(VTK_USE_MANGLED_MESA)

  INCLUDE_DIRECTORIES(${PVWRAP_OPEN_GL_DIRS})
  SET(CMAKE_C_FLAGS "${CMAKE_ANSI_CFLAGS} ${CMAKE_C_FLAGS}")

  ADD_EXECUTABLE(vtkWrapPython 
    ${VTK_SOURCE_DIR}/Wrapping/vtkWrapPython.c 
    ${VTK_SOURCE_DIR}/Wrapping/vtkParse.tab.c)

  TARGET_LINK_LIBRARIES(vtkWrapPython
    ${PYTHON_LIBRARY}
    )

  ADD_EXECUTABLE(vtkWrapPythonInit 
    ${VTK_SOURCE_DIR}/Wrapping/vtkWrapPythonInit.c)

  SET(Kit_SRCS
    "${VTK_SOURCE_DIR}/Common/vtkCollection.cxx"
    "${VTK_SOURCE_DIR}/Common/vtkCollectionIterator.cxx"
    "${VTK_SOURCE_DIR}/Common/vtkObject.cxx"
    "${VTK_SOURCE_DIR}/Common/vtkObjectBase.cxx"
    "${VTK_SOURCE_DIR}/Common/vtkTimerLog.cxx"
    "${VTK_SOURCE_DIR}/IO/vtkXMLParser.cxx"
    "${VTK_SOURCE_DIR}/Common/vtkAbstractArray.cxx"
    "${VTK_SOURCE_DIR}/Common/vtkDataArray.cxx"
    "${VTK_SOURCE_DIR}/Common/vtkStringArray.cxx"
# "${VTK_SOURCE_DIR}/Common/vtk__Int64Array.cxx"
    "${VTK_SOURCE_DIR}/Common/vtkBitArray.cxx"
    "${VTK_SOURCE_DIR}/Common/vtkCharArray.cxx"
    "${VTK_SOURCE_DIR}/Common/vtkDoubleArray.cxx"
    "${VTK_SOURCE_DIR}/Common/vtkFloatArray.cxx"
    "${VTK_SOURCE_DIR}/Common/vtkIdTypeArray.cxx"
    "${VTK_SOURCE_DIR}/Common/vtkIntArray.cxx"
    "${VTK_SOURCE_DIR}/Common/vtkLongArray.cxx"
    "${VTK_SOURCE_DIR}/Common/vtkLongLongArray.cxx"
    "${VTK_SOURCE_DIR}/Common/vtkShortArray.cxx"
#  "${VTK_SOURCE_DIR}/Common/vtkUnsigned__Int64Array.cxx"
    "${VTK_SOURCE_DIR}/Common/vtkUnsignedCharArray.cxx"
    "${VTK_SOURCE_DIR}/Common/vtkUnsignedIntArray.cxx"
    "${VTK_SOURCE_DIR}/Common/vtkUnsignedLongArray.cxx"
    "${VTK_SOURCE_DIR}/Common/vtkUnsignedLongLongArray.cxx"
    "${VTK_SOURCE_DIR}/Common/vtkUnsignedShortArray.cxx"

    "${VTK_SOURCE_DIR}/Filtering/vtkDataObject.cxx"
    "${VTK_SOURCE_DIR}/Filtering/vtkFieldData.cxx"
    "${VTK_SOURCE_DIR}/Filtering/vtkDataSetAttributes.cxx"
    "${VTK_SOURCE_DIR}/Filtering/vtkCellData.cxx"
    "${VTK_SOURCE_DIR}/Filtering/vtkPointData.cxx"

    "${VTK_SOURCE_DIR}/Filtering/vtkDataSet.cxx"
    "${VTK_SOURCE_DIR}/Filtering/vtkHyperOctree.cxx"
    "${VTK_SOURCE_DIR}/Filtering/vtkImageData.cxx"
    "${VTK_SOURCE_DIR}/Filtering/vtkPointSet.cxx"
    "${VTK_SOURCE_DIR}/Filtering/vtkRectilinearGrid.cxx"
    "${VTK_SOURCE_DIR}/Filtering/vtkStructuredPoints.cxx"
    "${VTK_SOURCE_DIR}/Filtering/vtkUniformGrid.cxx"
    "${VTK_SOURCE_DIR}/Filtering/vtkPolyData.cxx"
    "${VTK_SOURCE_DIR}/Filtering/vtkStructuredGrid.cxx"
    "${VTK_SOURCE_DIR}/Filtering/vtkUnstructuredGrid.cxx"

    )

  SET_SOURCE_FILES_PROPERTIES(
    "${VTK_SOURCE_DIR}/Common/vtkObjectBase.cxx"
    ABSTRACT
    )

  SET(EXE_EXT "")
  IF(WIN32)
    SET(EXE_EXT ".exe")
  ENDIF(WIN32)

  SET(MAKE_SYSTEM)
  IF(CMAKE_BUILD_TOOL MATCHES "make")
    SET(MAKE_SYSTEM 1)
  ENDIF(CMAKE_BUILD_TOOL MATCHES "make")

  SET(CFG_INIT "/${CMAKE_CFG_INTDIR}")
  IF(MAKE_SYSTEM OR UNIX)
    SET(CFG_INIT "")
  ENDIF(MAKE_SYSTEM OR UNIX)

  SET(VTK_WRAP_PYTHON_EXE 
    "${EXECUTABLE_OUTPUT_PATH}${CFG_INIT}/vtkWrapPython${EXE_EXT}"
    CACHE INTERNAL "Location of program to do Python wrapping")
  SET(VTK_WRAP_PYTHON_INIT_EXE 
    "${EXECUTABLE_OUTPUT_PATH}${CFG_INIT}/vtkWrapPythonInit${EXE_EXT}"
    CACHE INTERNAL "Location of program to do Python wrapping")

  # Tell vtkWrapPython.cmake to set VTK_PYTHON_LIBRARIES for us.
  SET(VTK_WRAP_PYTHON_FIND_LIBS 1)

  INCLUDE("${VTK_CMAKE_DIR}/vtkWrapPython.cmake")
IF (PARAVIEW_WRAP_PYTHON AND NOT VTK_WRAP_PYTHON)
  SET(KIT Common)
  SET(VTK_WRAP_PYTHON3_INIT_DIR "${VTK_SOURCE_DIR}/Wrapping")
  SET(Kit_PYTHON_EXTRA_SRCS
    "${VTK_SOURCE_DIR}/${KIT}/vtkPythonUtil.cxx")
  SET(KIT_PYTHON_LIBS
    vtkIO
    vtkRendering
    )

  IF(UNIX)
    # On UNIX, Python needs the math library.
    SET(KIT_PYTHON_LIBS ${KIT_PYTHON_LIBS} -lm)

    # If using X with Tk, we need to link to X when we link
    # to Tk in order to support static linking.
    IF(CMAKE_HAS_X)
      SET(KIT_PYTHON_LIBS ${KIT_PYTHON_LIBS} -lXt ${CMAKE_X_LIBS})
    ENDIF(CMAKE_HAS_X)
  ENDIF(UNIX)
  SET(KIT_PYTHON_DEPS)
  SET(VTK_INSTALL_NO_LIBRARIES 1)
  IF(VTKPythonWrapping_INSTALL_BIN_DIR)
    SET(VTK_INSTALL_NO_LIBRARIES)
  ENDIF(VTKPythonWrapping_INSTALL_BIN_DIR)
  SET(VTK_INSTALL_LIB_DIR "${VTKPythonWrapping_INSTALL_LIB_DIR}")
  SET(VTK_INSTALL_BIN_DIR "${VTKPythonWrapping_INSTALL_BIN_DIR}")

  # include python wrapping from VTK
  INCLUDE(KitCommonPythonWrapBlock)
ENDIF (PARAVIEW_WRAP_PYTHON AND NOT VTK_WRAP_PYTHON)

IF (PARAVIEW_WRAP_PYTHON)

  # Install yje vtk${KIT}Python library.
  IF(NOT PV_INSTALL_NO_LIBRARIES)
    SET (targets_to_install ${targets_to_install} vtk${KIT}Python)
      
    IF(PV_INSTALL_HAS_CMAKE_24)
      INSTALL(TARGETS ${targets_to_install}
        RUNTIME DESTINATION ${PV_INSTALL_BIN_DIR_CM24} COMPONENT Runtime
        LIBRARY DESTINATION ${PV_INSTALL_LIB_DIR_CM24} COMPONENT Runtime
        ARCHIVE DESTINATION ${PV_INSTALL_LIB_DIR_CM24} COMPONENT Development)
    ELSE(PV_INSTALL_HAS_CMAKE_24)
      INSTALL_TARGETS(${PV_INSTALL_LIB_DIR}
        vtk${KIT}
        ${targets_to_install}
        )
    ENDIF(PV_INSTALL_HAS_CMAKE_24)

  ENDIF(NOT PV_INSTALL_NO_LIBRARIES)

  SET(PV_PYTHON_MODULE_SOURCE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/paraview")
  SET(PV_PYTHON_MODULE_BINARY_DIR "${CMAKE_CURRENT_BINARY_DIR}/paraview")
  SET(PV_PYTHON_MODULE_DIR "${CMAKE_CURRENT_BINARY_DIR}" CACHE INTERNAL "Path to paraview module.")
  MAKE_DIRECTORY(${PV_PYTHON_MODULE_BINARY_DIR})
# Handle out-of-source builds correctly.
#
#  1. Create a list of Python files to be installed/copied.
#  2. Copy them to the binary directory. Since paraview cannot be built
#     in place, we must always copy the files to the binary directory.
#  3. Use Python's compileall to compile the copied files.
#
# *** Step 1 has to be done carefully to avoid missing out files ***


# List all python source files.
# All files paraview/*.py included in the paraview module.
  SET(PV_PYTHON_FILES
    __init__
    numeric
  )



# Now copy these files if necessary.
  SET(PV_PYTHON_SOURCE_FILES)
  SET(PV_PYTHON_OUTPUT_FILES)
  FOREACH(file ${VTK_PYTHON_FILES})
    SET(src "${PV_PYTHON_MODULE_SOURCE_DIR}/${file}.py")
    SET(PV_PYTHON_SOURCE_FILES ${VTK_PYTHON_SOURCE_FILES} ${src})
  ENDFOREACH(file)

  IF ("${VTK_BINARY_DIR}" MATCHES "^${VTK_SOURCE_DIR}$")
    #MESSAGE("In source build -- no need to copy Python files.")
  ELSE ("${VTK_BINARY_DIR}" MATCHES "^${VTK_SOURCE_DIR}$")
    FOREACH(file ${PV_PYTHON_FILES})
      SET(src "${PV_PYTHON_MODULE_SOURCE_DIR}/${file}.py")
      SET(tgt "${PV_PYTHON_MODULE_BINARY_DIR}/${file}.py")
      ADD_CUSTOM_COMMAND(DEPENDS ${src}
        COMMAND ${CMAKE_COMMAND}
        ARGS -E copy ${src} ${tgt}
        OUTPUT ${tgt}
        COMMENT "source copy")
      SET(PV_PYTHON_OUTPUT_FILES ${PV_PYTHON_OUTPUT_FILES} ${tgt})
    ENDFOREACH(file)
  ENDIF ("${VTK_BINARY_DIR}" MATCHES "^${VTK_SOURCE_DIR}$")

# Byte compile the Python files.
  CONFIGURE_FILE(${PV_PYTHON_MODULE_SOURCE_DIR}/compile_all_pv.py.in
    ${PV_PYTHON_MODULE_BINARY_DIR}/compile_all_pv.py
    @ONLY IMMEDIATE)

  ADD_CUSTOM_COMMAND(
    COMMAND ${PYTHON_EXECUTABLE}
    ARGS ${PV_PYTHON_MODULE_BINARY_DIR}/compile_all_pv.py
    DEPENDS ${PV_PYTHON_SOURCE_FILES} ${PV_PYTHON_MODULE_BINARY_DIR}/compile_all_pv.py
      ${PV_PYTHON_OUTPUT_FILES}
    OUTPUT "${PV_PYTHON_MODULE_BINARY_DIR}/pv_compile_complete"
    )

  ADD_CUSTOM_TARGET(paraview_pyc ALL 
    DEPENDS "${PV_PYTHON_MODULE_BINARY_DIR}/pv_compile_complete")

# Install the paraview module files.
  IF (NOT PV_INSTALL_NO_LIBRARIES)
    IF(PV_INSTALL_HAS_CMAKE_24)
      INSTALL(FILES ${PV_PYTHON_OUTPUT_FILES} 
        DESTINATION ${PV_INSTALL_LIB_DIR_CM24}/paraview COMPONENT Runtime)
    ELSE(PV_INSTALL_HAS_CMAKE_24)
      INSTALL_FILES(${PV_INSTALL_LIB_DIR}/paraview FILES ${PV_PYTHON_OUTPUT_FILES})
    ENDIF(PV_INSTALL_HAS_CMAKE_24)
  ENDIF (NOT PV_INSTALL_NO_LIBRARIES)
ENDIF (PARAVIEW_WRAP_PYTHON)
