#-----------------------------------------------------------------------------
set(proj python)
set(python_base ${CMAKE_CURRENT_BINARY_DIR}/${proj})
set(python_build ${CMAKE_CURRENT_BINARY_DIR}/${proj}-build)
set(python_BUILD_IN_SOURCE 1)

if(WIN32)

  set(python_sln ${python_build}/PCbuild/pcbuild.sln)
  string(REPLACE "/" "\\" python_sln ${python_sln})

  get_filename_component(python_base ${python_sln} PATH)
  get_filename_component(python_home ${python_base} PATH)

  if("${CMAKE_SIZEOF_VOID_P}" EQUAL 8)
    set(python_configuration "Release|x64")
    set(PythonPCBuildDir ${CMAKE_BINARY_DIR}/python-build/PCbuild/amd64)
  else()
    set(python_configuration "Release|Win32")
    set(PythonPCBuildDir ${CMAKE_BINARY_DIR}/python-build/PCbuild)
  endif()

  ExternalProject_Add(${proj}
    URL ${PYTHON_URL}/${PYTHON_GZ}
    URL_MD5 ${PYTHON_MD5}
    DOWNLOAD_DIR ${CMAKE_CURRENT_BINARY_DIR}
    SOURCE_DIR ${python_build}
    PATCH_COMMAND ${CMAKE_COMMAND} -E copy ${ParaViewSuperBuild_CMAKE_SOURCE_DIR}/PythonPatches/pyproject.vsprops ${python_base}
    CONFIGURE_COMMAND ""
    BUILD_IN_SOURCE ${python_BUILD_IN_SOURCE}
    BUILD_COMMAND ${CMAKE_BUILD_TOOL} ${python_sln} /build ${python_configuration} /project select
    INSTALL_COMMAND ""
    DEPENDS
      ${python_DEPENDENCIES}
  )

  # Convenient helper macro
  macro(build_python_target target depend)
    ExternalProject_Add_Step(${proj} Build_${target}
      COMMAND ${CMAKE_BUILD_TOOL} ${python_sln} /build ${python_configuration} /project ${target}
      DEPENDEES ${depend}
      )
  endmacro(build_python_target)

  build_python_target(make_versioninfo build)
  build_python_target(make_buildinfo Build_make_versioninfo)
  build_python_target(kill_python Build_make_buildinfo)
  build_python_target(w9xpopen Build_kill_python)
  build_python_target(pythoncore Build_w9xpopen)
  build_python_target(_socket Build_pythoncore)
  build_python_target(_testcapi Build__socket)
  build_python_target(_msi Build__testcapi)
  build_python_target(_elementtree Build__msi)
  build_python_target(_ctypes_test Build__elementtree)
  build_python_target(_ctypes Build__ctypes_test)
  build_python_target(winsound Build__ctypes)
  build_python_target(pyexpat Build_winsound)
  build_python_target(pythonw Build_pyexpat)
  build_python_target(_multiprocessing Build_pythonw)

  ExternalProject_Add_Step(${proj} Build_python
    COMMAND ${CMAKE_BUILD_TOOL} ${python_sln} /build ${python_configuration} /project python
    DEPENDEES Build__multiprocessing
    DEPENDERS install
    )

  ExternalProject_Add_Step(${proj} CopyPythonLib
    COMMAND ${CMAKE_COMMAND} -E copy ${PythonPCBuildDir}/python${PYVER_SHORT}.lib ${CMAKE_BINARY_DIR}/python-build/Lib/python${PYVER_SHORT}.lib
    DEPENDEES install
    )
  ExternalProject_Add_Step(${proj} Copy_socketPyd
    COMMAND ${CMAKE_COMMAND} -E copy ${PythonPCBuildDir}/_socket.pyd ${CMAKE_BINARY_DIR}/python-build/Lib/_socket.pyd
    DEPENDEES install
    )
  ExternalProject_Add_Step(${proj} Copy_ctypesPyd
    COMMAND ${CMAKE_COMMAND} -E copy ${PythonPCBuildDir}/_ctypes.pyd ${CMAKE_BINARY_DIR}/python-build/Lib/_ctypes.pyd
    DEPENDEES install
    )

  ExternalProject_Add_Step(${proj} CopyPythonDll
    COMMAND ${CMAKE_COMMAND} -E copy ${PythonPCBuildDir}/python${PYVER_SHORT}.dll ${CMAKE_BINARY_DIR}/python-build/bin/${CMAKE_CFG_INTDIR}/python${PYVER_SHORT}.dll
    DEPENDEES install
    )

  ExternalProject_Add_Step(${proj} CopyPyconfigHeader
    COMMAND ${CMAKE_COMMAND} -E copy ${CMAKE_BINARY_DIR}/python-build/PC/pyconfig.h ${CMAKE_BINARY_DIR}/python-build/Include/pyconfig.h
    DEPENDEES install
    )

