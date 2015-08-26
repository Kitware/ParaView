
#--------------------------------------------------
# Find and Use ParaView
#--------------------------------------------------
if (ParaView_SOURCE_DIR)
  include_directories( ${PARAVIEW_INCLUDE_DIRS} ${VTK_INCLUDE_DIRS})
else()
  find_package(ParaView REQUIRED)
  include(${PARAVIEW_USE_FILE})
endif()

add_paraview_plugin(PointGaussian "1.0"
  SERVER_MANAGER_XML PointGaussian.xml
  SERVER_MANAGER_SOURCES
    vtkPointGaussianRepresentation.cxx
    vtkPointGaussianRepresentation.h
)

# Add testing if necessary
if (BUILD_TESTING)
  add_subdirectory(Testing)
endif()