################################################################################
# ParaView ServerManager Unit Tests
if (PARAVIEW_ENABLE_QT_SUPPORT)
  include(ParaViewQt)
  pv_find_package_qt(qt_targets REQUIRED QT4_COMPONENTS QtTest QT5_COMPONENTS Test)
  set(UNITTESTS
    SMProxy
    SMProxyLink
    SMDoubleVectorProperty
    SMIntVectorProperty
    SMStringVectorProperty
    SMUndoStack
    SMPropertyHelper
  )
  foreach(name IN LISTS UNITTESTS)
    pv_qt_wrap_cpp(${name}_MOC_SOURCES vtk${name}Test.h)
    list(APPEND test_sources Test${name}.cxx)
    list(APPEND extra_sources vtk${name}Test.cxx ${${name}_MOC_SOURCES})
  endforeach()
endif()

paraview_add_test_cxx(${vtk-module}CxxTests tests
  NO_DATA NO_VALID NO_OUTPUT
  ParaViewCoreServerManagerPrintSelf.cxx
  TestComparativeAnimationCueProxy.cxx
  # Disabled previously.
  #TestMultipleSessions.cxx
  #TestSubProxy.cxx
  TestProxyAnnotation.cxx
  TestXMLSaveLoadState.cxx
  ${test_sources}
  )
vtk_test_cxx_executable(${vtk-module}CxxTests tests
  ${extra_sources}
  )
if (PARAVIEW_ENABLE_QT_SUPPORT)
  target_link_libraries(${vtk-module}CxxTests LINK_PRIVATE ${qt_targets})
endif ()

paraview_test_load_data_dirs(""
  SPCTH
  )
paraview_test_load_data(""
  ,REGEX:poisson_3d.*\\.h5
  "iron protein.vtk"
  SPCTH/ball_and_box.spcth
  sonic.pht
  sonicgeom.dat.1
  sonicrestart.1
  )