elseif(UNIX)
  set(python_source ${CMAKE_CURRENT_BINARY_DIR}/python)
  set(python_install ${CMAKE_CURRENT_BINARY_DIR})

  configure_file(${ParaViewSuperBuild_CMAKE_SOURCE_DIR}/python_patch_step.cmake.in
    ${CMAKE_CURRENT_BINARY_DIR}/python_patch_step.cmake
    @ONLY)

  configure_file(${ParaViewSuperBuild_CMAKE_SOURCE_DIR}/python_configure_step.cmake.in
    ${CMAKE_CURRENT_BINARY_DIR}/python_configure_step.cmake
    @ONLY)

  configure_file(${ParaViewSuperBuild_CMAKE_SOURCE_DIR}/python_make_step.cmake.in
    ${CMAKE_CURRENT_BINARY_DIR}/python_make_step.cmake
    @ONLY)

  configure_file(${ParaViewSuperBuild_CMAKE_SOURCE_DIR}/python_install_step.cmake.in
    ${CMAKE_CURRENT_BINARY_DIR}/python_install_step.cmake
    @ONLY)

  set(python_PATCH_COMMAND ${CMAKE_COMMAND} -P ${CMAKE_CURRENT_BINARY_DIR}/python_patch_step.cmake)
  set(python_CONFIGURE_COMMAND ${CMAKE_COMMAND} -P ${CMAKE_CURRENT_BINARY_DIR}/python_configure_step.cmake)
  set(python_BUILD_COMMAND ${CMAKE_COMMAND} -P ${CMAKE_CURRENT_BINARY_DIR}/python_make_step.cmake)
  set(python_INSTALL_COMMAND ${CMAKE_COMMAND} -P ${CMAKE_CURRENT_BINARY_DIR}/python_install_step.cmake)

  ExternalProject_Add(python
    URL ${PYTHON_URL}/${PYTHON_GZ}
    URL_MD5 ${PYTHON_MD5}
    DOWNLOAD_DIR ${CMAKE_CURRENT_BINARY_DIR}
    SOURCE_DIR ${python_source}
    INSTALL_DIR ${python_install}
    BUILD_IN_SOURCE 1
    PATCH_COMMAND ${python_PATCH_COMMAND}
    CONFIGURE_COMMAND ${python_CONFIGURE_COMMAND}
    BUILD_COMMAND ${python_BUILD_COMMAND}
    INSTALL_COMMAND ${python_INSTALL_COMMAND}
    DEPENDS
      ${python_DEPENDENCIES}
    )

endif()

#-----------------------------------------------------------------------------
# Set PYTHON_INCLUDE and PYTHON_LIBRARY variables
#

set(PYTHON_INCLUDE)
set(PYTHON_LIBRARY)
set(PYTHON_EXECUTABLE)
set(PYTHON_SITE_PACKAGES ${CMAKE_BINARY_DIR}/python-build/lib/python${PYVER}/site-packages)

if(WIN32)
  set(PYTHON_INCLUDE_DIR ${python_build}/Include)
  set(PYTHON_LIBRARY ${PythonPCBuildDir}/python${PYVER_SHORT}${_LINK_LIBRARY_SUFFIX})
  set(PYTHON_EXECUTABLE ${PythonPCBuildDir}/python.exe)
else()
  set(PYTHON_INCLUDE_DIR ${python_install}/include/python${PYVER})
  set(PYTHON_LIBRARY ${python_install}/lib/libpython${PYVER}${_LINK_LIBRARY_SUFFIX})
  set(PYTHON_LIBRARY_DIR ${python_install}/lib)
  set(PYTHON_EXECUTABLE ${python_install}/bin/python)
endif()
