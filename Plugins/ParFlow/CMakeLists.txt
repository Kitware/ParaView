set(CMAKE_CXX_STANDARD 11)

if (PARAVIEW_USE_PYTHON)
  set(python_modules
    parflow.py
  )
endif()

paraview_add_plugin(
  ParFlow
  REQUIRED_ON_SERVER
  REQUIRED_ON_CLIENT
  VERSION "1.0"
  MODULES ParFlow::IO
  MODULE_FILES "${CMAKE_CURRENT_SOURCE_DIR}/IO/vtk.module"
  PYTHON_MODULES ${python_modules}
)

if (PARAVIEW_USE_PYTHON)
  set(python_copied_modules)
  foreach (python_file IN LISTS python_modules)
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
      COMPONENT   "python"
    )
    list(APPEND python_copied_modules "${output_python_file}")
  endforeach ()
  add_custom_target(parflow_python_copy ALL
    DEPENDS
      ${python_copied_modules}
  )
endif()
