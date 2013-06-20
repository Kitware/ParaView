# We need to locate smooth.flash since it's not included in the default testing
# datasets.

find_file(smooth_flash 
          NAMES smooth.flash
          DOC "Path to smooth.flash data file."
          NO_DEFAULT_PATH)
mark_as_advanced(smooth_flash)

set (MyTests
  TestResampledAMRImageSourceWithPointData.cxx)

if (EXISTS "${smooth_flash}")
  get_filename_component(smooth_flash_dir "${smooth_flash}" PATH)
  set(MyTests ${MyTests}
    TestResampledAMRImageSource.cxx
  )
  set (TestResampledAMRImageSource_DATA_DIR "${smooth_flash_dir}")
endif()

# Tests with data
if (PARAVIEW_DATA_ROOT)
  # TODO: update these to use the new external data
  # Use the testing object factory, to reduce boilerplate code in tests.
  #include(vtkTestingObjectFactory)
  #vtk_module_test_executable(${vtk-module}CxxTests ${Tests})

  set(TestsToRun ${Tests})
  remove(TestsToRun CxxTests.cxx)

  # Add all the executables
  foreach(test ${TestsToRun})
    get_filename_component(TName ${test} NAME_WE)
    set (data_dir "${PARAVIEW_DATA_ROOT}")
    if (${TName}_DATA_DIR)
      set (data_dir "${${TName}_DATA_DIR}")
    endif()
    add_test(NAME ${vtk-module}-${TName}
      COMMAND ${vtk-module}CxxTests ${TName}
        -D ${data_dir}
        -T ${PARAVIEW_TEST_DIR}
        -V Baseline/${TName}.png)
  endforeach ()
endif()
