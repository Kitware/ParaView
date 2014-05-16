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

if(PARAVIEW_ENABLE_COSMOTOOLS 
    AND BUILD_TESTING
    AND PARAVIEW_DATA_ROOT
    AND PARAVIEW_BUILD_QT_GUI)
    include(ParaViewTestingMacros)
    add_pv_test("pv" "_DISABLE_C"
      COMMAND --client ${CLIENT_EXECUTABLE}
              --enable-bt
              -dr
              --enable-streaming
              --test-directory=${PARAVIEW_TEST_DIR}
              --test-plugin=StreamingParticles
      BASELINE_DIR ${PARAVIEW_DATA_ROOT}/Baseline
      TEST_SCRIPTS ${CMAKE_CURRENT_SOURCE_DIR}/Testing/StreamingParticles.xml
      )
endif()
