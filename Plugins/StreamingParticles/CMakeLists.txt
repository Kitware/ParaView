#--------------------------------------------------
# Find and Use ParaView
#--------------------------------------------------
if (ParaView_SOURCE_DIR)
  include_directories( ${PARAVIEW_INCLUDE_DIRS} ${VTK_INCLUDE_DIRS})
else()
  find_package(ParaView REQUIRED)
  include(${PARAVIEW_USE_FILE})
endif()

add_paraview_plugin(StreamingParticles "1.0"
  SERVER_MANAGER_XML StreamingParticles.xml
  SERVER_MANAGER_SOURCES
    vtkStreamingParticlesRepresentation.cxx
    vtkStreamingParticlesRepresentation.h
    vtkStreamingParticlesPriorityQueue.cxx
    vtkStreamingParticlesPriorityQueue.h
)
