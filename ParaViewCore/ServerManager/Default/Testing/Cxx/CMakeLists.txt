#------------------------------------------------------------------------------
vtk_module_test_executable(TestSubProxy TestSubProxy.cxx)
# add_test(NAME TestSubProxy COMMAND TestSubProxy)

#------------------------------------------------------------------------------
vtk_module_test_executable(TestMultipleSessions
  TestMultipleSessions.cxx)

#------------------------------------------------------------------------------
set(test_srcs
  ParaViewCoreServerManagerPrintSelf
  TestComparativeAnimationCueProxy
  TestXMLSaveLoadState
  TestProxyAnnotation
  )

foreach (name ${test_srcs})
  vtk_module_test_executable(${name}
    ${name}.cxx)
  add_test(NAME ${name}
      COMMAND ${name} ${name} -D ${VTK_DATA_ROOT})
  set_tests_properties(${name} PROPERTIES LABELS "PARAVIEW")
endforeach()

#------------------------------------------------------------------------------
if (PARAVIEW_DATA_ROOT)
  # This is the executable that can load any Server Manager state (*.pvsm) file
  # and test the rendered image.
  vtk_module_test_executable(ServerManagerStateLoader
    ServerManagerStateLoader.cxx
    vtkTestingProcessModuleGUIHelper.cxx
    vtkTestingProcessModuleGUIHelper.h
    vtkTestingOptions.cxx
    vtkTestingOptions.h)

  # Add the *.pvsm file names to load and test.
  set (TEST_STATES
    Arrow
    Axes
    CalcInput
    ExtractGrid
    TestPHT
    SPCTH2)

  foreach (tfile ${TEST_STATES})
    add_test(NAME ${tfile}-ServerManager
      COMMAND ServerManagerStateLoader
        --enable-bt
        -D=${PARAVIEW_DATA_ROOT}
        -T=${ParaView_BINARY_DIR}/Testing/Temporary
        -V=${PARAVIEW_DATA_ROOT}/Baseline/${tfile}.png
        ${CMAKE_CURRENT_SOURCE_DIR}/../SMStates/${tfile}.pvsm)
    set_tests_properties(${tfile}-ServerManager PROPERTIES LABELS "PARAVIEW")
  endforeach()
endif()

################################################################################
# ParaView ServerManager Unit Tests
if (PARAVIEW_ENABLE_QT_SUPPORT)
  set(QT_DONT_USE_QTGUI TRUE)
  set(QT_USE_QTTEST TRUE)
  include(${QT_USE_FILE})
  set(UNITTESTS
    vtkSMProxyTest
    vtkSMProxyLinkTest
    vtkSMDoubleVectorPropertyTest
    vtkSMIntVectorPropertyTest
    vtkSMStringVectorPropertyTest
    vtkSMUndoStackTest
    vtkSMPropertyHelperTest
  )
  foreach(name ${UNITTESTS})
    qt4_wrap_cpp(${name}_MOC_SOURCES ${name}.h)
    vtk_module_test_executable(${name} ${name}.cxx ${${name}_MOC_SOURCES})
    add_test(NAME pvunit.${name}
             COMMAND ${name})
    target_link_libraries(${name} ${QT_LIBRARIES})
    set_tests_properties(pvunit.${name} PROPERTIES LABELS "PARAVIEW")
  endforeach()
endif()
