set(python_files
  paraview/__init__.py
  paraview/_backwardscompatibilityhelper.py
  paraview/_colorMaps.py
  paraview/algorithms/__init__.py
  paraview/algorithms/openpmd.py
  paraview/apps/__init__.py
  paraview/apps/__main__.py
  paraview/apps/_internals.py
  paraview/apps/divvy.py
  paraview/apps/flow.py
  paraview/apps/glance.py
  paraview/apps/lite.py
  paraview/apps/visualizer.py
  paraview/benchmark/__init__.py
  paraview/benchmark/basic.py
  paraview/benchmark/logbase.py
  paraview/benchmark/logparser.py
  paraview/benchmark/manyspheres.py
  paraview/benchmark/waveletcontour.py
  paraview/benchmark/waveletvolume.py
  paraview/catalyst/__init__.py
  paraview/catalyst/bridge.py
  paraview/catalyst/detail.py
  paraview/catalyst/importers.py
  paraview/catalyst/v2_internals.py
  paraview/collaboration.py
  paraview/coprocessing.py
  paraview/cpstate.py
  paraview/demos/__init__.py
  paraview/demos/export_catalyst_state.py
  paraview/demos/filedriver_miniapp.py
  paraview/demos/show_grid_as_background.py
  paraview/demos/wavelet_miniapp.py
  paraview/demos/wavelet_miniapp_plugin.py
  paraview/detail/__init__.py
  paraview/detail/annotation.py
  paraview/detail/calculator.py
  paraview/detail/catalyst_export.py
  paraview/detail/cdbwriter.py
  paraview/detail/exportnow.py
  paraview/detail/extract_selection.py
  paraview/detail/loghandler.py
  paraview/detail/pythonalgorithm.py
  paraview/detail/python_selector.py
  paraview/detail/catalyst_export.py
  paraview/live.py
  paraview/lookuptable.py
  paraview/modules/__init__.py
  paraview/numeric.py
  paraview/numpy_support.py
  paraview/pv-vtk-all.py
  paraview/python_view.py
  paraview/selection.py
  paraview/servermanager.py
  paraview/simple.py
  paraview/smstate.py
  paraview/smtesting.py
  paraview/smtrace.py
  paraview/spatiotemporalparallelism.py
  paraview/tests/__init__.py
  paraview/tests/validate_extracts.py
  paraview/tests/verify_eyedomelighting.py
  paraview/tpl/__init__.py
  paraview/util/__init__.py
  paraview/util/vtkAlgorithm.py
  paraview/variant.py
  paraview/vtkConstants.py
  paraview/vtk.py)

set(python_copied_modules)
foreach (python_file IN LISTS python_files)
  set(output_python_file
    "${CMAKE_BINARY_DIR}/${PARAVIEW_PYTHON_SITE_PACKAGES_SUFFIX}/${python_file}")
  add_custom_command(
    OUTPUT  "${output_python_file}"
    DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/${python_file}"
    COMMAND "${CMAKE_COMMAND}" -E copy_if_different
            "${CMAKE_CURRENT_SOURCE_DIR}/${python_file}"
            "${output_python_file}"
    COMMENT "Copying ${python_file} to the binary directory")
  if (BUILD_SHARED_LIBS)
    get_filename_component(python_file_directory "${python_file}" DIRECTORY)
    install(
      FILES       "${python_file}"
      DESTINATION "${PARAVIEW_PYTHON_SITE_PACKAGES_SUFFIX}/${python_file_directory}"
      COMPONENT   "python")
  endif ()
  list(APPEND python_copied_modules
    "${output_python_file}")
endforeach ()

set(_paraview_python_zip)
if (NOT BUILD_SHARED_LIBS)
  # For static builds, we generate a zip file for all ParaView Python modules and packages

  # scan all ParaView modules and collect lists of Python modules they
  # provide/generate.
  set(_paraview_python_files)
  set(_paraview_python_modules)
  foreach (_paraview_module IN LISTS paraview_modules paraview_client_modules)
    _vtk_module_get_module_property("${_paraview_module}"
      PROPERTY "python_modules"
      VARIABLE _paraview_module_python_files)
    list(APPEND _paraview_python_files
      ${_paraview_module_python_files})
    list(APPEND _paraview_python_modules
      "${_paraview_module}")
  endforeach()

  list(APPEND _paraview_python_files
    ${python_copied_modules})

  set(_paraview_python_zip "${CMAKE_BINARY_DIR}/${VTK_PYTHON_SITE_PACKAGES_SUFFIX}/_paraview.zip")
  add_custom_command(
    OUTPUT            "${_paraview_python_zip}"
    COMMAND           ${CMAKE_COMMAND} -E tar "cfv"
                      "${_paraview_python_zip}" --format=zip ${_paraview_python_files}
    DEPENDS           ${_paraview_python_modules}
                      ${_paraview_python_files}
    WORKING_DIRECTORY "${CMAKE_BINARY_DIR}/${VTK_PYTHON_SITE_PACKAGES_SUFFIX}"
    COMMENT           "Creating _paraview.zip Python modules archive")
  install(
    FILES       "${_paraview_python_zip}"
    DESTINATION "${PARAVIEW_PYTHON_SITE_PACKAGES_SUFFIX}/"
    COMPONENT   "python")
endif()
add_custom_target(paraview_python_copy ALL
  DEPENDS
    ${python_copied_modules}
    ${_paraview_python_zip})
