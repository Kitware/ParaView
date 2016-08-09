cmake_minimum_required(VERSION 3.3)
project(MantaView)

# FIXME: this is ABSURD!!! Needs to be removed. I don't have time right now.
set (BUILD_AGAINST_PARAVIEW 1)

# -----------------------------------------------------------------------------
# Find Manta and take settings we need from it ###############################
SET ( MANTA_BUILD_NAMES
  buildmanta build_manta build-manta buildManta build_Manta build-Manta
  )

SET ( MANTA_BUILD_SEARCH_PATH "" )
FOREACH ( manta_build_name ${MANTA_BUILD_NAMES} )
  SET ( MANTA_BUILD_SEARCH_PATH
    ${MANTA_BUILD_SEARCH_PATH}
    ../${manta_build_name}
    /bin/${manta_build_name}
    /usr/bin/${manta_build_name}
    /usr/local/bin/${manta_build_name}
    )
ENDFOREACH ()

FIND_PATH ( MANTA_BUILD 
  MantaConfigure.cmake ${MANTA_BUILD_SEARCH_PATH} 
  DOC "Manta build directory" 
  )
IF ( MANTA_BUILD )
  SET ( MANTA_BUILD_INCLUDE ${MANTA_BUILD}/include)
  SET ( MANTA_BUILD_LIB ${MANTA_BUILD}/lib)
ENDIF ()

#TODO: Change Manta's config.h.in to point to source tree then autofind it
# Locate Manta source directory through Manta build directory
FIND_PATH ( MANTA_SOURCE 
  manta-commands.txt ../manta ../Manta 
  DOC "Manta source directory" 
  )
IF ( MANTA_BUILD AND NOT MANTA_SOURCE )
  INCLUDE ( ${MANTA_BUILD}/CMakeFiles/CMakeDirectoryInformation.cmake )
  SET ( MANTA_SOURCE ${CMAKE_RELATIVE_PATH_TOP_SOURCE} )
ENDIF ()

# Include Manta source and build-include directories
INCLUDE_DIRECTORIES(${MANTA_SOURCE} ${MANTA_BUILD_INCLUDE})
LINK_DIRECTORIES ( ${MANTA_BUILD_LIB} )

# Set specific Manta libraries to link against
SET ( MANTA_LINK_LIBRARIES
  Manta_Factory
  Manta_UserInterface
  Manta_Engine
  Manta_Model
  Manta_Image
  Manta_Interface
  Manta_Core
  )

# -----------------------------------------------------------------------------
# Process all vtk-modules defined in this plugin. This generates the necessary
# wrappings for all these modules as well.
# -----------------------------------------------------------------------------
pv_process_modules()

# -----------------------------------------------------------------------------
# Build the Paraview plugins
# -----------------------------------------------------------------------------
add_subdirectory(ParaView)
