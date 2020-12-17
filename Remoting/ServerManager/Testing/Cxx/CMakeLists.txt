if (PARAVIEW_USE_QT)
  find_package(Qt5 REQUIRED COMPONENTS Test)
  set(CMAKE_AUTOMOC 1)
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
    list(APPEND test_sources Test${name}.cxx)
    list(APPEND extra_sources vtk${name}Test.cxx ${${name}_MOC_SOURCES})
  endforeach()
endif()

vtk_add_test_cxx(vtkRemotingServerManagerCxxTests tests
  NO_DATA NO_VALID
  TestAdjustRange.cxx
  TestMultiplexerSourceProxy.cxx
  TestProxyAnnotation.cxx
  TestRecreateVTKObjects.cxx
  TestSelfGeneratingSourceProxy.cxx
  TestSessionProxyManager.cxx
  TestSettings.cxx
  TestValidateProxies.cxx
  TestXMLSaveLoadState.cxx)

vtk_add_test_cxx(vtkRemotingServerManagerCxxTests tests
  NO_DATA NO_VALID NO_OUTPUT
  ${test_sources})

vtk_test_cxx_executable(vtkRemotingServerManagerCxxTests tests
  ${extra_sources})

if (PARAVIEW_USE_QT)
  target_link_libraries(vtkRemotingServerManagerCxxTests PRIVATE Qt5::Test)
endif ()

set(built_shared 0)
if (BUILD_SHARED_LIBS)
  set(built_shared 1)
endif ()
set_property(SOURCE TestValidateProxies.cxx APPEND
  PROPERTY
    COMPILE_DEFINITIONS "BUILD_SHARED_LIBS=${built_shared}")
