project(ParticleAdaptor)
cmake_minimum_required(VERSION 2.6)
include_directories(${CoProcessor_SOURCE_DIR})

#--------------------------------------------------
# Find and Use ParaView
#--------------------------------------------------
IF (ParaView_SOURCE_DIR)
  INCLUDE_DIRECTORIES(
    ${MPI_INCLUDE_PATH}
    ${PARAVIEW_INCLUDE_DIRS}
    ${PARAVIEW_GUI_INCLUDE_DIRS}
    ${PARAVIEW_KWSYS_INCLUDE_DIRS}
  )
ELSE (ParaView_SOURCE_DIR)
  FIND_PACKAGE(ParaView REQUIRED)
  INCLUDE(${PARAVIEW_USE_FILE})
ENDIF (ParaView_SOURCE_DIR)

set(ParticleAdaptor_SRC
  ParticleAdaptor.cxx vtkParticlePipeline.cxx
)
add_library(ParticleAdaptor ${ParticleAdaptor_SRC})
target_link_libraries(
  ParticleAdaptor vtkCoProcessor
)
add_executable(ParticleViewer ParticleViewer.cxx)

set(ParticleViewer_LIBS
  vtkPVFilters
  vtkParallel 
  vtkGraphics
  vtkRendering 
  vtkIO 
  vtkFiltering 
  vtkCommon 
  )

IF(VTK_USE_X AND OPENGL_gl_LIBRARY)
  set(ParticleViewer_LIBS
    ${ParticleViewer_LIBS}
    ${OPENGL_gl_LIBRARY}
    )
ELSE(VTK_USE_X AND OPENGL_gl_LIBRARY)
  IF(VTK_OPENGL_HAS_OSMESA AND OSMESA_LIBRARY)
    set(ParticleViewer_LIBS
      ${ParticleViewer_LIBS}
      ${OSMESA_LIBRARY}
      )
  ENDIF(VTK_OPENGL_HAS_OSMESA AND OSMESA_LIBRARY)
ENDIF(VTK_USE_X AND OPENGL_gl_LIBRARY)

target_link_libraries(
  ParticleViewer ${ParticleViewer_LIBS}
)

# Install
#----------------------------------------------------------------------------------------------------
# Libraries
IF(NOT PV_INSTALL_NO_LIBRARIES)

  INSTALL(TARGETS ParticleAdaptor
      RUNTIME DESTINATION ${PV_INSTALL_BIN_DIR} COMPONENT Runtime
      LIBRARY DESTINATION ${PV_INSTALL_LIB_DIR} COMPONENT Runtime
      ARCHIVE DESTINATION ${PV_INSTALL_LIB_DIR} COMPONENT Development)
ENDIF(NOT PV_INSTALL_NO_LIBRARIES)
#
# Development
IF (NOT PV_INSTALL_NO_DEVELOPMENT)
  GLOB_INSTALL_DEVELOPMENT(
      ${CMAKE_CURRENT_SOURCE_DIR}
      ${PV_INSTALL_INCLUDE_DIR}
      "*.h;*.hxx;*.txx")
ENDIF (NOT PV_INSTALL_NO_DEVELOPMENT)
