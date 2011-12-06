CMAKE_MINIMUM_REQUIRED(VERSION 2.8)

IF(COMMAND CMAKE_POLICY)
  CMAKE_POLICY(SET CMP0003 NEW)
  CMAKE_POLICY(SET CMP0017 NEW)
ENDIF(COMMAND CMAKE_POLICY)

project(Nektar)

if(NOT MPI_LIBRARY)
  find_package(MPI REQUIRED)
endif()

if(NOT BLAS_blas_LIBRARY)
  find_package(BLAS)
endif()

if(NOT LAPACK_lapack_LIBRARY)
  find_PACKAGE(LAPACK)
endif()

add_subdirectory(gs)
add_subdirectory(Veclib)
add_subdirectory(metis)
add_subdirectory(Hlib)
add_subdirectory(Utilities)
add_subdirectory(NektarObj)

# ParaView
if(NOT ParaView_SOURCE_DIR)
  find_package(ParaView REQUIRED)
  include(${PARAVIEW_USE_FILE})
endif()

include_directories(
  ${CMAKE_CURRENT_SOURCE_DIR}/include
  ${CMAKE_CURRENT_SOURCE_DIR}/NektarObj
  )

add_paraview_plugin(pvNektarReader "1.0"
   SERVER_MANAGER_XML pvNektarReader.xml
   SERVER_MANAGER_SOURCES vtkNektarReader.cxx
   GUI_RESOURCE_FILES pvNektarReaderGUI.xml)

target_link_libraries(pvNektarReader
  nektar
  hybrid
  vec
  metis
  gs
  nektar
  nektarobj
  ${BLAS_blas_LIBRARY}
  ${LAPACK_lapack_LIBRARY}
  )
