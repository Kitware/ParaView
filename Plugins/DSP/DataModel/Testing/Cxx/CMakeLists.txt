# On Windows, cxx tests executables need to find VTK module dlls.
# But as we are inside a ParaView plugin, test executables are not put in the \bin location,
# where needed dlls are.
# Currently there is no proper way to find them in the VTK testing framework,
# so we disable tests on Windows for now.
# See related issue: https://gitlab.kitware.com/paraview/paraview/-/issues/22154
if (WIN32)
  return ()
endif ()

# Point to ParaView ExternalData when finding test data (used in vtk_add_test_cxx)
set(_vtk_build_TEST_OUTPUT_DATA_DIRECTORY ${paraview_test_data_directory_output})

vtk_add_test_cxx(vtkDigitalSignalProcessingDSPDataModelCxxTests tests
  NO_DATA NO_OUTPUT NO_VALID
  TestMultiDimensionalArray.cxx
  TestMultiDimensionalImplicitBackend.cxx
)

vtk_add_test_cxx(vtkDigitalSignalProcessingDSPDataModelCxxTests tests
  NO_OUTPUT NO_VALID
  TestTemporalDataToMultiDimensionalArray.cxx
)

if (BUILD_SHARED_LIBS)
  vtk_add_test_cxx(vtkDigitalSignalProcessingDSPDataModelCxxTests tests
    NO_OUTPUT NO_VALID
    TestDSPIterator.cxx
    TestDSPIteratorIntegration.cxx
  )
endif()

vtk_test_cxx_executable(vtkDigitalSignalProcessingDSPDataModelCxxTests tests)

# XXX(cmake-3.13)
if (COMMAND target_link_options)
  # XXX(cmake-3.29): Suppress warnings with Xcode 15+ linkers; resolved with
  # CMake 3.29.
  if (CMAKE_CXX_COMPILER_ID STREQUAL "AppleClang" AND
      CMAKE_CXX_COMPILER_VERSION VERSION_GREATER "15")
    target_link_options(vtkDigitalSignalProcessingDSPDataModelCxxTests
      PRIVATE
        "$<$<PLATFORM_ID:Darwin>:LINKER:-no_warn_duplicate_libraries>")
  endif ()
endif ()
