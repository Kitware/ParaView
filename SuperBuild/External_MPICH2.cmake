
# The MPICH2 external project for ParaView
set(MPICH2_source "${CMAKE_CURRENT_BINARY_DIR}/MPICH2")
set(MPICH2_build "${CMAKE_CURRENT_BINARY_DIR}/MPICH2-build")
set(MPICH2_install "${CMAKE_CURRENT_BINARY_DIR}")

# If Windows we use CMake otherwise ./configure
if(WIN32)
  
  message("Fatal Error. Don't Build MPICH2 on Windows, it requires a service!")
  ExternalProject_Add(MPICH2
    DOWNLOAD_DIR ${CMAKE_CURRENT_BINARY_DIR}
    SOURCE_DIR ${MPICH2_source}
    BINARY_DIR ${MPICH2_build}
    INSTALL_DIR ${MPICH2_install}
    URL ${MPICH2_URL}/${MPICH2_GZ}
    URL_MD5 ${MPICH2_MD5}
    CMAKE_CACHE_ARGS
      -DCMAKE_CXX_FLAGS:STRING=${pv_tpl_cxx_flags}
      -DCMAKE_C_FLAGS:STRING=${pv_tpl_c_flags}
      -DCMAKE_BUILD_TYPE:STRING=${CMAKE_CFG_INTDIR}
      ${pv_tpl_compiler_args}
      ${MPICH2_EXTRA_ARGS}
    CMAKE_ARGS
      -DCMAKE_INSTALL_PREFIX:PATH=<INSTALL_DIR>
  )
else()

  set(mpich2_configure_args "--disable-rpath^^--disable-f77^^--disable-fc")

  # on linux build the binaries static
  ExternalProject_Add(MPICH2
    LIST_SEPARATOR ^^
    DOWNLOAD_DIR ${CMAKE_CURRENT_BINARY_DIR}
    SOURCE_DIR ${MPICH2_source}
    INSTALL_DIR ${MPICH2_install}
    URL ${MPICH2_URL}/${MPICH2_GZ}
    URL_MD5 ${MPICH2_MD5}
    BUILD_IN_SOURCE 1
    PATCH_COMMAND ""
    CONFIGURE_COMMAND ${CMAKE_COMMAND} -DADDITIONAL_CFLAGS=-fPIC -DADDITIONAL_CXXFLAGS=-fPIC -DADDITIONAL_FFLAGS=-fPIC -DINSTALL_DIR=<INSTALL_DIR> -DWORKING_DIR=<SOURCE_DIR> CONFIGURE_ARGS=${mpich2_configure_args} -P ${ParaViewSuperBuild_CMAKE_BINARY_DIR}/paraview_configure_step.cmake
    BUILD_COMMAND ${CMAKE_COMMAND} -DADDITIONAL_CFLAGS=-fPIC -DADDITIONAL_CXXFLAGS=-fPIC -DADDITIONAL_FFLAGS=-fPIC -DWORKING_DIR=<SOURCE_DIR> -P ${ParaViewSuperBuild_CMAKE_BINARY_DIR}/paraview_make_step.cmake
  )
endif()

set(MPIEXEC ${MPICH2_install}/bin/mpiexec${CMAKE_EXECUTABLE_SUFFIX})
set(MPICC ${MPICH2_install}/bin/mpicc${CMAKE_EXECUTABLE_SUFFIX})
set(MPICXX ${MPICH2_install}/bin/mpic++${CMAKE_EXECUTABLE_SUFFIX})
set(MPI_INCLUDE_PATH ${MPICH2_install}/include)
set(MPI_INSTALL ${MPICH2_install})
if(WIN32)
  set(MPI_LIBRARY optimized ${MPICH2_install}/lib/libmpi${_LINK_LIBRARY_SUFFIX} debug ${MPICH2_install}/lib/libmpid${_LINK_LIBRARY_SUFFIX})
  set(MPI_EXTRA_LIBRARY optimized ${MPICH2_install}/lib/libmpi_cxx${_LINK_LIBRARY_SUFFIX} debug ${MPICH2_install}/lib/libmpi_cxxd${_LINK_LIBRARY_SUFFIX})
else()
  set(MPI_LIBRARY ${MPICH2_install}/lib/libmpich.a)
  set(MPI_EXTRA_LIBRARY ${MPICH2_install}/lib/libmpichcxx.a;${MPICH2_install}/lib/libopa.a;${MPICH2_install}/lib/libmpl.a)
  set(MPI_C_COMPILER ${MPICH2_install}/bin/mpicc)
  set(MPI_CXX_COMPILER ${MPICH2_install}/bin/mpic++)
endif()
