PROJECT(Development)
INCLUDE_REGULAR_EXPRESSION("^vtk.*$")

SET(Development_SRCS
   )

IF(NOT PARAVIEW_NO_HDF5)
  SET(Development_SRCS
      ${Development_SRCS}
      vtkChomboReader.cxx
     )
ENDIF(NOT PARAVIEW_NO_HDF5)

IF(Development_SRCS)
  ADD_LIBRARY(vtkPVDevelopment ${Development_SRCS})
  TARGET_LINK_LIBRARIES(vtkPVDevelopment vtkHybrid)

  IF(NOT PARAVIEW_NO_HDF5)
    TARGET_LINK_LIBRARIES(vtkPVDevelopment ${PARAVIEW_HDF5_LIBRARIES})
  ENDIF(NOT PARAVIEW_NO_HDF5)

# Create TCL wrappers for development library.
  VTK_WRAP_TCL2(vtkPVDevelopmentTCL DevelopmentTCL_SRCS ${Development_SRCS})
  ADD_LIBRARY(vtkPVDevelopmentTCL ${DevelopmentTCL_SRCS})
  TARGET_LINK_LIBRARIES(vtkPVDevelopmentTCL vtkHybridTCL vtkPVDevelopment)

  # Create ClientServer wrappers for development library.
  VTK_WRAP_ClientServer(vtkPVDevelopmentCS DevelopmentCS_SRCS ${Development_SRCS})
  ADD_LIBRARY(vtkPVDevelopmentCS ${DevelopmentCS_SRCS})
  TARGET_LINK_LIBRARIES(vtkPVDevelopmentCS vtkHybridCS vtkPVDevelopment)

  IF(BUILD_SHARED_LIBS)
    INSTALL_TARGETS(${KW_INSTALL_LIB_DIR} vtkPVDevelopmentTCL)
  ENDIF(BUILD_SHARED_LIBS)
ENDIF(Development_SRCS)
