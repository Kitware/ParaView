cmake_minimum_required(VERSION 2.8)

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

#find_library(F2C_LIBRARY f2c)
#find_library(SCALAPACK_LIBRARY 
#  NAMES scalapack scalapackd
#  )

#find_library(SCALAPACK_LIBRARY 
#  NAMES scalapack scalapackd
#  )

#find_library(BLACS_LIBRARY
#  NAMES blacs blacsd
#  )
  
#find_library(BLACS_C_init_LIBRARY 
#  NAMES BLACS_Cinit BLACS_Cinitd
#  )

#find_library(BLACS_F_init_LIBRARY 
#  NAMES BLACS_Finit BLACS_Finitd
#  )

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
#  ${CMAKE_CURRENT_SOURCE_DIR}
#  ${MPI_INCLUDE_PATH}
  )

add_library(nektarobj STATIC nektarObject.C)

add_paraview_plugin(pvNektarReader "1.0"
   SERVER_MANAGER_XML pvNektarReader.xml
   SERVER_MANAGER_SOURCES vtkNektarReader.cxx
   GUI_RESOURCE_FILES pvNektarReaderGUI.xml)

target_link_libraries(pvNektarReader
  #/home/partyd/Projects/Nektar/Nektar-PARALLEL/Hlib/Linux/libhybridmopt.a
  #/home/partyd/Projects/Nektar/Nektar-PARALLEL/Veclib/libvec.a
  #/home/partyd/Projects/Nektar/Nektar-PARALLEL/Metis/metis-4.0/libmetis.a
  #/home/partyd/Projects/Nektar/Nektar-PARALLEL/gs/libgs.a
  #/home/partyd/Projects/Nektar/Nektar-PARALLEL/Utilities/Linux/libnektarmopt.a
  #/home/partyd/Projects/Nektar/NEKTAR_OBJECT/libnektarobj.a
  #/home/partyd/Projects/Nektar/Nektar-PARALLEL/Utilities/Linux/libblas.a
  #/home/partyd/Projects/Nektar/Nektar-PARALLEL/Utilities/Linux/liblapack.a
  hybrid
  nektar
  vec
  metis
  gs
  nektar
  nektarobj
  ${BLAS_blas_LIBRARY}
  ${LAPACK_lapack_LIBRARY}
#  ${F2C_LIBRARY}
#  ${BLACS_C_init_LIBRARY}
#  ${BLACS_LIBRARY}
#  ${SCALAPACK_LIBRARY}
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
