set(python_files
  paraview/__init__.py
  paraview/_backwardscompatibilityhelper.py
  paraview/_colorMaps.py
  paraview/benchmark/__init__.py
  paraview/benchmark/basic.py
  paraview/benchmark/logbase.py
  paraview/benchmark/logparser.py
  paraview/benchmark/manyspheres.py
  paraview/benchmark/waveletcontour.py
  paraview/benchmark/waveletvolume.py
  paraview/collaboration.py
  paraview/coprocessing.py
  paraview/cpexport.py
  paraview/cpstate.py
  paraview/demos/demo1.py
  paraview/demos/show_grid_as_background.py
  paraview/detail/__init__.py
  paraview/detail/annotation.py
  paraview/detail/calculator.py
  paraview/detail/exportnow.py
  paraview/detail/extract_selection.py
  paraview/detail/pythonalgorithm.py
  paraview/detail/python_selector.py
  paraview/lookuptable.py
  paraview/modules/__init__.py
  paraview/numeric.py
  paraview/numpy_support.py
  paraview/pv-vtk-all.py
  paraview/python_view.py
  paraview/servermanager.py
  paraview/simple.py
  paraview/smstate.py
  paraview/smtesting.py
  paraview/smtrace.py
  paraview/spatiotemporalparallelism.py
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
  get_filename_component(python_file_directory "${python_file}" DIRECTORY)
  install(
    FILES       "${python_file}"
    DESTINATION "${PARAVIEW_PYTHON_SITE_PACKAGES_SUFFIX}/${python_file_directory}"
    COMPONENT   "python")
  list(APPEND python_copied_modules
    "${output_python_file}")
endforeach ()
add_custom_target(paraview_python_copy ALL
  DEPENDS
    ${python_copied_modules})
