#vtkManta is an interface from VTK's rendering classes to Manta,
#a "real time" Ray Tracer.

#It creates a library that includes Manta specific subclasses
#of VTK's rendering classes, Renderer, Camera, etc.
#These can be instantiated by name, or by using object factory overloading
#at post compile time (ie, set VTK_AUTOLOAD_PATH to vtkManta's build
#directory and run a normal vtk app)

project(vtkManta)


#Configure vtkManta source proper ############################################

# Define a flag
if (BUILD_AGAINST_PARAVIEW)
  add_definitions(-DVTKMANTA_FOR_PARAVIEW)
endif()

# setup include paths
INCLUDE_DIRECTORIES(${CMAKE_CURRENT_BINARY_DIR})
INCLUDE_DIRECTORIES(${PROJECT_SOURCE_DIR})
INCLUDE_DIRECTORIES(${PROJECT_BINARY_DIR})

# Set vtkManta source files.
SET ( MANTA_CLASSES
  vtkMantaActor.cxx
  vtkMantaCamera.cxx
  vtkMantaLight.cxx
  vtkMantaManager.cxx
  vtkMantaPolyDataMapper.cxx
  vtkMantaProperty.cxx
  vtkMantaRenderer.cxx
  vtkMantaTestSource.cxx
  vtkMantaTexture.cxx
  )

IF (BUILD_AGAINST_PARAVIEW)
  # paraview uses its own LOD actor, put it in here
  SET ( MANTA_CLASSES ${MANTA_CLASSES}
    vtkMantaLODActor.cxx
    vtkPVMantaRepresentation.cxx
    vtkMantaCompositeMapper.cxx)
ENDIF ()

SET ( MANTA_SRCS
  ${MANTA_CLASSES}
  vtkMantaObjectFactory.cxx
  )

set (${vtk-module}_NO_HeaderTest 1)

vtk_module_library(${vtk-module} ${MANTA_SRCS})
TARGET_LINK_LIBRARIES (${vtk-module} LINK_PRIVATE ${MANTA_LINK_LIBRARIES})
include(vtkOpenGL)
vtk_opengl_link(${vtk-module})

# Manta_Core uses MMTime which is in it's own special library.
if(WIN32)
  TARGET_LINK_LIBRARIES(${PROJECT_NAME} LINK_PRIVATE Winmm)
endif()

#Examples ####################################################################
IF ( BUILD_EXAMPLES )
  ADD_SUBDIRECTORY(Examples)
ENDIF ()
