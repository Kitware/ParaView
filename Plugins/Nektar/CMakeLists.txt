cmake_minimum_required(VERSION 2.6)

IF(COMMAND CMAKE_POLICY)
  CMAKE_POLICY(SET CMP0003 NEW)
  CMAKE_POLICY(SET CMP0017 NEW)
ENDIF(COMMAND CMAKE_POLICY)

if(NOT WIN32)
  SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fPIC -O3 -fpermissive -ffast-math -funroll-loops -fstrict-aliasing -w")
  SET(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -fPIC -O3 -fpermissive -ffast-math -funroll-loops -fstrict-aliasing -w")
endif()


project(Nektar)

find_package(MPI REQUIRED)
find_package(BLAS)
#find_PACKAGE(LAPACK REQUIRED)

find_library(F2C_LIBRARY f2c)

add_subdirectory(gs)
add_subdirectory(Veclib)
add_subdirectory(metis)
add_subdirectory(Hlib)
add_subdirectory(Utilities)

# ParaView
if(NOT ParaView_SOURCE_DIR)
  find_package(ParaView REQUIRED)
  include(${PARAVIEW_USE_FILE})
endif()

include_directories(
  ${CMAKE_CURRENT_SOURCE_DIR}/include
  ${CMAKE_CURRENT_SOURCE_DIR}
  ${MPI_INCLUDE_PATH}
  )


add_paraview_plugin(pvNektarReader "1.0"
   SERVER_MANAGER_XML pvNektarReader.xml
   SERVER_MANAGER_SOURCES vtkNektarReader.cxx
   GUI_RESOURCE_FILES pvNektarReaderGUI.xml)

target_link_libraries(pvNektarReader
  hybrid
  vec
  metis
  gs
  nektar
  ${BLAS_blas_LIBRARY}
  ${LAPACK_lapack_LIBRARY}
  ${F2C_LIBRARY}
#  nektarobj
  #/home/insley/TEST/PV_NEKTAR/Nektar-PARALLEL/Hlib/Linux/libhybridmopt.a
  #/home/insley/TEST/PV_NEKTAR/Nektar-PARALLEL/Veclib/libvec.a
  #/home/insley/TEST/PV_NEKTAR/Nektar-PARALLEL/Metis/metis-4.0/libmetis.a
  #/home/insley/TEST/PV_NEKTAR/Nektar-PARALLEL/gs/libgs.a
  #/home/insley/TEST/PV_NEKTAR/Nektar-PARALLEL/Utilities/Linux/libnektarmopt.a
  #/home/insley/TEST/PV_NEKTAR/NEKTAR_OBJECT/libnektarobj.a
  #/home/insley/TEST/PV_NEKTAR/Nektar-PARALLEL/Utilities/Linux/libblas.a
  #/home/insley/TEST/PV_NEKTAR/Nektar-PARALLEL/Utilities/Linux/liblapack.a
  #/usr/lib64/libgfortran.so.1
  )
