INCLUDE(ParaViewTestingMacros)

set(EYE_DOME_LIGHTING_TESTS
  ${CMAKE_CURRENT_SOURCE_DIR}/EyeDomeLighting.xml
)

set (EyeDomeLighting_BREAK TRUE)

IF (PARAVIEW_BUILD_QT_GUI AND BUILD_SHARED_LIBS)

  add_client_tests("pv"
    LOAD_PLUGIN "EyeDomeLightingView"
    BASELINE_DIR ${PARAVIEW_TEST_BASELINE_DIR}
    TEST_SCRIPTS ${EYE_DOME_LIGHTING_TESTS}
  )

ENDIF ()


if (PARAVIEW_ENABLE_PYTHON AND BUILD_SHARED_LIBS)
  # this test is not specific to the EyeDomeLightingView plugin, it is meant
  # to test that we can reload a plugin multiple times without a crash.
  # for complex Python scripts we may not know that a plugin has already
  # been loaded.
  add_test(NAME ReloadPlugin
    COMMAND $<TARGET_FILE:pvpython> ${CMAKE_CURRENT_SOURCE_DIR}/ReloadPlugin.py)
  set_tests_properties(ReloadPlugin PROPERTIES LABELS PARAVIEW)
endif()
