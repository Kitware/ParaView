PROJECT(netCDFReaders)

INCLUDE(${QT_USE_FILE})

INCLUDE_DIRECTORIES(
  ${PARAVIEW_INCLUDE_DIRS}
  ${VTK_INCLUDE_DIR}
  ${pqCore_SOURCE_DIR}
  ${VTK_SOURCE_DIR}/Utilities/vtknetcdf
  ${VTK_BINARY_DIR}/Utilities/vtknetcdf
  )

# This is just a simple variable that lists the source files.
SET(SRCS
  vtkPSLACReader.cxx
  vtkSLACReader.cxx
  vtkSLACParticleReader.cxx
  )

IF(PARAVIEW_BUILD_QT_GUI)
  ADD_PARAVIEW_PLUGIN(netCDFReaders "1.0"
    SERVER_MANAGER_SOURCES ${SRCS}
    SERVER_MANAGER_XML netCDFReadersSM.xml
    GUI_RESOURCE_FILES netCDFReadersGUI.xml
  )
ELSE(PARAVIEW_BUILD_QT_GUI)
  ADD_PARAVIEW_PLUGIN(netCDFReaders "1.0"
    SERVER_MANAGER_SOURCES ${SRCS}
    SERVER_MANAGER_XML netCDFReadersSM.xml
  )
ENDIF(PARAVIEW_BUILD_QT_GUI)

# We need to add library dependencies for the plugin.
TARGET_LINK_LIBRARIES(netCDFReaders
  vtkFiltering
  vtkNetCDF
  )
