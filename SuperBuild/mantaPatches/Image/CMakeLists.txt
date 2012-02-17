

###############################################################################
# Check to see if nrrd dependent files should be included.
IF (FOUND_TEEM) 
  INCLUDE_DIRECTORIES(${Teem_INCLUDE_DIRS})
  LINK_DIRECTORIES(${Teem_LIBRARY_DIRS})
  # Add the Nrrd read/write files to the build
  SET (NRRD_IMAGE_SRC NRRDFile.h NRRDFile.cc)  
ELSE (FOUND_TEEM)
  # Add stubs for these functions
  SET (NRRD_IMAGE_SRC NRRDFile.h NRRDFile-stub.cc)  
ENDIF (FOUND_TEEM)

###############################################################################
# Locate Image loading libraries.  On Apple we look for CoreGrahics.
# On other systems we look for ImageMagick++.
# Note that CoreGraphics might not load all image types that ImageMagick can, so we
# still try loading ImageMagick as a backup.
IF(APPLE)
  # Locate CoreGraphics
  FIND_LIBRARY(COREGRAPHICS_FRAMEWORK ApplicationServices REQUIRED)

  SET (CoreGraphics_SRC CoreGraphicsFile.h CoreGraphicsFile.cc)
  SET (ImageMagick_SRC ImageMagickFile.h ImageMagickFile-stub.cc)
  SET (EXR_SRC EXRFile.h EXRFile-stub.cc)
ELSE(APPLE)
  # Non APPLE systems
  SET (CoreGraphics_SRC CoreGraphicsFile.h CoreGraphicsFile-stub.cc)
ENDIF(APPLE)

#############################################
# Look for ImageMagick++ 
#FIND_PACKAGE(ImageMagick COMPONENTS Magick++)

IF (ImageMagick_Magick++_FOUND)
  # Add the ImageMagick files
  SET (ImageMagick_SRC ImageMagickFile.h ImageMagickFile.cc)
  INCLUDE_DIRECTORIES(${ImageMagick_Magick++_INCLUDE_DIR})
ELSE(ImageMagick_Magick++_FOUND)
  SET (ImageMagick_SRC ImageMagickFile.h ImageMagickFile-stub.cc)
ENDIF(ImageMagick_Magick++_FOUND)

MARK_AS_ADVANCED(ImageMagick_EXECUTABLE_DIR ImageMagick_Magick++_INCLUDE_DIR
  ImageMagick_Magick++_LIBRARY)

#############################################
# Locate OpenEXR
INCLUDE (${CMAKE_SOURCE_DIR}/CMake/FindOpenEXR.cmake)

IF (OpenEXR_FOUND)
  # Add the EXR files
  SET (EXR_SRC EXRFile.h EXRFile.cc)
  INCLUDE_DIRECTORIES(${OpenEXR_Include_Dir})
ELSE(OpenEXR_FOUND)
  SET (EXR_SRC EXRFile.h EXRFile-stub.cc)
ENDIF(OpenEXR_FOUND)


###############################################################################
# Create the library
ADD_LIBRARY (Manta_Image 
             NullImage.cc
             NullImage.h
             PPMFile.cc
             PPMFile.h
             Pixel.h
             RGBEFile.cc
             RGBEFile.h
             SimpleImageBase.h
             SimpleImageBase.cc
             SimpleImage.h
             SimpleImage_special.cc
             SimpleImage_templates.cc
             TGAFile.h
             TGAFile.cc
             ${NRRD_IMAGE_SRC}
             ${ImageMagick_SRC}
             ${EXR_SRC}
             ${CoreGraphics_SRC}
             )

TARGET_LINK_LIBRARIES(Manta_Image
                      Manta_Interface 
                      Manta_Core
                      )

# Link against optional libraries
IF (ImageMagick_Magick++_FOUND)
  TARGET_LINK_LIBRARIES(Manta_Image ${ImageMagick_LIBRARIES})
ENDIF(ImageMagick_Magick++_FOUND)
                    
IF (OpenEXR_FOUND)
  TARGET_LINK_LIBRARIES(Manta_Image ${OpenEXR_LIBRARIES} )
ENDIF(OpenEXR_FOUND)
                    
IF(FOUND_TEEM)
  # teem_LIB_DEPENDS comes from the TEEM intall CMake file
  # (TEEMLibraryDepends.cmake).  It is necessary to explicitly list
  # these files (to link against) when TEEM doesn't provide a shared
  # library.  (It doesn't hurt if TEEM does provide a shared lib.)
  TARGET_LINK_LIBRARIES(Manta_Image ${Teem_LIBRARIES} ${teem_LIB_DEPENDS})
ENDIF(FOUND_TEEM)

IF(APPLE)
  TARGET_LINK_LIBRARIES(Manta_Image ${COREGRAPHICS_FRAMEWORK})
ENDIF(APPLE)
