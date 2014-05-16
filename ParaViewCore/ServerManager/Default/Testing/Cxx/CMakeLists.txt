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

################################################################################
# ParaView ServerManager Unit Tests
if (PARAVIEW_ENABLE_QT_SUPPORT)
  if (PARAVIEW_QT_VERSION VERSION_GREATER "4")
    set(Qt5_FIND_COMPONENTS Test)
    include (ParaViewQt5)
  else ()
    set(QT_DONT_USE_QTGUI TRUE)
    set(QT_USE_QTTEST TRUE)
    include(${QT_USE_FILE})
  endif ()
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
    if (PARAVIEW_QT_VERSION VERSION_GREATER "4")
      qt5_wrap_cpp(${name}_MOC_SOURCES ${name}.h)
    else ()
      qt4_wrap_cpp(${name}_MOC_SOURCES ${name}.h)
    endif ()
    vtk_module_test_executable(${name} ${name}.cxx ${${name}_MOC_SOURCES})
    add_test(NAME pvunit.${name}
             COMMAND ${name})
    target_link_libraries(${name} LINK_PRIVATE ${QT_LIBRARIES})
    if (PARAVIEW_QT_VERSION VERSION_GREATER "4")
      set_target_properties(${name} PROPERTIES
          COMPILE_FLAGS "${Qt5Test_EXECUTABLE_COMPILE_FLAGS}")
    endif ()
    set_tests_properties(pvunit.${name} PROPERTIES LABELS "PARAVIEW")
  endforeach()
endif()
