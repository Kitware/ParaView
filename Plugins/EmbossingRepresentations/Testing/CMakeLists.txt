include(ParaViewTesting)

if (BUILD_SHARED_LIBS)
  paraview_add_client_tests(
    LOAD_PLUGIN "EmbossingRepresentations"
    BASELINE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/Data/Baseline"
    TEST_DATA_TARGET ParaViewData
    TEST_SCRIPTS ${CMAKE_CURRENT_SOURCE_DIR}/TestExtrusionRepresentationPointData.xml
    ${CMAKE_CURRENT_SOURCE_DIR}/TestExtrusionRepresentationCellData.xml
    ${CMAKE_CURRENT_SOURCE_DIR}/TestBumpMapRepresentationPointData.xml
    ${CMAKE_CURRENT_SOURCE_DIR}/TestExtrusionRepresentationPointVector.xml
    ${CMAKE_CURRENT_SOURCE_DIR}/TestExtrusionRepresentationRecalculateNormals.xml)
  paraview_add_client_tests(
    LOAD_PLUGIN "EmbossingRepresentations"
    TEST_DATA_TARGET ParaViewData
    TEST_SCRIPTS ${CMAKE_CURRENT_SOURCE_DIR}/ExtrusionRepresentationShaderGeneration.xml)
endif()
