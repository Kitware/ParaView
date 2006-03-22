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
IF (PARAVIEW_WRAP_PYTHON)
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
ENDIF (PARAVIEW_WRAP_PYTHON)
