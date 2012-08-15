#vtkManta is an interface from VTK's rendering classes to Manta,
#a "real time" Ray Tracer. 

#It creates a library that includes Manta specific subclasses
#of VTK's rendering classes, Renderer, Camera, etc.
#These can be instantiated by name, or by using object factory overloading 
#at post compile time (ie, set VTK_AUTOLOAD_PATH to vtkManta's build 
#directory and run a normal vtk app)

project(vtkManta)


#Configure vtkManta source proper ############################################

# Define a flag exported to vtkMantaConfigure.h which is used in some
# ParaView-dependent #ifdefs
IF (NOT BUILD_AGAINST_PARAVIEW)
  SET ( BUILD_AGAINST_PARAVIEW 0 )
ENDIF (NOT BUILD_AGAINST_PARAVIEW)

# setup include paths
INCLUDE_DIRECTORIES(${CMAKE_CURRENT_BINARY_DIR})
INCLUDE_DIRECTORIES(${PROJECT_SOURCE_DIR})
INCLUDE_DIRECTORIES(${PROJECT_BINARY_DIR})

# Setup the output directories.
#TODO: What are these for, are they necessary?
SET ( CMAKE_LIBRARY_OUTPUT_DIRECTORY ${PROJECT_BINARY_DIR}/bin)
SET ( EXECUTABLE_OUTPUT_PATH ${PROJECT_BINARY_DIR}/bin CACHE PATH
      "Single output directory for building all executables."
    )
MARK_AS_ADVANCED ( CMAKE_LIBRARY_OUTPUT_DIRECTORY EXECUTABLE_OUTPUT_PATH )

# Create the header to configure the classes.
CONFIGURE_FILE ( ${CMAKE_CURRENT_SOURCE_DIR}/vtkMantaConfigure.h.in
                 ${CMAKE_CURRENT_BINARY_DIR}/vtkMantaConfigure.h
                 @ONLY
                 IMMEDIATE
               )

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
ENDIF (BUILD_AGAINST_PARAVIEW)

SET ( MANTA_SRCS
  ${MANTA_CLASSES}
  vtkMantaObjectFactory.cxx
  )


vtk_module_library(${vtk-module} ${MANTA_SRCS})
TARGET_LINK_LIBRARIES (${vtk-module}
  ${VTK_LINK_LIBRARIES} ${MANTA_LINK_LIBRARIES})

# Manta_Core uses MMTime which is in it's own special library.
if(WIN32)
  TARGET_LINK_LIBRARIES(${PROJECT_NAME} Winmm)
endif()

#Examples ####################################################################
IF ( BUILD_EXAMPLES )
  ADD_SUBDIRECTORY(Examples)
ENDIF ( BUILD_EXAMPLES )

#Tests #######################################################################
#TODO: Make these tests meaningful and cdash them regularly
IF ( BUILD_TESTING )
  ENABLE_TESTING()
  INCLUDE( CTest )
  ADD_SUBDIRECTORY(Testing)
ENDIF ( BUILD_TESTING )
