INCLUDE_REGULAR_EXPRESSION("^(vtk|kw|icons).*$")
SOURCE_FILES( KWCommon_SRCS
vtkKWHashTable
vtkKWHashTableIterator
vtkKWIcon
vtkKWProcessStatistics
vtkKWRegisteryUtilities
vtkKWSerializer 
)

#ABSTRACT_FILES(
#)


IF (WIN32)
  SOURCE_FILES( KWCommon_SRCS 
    vtkKWWin32RegisteryUtilities
  )
  WRAP_EXCLUDE_FILES(
    vtkKWWin32RegisteryUtilities
  )
ELSE (WIN32)
  ADD_DEFINITIONS(-D_HPUX_SOURCE)
  SOURCE_FILES( KWCommon_SRCS 
    vtkKWUNIXRegisteryUtilities
  )
  WRAP_EXCLUDE_FILES(
    vtkKWUNIXRegisteryUtilities
  )
ENDIF (WIN32)
  
WRAP_EXCLUDE_FILES(
  vtkKWHashTable 
  vtkKWRegisteryUtilities
)

VTK_WRAP_TCL(vtkKWCommonTCL KWCommonTCL_SRCS KWCommon_SRCS)
ADD_LIBRARY(vtkKWCommonTCL KWCommonTCL_SRCS KWCommon_SRCS)

LINK_LIBRARIES( vtkpng vtkzlib vtkjpeg )

IF (VTK_USE_PARALLEL)
  LINK_LIBRARIES(vtkParallelTCL vtkParallel)
  IF (WIN32)
   LINK_LIBRARIES( wsock32 )
  ENDIF (WIN32)
ENDIF (VTK_USE_PARALLEL)

IF (VTK_USE_PATENTED)
  LINK_LIBRARIES(vtkPatentedTCL vtkPatented)
ENDIF (VTK_USE_PATENTED)

IF (VTK_USE_HYBRID)
  LINK_LIBRARIES(vtkHybridTCL vtkHybrid)
ENDIF (VTK_USE_HYBRID)

IF (WIN32)
  IF (TK_XLIB_PATH)
    INCLUDE_DIRECTORIES(${TK_XLIB_PATH})
  ENDIF (TK_XLIB_PATH) 
ENDIF (WIN32)

LINK_LIBRARIES (
vtkRenderingTCL 
vtkImagingTCL 
vtkGraphicsTCL 
vtkIOTCL
vtkFilteringTCL   
vtkCommonTCL 
vtkRendering
vtkImaging
vtkGraphics
vtkIO
vtkFiltering
vtkCommon 
${TCL_LIBRARY}
${TK_LIBRARY}
)

IF(CMAKE_HAS_X)
   LINK_LIBRARIES(-lXt)
   LINK_LIBRARIES(${CMAKE_X_LIBS})
   ADD_DEFINITIONS(${CMAKE_X_CFLAGS})
ENDIF(CMAKE_HAS_X)

INSTALL_TARGETS(/lib vtkKWCommonTCL)

IF(BUILD_TESTING)
  SUBDIRS(Testing)
ENDIF(BUILD_TESTING)

OPTION(USE_NOTEBOOK_ICONS "Display Notebook with icons." OFF)
MARK_AS_ADVANCED(USE_NOTEBOOK_ICONS)