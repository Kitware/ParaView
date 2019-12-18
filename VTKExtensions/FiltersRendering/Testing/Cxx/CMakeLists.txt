# We need to locate smooth.flash since it's not included in the default testing
# datasets.

find_file(smooth_flash
          NAMES smooth.flash
          DOC "Path to smooth.flash data file."
          NO_DEFAULT_PATH)
mark_as_advanced(smooth_flash)

vtk_add_test_cxx(vtkPVVTKExtensionsRenderingCxxTests tests
  NO_VALID NO_OUTPUT
# This was basically ignored in the previous version.
#  TestResampledAMRImageSourceWithPointData.cxx
  TestImageCompressors.cxx
  )

#if (EXISTS "${smooth_flash}")
#  get_filename_component(smooth_flash_dir "${smooth_flash}" PATH)
#  set(vtkPVVTKExtensionsRendering_DATA_DIR "${smooth_flash_dir}")
#  # This was basically ignored in the previous version.
#  #vtk_add_test_cxx(vtkPVVTKExtensionsRenderingCxxTests smooth_flash_tests
#  #  TestResampledAMRImageSource.cxx
#  #  )
#  list(APPEND tests
#    ${smooth_flash_tests})
#endif()

# This was basically ignored in the previous version.
vtk_test_cxx_executable(vtkPVVTKExtensionsRenderingCxxTests tests)
