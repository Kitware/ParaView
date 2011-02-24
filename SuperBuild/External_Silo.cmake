
# The Silo external project for ParaView
set(Silo_source "${CMAKE_CURRENT_BINARY_DIR}/Silo")
set(Silo_install "${CMAKE_CURRENT_BINARY_DIR}/Silo-install")

# If Windows we use CMake otherwise ./configure
if(WIN32)

  # run's copysilo.bat which generates silo.h
  configure_file(${ParaViewSuperBuild_CMAKE_SOURCE_DIR}/Silo_configure_step.cmake.in
    ${CMAKE_CURRENT_BINARY_DIR}/Silo_configure_step.cmake
    @ONLY)

  configure_file(${ParaViewSuperBuild_CMAKE_SOURCE_DIR}/Silo_build_step.cmake.in
    ${CMAKE_CURRENT_BINARY_DIR}/Silo_build_step.cmake
    @ONLY)

  
  #set(python_tkinter ${python_base}/pyproject.vsprops)
    #string(REPLACE "/" "\\" python_tkinter ${python_tkinter})

    #set(script ${CMAKE_CURRENT_SOURCE_DIR}/CMake/StringFindReplace.cmake)
    #set(out ${python_tkinter})
    #set(in ${python_tkinter})

    #set(python_PATCH_COMMAND 
    #  ${CMAKE_COMMAND} -Din=${in} -Dout=${out} -Dfind=tcltk\" -Dreplace=tcl-build\" -P ${script})
    
  #set(Silo_PATCH_COMMAND ${CMAKE_COMMAND} -Din=${in} -Dout=${out} -Dfind=hdf5dll
  set(Silo_CONFIGURE_COMMAND ${CMAKE_COMMAND} -P ${CMAKE_CURRENT_BINARY_DIR}/Silo_configure_step.cmake)
  set(Silo_BUILD_COMMAND ${CMAKE_COMMAND} -P ${CMAKE_CURRENT_BINARY_DIR}/Silo_build_step.cmake)
  set(Silo_INSTALL_COMMAND ${CMAKE_COMMAND} -P ${CMAKE_CURRENT_BINARY_DIR}/Silo_install_step.cmake)

  ExternalProject_Add(Silo
    DOWNLOAD_DIR ${CMAKE_CURRENT_BINARY_DIR}
    SOURCE_DIR ${Silo_source}
    INSTALL_DIR ${Silo_install}
    URL ${SILO_URL}/${SILO_GZ}
    URL_MD5 ${SILO_MD5}
    BUILD_IN_SOURCE 1
    UPDATE_COMMAND ""
    PATCH_COMMAND ""
    CONFIGURE_COMMAND ${Silo_CONFIGURE_COMMAND}
    BUILD_COMMAND ${Silo_BUILD_COMMAND}
    INSTALL_COMMAND ""
  )

else()

  ExternalProject_Add(Silo
    DOWNLOAD_DIR ${CMAKE_CURRENT_BINARY_DIR}
    SOURCE_DIR ${Silo_source}
    INSTALL_DIR ${Silo_install}
    URL ${SILO_URL}/${SILO_GZ}
    URL_MD5 ${SILO_MD5}
    BUILD_IN_SOURCE 1
    PATCH_COMMAND ""
    CONFIGURE_COMMAND <SOURCE_DIR>/configure --prefix=<INSTALL_DIR>
  )

endif()

set(Silo_DIR "${Silo_binary}" CACHE PATH "Silo binary directory" FORCE)
mark_as_advanced(Silo_DIR)
