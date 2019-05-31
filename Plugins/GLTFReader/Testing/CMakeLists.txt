if (PARAVIEW_BUILD_QT_GUI AND BUILD_SHARED_LIBS)
  ExternalData_Expand_Arguments("ParaViewData" _
    "DATA{Data/glTF/AnimatedMorphCube/AnimatedMorphCube.gltf}"
    "DATA{Data/glTF/AnimatedMorphCube/AnimatedMorphCube.bin}"
    "DATA{Data/glTF/BoxAnimated/BoxAnimated.gltf}"
    "DATA{Data/glTF/BoxAnimated/BoxAnimated0.bin}"
    "DATA{Data/glTF/InterpolationTest/InterpolationTest.gltf}"
    "DATA{Data/glTF/InterpolationTest/InterpolationTest_data.bin}"
    "DATA{Data/glTF/Triangle/Triangle.glb}"
    "DATA{Data/glTF/SimpleSkin/SimpleSkin.gltf}"
    "DATA{Data/Baseline/glTFReaderTriangle.png}"
    "DATA{Data/Baseline/glTFReaderAnimatedMorphing.png}"
    "DATA{Data/Baseline/glTFReaderAnimatedMorphing_IsLoaded.png}"
    "DATA{Data/Baseline/glTFReaderMultipleScenes_IsLoaded.png}"
    "DATA{Data/Baseline/glTFReaderMultipleScenes_SceneOne.png}"
    "DATA{Data/Baseline/glTFReaderMultipleScenes.png}"
    "DATA{Data/Baseline/glTFReaderMultipleAnimations_IsLoaded.png}"
    "DATA{Data/Baseline/glTFReaderMultipleAnimations_ThreeAnimations.png}"
    "DATA{Data/Baseline/glTFReaderMultipleAnimations.png}"
    "DATA{Data/Baseline/glTFReaderToggleDeformation_IsLoaded.png}"
    "DATA{Data/Baseline/glTFReaderToggleDeformation_IsSkinned.png}"
    "DATA{Data/Baseline/glTFReaderToggleDeformation.png}"
    )

  paraview_add_client_tests(
  LOAD_PLUGIN "GLTFReader"
  TEST_DATA_TARGET ParaViewData
  BASELINE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/Data/Baseline"
  TEST_SCRIPTS glTFReaderAnimatedMorphing.xml)

  paraview_add_client_tests(
  LOAD_PLUGIN "GLTFReader"
  TEST_DATA_TARGET ParaViewData
  BASELINE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/Data/Baseline"
  TEST_SCRIPTS glTFReaderMultipleAnimations.xml)

  paraview_add_client_tests(
  LOAD_PLUGIN "GLTFReader"
  TEST_DATA_TARGET ParaViewData
  BASELINE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/Data/Baseline"
  TEST_SCRIPTS glTFReaderMultipleScenes.xml)

  paraview_add_client_tests(
  LOAD_PLUGIN "GLTFReader"
  TEST_DATA_TARGET ParaViewData
  BASELINE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/Data/Baseline"
  TEST_SCRIPTS glTFReaderTriangle.xml)

  paraview_add_client_tests(
  LOAD_PLUGIN "GLTFReader"
  TEST_DATA_TARGET ParaViewData
  BASELINE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/Data/Baseline"
  TEST_SCRIPTS glTFReaderToggleDeformation.xml)
endif ()
