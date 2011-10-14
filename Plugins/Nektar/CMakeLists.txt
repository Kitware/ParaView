cmake_minimum_required(VERSION 2.6)

IF(COMMAND CMAKE_POLICY)
  CMAKE_POLICY(SET CMP0003 NEW)
  CMAKE_POLICY(SET CMP0017 NEW)
ENDIF(COMMAND CMAKE_POLICY)

# ParaView
if(NOT ParaView_SOURCE_DIR)
  find_package(ParaView REQUIRED)
  include(${PARAVIEW_USE_FILE})
endif()

find_package(MPI REQUIRED)

include_directories(
  ${CMAKE_CURRENT_SOURCE_DIR}/Nektar-PARALLEL/include
  ${CMAKE_CURRENT_SOURCE_DIR}
  ${MPI_INCLUDE_PATH}
  )

add_subdirectory(gs)

#FIND_LIBRARY(NEKTARMOPT_LIBRARY nektarmopt REQUIRED
#  /home/insley/TEST/PV_NEKTAR/Nektar-PARALLEL/Utilities/Linux
#)

#add_library(nektarobj nektarObject.C)


add_paraview_plugin(pvNektarReader "1.0"
   SERVER_MANAGER_XML pvNektarReader.xml
   SERVER_MANAGER_SOURCES vtkNektarReader.cxx
   GUI_RESOURCE_FILES pvNektarReaderGUI.xml)

target_link_libraries(pvNektarReader
  ${NEKTARMOPT_LIBRARY}
  gs
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