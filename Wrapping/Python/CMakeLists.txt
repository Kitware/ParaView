set(python_files
  paraview/__init__.py.in
  paraview/_backwardscompatibilityhelper.py
  paraview/algorithms/__init__.py
  paraview/algorithms/openpmd.py
  paraview/algorithms/savgreader.py
  paraview/apps/__init__.py
  paraview/apps/__main__.py
  paraview/apps/_internals.py
  paraview/apps/glance.py
  paraview/apps/trame.py
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
  paraview/incubator/__init__.py.in
  paraview/inspect.py
  paraview/live.py
  paraview/modules/__init__.py.in
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
  paraview/tests/basic_rendering.py
  paraview/tests/__init__.py
  paraview/tests/internals.py
  paraview/tests/__main__.py
  paraview/tests/proxy_deprecation.py
  paraview/tests/validate_extracts.py
  paraview/tests/verify_eyedomelighting.py
  paraview/tpl/__init__.py
  paraview/util/__init__.py
  paraview/util/vtkAlgorithm.py
  paraview/variant.py
  paraview/vtkConstants.py
  paraview/vtk.py)

# build the "__all__ = [...]" list for paraview/incubator/__init__.py
set(_paraview_incubator)
foreach (_paraview_python_module IN LISTS paraview_incubator_python_wrapped_modules)
  get_property(_paraview_python_library_name
    TARGET    "${_paraview_python_module}"
    PROPERTY  "INTERFACE_vtk_module_library_name")
  string(APPEND _paraview_incubator_python_modules_all
    "    \'${_paraview_python_library_name}\',\n")
endforeach ()

# build the "__all__ = [...]" list for paraview/modules/__init__.py
set(_paraview_python_modules_all)
foreach (_paraview_python_module IN LISTS paraview_python_wrapped_modules)
  get_property(_paraview_python_library_name
    TARGET    "${_paraview_python_module}"
    PROPERTY  "INTERFACE_vtk_module_library_name")
  string(APPEND _paraview_python_modules_all
    "    \'${_paraview_python_library_name}\',\n")
endforeach ()

set(_python_depends_args)
if (CMAKE_VERSION VERSION_GREATER_EQUAL "3.27")
  list(APPEND _python_depends_args
    DEPENDS_EXPLICIT_ONLY)
endif ()

set(python_copied_modules)
foreach (python_file IN LISTS python_files)
  set(output_python_file
    "${CMAKE_BINARY_DIR}/${PARAVIEW_PYTHON_SITE_PACKAGES_SUFFIX}/${python_file}")
  if (python_file MATCHES "\\.in$")
    string(REPLACE ".in" "" output_python_file "${output_python_file}")
    configure_file(
      "${python_file}"
      "${output_python_file}"
      @ONLY)
  else ()
    add_custom_command(
      OUTPUT  "${output_python_file}"
      DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/${python_file}"
      COMMAND "${CMAKE_COMMAND}" -E copy_if_different
              "${CMAKE_CURRENT_SOURCE_DIR}/${python_file}"
              "${output_python_file}"
      COMMENT "Copying ${python_file} to the binary directory"
      ${_python_depends_args})
  endif ()
  if (BUILD_SHARED_LIBS)
    get_filename_component(python_file_directory "${python_file}" DIRECTORY)
    install(
      FILES       "${output_python_file}"
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
  foreach (_paraview_module IN LISTS paraview_modules paraview_client_modules paraview_incubator_modules)
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
