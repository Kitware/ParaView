# -----------------------------------------------------------------------------
# Set of basic source files (not the OpenGL or GLSL ones)
# -----------------------------------------------------------------------------
set(KIT_SRCS
  vtkCellPointsFilter.cxx
  vtk1DLookupTableTransferFunction.cxx
  vtk1DTransferFunction.cxx
  vtk1DTransferFunctionChooser.cxx
  vtk1DTransferFunctionFilter.cxx
  vtk1DGaussianTransferFunction.cxx
  )


# -----------------------------------------------------------------------------
# List the kits from VTK that are needed by this project
# -----------------------------------------------------------------------------
set(KIT_LIBS
  vtkFiltersCore
  )

# -----------------------------------------------------------------------------
# Create the library
# -----------------------------------------------------------------------------
add_library(PointSprite_Graphics ${KIT_SRCS})
target_link_libraries(PointSprite_Graphics ${KIT_LIBS})

if (PV_INSTALL_BIN_DIR)
  install(TARGETS PointSprite_Graphics
    RUNTIME DESTINATION ${PV_INSTALL_BIN_DIR} COMPONENT Runtime
    LIBRARY DESTINATION ${PV_INSTALL_LIB_DIR} COMPONENT Runtime
    ARCHIVE DESTINATION ${PV_INSTALL_LIB_DIR} COMPONENT Development
    )
else (PV_INSTALL_BIN_DIR)
  install(TARGETS PointSprite_Graphics
    RUNTIME DESTINATION bin COMPONENT Runtime
    LIBRARY DESTINATION lib COMPONENT Runtime
    ARCHIVE DESTINATION lib COMPONENT Development
    )
endif (PV_INSTALL_BIN_DIR)

# -----------------------------------------------------------------------------
# This make it easy for other projects to get the list of files etc. in this
# kit.
# -----------------------------------------------------------------------------
# needed by vtkExportKit.cmake
include(${ParaView_CMAKE_DIR}/VTKMono/vtkExportKit.cmake)
IF(NOT VTK_INSTALL_NO_DEVELOPMENT)
  STRING(REGEX REPLACE "^/" "" VTK_INSTALL_PACKAGE_DIR_CM24 "${VTK_INSTALL_PACKAGE_DIR}")
ENDIF(NOT VTK_INSTALL_NO_DEVELOPMENT)

vtk_export_kit2(
  "PointSprite_Graphics" 
  "POINTSPRITE_GRAPHICS" 
  ${CMAKE_CURRENT_BINARY_DIR} 
  "${KIT_SRCS}"
)
