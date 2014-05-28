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
    SMProxy
    SMProxyLink
    SMDoubleVectorProperty
    SMIntVectorProperty
    SMStringVectorProperty
    SMUndoStack
    SMPropertyHelper
  )
  foreach(name IN LISTS UNITTESTS)
    if (PARAVIEW_QT_VERSION VERSION_GREATER "4")
      qt5_wrap_cpp(${name}_MOC_SOURCES vtk${name}Test.h)
    else ()
      qt4_wrap_cpp(${name}_MOC_SOURCES vtk${name}Test.h)
    endif ()
    list(APPEND test_sources
      Test${name}.cxx)
    list(APPEND extra_sources
      vtk${name}Test.cxx
      ${${name}_MOC_SOURCES})
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
  target_link_libraries(${vtk-module}CxxTests LINK_PRIVATE ${QT_LIBRARIES})
  if (PARAVIEW_QT_VERSION VERSION_GREATER "4")
    set_target_properties(${vtk-module}CxxTests PROPERTIES
        COMPILE_FLAGS "${Qt5Test_EXECUTABLE_COMPILE_FLAGS}")
  endif ()
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
