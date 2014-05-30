# Try building a test executable to see if all dependencies
# of the plugin library are satisfied.
#option(BUILD_TEST_EXE "Build a test executable." OFF)
#mark_as_advanced(BUILD_TEST_EXE)
#if(BUILD_TEST_EXE)
#  add_definitions(-DBUILD_EXE)
#endif()

if(NOT MPI_LIBRARY)
  find_package(MPI REQUIRED)
endif()

enable_language(Fortran)
if(NOT BLAS_LIBRARIES)
  find_package(BLAS REQUIRED)
endif()

if(NOT LAPACK_lapack_LIBRARY)
  find_PACKAGE(LAPACK)
endif()

try_compile(BLAS_MANGLING_HAS_UNDERSCORE
  ${CMAKE_BINARY_DIR}
  ${CMAKE_CURRENT_SOURCE_DIR}/linkBLAS.cxx
  CMAKE_FLAGS "-DLINK_LIBRARIES:STRING=${BLAS_LIBRARIES}"
  COMPILE_DEFINITIONS "-DBLAS_MANGLING_HAS_UNDERSCORE"
  OUTPUT_VARIABLE compileResult
)

try_compile(NORMAL_BLAS_CALLS
  ${CMAKE_BINARY_DIR}
  ${CMAKE_CURRENT_SOURCE_DIR}/linkBLAS.cxx
  CMAKE_FLAGS "-DLINK_LIBRARIES:STRING=${BLAS_LIBRARIES}"
  COMPILE_DEFINITIONS "-DNO_BLAS_MANGLING"
  OUTPUT_VARIABLE compileResult
)

if(BLAS_MANGLING_HAS_UNDERSCORE)
  #message("calling blas routines from C/C++ requires underscore")
  add_definitions(-DBLAS_MANGLING_HAS_UNDERSCORE)
elseif(NORMAL_BLAS_CALLS)
  #message("calling blas routines from C/C++ without modifications")
  add_definitions(-DNO_BLAS_MANGLING)
else()
  message("NektarReaderPlugin: don't know how to call blas routines from C/C++")
endif()

# ParaView
if(NOT ParaView_SOURCE_DIR)
  find_package(ParaView 3.98 REQUIRED)
  include(${PARAVIEW_USE_FILE})
else()
  include_directories(${VTK_INCLUDE_DIRS})
endif()

include_directories(
  ${CMAKE_CURRENT_SOURCE_DIR}/../include
  ${CMAKE_CURRENT_SOURCE_DIR}/../NektarObj
  ${MPI_CXX_INCLUDE_PATH}
  )

add_paraview_plugin(pvNektarReader "2.0"
  SERVER_MANAGER_XML pvNektarReader.xml
  SERVER_MANAGER_SOURCES vtkNektarReader.cxx
  REQUIRED_ON_SERVER)

target_link_libraries(pvNektarReader
  LINK_PUBLIC
  nektar
  hybrid
  vec
  metis
  nektar
  nektarobj
  LINK_PRIVATE
  ${BLAS_LIBRARIES}
  ${LAPACK_lapack_LIBRARY}
  ${BLAS_LIBRARIES}
  )

#if(BUILD_TEST_EXE)
#  add_executable(TestReader TestReader.cxx)
#  target_link_libraries(TestReader
#    pvNektarReader
#    ${LAPACK_lapack_LIBRARY}
#    ${BLAS_LIBRARIES}
#    )
#endif()

