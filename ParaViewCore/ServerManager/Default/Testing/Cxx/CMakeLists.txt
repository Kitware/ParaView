################################################################################
# ParaView ServerManager Unit Tests
if (PARAVIEW_ENABLE_QT_SUPPORT)
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

vtk_add_test_cxx(vtkPVServerManagerDefaultCxxTests tests
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
vtk_test_cxx_executable(vtkPVServerManagerDefaultCxxTests tests
  ${extra_sources}
  )
if (PARAVIEW_ENABLE_QT_SUPPORT)
  target_link_libraries(vtkPVServerManagerDefaultCxxTests PRIVATE Qt5::Test)
endif ()
