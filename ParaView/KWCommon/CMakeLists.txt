PROJECT(KWCommon)
INCLUDE_REGULAR_EXPRESSION("^(vtk|kw|icons).*$")
SET( KWCommon_SRCS
vtkKWIcon.cxx
vtkKWProcessStatistics.cxx
vtkKWRegisteryUtilities.cxx
vtkKWSerializer.cxx
)

IF (WIN32)
  SET( KWCommon_SRCS ${KWCommon_SRCS}
    vtkKWWin32RegisteryUtilities.cxx
  )
  SET_SOURCE_FILES_PROPERTIES(
    vtkKWWin32RegisteryUtilities
    WRAP_EXCLUDE
    )
ELSE (WIN32)
  ADD_DEFINITIONS(-D_HPUX_SOURCE)
  SET( KWCommon_SRCS ${KWCommon_SRCS}
    vtkKWUNIXRegisteryUtilities.cxx
  )
  SET_SOURCE_FILES_PROPERTIES(
    vtkKWUNIXRegisteryUtilities
    WRAP_EXCLUDE
    )
ENDIF (WIN32)
  
VTK_WRAP_TCL(vtkKWCommonTCL KWCommonTCL_SRCS ${KWCommon_SRCS})
ADD_LIBRARY(vtkKWCommonTCL ${KWCommonTCL_SRCS} ${KWCommon_SRCS})

TARGET_LINK_LIBRARIES (vtkKWCommonTCL
vtkImagingTCL 
vtkGraphicsTCL 
vtkIOTCL
vtkFilteringTCL   
vtkImaging
vtkGraphics
vtkIO
vtkFiltering
${TCL_LIBRARY}
${TK_LIBRARY}
)

INSTALL_TARGETS(/lib vtkKWCommonTCL)

IF(BUILD_TESTING)
  SUBDIRS(Testing)
ENDIF(BUILD_TESTING)

SUBDIRS(Utilities)

OPTION(USE_NOTEBOOK_ICONS "Display Notebook with icons." OFF)
MARK_AS_ADVANCED(USE_NOTEBOOK_ICONS)